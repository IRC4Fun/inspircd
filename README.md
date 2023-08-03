## About

InspIRCd is a modular C++ Internet Relay Chat (IRC) server for UNIX-like and Windows systems.

## Supported Platforms

InspIRCd is supported on the following platforms:

- Most recent BSD variants using the Clang or GCC compilers and the GNU toolchains (Make, etc).

- Most recent Linux distributions using the Clang or GCC compilers and the GNU toolchain.

- The most recent three major releases of macOS using the AppleClang, Clang, or GCC (*not* LLVM-GCC) compilers and the GNU toolchains.

- Windows 7 or newer using the MSVC 14 (Visual Studio 2015) compiler and CMake 3.8 or newer.

Other platforms and toolchains may also work but are not officially supported by the InspIRCd team. Generally speaking if you are using a reasonably modern UNIX-like system you should be able to build InspIRCd on it. If you can not and you wish to submit a patch we are happy to accept it as long as it is not extremely large.

If you encounter any bugs then [please file an issue](https://github.com/inspircd/inspircd/issues/new/choose).

## Installation

Most InspIRCd users running a UNIX-like system build from source. A guide about how to do this is available on [the InspIRCd docs site](https://docs.inspircd.org/3/installation/source).

Building from source on Windows is generally not recommended but [a guide is available](https://github.com/inspircd/inspircd/blob/master/win/README.txt) if you wish to do this.

If you are running on CentOS 7, Debian 11/12, Rocky Linux 8/9, Ubuntu 18.04/20.04/22.04, or Windows 8+ binary packages are available from [the downloads page](https://github.com/inspircd/inspircd/releases/latest).

*Recommended Installation*

  1.  Git CLONE from our InspIRCd Github: `git clone https://github.com/IRC4Fun/inspircd.git`  
        *Dependencies:* `build-essential , curl , libwww-perl`  
  2.  `cd inspircd` and run `./configure` — The path(s) should be `/home/acct/inspircd`  
  3.  Allow the configuration manager to enable all extra plugins at once.  
  4.  When asked if you would like to create your own self-signed SSL certificate, type NO.  
      When asked if you would like to DELETE the self-signed certificate, type YES.  
  5.  Once completed, you will need to use InspIRCd3’s modulemanager to install the following contrib modules.  
        `./modulemanager install m_shed_users`  
        `./modulemanager install m_opmoderated`  
        `./modulemanager install m_joinpartsno`  
        `./modulemanager install m_stats_unlinked`  
        `./modulemanager install m_require_auth`  
        `./modulemanager install m_svsoper`  
        `./modulemanager install m_tag_iphost`  
        `./modulemanager install m_changecap`  
        `./modulemanager install m_blockhighlight`  
        `./modulemanager install m_extbanredirect`  
        `./modulemanager install m_antiknocker`  
        `./modulemanager install m_cve_2022_2663`  

  6.  Now you will need to run `make`, followed by `make install`.  
  7.  Setup your *inspircd.conf* _(found in *inspircd/run/conf*)_ using the template provided.  
  8.  Your server is now ready for configuration before being run.  Configuration files are provided if your application is passed into Testlink.  You may run a temporary configuration of your own, if you wish to have the server running before it is reviewed.  (However, understand that the IRC4Fun configuration files will only be provided should the application be voted into Testlink.)  


A [Docker](https://www.docker.com) image is also available. See [the inspircd-docker repository](https://github.com/inspircd/inspircd-docker) for more information.

Some distributions ship an InspIRCd package in their package managers. We generally do not recommend the use of such packages as in the past distributions have made broken modifications to InspIRCd and not kept their packages up to date with essential security updates.

## License

InspIRCd is licensed under [version 2 of the GNU General Public License](https://docs.inspircd.org/license).

## External Links

* [Website](https://www.inspircd.org)
* [Documentation](https://docs.inspircd.org)
* [GitHub](https://github.com/inspircd)
* [Social Media](https://docs.inspircd.org/social)
* Support IRC channel &mdash; \#inspircd on irc.inspircd.org
* Development IRC channel &mdash; \#inspircd.dev on irc.inspircd.org
* InspIRCd test network &mdash; testnet.inspircd.org
