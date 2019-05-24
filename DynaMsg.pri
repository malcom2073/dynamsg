QT += network

INCLUDEPATH += $$PWD/src

SOURCES += \
    $$PWD/src/dynamsg.cpp \
    $$PWD/src/dynamsgconnection.cpp \
    $$PWD/src/dynamsgdatastore.cpp \
    $$PWD/src/dynamsgparser.cpp \
    $$PWD/src/dynamsgserver.cpp
HEADERS += \
    $$PWD/src/dynamsg.h \
    $$PWD/src/dynamsgconnection.h \
    $$PWD/src/dynamsgdatastore.h \
    $$PWD/src/dynamsgparser.h \
    $$PWD/src/dynamsgserver.h

