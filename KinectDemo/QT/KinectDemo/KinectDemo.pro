#-------------------------------------------------
#
# Project created by QtCreator 2016-02-22T15:29:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KinectDemo
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui

INCLUDEPATH += $$(OPEN_NI_INCLUDE)

LIBS += -L$$(OPEN_NI_LIB) -lopenNI

