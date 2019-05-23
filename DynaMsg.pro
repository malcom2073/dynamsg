QT += core
QT -= gui

TARGET = DynaMsg
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    dynamsg.cpp

DISTFILES += \
    DynaMsg.pri

HEADERS += \
    dynamsg.h

