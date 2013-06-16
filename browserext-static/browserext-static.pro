#-------------------------------------------------
#
# Project created by QtCreator 2013-05-02T15:18:43
#
#-------------------------------------------------

QT       += network webkit

TARGET = phpbrowser
TEMPLATE = lib
CONFIG += staticlib

SOURCES += downloader.cpp \
    guithread.cpp \
    initializer.cpp \
    phpbrowser.cpp \
    phpwebpage.cpp \
    phpwebview.cpp \
    proxycheckthread.cpp \
    webelementts.cpp \
    xpathinspector.cpp \
    nativeguithread.cpp


HEADERS += phpbrowser.h \
    downloader.h \
    phpwebpage.h \
    proxycheckthread.h \
    xpathinspector.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
