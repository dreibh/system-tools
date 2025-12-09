Name: td-system-tools
Version: 2.2.2
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
Requires: td-system-tools-reset-machine-id = %{version}-%{release}
Requires: td-system-tools-system-info = %{version}-%{release}
Requires: td-system-tools-system-maintenance = %{version}-%{release}
Requires: td-system-tools-text-block = %{version}-%{release}
Requires: td-system-tools-x509-tools = %{version}-%{release}
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
%{_datadir}/bash-completion/completions/System-Info
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
%{_datadir}/bash-completion/completions/System-Maintenance
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
%{_datadir}/bash-completion/completions/Reset-Machine-ID
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
%{_datadir}/bash-completion/completions/Fingerprint-SSH-Keys
%{_datadir}/locale/*/LC_MESSAGES/Fingerprint-SSH-Keys.mo
%{_mandir}/man1/Fingerprint-SSH-Keys.1.gz


%package configure-grub
Summary: Helper tool to adjust GRUB configuration
Group: Applications/System
BuildArch: noarch
Requires: gettext-runtime
Recommends: td-system-tools-system-maintenance

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
%{_datadir}/bash-completion/completions/configure-grub
%{_datadir}/configure-grub/grub-defaults-nornet
%{_datadir}/configure-grub/grub-defaults-standard
%{_mandir}/man1/configure-grub.1.gz


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
%{_datadir}/bash-completion/completions/get-system-info
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
%{_datadir}/bash-completion/completions/print-utf8
%{_datadir}/locale/*/LC_MESSAGES/print-utf8.mo


%package text-block
Summary: Apply modifications to text
Group: Applications/System

%description text-block
text-block reads text from standard input or given file, and writes it to standard output or a given file. On the text, various modifications can be applied, according to the operation mode.

%files text-block
%{_bindir}/text-block
%{_mandir}/man1/text-block.1.gz
%{_datadir}/bash-completion/completions/text-block
%{_datadir}/locale/*/LC_MESSAGES/text-block.mo


%package x509-tools
Summary: X.509 certificate handling tools
Group: Applications/System
BuildArch: noarch
Requires: td-system-tools-text-block = %{version}-%{release}
Requires: openssl
Recommends: gnutls-utils
Recommends: nss-tools

%description x509-tools
 This package contains four simple tools:
 view-certificate displays an X.509 certificate and its hierarchy.
 check-certificate verifies an X.509 certificate using a CA certificate
 and optionally a revocation list.
 extract-pem extracts a PEM file.
 test-tls-connection tests a TCP TLS connection to a remote endpoint.

%files x509-tools
%{_bindir}/check-certificate
%{_bindir}/extract-pem
%{_bindir}/test-tls-connection
%{_bindir}/view-certificate
%{_datadir}/bash-completion/completions/check-certificate
%{_datadir}/bash-completion/completions/extract-pem
%{_datadir}/bash-completion/completions/test-tls-connection
%{_datadir}/bash-completion/completions/view-certificate
%{_datadir}/locale/*/LC_MESSAGES/check-certificate.mo
%{_datadir}/locale/*/LC_MESSAGES/extract-pem.mo
%{_datadir}/locale/*/LC_MESSAGES/test-tls-connection.mo
%{_datadir}/locale/*/LC_MESSAGES/view-certificate.mo
%{_mandir}/man1/check-certificate.1.gz
%{_mandir}/man1/extract-pem.1.gz
%{_mandir}/man1/test-tls-connection.1.gz
%{_mandir}/man1/view-certificate.1.gz


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
%{_bindir}/text-block
%{_bindir}/try-hard
%{_datadir}/bash-completion/completions/random-sleep
%{_datadir}/bash-completion/completions/text-block
%{_datadir}/bash-completion/completions/try-hard
%{_datadir}/locale/*/LC_MESSAGES/random-sleep.mo
%{_datadir}/locale/*/LC_MESSAGES/try-hard.mo
%{_datadir}/text-block/example1.txt
%{_datadir}/text-block/example2.txt
%{_datadir}/text-block/insert.txt
%{_datadir}/text-block/numbers.txt
%{_mandir}/man1/random-sleep.1.gz
%{_mandir}/man1/text-block.1.gz
%{_mandir}/man1/try-hard.1.gz


%changelog
* Tue Dec 09 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.2-1
- New upstream release.
* Fri Nov 28 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.1-1
- New upstream release.
* Wed Nov 26 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.0-1
- New upstream release.
* Fri Nov 14 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.9-1
- New upstream release.
* Sat Nov 01 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.8-1
- New upstream release.
* Mon Oct 27 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.7-1
- New upstream release.
* Fri Oct 24 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.6-1
- New upstream release.
* Wed Oct 08 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.5-1
- New upstream release.
* Sun Sep 21 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.4-1
- New upstream release.
* Tue Jul 08 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.3
- New upstream release.
* Wed Jun 18 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.2
- New upstream release.
* Mon May 12 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.1
- New upstream release.
* Sun Apr 27 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.1.0
- New upstream release.
* Sat Apr 26 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.10
- New upstream release.
* Mon Apr 14 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.9
- New upstream release.
* Wed Mar 05 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.8
- New upstream release.
* Fri Dec 20 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.7
- New upstream release.
* Wed Dec 18 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.6
- New upstream release.
* Fri Dec 13 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.5
- New upstream release.
* Tue Dec 03 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.4
- New upstream release.
* Tue Nov 12 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.3
- New upstream release.
* Sat Nov 02 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.2
- New upstream release.
* Fri Oct 25 2024 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.0.1
- New upstream release.
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
