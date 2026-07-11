Name: td-system-tools
Version: 2.7.8
Release: 1
Summary: Tools for basic system management
License: GPL-3.0-or-later
URL: https://www.nntb.no/~dreibh/system-tools/
Source: https://www.nntb.no/~dreibh/system-tools/download/%{name}-%{version}.tar.xz

BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: gettext

Requires: %{name}-basic = %{version}-%{release}


%description
System-Tools is a collection of utilities for system management and
maintenance.

The package includes:
- System-Info: Displays system status (CPU, memory, storage, network)
  and configurable login banners.
- System-Maintenance: Automates package updates, old kernel removal,
  and storage cleanup (e.g., SSD trimming).
- Reset-Machine-ID: Resets machine IDs, hostnames, and SSH keys for
  cloned machines.
- Fingerprint-SSH-Keys: Shows the machine's SSH public key fingerprints
  in different formats.
- Configure-GRUB: Configures options for the GRUB boot loader.
- Print-UTF8: Prints UTF-8 text with options for centering,
  adjusting, etc.
- Text-Block: Edits files or streams by inserting, replacing, or
  removing text blocks.
- Unix-Timestamp-Tools: Convert Unix timestamps (s, ms, us, ns) to
  and from date/time strings.
- Try-Hard: Retries commands with a configurable backoff.
- Random-Sleep: Waits for a random time span, with support for
  fractional seconds.
- X.509-Tools: Provide utilities for viewing, verifying, and
  converting X.509 certificates, and testing TLS connections.

The utilities are suitable for non-interactive use in shell scripts and
feature native internationalization support via GNU gettext.

%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_SYSCONFDIR=/etc
%cmake_build

%install
%cmake_install
%find_lang Fingerprint-SSH-Keys
%find_lang Reset-Machine-ID
%find_lang System-Info
%find_lang System-Maintenance
%find_lang check-certificate
%find_lang extract-pem
%find_lang print-utf8
%find_lang random-sleep
%find_lang test-tls-connection
%find_lang text-block
%find_lang time2unixts
%find_lang try-hard
%find_lang unixts2time
%find_lang view-certificate
%find_lang view-crl

# ====== Apply shebang fix for scripts ======================================
for directory in %{_bindir} \
                 %{_datadir}/System-Info/ \
                 %{_datadir}/system-tools/ \
                 %{_sysconfdir}/system-info.d/ \
                 %{_sysconfdir}/system-maintenance.d/ \
                 ; do
   find "%{buildroot}/$directory" -type f -exec sed -i \
      -e 's|^#!/usr/bin/env bash|#!/usr/bin/bash|' \
      -e 's|^#!/usr/bin/env python3|#!/usr/bin/python3|' \
      -e 's|^#!/usr/bin/env Rscript|#!/usr/bin/Rscript|' \
      {} +
done
# ===========================================================================

%files


%package system-info
Summary: Print basic system information and banners
BuildArch: noarch
Requires: %{name}-get-system-info = %{version}-%{release}
Requires: %{name}-print-utf8 = %{version}-%{release}
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

%files system-info -f System-Info.lang
%{_bindir}/System-Info
%{_datadir}/bash-completion/completions/System-Info
%dir %attr(0755, root, root) %{_datadir}/System-Info
%{_datadir}/System-Info/01-example
%{_datadir}/System-Info/09-hostname-example
%{_datadir}/System-Info/10-company-logo-example
%{_datadir}/System-Info/banner-helper
%{_mandir}/man1/System-Info.1.gz
%config(noreplace) %{_sysconfdir}/profile.d/system-info.sh
%{_sysconfdir}/profile.d/system-info.sh
%config(noreplace) %{_sysconfdir}/profile.d/system-info.csh
%{_sysconfdir}/profile.d/system-info.csh
%dir %attr(0755, root, root) %{_sysconfdir}/system-info.d
%config(noreplace) %{_sysconfdir}/system-info.d/banner-helper
%{_sysconfdir}/system-info.d/banner-helper
%config(noreplace) %{_sysconfdir}/system-info.d/01-example
%{_sysconfdir}/system-info.d/01-example


%package get-system-info
Summary: Obtain basic system information

%description get-system-info
This small program obtains basic status information about the system:
hostname, uptime, CPU, memory statistics, and networking information.
The output is printed in machine-readable form, which can be evaluated
in shell scripts for further processing.

%files get-system-info
%{_bindir}/get-system-info
%{_datadir}/bash-completion/completions/get-system-info
%{_mandir}/man1/get-system-info.1.gz


%package system-maintenance
Summary: Perform basic system maintenance
BuildArch: noarch
Requires: gettext-runtime
Requires: sudo
Recommends: %{name}-system-info

