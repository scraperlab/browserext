#-------------------------------------------------
#
# Project created by QtCreator 2013-06-11T13:22:32
#
#-------------------------------------------------

QT       += core network webkit widgets gui webkitwidgets

TARGET = browser
TEMPLATE = app
CONFIG += release c++11


SOURCES += main.cpp


unix:!macx:!symbian: LIBS += -L$$PWD/../browserext-static -lphpbrowser

INCLUDEPATH += $$PWD/../browserext-static
DEPENDPATH += $$PWD/../browserext-static

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../browserext-static/libphpbrowser.a


