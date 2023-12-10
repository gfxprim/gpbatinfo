#
# GPBATINFO specfile
#
# (C) Cyril Hrubis metan{at}ucw.cz 2013-2022
#
#

Summary: App to shows battery charge level and estimates.
Name: gpbatinfo
Version: git
Release: 1
License: GPL-2.0-or-later
Group: System/Monitoring
Url: https://github.com/gfxprim/gpbatinfo
Source: gpbatinfo-%{version}.tar.bz2
BuildRequires: libgfxprim-devel
BuildRequires: libsysinfo-devel

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
App to shows battery charge level and estimates.

%prep
%setup -n gpbatinfo-%{version}

%build
make %{?jobs:-j%jobs}

%install
DESTDIR="$RPM_BUILD_ROOT" make install

%files -n gpbatinfo
%defattr(-,root,root)
%{_bindir}/gpbatinfo
%{_sysconfdir}/gp_apps/
%{_sysconfdir}/gp_apps/gpbatinfo/
%{_sysconfdir}/gp_apps/gpbatinfo/*
#%{_datadir}/applications/gpbatinfo.desktop
#%{_datadir}/gpbatinfo/
#%{_datadir}/gpbatinfo/gpbatinfo.png

%changelog
* Tue Jun 28 2022 Cyril Hrubis <metan@ucw.cz>

  Initial version.
