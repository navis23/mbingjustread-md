QT += core gui qml quick quickcontrols2 testlib dbus
CONFIG += c++17
CONFIG -= testcase
TARGET = shot
TEMPLATE = app

INCLUDEPATH += ../../src
LIBS += -lmd4c

HEADERS += ../../src/backend.h ../../src/filepicker.h ../../src/highlighter.h \
    ../../src/markdown.h ../../src/theme.h ../../src/translator.h
SOURCES += shot.cpp ../../src/backend.cpp ../../src/highlighter.cpp \
    ../../src/markdown.cpp ../../src/theme.cpp ../../src/translator.cpp
RESOURCES += ../../src/resources.qrc
