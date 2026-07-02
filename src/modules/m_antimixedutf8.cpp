/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   (C) 2026 reverse - mike.chevronnet@gmail.com
 *
 * Detects spam that mixes Unicode scripts within words (e.g. Latin letters
 * with Cyrillic/Greek look-alikes — "ＦᏒｅe Ⅴ1аgrа"), a very common spam
 * obfuscation. Inspired by UnrealIRCd's antimixedutf8 module.
 *
 * Per word: if the word contains letters from more than one script, it scores.
 * The message score is the number of such mixed words (plus a bonus for a high
 * ratio of non-Latin letters in a mostly-ASCII message). At/above <threshold>
 * the configured action is taken (block | kill | gline | zline | kline).
 */

/// $ModAuthor: reverse - mike.chevronnet@gmail.com
/// $ModDepends: core 4
/// $ModDesc: Blocks spam that mixes Unicode scripts within words (look-alike obfuscation).
/// $ModConfig: <antimixedutf8 threshold="8" minlen="10" action="block" duration="1h" reason="Mixed-script text (spam)." target="both">

#include "inspircd.h"
#include "numerichelper.h"
#include "modules/account.h"
#include "modules/ssl.h"
#include "xline.h"
#include "timeutils.h"

namespace
{
	enum class Script
	{
		OTHER,   // digits, punctuation, symbols, emoji — ignored for mixing
		LATIN,
		CYRILLIC,
		GREEK,
		ARMENIAN,
		HEBREW,
		ARABIC,
		CJK,
	};

	// Decode one UTF-8 codepoint starting at i; advances i past it.
	uint32_t DecodeUTF8(const std::string& s, size_t& i)
	{
		const unsigned char c = static_cast<unsigned char>(s[i]);
		uint32_t cp;
		size_t extra;

		if (c < 0x80)      { cp = c;          extra = 0; }
		else if ((c >> 5) == 0x6) { cp = c & 0x1F; extra = 1; }
		else if ((c >> 4) == 0xE) { cp = c & 0x0F; extra = 2; }
		else if ((c >> 3) == 0x1E){ cp = c & 0x07; extra = 3; }
		else { i += 1; return 0xFFFD; } // invalid lead byte

		for (size_t k = 0; k < extra; ++k)
		{
			if (i + 1 + k >= s.size())
				{ i = s.size(); return 0xFFFD; }
			const unsigned char cc = static_cast<unsigned char>(s[i + 1 + k]);
			if ((cc >> 6) != 0x2)
				{ i += 1; return 0xFFFD; } // invalid continuation
			cp = (cp << 6) | (cc & 0x3F);
		}
		i += 1 + extra;
		return cp;
	}

	// Map a codepoint to a script. Returns OTHER for anything that isn't a
	// letter we care about (so spaces/digits/punct don't count as "mixing").
	Script ClassifyScript(uint32_t cp)
	{
		if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'))
			return Script::LATIN;
		if (cp >= 0x00C0 && cp <= 0x024F) return Script::LATIN;    // Latin-1 suppl + extended
		if (cp >= 0x0370 && cp <= 0x03FF) return Script::GREEK;
		if (cp >= 0x0400 && cp <= 0x04FF) return Script::CYRILLIC;
		if (cp >= 0x0530 && cp <= 0x058F) return Script::ARMENIAN;
		if (cp >= 0x0590 && cp <= 0x05FF) return Script::HEBREW;
		if (cp >= 0x0600 && cp <= 0x06FF) return Script::ARABIC;
		if (cp >= 0x4E00 && cp <= 0x9FFF) return Script::CJK;      // CJK unified
		if (cp >= 0x3040 && cp <= 0x30FF) return Script::CJK;      // hiragana/katakana
		return Script::OTHER;
	}

