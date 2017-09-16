#
# spec file for package keyleds
#
# Copyright (c) 2017 Julien Hartmann <juli1.hartmann@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself.

Summary: Logitech Keyboard per-key lighting control
Name: keyleds
Version: 0.4.2
Release: 1%{?dist}
License: GPL-3.0+
Group: Applications/System
Source: https://github.com/spectras/keyleds/archive/v%{version}/%{name}-%{version}.tar.gz
URL: https://github.com/spectras/keyleds
BuildRequires: cmake, make, gcc, gcc-c++, libudev-devel, libxml2-devel, libyaml-devel, libX11-devel, libXi-devel
%if 0%{?suse_version}
BuildRequires: libqt4-devel
%else
BuildRequires: qt4-devel
%endif
Requires: udev

%description
Includes a background service and a command-line tool to handle Logitech
Gaming Keyboard lights.
The following features are supported:
 - Device enumeration and information (keyboard type, firmware version, ...)
 - Inspection of device capabilities and led configuration.
 - Individual key led control.
 - Game-mode configuration
 - Report rate configuration.
 - Session service responding to X window events

%prep
%setup -q

%build
%if 0%{!?suse_version:1}
cd build
%endif
%cmake ..
make %{?_smp_mflags}
gzip -9 ../keyledsd/keyledsd.1 -c > keyledsd.1.gz
gzip -9 ../keyledsctl/keyledsctl.1 -c > keyledsctl.1.gz

%install
cd build
%make_install
cd ..
install -m 644 -D keyledsd/keyledsd.conf.sample %{buildroot}/%{_sysconfdir}/keyledsd.conf
install -d %{buildroot}/%{_sysconfdir}/xdg/autostart
ln -s %{_datadir}/keyledsd/keyledsd.desktop %{buildroot}/%{_sysconfdir}/xdg/autostart/
install -m 644 -D logitech.rules %{buildroot}/%{_datadir}/keyledsd/logitech.rules
install -d %{buildroot}/%{_udevrulesdir}
ln -s %{_datadir}/keyledsd/logitech.rules %{buildroot}/%{_udevrulesdir}/70-logitech-hidpp.rules
install -m 644 -D build/keyledsd.1.gz %{buildroot}/%{_mandir}/man1/keyledsd.1
install -m 644 -D build/keyledsctl.1.gz %{buildroot}/%{_mandir}/man1/keyledsctl.1

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc LICENSE
%{_bindir}/*
%{_datadir}/keyledsd
%{_includedir}/*
%{_libdir}/*
%{_mandir}/man1/*
%config(noreplace) %{_sysconfdir}/keyledsd.conf
%{_sysconfdir}/xdg/autostart/keyledsd.desktop
%{_udevrulesdir}/70-logitech-hidpp.rules

