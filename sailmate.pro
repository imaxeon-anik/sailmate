TEMPLATE = app
TARGET = harbour-sailmate
CONFIG += sailfishapp c++11 link_pkgconfig
QT += core gui qml quick network sql
PKGCONFIG += sailfishsecrets
SOURCES += src/main.cpp src/chatcontroller.cpp src/credentialstore.cpp
HEADERS += src/chatcontroller.h src/credentialstore.h

DISTFILES += qml/sailmate.qml qml/pages/*.qml rpm/harbour-sailmate.spec

qml.files = qml
qml.path = /usr/share/harbour-sailmate
desktop.files = rpm/harbour-sailmate.desktop
desktop.path = /usr/share/applications
icon.files = rpm/harbour-sailmate.svg
icon.path = /usr/share/icons/hicolor/scalable/apps
INSTALLS += qml desktop icon