	// Is this codepoint a non-Latin letter that LOOKS like an ASCII Latin
	// letter? These are the homoglyphs spammers swap in. Detecting them
	// directly catches pure-homoglyph words (e.g. all-Cyrillic "Ѕесurіtу")
	// that script-mixing alone would miss, while never tripping on genuine
	// monolingual text (which a human reads as its own script, not as Latin).
	bool IsLatinConfusable(uint32_t cp)
	{
		switch (cp)
		{
			// Cyrillic look-alikes
			case 0x0430: case 0x0410: // а А -> a A
			case 0x0435: case 0x0415: // е Е -> e E
			case 0x043E: case 0x041E: // о О -> o O
			case 0x0440: case 0x0420: // р Р -> p P
			case 0x0441: case 0x0421: // с С -> c C
			case 0x0443: case 0x0423: // у У -> y Y
			case 0x0445: case 0x0425: // х Х -> x X
			case 0x0456: case 0x0406: // і І -> i I
			case 0x0455: case 0x0405: // ѕ Ѕ -> s S
			case 0x0458: case 0x0408: // ј Ј -> j J
			case 0x043A: case 0x041A: // к К -> k (loose)
			case 0x043C: case 0x041C: // м М -> m M
			case 0x043D: case 0x041D: // н Н -> h H (loose)
			case 0x0432: case 0x0412: // в В -> b B (loose)
			case 0x0442: case 0x0422: // т Т -> t T
			// Greek look-alikes
			case 0x03BF: case 0x039F: // ο Ο -> o O
			case 0x03B1: case 0x0391: // α Α -> a A
			case 0x03B5: case 0x0395: // ε Ε -> e E
			case 0x03C1: case 0x03A1: // ρ Ρ -> p P
			case 0x03C5: case 0x03A5: // υ Υ -> y Y
			case 0x03BD:              // ν -> v
			case 0x03BA: case 0x039A: // κ Κ -> k K
			case 0x03B9: case 0x0399: // ι Ι -> i I
			case 0x03BC:              // μ -> u (loose)
			case 0x0392:              // Β -> B
			case 0x039D:              // Ν -> N
			case 0x03A4:              // Τ -> T
			case 0x0397:              // Η -> H
			case 0x03A7:              // Χ -> X
			case 0x0396:              // Ζ -> Z
				return true;
			default:
				return false;
		}
	}

	// "Fancy" Latin: fullwidth, mathematical alphanumerics, enclosed/parenthesised
	// letters. These render as styled ASCII ("𝐅𝐫𝐞𝐞", "Ｆｒｅｅ", "🅵🆁🅴🅴") and are a
	// pure-obfuscation signal — normal users don't type whole words like this.
	bool IsFancyLatin(uint32_t cp)
	{
		if (cp >= 0xFF21 && cp <= 0xFF5A) return true;            // fullwidth A-Z a-z
		if (cp >= 0x1D400 && cp <= 0x1D7FF) return true;          // mathematical alphanumeric symbols
		if (cp >= 0x1F130 && cp <= 0x1F189) return true;          // squared/enclosed latin
		if (cp >= 0x24B6 && cp <= 0x24E9) return true;            // circled latin
		if (cp >= 0x2460 && cp <= 0x24FF) return true;            // enclosed alphanumerics (loose)
		return false;
	}

	// Invisible / zero-width characters used to split words and evade filters.
	bool IsInvisible(uint32_t cp)
	{
		switch (cp)
		{
			case 0x00AD: // soft hyphen
			case 0x200B: // zero width space
			case 0x200C: // zero width non-joiner
			case 0x200D: // zero width joiner
			case 0x2060: // word joiner
			case 0xFEFF: // BOM / zero width no-break space
			case 0x180E: // mongolian vowel separator
				return true;
			default:
				return false;
		}
	}

