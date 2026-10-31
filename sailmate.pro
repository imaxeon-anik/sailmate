TEMPLATE = app
TARGET = harbour-sailmate
CONFIG += c++11 link_pkgconfig
QT += core gui qml quick network sql concurrent
PKGCONFIG += sailfishapp sailfishsecrets
target.path = /usr/bin
SOURCES += src/main.cpp src/chatcontroller.cpp src/credentialstore.cpp
SOURCES += src/chatimage.cpp
HEADERS += src/chatimage.h
isEmpty(APP_VERSION): APP_VERSION = 1.0.0
DEFINES += APP_VERSION=\\\"$$APP_VERSION\\\"
HEADERS += src/chatcontroller.h src/credentialstore.h src/providerprofile.h src/appsettings.h

DISTFILES += qml/sailmate.qml qml/pages/*.qml rpm/harbour-sailmate.spec

qml.files = qml
qml.path = /usr/share/harbour-sailmate
desktop.files = rpm/harbour-sailmate.desktop
desktop.path = /usr/share/applications
INSTALLS += target qml desktop
iconSizes = 86 108 128 172
for(size, iconSizes) {
    icon$${size}.files = icons/$${size}x$${size}/harbour-sailmate.png
    icon$${size}.path = /usr/share/icons/hicolor/$${size}x$${size}/apps
    INSTALLS += icon$${size}
}
