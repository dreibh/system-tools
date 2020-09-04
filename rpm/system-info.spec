Name: system-info
Version: 1.0.0~alpha1.2
Release: 1
Summary: Print basic system information and banners
Group: Applications/System
License: GPL-3+
URL: https://www.uni-due.de/~be0001/system-info/
Source: https://www.uni-due.de/~be0001/system-info/download/%{name}-%{version}.tar.xz

AutoReqProv: on
BuildRequires: cmake
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%define _unpackaged_files_terminate_build 0

%description
This program displays basic status information about the system:
hostname, uptime, CPU, memory statistics, disk space statistics,
SSH public key hashes, and networking information. Furthermore,
it can be configured to show one or more banners (for example,
a project name).
System-Info can be configured to be automatically run when logging
in, providing the user an up-to-date overview of the system.

%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr .
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

%files
%{_bindir}/System-Info
%{_mandir}/man1/System-Info.1.gz
%{_sysconfdir}/system-info/01-example

%post
if [ ! -e /etc/profile.d/systeminfo.csh ] ; then
   ln -s /usr/bin/System-Info /etc/profile.d/systeminfo.csh
fi
if [ ! -e /etc/profile.d/systeminfo.sh ] ; then
   ln -s /usr/bin/System-Info /etc/profile.d/systeminfo.sh
fi

%preun
rm -f /etc/profile.d/system-info.sh
rm -f /etc/profile.d/system-info.csh


%changelog
