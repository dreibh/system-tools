# System-Tools
Tools for Basic System Management

## Description

### System-information

System-Info displays basic status information about the system: hostname, uptime, CPU, memory statistics, disk space statistics, SSH public key hashes, and networking information. Furthermore, it can be configured to show one or more banners (for example, a project name). System-Info can be configured to be automatically run when logging in, providing the user an up-to-date overview of the system.

One main purpose of System-Info is to run on login, to particularly show a nice login banner (for example, a project or company logo) and then present the basic system information. For this purpose, System-Info can be configured with banner scripts (by default looked up in /etc/system-info.d or /usr/local/etc/system-info.d), which are processed in alphabetically descending order by file name, like:

- ``95-application-logo``,
- ``90-project-logo``,
- ``60-department-logo``,
- ``50-company-logo``,
- ``01-example``.
</ul>
<p class="description">
The names of all scripts MUST begin with two decimal numbers. That is, scripts must be named [0-9][0-9]... to be processed by System-Info!
</p>
<p class="description">
If one of the scripts exits with non-zero exit code, the processing of further banner scripts is stopped. This can be used for preconfiguring a system for example with a department and company logo, where the company logo script terminates further processing. A modified system for a certain project can add a project logo as well. The project logo script may terminate further processing, not showing department and company logos. This may be combined with packaging scripts, for example adding an application logo as part of the application's install package (like adding a script 95-application-logo).
</p>

### System-Maintenance

System-Maintenance runs some system maintenance tasks to keep the system clean and up to date. These tasks are:

- Ensuring that all packages are configured,
- Updating the package repositories,
- Removing obsolete kernels,
- Installing all available package updates,
- Auto-removing unused packages,
- Ensuring that Grub (the bootloader) is installed and up-to-date,
- Delete network interface mapping (only on request by option, see below),
- Updating package and file search caches,
- Updating firmware,
- Trimming SSDs and virtual storage.

## Binary Package Installation

Please use the issue tracker at https://github.com/dreibh/system-tools/issues to report bugs and issues!

### Ubuntu Linux

For ready-to-install Ubuntu Linux packages of System-Tools, see Launchpad PPA for Thomas Dreibholz!

```
sudo apt-add-repository -sy ppa:dreibh/ppa
sudo apt-get update
sudo apt-get install td-system-tools
```

### Fedora Linux

For ready-to-install Fedora Linux packages of System-Tools, see COPR PPA for Thomas Dreibholz!

```
sudo dnf copr enable -y dreibh/ppa
sudo dnf install td-system-tools
```

### FreeBSD

For ready-to-install FreeBSD packages of System-Tools, it is included in the ports collection, see Index of /head/net/td-system-tools/!

   pkg install td-system-tools

Alternatively, to compile it from the ports sources:

```
cd /usr/ports/net/td-system-tools
make
make install
```

## Sources Download

System-Tools is released under the GNU General Public Licence (GPL).

Please use the issue tracker at https://github.com/dreibh/system-tools/issues to report bugs and issues!

### Development Version

The Git repository of the System-Tools sources can be found at https://github.com/dreibh/system-tools:

```
git clone https://github.com/dreibh/system-tools
cd system-tools
cmake .
make
```

Contributions:

- Issue tracker: https://github.com/dreibh/system-tools/issues.
  Please submit bug reports, issues, questions, etc. in the issue tracker!

- Pull Requests for System-Tools: https://github.com/dreibh/system-tools/pulls.
  Your contributions to System-Tools are always welcome!

- CI build tests of System-Tools: https://github.com/dreibh/system-tools/actions.

- Coverity Scan analysis of System-Tools: https://scan.coverity.com/projects/dreibh-td-system-tools.

### Current Stable Release

See https://www.nntb.no/~dreibh/system-tools/#Download!
