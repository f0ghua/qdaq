#-------------------------------------------------
#
# Project created by QtCreator 2018-02-21T17:44:36
#
#-------------------------------------------------

QT       -= gui
QT       += script
CONFIG   += plugin
INCLUDEPATH  += ../../lib/daq ../../lib/core
LIBS += -lgpib
TARGET = $$qtLibraryTarget(qdaqlinuxgpibplugin)
TEMPLATE = lib
DESTDIR = ../../qdaq/plugins

DEFINES += LINUXGPIB_LIBRARY

SOURCES += linuxgpib.cpp

HEADERS += linuxgpib.h\
        linux-gpib_global.h

unix {
    target.path = $$[QT_INSTALL_PLUGINS]/qdaq
    INSTALLS += target
}

DISTFILES += \
    qdaqlinuxgpib.json


