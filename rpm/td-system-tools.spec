Name: td-system-tools
Version: 1.1.3~rc0
Release: 1
Summary: Print basic system information and banners
Group: Applications/System
License: GPL-3+
URL: https://www.uni-due.de/~be0001/system-tools/
Source: https://www.uni-due.de/~be0001/system-tools/download/%{name}-%{version}.tar.xz

AutoReqProv: on
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%define _unpackaged_files_terminate_build 0


BuildArch: noarch

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

%package system-info
Summary: Perform basic system maintenance
Group: Applications/System
BuildArch: noarch
Recommends: subnetcalc
Suggests: banner
Suggests: figlet
Suggests: toilet

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
%{_mandir}/man1/System-Info.1.gz
%{_sysconfdir}/system-info.d/01-example

%post system-info
if [ ! -e /etc/profile.d/systeminfo.csh ] ; then
   ln -s /usr/bin/System-Info /etc/profile.d/systeminfo.csh
fi
if [ ! -e /etc/profile.d/systeminfo.sh ] ; then
   ln -s /usr/bin/System-Info /etc/profile.d/systeminfo.sh
fi

%preun system-info
rm -f /etc/profile.d/system-info.sh
rm -f /etc/profile.d/system-info.csh


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
%{_mandir}/man1/System-Maintenance.1.gz
%{_sysconfdir}/system-maintenance.d/XX-example


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
%{_datadir}/configure-grub/grub-defaults


%package all
Summary: Meta package for system information and maintenance tools
Group: Applications/System
BuildArch: noarch
Requires: td-system-tools-system-info
Requires: td-system-tools-system-maintenance
Recommends: td-system-tools-configure-grub

%description all
This package is a meta package for the system information and maintenance
tools.

%files all


%changelog
