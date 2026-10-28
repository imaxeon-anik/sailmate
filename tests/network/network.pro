QT += core gui network sql testlib concurrent
CONFIG += console testcase c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_chatcontroller
INCLUDEPATH += ../../src
SOURCES += tst_chatcontroller.cpp ../../src/chatcontroller.cpp ../../src/chatimage.cpp
HEADERS += ../../src/chatcontroller.h ../../src/appsettings.h
