/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2026 IRC4Fun
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// $ModAuthor: IRC4Fun
/// $ModDesc: Adds the /GUPDATE command to trigger updates.sh across all network servers.

#include "inspircd.h"

#ifndef _WIN32
# include <fcntl.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

class CommandGUpdate;

class UpdateCheckTimer final
	: public Timer
{
private:
	pid_t childpid;
	std::string requestor;
	std::string logfile;
	CommandGUpdate& cmd;

public:
	UpdateCheckTimer(pid_t pid, const std::string& nick, const std::string& log, CommandGUpdate& c)
		: Timer(2, true)
		, childpid(pid)
		, requestor(nick)
		, logfile(log)
		, cmd(c)
	{
	}

	bool Tick() override;
};

class CommandGUpdate final
	: public Command
{
	friend class UpdateCheckTimer;

private:
	std::string scriptpath;
	std::string logpath;
	bool updating = false;
	UpdateCheckTimer* activetimer = nullptr;
	pid_t childpid = -1;

public:
	CommandGUpdate(Module* Creator)
		: Command(Creator, "GUPDATE", 0, 1)
	{
		access_needed = CmdAccess::OPERATOR;
		syntax = { "[<servermask>]" };
	}

	~CommandGUpdate()
	{
		// Clean up timer if module is unloaded mid-update.
		// Timer destructor calls DelTimer, safely removing from TimerManager.
		delete activetimer;

		// Reap child process if still running to prevent zombies.
		if (childpid > 0)
		{
			int status;
			if (waitpid(childpid, &status, WNOHANG) == 0)
			{
				// Child still running; send SIGTERM and do a brief wait.
				kill(childpid, SIGTERM);
				waitpid(childpid, &status, 0);
			}
		}
	}

	void SetPaths(const std::string& script, const std::string& log)
	{
		scriptpath = script;
		logpath = log;
	}

	CmdResult Handle(User* user, const Params& parameters) override
	{
		std::string servermask = parameters.empty() ? "*" : parameters[0];

		if (!InspIRCd::Match(ServerInstance->Config->ServerName, servermask))
		{
			ServerInstance->SNO.WriteToSnoMask('a', "UPDATE broadcast by '{}' (not targeting this server).", user->nick);
			return CmdResult::SUCCESS;
		}

		if (updating)
		{
			user->WriteNotice(INSP_FORMAT("*** UPDATE already in progress on {}.", ServerInstance->Config->ServerName));
			return CmdResult::FAILURE;
		}

		if (access(scriptpath.c_str(), X_OK) != 0)
		{
			user->WriteNotice(INSP_FORMAT("*** UPDATE script not found or not executable: {}", scriptpath));
			ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 \x03" "04FAILED\x03: script '{}' not found or not executable.",
				ServerInstance->Config->ServerName, scriptpath);
			return CmdResult::FAILURE;
		}

		std::string timestamp = std::to_string(ServerInstance->Time());
		std::string logfile = logpath + "/update-" + timestamp + ".log";

		ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 \x03" "08started\x03 by '{}'. Running: {}",
			ServerInstance->Config->ServerName, user->nick, scriptpath);

		pid_t pid = fork();

		if (pid == -1)
		{
			user->WriteNotice(INSP_FORMAT("*** UPDATE fork failed on {}: {}", ServerInstance->Config->ServerName, strerror(errno)));
			ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 \x03" "04FAILED\x03: fork error: {}",
				ServerInstance->Config->ServerName, strerror(errno));
			return CmdResult::FAILURE;
		}

		if (pid == 0)
		{
			// Child process: close all inherited IRCd file descriptors.
			long maxfd = sysconf(_SC_OPEN_MAX);
			if (maxfd < 0)
				maxfd = 65536;
			for (long fd = 3; fd < maxfd; fd++)
				close(static_cast<int>(fd));

			// Redirect stdout/stderr to log file.
			int logfd = open(logfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
			if (logfd >= 0)
			{
				dup2(logfd, STDOUT_FILENO);
				dup2(logfd, STDERR_FILENO);
				if (logfd > STDERR_FILENO)
					close(logfd);
			}

			// Close stdin.
			close(STDIN_FILENO);

			// Reset signal handlers to defaults.
			signal(SIGPIPE, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);

			// Execute the update script.
			execl("/bin/sh", "sh", scriptpath.c_str(), nullptr);
			_exit(127);
		}

		// Parent: track state and poll for completion.
		childpid = pid;
		updating = true;
		activetimer = new UpdateCheckTimer(pid, user->nick, logfile, *this);
		ServerInstance->Timers.AddTimer(activetimer);

		user->WriteNotice(INSP_FORMAT("*** UPDATE started on {}. You will be notified via snomask 'a' when complete.", ServerInstance->Config->ServerName));
		return CmdResult::SUCCESS;
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override
	{
		return ROUTE_BROADCAST;
	}
};

bool UpdateCheckTimer::Tick()
{
	int status;
	pid_t result = waitpid(childpid, &status, WNOHANG);

	if (result == 0)
		return true; // Still running, keep polling.

	if (result == childpid)
	{
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
			ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 completed \x03" "03successfully\x03. Modules may need reloading (/GRELOADMODULE). Requested by '{}'.",
				ServerInstance->Config->ServerName, requestor);
		}
		else
		{
			int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
			ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 \x03" "04FAILED\x03 (exit code {}). Check log: {}. Requested by '{}'.",
				ServerInstance->Config->ServerName, exitcode, logfile, requestor);
		}
	}
	else
	{
		ServerInstance->SNO.WriteToSnoMask('a', "UPDATE on \x02{}\x02 \x03" "04FAILED\x03 (waitpid error: {}). Requested by '{}'.",
			ServerInstance->Config->ServerName, strerror(errno), requestor);
	}

	// Clean up: clear command state, then self-destruct.
	cmd.updating = false;
	cmd.childpid = -1;
	cmd.activetimer = nullptr;
	delete this;
	return false;
}

class ModuleRemoteUpdate final
	: public Module
{
private:
	CommandGUpdate cmd;

public:
	ModuleRemoteUpdate()
		: Module(VF_OPTCOMMON, "Adds the /GUPDATE command which allows server operators to trigger updates.sh on all network servers.")
		, cmd(this)
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		auto tag = ServerInstance->Config->ConfValue("remoteupdate");
		std::string script = tag->getString("script", "/home/ircd/updates.sh", 1);
		std::string logdir = tag->getString("logdir", "/home/ircd/inspircd4/run/logs", 1);

		if (access(script.c_str(), F_OK) != 0)
			throw ModuleException(this, "Update script not found: " + script);

		if (access(script.c_str(), X_OK) != 0)
			throw ModuleException(this, "Update script is not executable (permission denied): " + script);

		cmd.SetPaths(script, logdir);
	}
};

MODULE_INIT(ModuleRemoteUpdate)
