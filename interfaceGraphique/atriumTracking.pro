QT += widgets
QT += network
QT += core

win32 {
	LIBS += -lws2_32
}

SOURCES += \
    main.cpp \
    maingui.cpp

HEADERS += \
    maingui.h
