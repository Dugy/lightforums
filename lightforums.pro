TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    mainwindow.cpp \
    user.cpp \
    post.cpp \
    userlist.cpp \
    settings.cpp \
    translation.cpp \
	defines.cpp
unix: LIBS += -lwt -lwthttp

INCLUDEPATH += /usr/include/Wt
DEPENDPATH += /usr/include/Wt

HEADERS += \
    mainwindow.h \
    user.h \
    post.h \
    defines.h \
    userlist.h \
    atomic_unordered_map.h \
    atomic_vector.h \
    atomic_queue.h \
    settings.h \
	translation.h