	// Score a message for look-alike / obfuscated-text spam. Higher = more
	// suspicious. Designed to catch more than plain script-mixing while keeping
	// genuine monolingual text (any script) at score 0.
	unsigned int ScoreMessage(const std::string& text)
	{
		unsigned int mixedwords = 0;       // words mixing >1 real script
		unsigned int homoglyphwords = 0;   // ASCII + confusable letters in one word
		unsigned int purehomowords = 0;    // word made (almost) ENTIRELY of confusables
		unsigned int fancywords = 0;       // words containing fancy/styled latin
		unsigned int invisibles = 0;       // zero-width chars anywhere
		unsigned int totalletters = 0, latinletters = 0;

		bool wordhas[8] = { false };
		bool word_has_ascii = false, word_has_confusable = false, word_has_fancy = false;
		unsigned int word_letters = 0, word_confusables = 0;
		auto resetword = [&]() {
			for (bool& b : wordhas) b = false;
			word_has_ascii = word_has_confusable = word_has_fancy = false;
			word_letters = word_confusables = 0;
		};
		auto wordscripts = [&]() {
			unsigned int n = 0;
			for (int s = 1; s < 8; ++s) if (wordhas[s]) n++;
			return n;
		};
		auto endword = [&]() {
			// Confusable letter mixed WITH real ASCII Latin in one word = the
			// classic "swap a few letters" attack. This already IS script-mixing,
			// so count it ONCE here (not also as a mixedword) — otherwise a single
			// stray homoglyph in one word double-scores and false-positives.
			if (word_has_confusable && word_has_ascii) homoglyphwords++;
			else if (wordscripts() >= 2) mixedwords++;
			// A single-script word with NO ASCII Latin but built almost entirely
			// of Latin-confusable letters is Latin spelled in disguise (e.g.
			// all-Cyrillic "оссаѕіоп"). Genuine Russian/Greek words contain
			// non-confusable letters of their script, so they stay below 80%.
			else if (!word_has_ascii && wordscripts() == 1 && word_letters >= 4 &&
				word_confusables * 100 / word_letters >= 80)
				purehomowords++;
			if (word_has_fancy) fancywords++;
			resetword();
		};

		resetword();
		for (size_t i = 0; i < text.size(); )
		{
			uint32_t cp = DecodeUTF8(text, i);

			if (IsInvisible(cp)) { invisibles++; continue; } // don't treat as boundary

			const bool fancy = IsFancyLatin(cp);
			const bool confusable = IsLatinConfusable(cp);
			Script sc = ClassifyScript(cp);

			const bool isletter = (sc != Script::OTHER) || fancy;
			const bool isboundary = (cp == ' ' || cp == '\t' || cp == ',' || cp == '.' ||
				cp == '!' || cp == '?' || cp == 0xFFFD);

			if (isletter)
			{
				if (sc != Script::OTHER) wordhas[static_cast<int>(sc)] = true;
				totalletters++;
				word_letters++;
				if (sc == Script::LATIN) { latinletters++; word_has_ascii = true; }
				if (confusable) { word_has_confusable = true; word_confusables++; }
				if (fancy) word_has_fancy = true;
			}

			if (isboundary)
				endword();
		}
		endword(); // final word

		// Count how many "disguised words" the message contains. A single one is
		// almost always an accident (someone pasted one Cyrillic letter), so we
		// grant a 1-word grace: real attacks disguise MANY words. This is what
		// keeps "nоwhola..." (1 stray homoglyph) from being blocked while still
		// catching genuine multi-word obfuscation.
		const unsigned int disguised = homoglyphwords + mixedwords + purehomowords + fancywords;
		const unsigned int effective = disguised > 1 ? disguised - 1 : 0;

		unsigned int score = 0;
		score += effective * 5;      // each disguised word past the first
		score += fancywords * 1;     // small extra weight: styled unicode is rarely innocent
		score += invisibles * 3;     // zero-width evasion is always suspicious

		// Ratio bonus: only when there is REAL disguise (>=2 disguised words) and
		// non-Latin letters heavily dominate — never on a lone stray character.
		if (totalletters >= 8 && disguised >= 2)
		{
			const unsigned int nonlatin = totalletters - latinletters;
			if (nonlatin > 0 && latinletters > 0 && nonlatin * 100 / totalletters >= 40)
				score += 3;
		}

		return score;
	}
}

