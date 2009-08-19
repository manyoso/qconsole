# -------------------------------------------------
# Project created by QtCreator 2009-08-16T20:51:41
# -------------------------------------------------
TARGET = qconsole
TEMPLATE = app
SOURCES += main.cpp \
    qconsole.cpp \
    pseudoterminal.cpp
HEADERS += qconsole.h \
    pseudoterminal.h \
    pty.h
linux-* {
    SOURCES += pty_linux.cpp
}
win32-* {
    SOURCES += pty_cygwin.cpp
}
