#-------------------------------------------------
#
# Project created by QtCreator 2013-06-11T13:22:32
#
#-------------------------------------------------

QT       += core gui network webkit

TARGET = browser
TEMPLATE = app


SOURCES += main.cpp


unix:!macx:!symbian: LIBS += -L$$PWD/../browserext-static/Linux_Debug/ -lphpbrowser

INCLUDEPATH += $$PWD/../browserext-static
DEPENDPATH += $$PWD/../browserext-static

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../browserext-static/Linux_Debug/libphpbrowser.a


