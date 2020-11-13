Name:           recoverymanager
Version:        1.0
Release:        1%{?dist}
Summary:        Linux service recovery manager

License:        GPL2+
URL:            https://gitlab.com/anpopa/recoverymanager
Source:         %{url}/-/archive/master/recoverymanager-master.zip

BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  glib2-devel >= 2.60
BuildRequires:  systemd-devel >= 243
BuildRequires:  sqlite-devel >= 3.30

Requires: sqlite >= 3.30
Requires: systemd >= 243
Requires: glib2 >= 2.60

%description
%{summary}

%package devel
Summary:        Development tools for %{name}
Requires:       %{name}%{?_isa} = %{?epoch:%{epoch}:}%{version}-%{release}

%description devel
%{summary}.

%global _vpath_srcdir %{name}-master

%prep
%autosetup -c

%build
%meson
%meson_build

%install
%meson_install

%check
%meson_test

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%{_bindir}/recoverymanager
%{_sysconfdir}/recoverymanager.conf
%{_sysconfdir}/systemd/system/recoverymanager.service
%{_sysconfdir}/recoverymanager/sample.recovery
