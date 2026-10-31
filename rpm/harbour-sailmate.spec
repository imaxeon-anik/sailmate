Name: harbour-sailmate
Version: 1.0.0
Release: 1
Summary: Native multi-provider AI chat for Sailfish OS
License: MIT
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

%description
Sailmate is a native AI chat app for OpenAI, Anthropic Claude and custom
OpenAI-compatible endpoints. It streams replies, keeps chat history on the device
and can attach photos for image-capable models. You need your own API key;
usage is billed by the provider. Keys are stored with Sailfish Secrets.

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
%{_datadir}/icons/hicolor/86x86/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/108x108/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/128x128/apps/harbour-sailmate.png
%{_datadir}/icons/hicolor/172x172/apps/harbour-sailmate.png
