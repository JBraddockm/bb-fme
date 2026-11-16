APP_NAME = BB-FME

CONFIG += qt warn_on cascades10

QT += network

include(config.pri)

INCLUDEPATH += $$PWD/src

# Source providers (the two File-Manager-extension strategies) + shared bridge.
HEADERS += \
    $$PWD/src/RootShell.hpp \
    $$PWD/src/SourceProvider.hpp \
    $$PWD/src/LocalCopyProvider.hpp \
    $$PWD/src/WebDavProvider.hpp
SOURCES += \
    $$PWD/src/RootShell.cpp \
    $$PWD/src/LocalCopyProvider.cpp \
    $$PWD/src/WebDavProvider.cpp

# -lbb: bb::PpsObject etc.  -lbbdata: bb::data::JsonDataAccess (status PPS merge).
# -lbbsystem: bb::system::SystemToast (the QML on-screen delete notification).
LIBS += -lbb -lbbdata -lbbsystem
