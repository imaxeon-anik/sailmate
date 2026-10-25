Name: harbour-sailmate
Version: 0.1.0
Release: 3
Summary: Native multi-provider AI chat for Sailfish OS
License: LicenseRef-Proprietary
URL: https://github.com/imaxeon-anik/sailmate
Source0: %{name}-%{version}.tar.bz2
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(sailfishsecrets)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Sql)
Requires: sailfishsilica-qt5 >= 0.10.9
Requires: qt5-plugin-sqldriver-sqlite

%description
Sailmate connects directly to OpenAI and Anthropic using your own API keys.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 sailmate.pro
%make_build

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/harbour-sailmate
%{_datadir}/harbour-sailmate
%{_datadir}/applications/harbour-sailmate.desktop
%{_datadir}/icons/hicolor/scalable/apps/harbour-sailmate.svg
