## About

InspIRCd is a modular C++ Internet Relay Chat (IRC) server for UNIX-like and Windows systems.

## Supported Platforms

InspIRCd is supported on the following platforms:

- Most recent BSD variants using the Clang 5+ or GCC 7+ compilers and the GNU toolchains (Make, etc).

- Most recent Linux distributions using the Clang 5+ or GCC 7+ compilers and the GNU toolchain.

- The three most recent major releases of macOS using the AppleClang 10, Clang 5+, or GCC 7+ compilers and the GNU toolchain.

- Windows 10 April 2018 Update or newer using the MSVC 19.15+ (Visual Studio 15.8 2017) compiler and CMake 3.20 or newer.

Other platforms and toolchains may also work but are not officially supported by the InspIRCd team. Generally speaking if you are using a reasonably modern UNIX-like system you should be able to build InspIRCd on it. If you can not and you wish to submit a patch we are happy to accept it as long as it is not extremely large.

If you encounter any bugs then [please file an issue](https://github.com/inspircd/inspircd/issues/new/choose).

## Installation

*Recommended Installation*

  1.  Git CLONE from our InspIRCd Github: `git clone https://github.com/IRC4Fun/inspircd.git inspircd4`  
        *Dependencies:* `build-essential , curl , libwww-perl , libpsl-dev`  
  2.  `cd inspircd4` and run `./configure` — The path(s) should be `/home/acct/inspircd4`  
  3.  Allow the configuration manager to enable all extra plugins at once.  
  4.  Once completed, you will need to use InspIRCd4’s modulemanager to install the following contrib modules.  
        `./modulemanager install hostchange`
        `./modulemanager install m_blockhighlight`
        `./modulemanager install m_blocksock`
        `./modulemanager install m_require_auth`  
        `./modulemanager install m_antiknocker`  
        `./modulemanager install m_cve_2024_39844`  
        `./modulemanager install m_userip`  
        `./modulemanager install m_nopartmsg`  
        `./modulemanager install m_stats_unlinked`  

  5.  Now you will need to run `make`, followed by `make install`.  
  6.  `cp -R /home/acct/inspircd4/tools/ssl /home/acct/ssl`, then `nano /home/acct/ssl/daily_certcheck_update.sh` and update the PATH.  
  7.  `cp /home/acct/inspircd4/tools/updates.sh /home/acct/updates.sh`, then `nano /home/acct/updates.sh` and update the PATH.  
  8.  Setup your *inspircd.conf* _(found in *inspircd4/run/conf/examples*)_ using the template provided, and save as `inspircd4/run/conf/inspircd.conf`.  
  9.  Your server is now ready for configuration before being run.  Configuration files are provided if your application is passed into Testlink.  You may run a temporary configuration of your own, if you wish to have the server running before it is reviewed.  (However, understand that the IRC4Fun configuration files will only be provided should the application be voted into Testlink.)  


## License

InspIRCd is licensed under [version 2 of the GNU General Public License](https://docs.inspircd.org/license).

## External Links

* [Website](https://www.inspircd.org)
* [Documentation](https://docs.inspircd.org)
* [Support](https://docs.inspircd.org/support)
* [GitHub](https://github.com/inspircd)
* [Codeberg (read-only mirror)](https://codeberg.org/inspircd)
* Support IRC channel &mdash; \#inspircd on irc.teranova.net (TLS only)
* Development IRC channel &mdash; \#inspircd.dev on irc.teranova.net (TLS only)
* InspIRCd test network &mdash; testnet.inspircd.org (TLS only)
