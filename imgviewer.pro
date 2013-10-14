######################################################################
# Automatically generated by qmake (2.01a) s� 10. jun 21:25:03 2012
######################################################################

TEMPLATE = app
TARGET = imgviewer
DEPENDPATH += . debug src src/resources
INCLUDEPATH += .
LIBS += -lexif -llcms2
win32{
	LIBS += -lgdi32
	DEFINES += PORTABLE
}
QT += widgets
unix{
	QT += x11extras
	LIBS += -lxcb
}

# Input
HEADERS += src/imageCache.h \
           src/imageContainer.h \
           src/imageLoader.h \
           src/imageViewer.h \
           src/meta.h \
           src/qrect_extras.h \
           src/windowManager.h \
           src/fileManager.h \
           src/color.h
FORMS += src/controls_ui.ui
SOURCES += src/imageCache.cpp \
           src/imageContainer.cpp \
           src/imageLoader.cpp \
           src/imageViewer.cpp \
           src/main.cpp \
           src/meta.cpp \
           src/qrect_extras.cpp \
           src/windowManager.cpp \
           src/fileManager.cpp \
           src/color.cpp
RESOURCES += src/resources/resources.qrc

#Application icons
ICON = src/resources/appicon.icns
RC_FILE = src/resources/icon.rc


# Generate both debug and release on Linux (disabled)
# CONFIG += debug_and_release

# Position of binaries and build files
Release:DESTDIR = release
Release:UI_DIR = release/.ui
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.qrc

Debug:DESTDIR = debug
Debug:UI_DIR = debug/.ui
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.qrc