%description system-maintenance
This program runs basic system maintenance tasks:
trying to repair broken package management, updating the package
management databases, installing all available updates, checking
for old kernels and removing them, trimming SSDs, or unmapping
unused storage.

%files system-maintenance -f System-Maintenance.lang
%{_bindir}/System-Maintenance
%{_datadir}/bash-completion/completions/System-Maintenance
%{_mandir}/man1/System-Maintenance.1.gz
%dir %attr(0755, root, root) %{_sysconfdir}/system-maintenance.d
%config(noreplace) %{_sysconfdir}/system-maintenance.d/XX-example
%{_sysconfdir}/system-maintenance.d/XX-example


%package reset-machine-id
Summary: Reset machine identity state
BuildArch: noarch
Requires: gettext-runtime
Requires: sudo
Requires: uuid
Recommends: %{name}-system-info
Recommends: %{name}-system-maintenance

%description reset-machine-id
This program helps to reset the machine identity state: resetting the
machine ID, changing the hostname, replacing SSH keys, and suggesting
hardened SSH client and server settings.

%files reset-machine-id -f Reset-Machine-ID.lang
%{_bindir}/Reset-Machine-ID
%{_datadir}/bash-completion/completions/Reset-Machine-ID
%{_mandir}/man1/Reset-Machine-ID.1.gz


%package fingerprint-ssh-keys
Summary: Print SSH key fingerprints
BuildArch: noarch
Requires: gettext-runtime
Recommends: %{name}-system-info

%description fingerprint-ssh-keys
This program prints the SSH key fingerprints of the local machine
in different formats: SSH hash, DNS SSHFP RR.

%files fingerprint-ssh-keys -f Fingerprint-SSH-Keys.lang
%{_bindir}/Fingerprint-SSH-Keys
%{_datadir}/bash-completion/completions/Fingerprint-SSH-Keys
%{_mandir}/man1/Fingerprint-SSH-Keys.1.gz


%package configure-grub
Summary: Helper tool to adjust GRUB configuration
BuildArch: noarch
Requires: gettext-runtime
Recommends: %{name}-system-maintenance

%description configure-grub
This program adjusts a GRUB configuration file by applying a configuration
from a template, and merging the existing configuration settings with
additional customisations. It can for example be used to set a custom
screen resolution (GRUB_GFXMODE option) or startup tune (GRUB_INIT_TUNE
option).
Warning: This program is meant to be used by expert users! Do not modify
a working GRUB configuration without knowing how to boot the system from
a rescue medium to fix a broken configuration!

%files configure-grub
%{_bindir}/configure-grub
%{_datadir}/bash-completion/completions/configure-grub
%dir %attr(0755, root, root) %{_datadir}/configure-grub
%{_datadir}/configure-grub/grub-defaults-nornet
%{_datadir}/configure-grub/grub-defaults-standard
%{_mandir}/man1/configure-grub.1.gz


%package print-utf8
Summary: Print UTF-8 strings and obtain size/length/width information

%description print-utf8
The print-utf8 tool is a simple program to print UTF-8 strings in the
console with options for indentation, centering, and separators, as well
as size/length/width information.

%files print-utf8 -f print-utf8.lang
%{_bindir}/print-utf8
%{_mandir}/man1/print-utf8.1.gz
%{_datadir}/bash-completion/completions/print-utf8


%package text-block
Summary: Apply modifications to text

%description text-block
The text-block tool reads text from standard input or given file, and
writes it to standard output or a given file. Various modifications can
be applied to the text depending on the operation mode.

%files text-block -f text-block.lang
%{_bindir}/text-block
%{_mandir}/man1/text-block.1.gz
%{_datadir}/bash-completion/completions/text-block
%dir %attr(0755, root, root) %{_datadir}/text-block
%{_datadir}/text-block/example1.txt
%{_datadir}/text-block/example2.txt
%{_datadir}/text-block/insert.txt
%{_datadir}/text-block/numbers.txt


%package unixtimestamp-tools
Summary: Unix timestamp handling tools
Group: Applications/System

%description unixtimestamp-tools
This package contains two simple tools:
time2unixts converts a time string to a Unix timestamp.
unixts2time converts a Unix timestamp to a time string.
These tools support Unix timestamps (i.e. the time since
January 1, 1970, 00:00:00.000000000 UTC) in seconds,
milliseconds, microseconds, and nanoseconds.

%files unixtimestamp-tools -f unixts2time.lang -f time2unixts.lang
%{_bindir}/time2unixts
%{_bindir}/unixts2time
%{_datadir}/bash-completion/completions/time2unixts
%{_datadir}/bash-completion/completions/unixts2time
%{_mandir}/man1/time2unixts.1.gz
%{_mandir}/man1/unixts2time.1.gz


%package try-hard
Summary: Make multiple trials to successfully run a command
BuildArch: noarch
Conflicts: %{name}-misc
Requires: gettext-runtime

