QT += core network
QT -= gui

LIBS += -lreadline

CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = cvnirc-qt-cli

SOURCES += main.cpp \
    terminalui.cpp

HEADERS += \
    terminalui.h

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(../include/versioncheck.pro)

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../cvnirc-core/release/ -lcvnirc-core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../cvnirc-core/debug/ -lcvnirc-core
else:unix {
    LIBS += -L$$OUT_PWD/../cvnirc-core/ -lcvnirc-core
    PRE_TARGETDEPS += ../cvnirc-core/libcvnirc-core.so*

    include(../include/rpath.pro)
}

INCLUDEPATH += $$PWD/../cvnirc-core
DEPENDPATH += $$PWD/../cvnirc-core
