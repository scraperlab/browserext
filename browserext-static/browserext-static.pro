#-------------------------------------------------
#
# Project created by QtCreator 2013-05-02T15:18:43
#
#-------------------------------------------------

QT       += core network webkit widgets gui webkitwidgets

TARGET = phpbrowser
TEMPLATE = lib
CONFIG += staticlib release c++11

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
    proxycheckthread.h \
    xpathinspector.h
