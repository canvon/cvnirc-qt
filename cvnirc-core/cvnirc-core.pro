#-------------------------------------------------
#
# Project created by QtCreator 2017-11-19T16:26:51
#
#-------------------------------------------------

QT += network
QT -= gui

CONFIG += c++11

TEMPLATE = lib
TARGET = cvnirc-core
VERSION = 0.5.0

DEFINES += CVNIRCCORE_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += irccore.cpp \
    ircprotoclient.cpp \
    ircprotomessage.cpp \
    irccorecontext.cpp \
    commandlayer.cpp \
    command.cpp \
    commandgroup.cpp \
    commanddefinition.cpp \
    irccorecommandgroup.cpp

HEADERS += cvnirc-core_global.h \
    irccore.h \
    ircprotoclient.h \
    ircprotomessage.h \
    irccorecontext.h \
    commandlayer.h \
    command.h \
    commandgroup.h \
    commanddefinition.h \
    irccorecommandgroup.h

DISTFILES += \
    ../README.txt \
    ../TODO.txt

unix {
    target.path = /usr/lib
    INSTALLS += target
}

include(../include/versioncheck.pro)
