Name: td-system-tools
Version: 2.0.0
Release: 1
Summary: Print basic system information and banners
Group: Applications/System
License: GPL-3.0-or-later
URL: https://www.nntb.no/~dreibh/system-tools/
Source: https://www.nntb.no/~dreibh/system-tools/download/%{name}-%{version}.tar.xz

AutoReqProv: on
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: gettext
BuildRoot: %{_tmppath}/%{name}-%{version}-build

Requires: td-system-tools-fingerprint-ssh-keys = %{version}-%{release}
Requires: td-system-tools-misc = %{version}-%{release}
Requires: td-system-tools-system-info = %{version}-%{release}
Requires: td-system-tools-system-maintenance = %{version}-%{release}
Requires: td-system-tools-reset-machine-id = %{version}-%{release}
Recommends: td-system-tools-configure-grub = %{version}-%{release}


%description
This package contains programs for printing basic system
information and for system maintenance.
System-Info displays basic status information about the
system: hostname, uptime, CPU, memory statistics, disk
space statistics, SSH public key hashes, and networking
information. Furthermore, it can be configured to show
one or more banners (for example,  a project name).
System-Info can be configured to be automatically run when
logging in, providing the user an up-to-date overview of
the system.
System-Maintenance runs basic system maintenance tasks:
trying to repair broken package management, updating the
package management databases, installing all available
updates, checking for old kernels and removing them, trim
SSD or unmap unused storage.

%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_SYSCONFDIR=/etc .
%cmake_build

%install
%cmake_install

%files


%package system-info
Summary: Perform basic system maintenance
Group: Applications/System
BuildArch: noarch
Requires: td-system-tools-get-system-info = %{version}-%{release}
Requires: td-system-tools-print-utf8 = %{version}-%{release}
Requires: (figlet or toilet)
Requires: gettext-runtime
Requires: (mbuffer or buffer)
Recommends: subnetcalc
Suggests: banner

%description system-info
This program displays basic status information about the system:
hostname, uptime, CPU, memory statistics, disk space statistics,
SSH public key hashes, and networking information. Furthermore,
it can be configured to show one or more banners (for example,
a project name).
System-Info can be configured to be automatically run when logging
in, providing the user an up-to-date overview of the system.

%files system-info
%{_bindir}/System-Info
%{_datadir}/locale/*/LC_MESSAGES/System-Info.mo
%{_datadir}/System-Info/01-example
%{_datadir}/System-Info/09-hostname-example
%{_datadir}/System-Info/10-company-logo-example
%{_datadir}/System-Info/banner-helper
%{_mandir}/man1/System-Info.1.gz
%{_sysconfdir}/profile.d/system-info.sh
%{_sysconfdir}/profile.d/system-info.csh
%{_sysconfdir}/system-info.d/banner-helper
%{_sysconfdir}/system-info.d/01-example


%package system-maintenance
Summary: Perform basic system maintenance tasks
Group: Applications/System
BuildArch: noarch
Requires: gettext-runtime
Requires: sudo
Recommends: td-system-tools-system-info

%description system-maintenance
This program runs basic system maintenance tasks:
trying to repair broken package management, updating the package
management databases, installing all available updates, checking
for old kernels and removing them, trim SSD or unmap unused storage.

%files system-maintenance
%{_bindir}/System-Maintenance
%{_datadir}/locale/*/LC_MESSAGES/System-Maintenance.mo
%{_mandir}/man1/System-Maintenance.1.gz
%{_sysconfdir}/system-maintenance.d/XX-example


%package reset-machine-id
Summary: Reset machine identity state
Group: Applications/System
BuildArch: noarch
Requires: gettext-runtime
Requires: sudo
Recommends: td-system-tools-system-info
Recommends: td-system-tools-system-maintenance

%description reset-machine-id
This program helps to reset the machine identity state:
reset machine ID, change hostname, replace SSH keys, suggest hardened
SSH client and server settings.

%files reset-machine-id
%{_bindir}/Reset-Machine-ID
%{_datadir}/locale/*/LC_MESSAGES/Reset-Machine-ID.mo
%{_mandir}/man1/Reset-Machine-ID.1.gz


%package fingerprint-ssh-keys
Summary: Reset machine identity state
Group: Applications/System
BuildArch: noarch
Requires: gettext-runtime
Recommends: td-system-tools-system-info

%description fingerprint-ssh-keys
This program prints the SSH key fingerprints of the local machine
in different formats: SSH hash, DNS SSHFP RR.

%files fingerprint-ssh-keys
%{_bindir}/Fingerprint-SSH-Keys
%{_datadir}/locale/*/LC_MESSAGES/Fingerprint-SSH-Keys.mo
%{_mandir}/man1/Fingerprint-SSH-Keys.1.gz


%package configure-grub
Summary: Helper tool to adjust GRUB configuration
Group: Applications/System
BuildArch: noarch
Requires: gettext-runtime
Recommends: td-system-tools-system-info

%description configure-grub
This program adjusts a GRUB configuration file by applying a configuration
from a template, and merging the existing configurations settings with
additional customisations. It can for example be used to set a custom
screen resolution (GRUB_GFXMODE option) or startup tune (GRUB_INIT_TUNE
option).
Warning: This program is meant to be used by expert users! Do not modify
a working GRUB configuration without knowing how to boot the system from
a rescue media to fix a broken configuration!

%files configure-grub
%{_bindir}/configure-grub
%{_mandir}/man1/configure-grub.1.gz
%{_datadir}/configure-grub/grub-defaults-nornet
%{_datadir}/configure-grub/grub-defaults-standard


%package get-system-info
Summary: Obtain basic system information
Group: Applications/System
Requires: procps

%description get-system-info
This small program obtains basic status information about the system:
hostname, uptime, CPU, memory statistics, and networking information.
The output is printed in machine-readable form, which can be used
with evaluation in shell scripts for further processing.

%files get-system-info
%{_bindir}/get-system-info
%{_mandir}/man1/get-system-info.1.gz


%package print-utf8
Summary: Print UTF-8 strings and obtain size/length/width information
Group: Applications/System

%description print-utf8
print-utf8 is a simple program to print UTF-8 strings in the console with
options for indentation, centering, separator as well as size/length/width
information.

%files print-utf8
%{_bindir}/print-utf8
%{_mandir}/man1/print-utf8.1.gz


%package misc
Summary: Miscellaneous tools
Group: Applications/System
Recommends: td-system-tools-print-utf8

%description misc
This package contains two simple tools:
try-hard runs a command and retries for a given number of times in case
of error, with a delay between the trials.
random-sleep waits for a random time, selected from a given interval, with
support for fractional seconds.

%files misc
%{_bindir}/random-sleep
%{_bindir}/try-hard
%{_datadir}/locale/*/LC_MESSAGES/random-sleep.mo
%{_datadir}/locale/*/LC_MESSAGES/try-hard.mo
%{_mandir}/man1/random-sleep.1.gz
%{_mandir}/man1/try-hard.1.gz


%changelog
* Fri Oct 11 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.0
- New upstream release.
* Wed Aug 28 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.7.2
- New upstream release.
* Wed Aug 07 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.7.1
- New upstream release.
* Mon Jun 17 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.7.0
- New upstream release.
* Thu Apr 04 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.6.1
- New upstream release.
* Sun Mar 31 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.6.0
- New upstream release.
* Wed Dec 06 2023 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.5.6
- New upstream release.
* Wed Nov 15 2023 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 1.5.6
- New upstream release.
