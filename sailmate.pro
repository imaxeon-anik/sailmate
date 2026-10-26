TEMPLATE = app
TARGET = harbour-sailmate
CONFIG += c++11 link_pkgconfig
QT += core gui qml quick network sql
PKGCONFIG += sailfishapp sailfishsecrets
target.path = /usr/bin
SOURCES += src/main.cpp src/chatcontroller.cpp src/credentialstore.cpp
HEADERS += src/chatcontroller.h src/credentialstore.h src/providerprofile.h

DISTFILES += qml/sailmate.qml qml/pages/*.qml rpm/harbour-sailmate.spec

qml.files = qml
qml.path = /usr/share/harbour-sailmate
desktop.files = rpm/harbour-sailmate.desktop
desktop.path = /usr/share/applications
icon.files = rpm/harbour-sailmate.svg
icon.path = /usr/share/icons/hicolor/scalable/apps
INSTALLS += target qml desktop icon
iconSizes = 86 108 128 172
for(size, iconSizes) {
    icon$${size}.files = icons/$${size}x$${size}/harbour-sailmate.png
    icon$${size}.path = /usr/share/icons/hicolor/$${size}x$${size}/apps
    INSTALLS += icon$${size}
}
