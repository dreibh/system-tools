Name: td-system-tools
Version: 1.7.1
Release: 1
Summary: Print basic system information and banners
Group: Applications/System
License: GPL-3+
URL: https://www.nntb.no/~dreibh/system-tools/
Source: https://www.nntb.no/~dreibh/system-tools/download/%{name}-%{version}.tar.xz

AutoReqProv: on
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: gettext
BuildArch:     noarch
BuildRoot: %{_tmppath}/%{name}-%{version}-build

# Meta-package td-system-tools: install td-system-tools-all => install all sub-packages!
Requires: %{name}-all


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
Requires: (figlet or toilet)
Requires: gettext-runtime
Requires: iproute
Requires: (mbuffer or buffer)
Requires: procps
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


%package all
Summary: Meta package for system information and maintenance tools
Group: Applications/System
BuildArch: noarch
Requires: td-system-tools-fingerprint-ssh-keys
Requires: td-system-tools-system-info
Requires: td-system-tools-system-maintenance
Requires: td-system-tools-reset-machine-id
Recommends: td-system-tools-configure-grub

%description all
This package is a meta package for the system information and maintenance
tools.

%files all


%changelog
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
