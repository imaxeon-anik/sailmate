Name: harbour-sailmate
Version: 0.1.0
Release: 6
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
BuildRequires: pkgconfig(Qt5Concurrent)
Requires: sailfishsilica-qt5 >= 0.10.9
Requires: qt5-plugin-sqldriver-sqlite

%description
Sailmate connects directly to OpenAI and Anthropic using your own API keys.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 sailmate.pro APP_VERSION=%{version}-%{release}
%make_build

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/harbour-sailmate
%{_datadir}/harbour-sailmate
%{_datadir}/applications/harbour-sailmate.desktop
%{_datadir}/icons/hicolor/scalable/apps/harbour-sailmate.svg
%{_datadir}/icons/hicolor/86x86/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/108x108/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/128x128/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/172x172/apps/harbour-sailmate.png