%description try-hard
Try-hard runs a command and retries for a given number of times in case
of error, with a delay between the trials.

%files try-hard -f try-hard.lang
%{_bindir}/try-hard
%{_datadir}/bash-completion/completions/try-hard
%{_mandir}/man1/try-hard.1.gz


%package random-sleep
Summary: Wait for a random time span
Conflicts: %{name}-misc

%description random-sleep
Random-sleep waits for a random time span, selected from a given
interval, with support for fractional seconds.

%files random-sleep -f random-sleep.lang
%{_bindir}/random-sleep
%{_datadir}/bash-completion/completions/random-sleep
%{_mandir}/man1/random-sleep.1.gz


%package x509-tools
Summary: X.509 certificate handling tools
BuildArch: noarch
Requires: %{name}-print-utf8 = %{version}-%{release}
Requires: %{name}-text-block = %{version}-%{release}
Requires: gettext-runtime
Requires: (mbuffer or buffer)
Requires: openssl
Recommends: gnutls-utils
Recommends: nss-tools
Suggests: pwgen
Suggests: python3
Suggests: python3-netifaces

%description x509-tools
This package contains X.509 certificate handling tools:
* The view-certificate tool displays an X.509 certificate and its hierarchy.
* The view-crl tool shows an X.509 certificate revocation list (CRL).
* The check-certificate tool verifies an X.509 certificate using a CA
  certificate and optionally a revocation list.
* The extract-pem tool extracts a PEM file.
* The der2pem tool converts a certificate or CRL in DER format to PEM format.
* The pem2der tool converts a certificate or CRL in PEM format to DER format.
* The test-tls-connection tool tests a TCP TLS connection to a remote endpoint.

%files x509-tools -f check-certificate.lang -f extract-pem.lang -f test-tls-connection.lang -f view-certificate.lang -f view-crl.lang
%{_bindir}/check-certificate
%{_bindir}/der2pem
%{_bindir}/extract-pem
%{_bindir}/pem2der
%{_bindir}/test-tls-connection
%{_bindir}/view-certificate
%{_bindir}/view-crl
%{_datadir}/bash-completion/completions/check-certificate
%{_datadir}/bash-completion/completions/der2pem
%{_datadir}/bash-completion/completions/extract-pem
%{_datadir}/bash-completion/completions/pem2der
%{_datadir}/bash-completion/completions/test-tls-connection
%{_datadir}/bash-completion/completions/view-certificate
%{_datadir}/bash-completion/completions/view-crl
%dir %attr(0755, root, root) %{_datadir}/system-tools
%{_datadir}/system-tools/CertificateHelper.py
%{_datadir}/system-tools/generate-test-certificates
%{_datadir}/system-tools/make-test-certificates
%{_mandir}/man1/check-certificate.1.gz
%{_mandir}/man1/der2pem.1.gz
%{_mandir}/man1/extract-pem.1.gz
%{_mandir}/man1/pem2der.1.gz
%{_mandir}/man1/test-tls-connection.1.gz
%{_mandir}/man1/view-certificate.1.gz
%{_mandir}/man1/view-crl.1.gz


%package gimp-scripts
Summary: GIMP image processing scripts
Group: Applications/System
BuildArch: noarch
Requires: fontconfig
Requires: gimp
Requires: GraphicsMagick
Requires: util-linux
Recommends: fractgen-clifractgen
Recommends: open-sans-fonts
Recommends: urw-base35-fonts
Suggests: %{name}-gimp-scripts-examples

%description gimp-scripts
 This package contains some GIMP scripts for automated image processing.
 The main purpose of this script is to generate background images for
 desktops, boot splashes, etc. with some branding.

%files gimp-scripts
%{_bindir}/gs-bumpmap
%{_bindir}/gs-caption
%{_bindir}/gs-clothify
%{_bindir}/gs-glossytext
%{_bindir}/gs-list-fonts
%{_bindir}/gs-list-gradients
%{_bindir}/gs-list-patterns
%{_bindir}/gs-mosaic
%{_bindir}/gs-oilify
%{_bindir}/gs-oldphoto
%{_bindir}/gs-resize-with-cropping
%{_bindir}/gs-test-gimp
%{_datadir}/bash-completion/completions/gs-bumpmap
%{_datadir}/bash-completion/completions/gs-caption
%{_datadir}/bash-completion/completions/gs-clothify
%{_datadir}/bash-completion/completions/gs-glossytext
%{_datadir}/bash-completion/completions/gs-mosaic
%{_datadir}/bash-completion/completions/gs-oilify
%{_datadir}/bash-completion/completions/gs-oldphoto
%{_datadir}/bash-completion/completions/gs-resize-with-cropping
%{_datadir}/bash-completion/completions/gs-test-gimp
%{_mandir}/man1/gs-bumpmap.1.gz
%{_mandir}/man1/gs-caption.1.gz
%{_mandir}/man1/gs-clothify.1.gz
%{_mandir}/man1/gs-glossytext.1.gz
%{_mandir}/man1/gs-list-fonts.1.gz
%{_mandir}/man1/gs-list-gradients.1.gz
%{_mandir}/man1/gs-list-patterns.1.gz
%{_mandir}/man1/gs-mosaic.1.gz
%{_mandir}/man1/gs-oilify.1.gz
%{_mandir}/man1/gs-oldphoto.1.gz
%{_mandir}/man1/gs-resize-with-cropping.1.gz
%{_mandir}/man1/gs-test-gimp.1.gz


