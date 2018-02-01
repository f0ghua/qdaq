#-------------------------------------------------
#
# Project created by QtCreator 2018-01-14T09:21:28
#
#-------------------------------------------------

QT       += core gui widgets

lessThan(QT_MAJOR_VERSION, 5): error("This project needs Qt5")


TARGET = qdaq_widgets_test
TEMPLATE = app


SOURCES += main.cpp \
    led_widget.cpp

HEADERS  += \
    led_widget.h



win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../src/release/ -lQDaqWidgets
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../src/debug/ -lQDaqWidgets
else:unix: LIBS += -L$$OUT_PWD/../src/ -lQDaqWidgets

INCLUDEPATH += $$PWD/../src
DEPENDPATH += $$PWD/../src
