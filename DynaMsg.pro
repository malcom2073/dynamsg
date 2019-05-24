QT += core network
QT -= gui

TARGET = DynaMsg
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11
TEMPLATE = app
INCLUDEPATH += src
SOURCES += src/main.cpp \
    src/dynatest.cpp

DISTFILES += \
    DynaMsg.pri

include(DynaMsg.pri)

HEADERS += \
    src/dynatest.h
