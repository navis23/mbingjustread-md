QT += core gui quick quickcontrols2 testlib dbus
CONFIG += c++17 testcase
TARGET = mbingjustread-md_tests
TEMPLATE = app

INCLUDEPATH += ../src
LIBS += -lmd4c
DEFINES += FIXTURES=\\\"$$PWD/fixtures\\\"

HEADERS += ../src/backend.h ../src/filepicker.h ../src/highlighter.h ../src/markdown.h \
    ../src/theme.h ../src/translator.h
SOURCES += mbingjustread-md_tests.cpp ../src/backend.cpp ../src/highlighter.cpp ../src/markdown.cpp \
    ../src/theme.cpp ../src/translator.cpp