class ModuleAntiMixedUTF8 final
	: public Module
{
private:
	Account::API accountapi;

	unsigned int threshold = 8;
	size_t minlen = 10;
	std::string action = "block";
	unsigned long duration = 3600;
	std::string reason = "Mixed-script text (spam).";
	bool check_channel = true;
	bool check_private = true;

public:
	ModuleAntiMixedUTF8()
		: Module(VF_VENDOR, "Blocks spam that mixes Unicode scripts within words.")
		, accountapi(this)
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		const auto& tag = ServerInstance->Config->ConfValue("antimixedutf8");
		threshold = tag->getNum<unsigned int>("threshold", 8, 1);
		minlen    = tag->getNum<size_t>("minlen", 10, 1);
		action    = tag->getString("action", "block");
		duration  = tag->getDuration("duration", 3600, 1);
		reason    = tag->getString("reason", "Mixed-script text (spam).");

		const std::string tgt = tag->getString("target", "both");
		check_channel = (irc::equals(tgt, "both") || irc::equals(tgt, "channel"));
		check_private = (irc::equals(tgt, "both") || irc::equals(tgt, "private"));
	}

	ModResult OnUserPreMessage(User* user, MessageTarget& target, MessageDetails& details) override
	{
		LocalUser* luser = IS_LOCAL(user);
		if (!luser)
			return MOD_RES_PASSTHRU;

		// Exempt opers, exempt users, and logged-in accounts.
		if (luser->IsOper() || luser->exempt)
			return MOD_RES_PASSTHRU;
		if (accountapi && accountapi->GetAccountName(luser))
			return MOD_RES_PASSTHRU;

		if (target.type == MessageTarget::TYPE_CHANNEL && !check_channel)
			return MOD_RES_PASSTHRU;
		if (target.type == MessageTarget::TYPE_USER && !check_private)
			return MOD_RES_PASSTHRU;
		if (target.type == MessageTarget::TYPE_SERVER)
			return MOD_RES_PASSTHRU;

		// Only the ACTION body of CTCPs is checked; other CTCPs are skipped.
		std::string_view ctcpname;
		std::string_view body(details.text);
		if (details.IsCTCP(ctcpname, body))
		{
			if (!irc::equals(ctcpname, "ACTION"))
				return MOD_RES_PASSTHRU;
		}

		if (body.length() < minlen)
			return MOD_RES_PASSTHRU;

		const unsigned int score = ScoreMessage(std::string(body));
		if (score < threshold)
			return MOD_RES_PASSTHRU;

		ServerInstance->SNO.WriteGlobalSno('a',
			"ANTIMIXEDUTF8: blocked message from {} (score {} >= {})",
			luser->GetRealMask(), score, threshold);

		if (irc::equals(action, "block"))
		{
			const std::string msg = "Oups ! Votre message contient des caractères mélangés souvent utilisés par les spams. Réécrivez-le simplement et réessayez. 🙂";
			if (target.type == MessageTarget::TYPE_CHANNEL)
				luser->WriteNumeric(Numerics::CannotSendTo(target.Get<Channel>(), msg));
			else
				luser->WriteNotice("*** " + msg);
			return MOD_RES_DENY;
		}

		// Punitive actions: drop the message AND act on the user.
		if (irc::equals(action, "gline"))
			AddLine<GLine>("G-line", luser, luser->GetBanUser(true), luser->GetAddress());
		else if (irc::equals(action, "kline"))
			AddLine<KLine>("K-line", luser, luser->GetBanUser(true), luser->GetAddress());
		else if (irc::equals(action, "zline"))
			AddLine<ZLine>("Z-line", luser, luser->GetAddress());
		else if (irc::equals(action, "kill"))
			ServerInstance->Users.QuitUser(luser, reason);

		return MOD_RES_DENY;
	}

private:
	template <typename Line, typename... Extra>
	void AddLine(const char* type, LocalUser* user, Extra&&... extra)
	{
		auto* line = new Line(ServerInstance->Time(), duration, MODNAME "@" + ServerInstance->Config->ServerName,
			reason, std::forward<Extra>(extra)...);
		if (!ServerInstance->XLines->AddLine(line, nullptr))
		{
			delete line;
			ServerInstance->Users.QuitUser(user, reason);
			return;
		}
		ServerInstance->SNO.WriteToSnoMask('x', "{} added a timed {} on {}, expires in {}: {}",
			line->source, type, line->Displayable(), Duration::ToLongString(line->duration), line->reason);
		ServerInstance->XLines->ApplyLines();
	}
};

MODULE_INIT(ModuleAntiMixedUTF8)