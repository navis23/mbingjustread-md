QT += core gui qml quick quickcontrols2 dbus

CONFIG += c++17 release
TARGET = mbingjustread-md
TEMPLATE = app

# md4c is the Markdown parser Qt itself depends on, so it is always present.
LIBS += -lmd4c

HEADERS += \
    src/backend.h \
    src/filepicker.h \
    src/highlighter.h \
    src/markdown.h \
    src/portalfilepicker.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/highlighter.cpp \
    src/markdown.cpp \
    src/portalfilepicker.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