%package gimp-scripts-examples
Summary: Example files for the GIMP image processing scripts
Group: Applications/System
BuildArch: noarch

%description gimp-scripts-examples
The GIMP scripts are scripts for automated image processing.
The main purpose of this script is to generate background images for
desktops, boot splashes, etc. with some branding.
This package contains some example input files for the testing the
GIMP scripts.

%files gimp-scripts-examples
%dir %attr(0755, root, root) %{_datadir}/system-tools/gimp-scripts-examples
%{_datadir}/system-tools/gimp-scripts-examples/Bergen.webp
%{_datadir}/system-tools/gimp-scripts-examples/Fractal.fsf
%{_datadir}/system-tools/gimp-scripts-examples/Portobello.webp


%package basic
Summary: Metapackage for basic system tools sub-packages
BuildArch: noarch
Requires: %{name}-fingerprint-ssh-keys = %{version}-%{release}
Requires: %{name}-random-sleep = %{version}-%{release}
Requires: %{name}-reset-machine-id = %{version}-%{release}
Requires: %{name}-system-info = %{version}-%{release}
Requires: %{name}-system-maintenance = %{version}-%{release}
Requires: %{name}-text-block = %{version}-%{release}
Requires: %{name}-try-hard = %{version}-%{release}
Requires: %{name}-unixtimestamp-tools = %{version}-%{release}
Requires: %{name}-x509-tools = %{version}-%{release}
Recommends: %{name}-configure-grub = %{version}-%{release}

%description basic
This package is a metapackage for the system information and maintenance
tools. It installs the basic sub-packages.
Note that td-system-configure-grub is only added as weak dependency
("recommends"),  since it is only available on selected architectures.

%files basic


%package complete
Summary: Metapackage for complete system tools sub-packages
BuildArch: noarch
Requires: %{name}-basic = %{version}-%{release}
Requires: %{name}-gimp-scripts = %{version}-%{release}
Requires: %{name}-gimp-scripts-examples = %{version}-%{release}

%description complete
This package is a metapackage for the system information and maintenance
tools. It installs all sub-packages.

%files complete


%changelog
* Sat Jul 11 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.8-1
- New upstream release.
* Wed Jul 01 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.7-1
- New upstream release.
* Tue Jun 30 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.6-1
- New upstream release.
* Fri Jun 26 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.5-1
- New upstream release.
* Fri Jun 12 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.4-1
- New upstream release.
* Fri Jun 05 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.3-1
- New upstream release.
* Mon Jun 01 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.2-1
- New upstream release.
* Sun May 31 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.1-1
- New upstream release.
* Sun May 31 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.7.0-1
- New upstream release.
* Sat May 23 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.6.4-1
- New upstream release.
* Fri May 22 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.6.3-1
- New upstream release.
* Sun May 17 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.6.2-1
- New upstream release.
* Sat May 09 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.6.1-1
- New upstream release.
* Fri May 08 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.6.0-1
- New upstream release.
* Fri May 08 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.6-1
- New upstream release.
* Thu May 07 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.5-1
- New upstream release.
* Wed May 06 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.4-1
- New upstream release.
* Tue May 05 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.3-1
- New upstream release.
* Mon May 04 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.2-1
- New upstream release.
* Sun May 03 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.1-1
- New upstream release.
* Sat May 02 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.5.0-1
- New upstream release.
* Thu Apr 30 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.4.4-1
- New upstream release.
* Thu Apr 30 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.4.3-1
- New upstream release.
* Sun Apr 26 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.4.2-1
- New upstream release.
* Sat Apr 25 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.4.1-1
- New upstream release.
* Fri Apr 24 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.4.0-1
- New upstream release.
* Thu Apr 23 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.3.2-1
- New upstream release.
* Mon Apr 20 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.3.1-1
- New upstream release.
* Sun Apr 19 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.3.0-1
- New upstream release.
* Wed Feb 25 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.5-1
- New upstream release.
* Mon Feb 09 2026 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.4-1
- New upstream release.
* Tue Dec 09 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.3-1
- New upstream release.
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
