TEMPLATE = app
TARGET = imgviewer
DEPENDPATH += . debug src src/resources
INCLUDEPATH += .
LIBS += -lexif -llcms2 -lpng -lz -ljpeg
win32{
	LIBS += -lgdi32
	DEFINES += PORTABLE
	DEFINES += WIN_TOOLBAR
	QT += winextras
}
QT += widgets concurrent
unix{
	QT += x11extras
	LIBS += -lxcb
}

#Release debugging
#QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
#QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

# C++14 support
QMAKE_CXXFLAGS += -std=c++14

# Input
HEADERS += src/imageContainer.h \
           src/imageLoader.h \
           src/meta.h \
           src/windowManager.h \
           src/fileManager.h
FORMS += src/controls_ui.ui
SOURCES += src/imageContainer.cpp \
           src/imageLoader.cpp \
           src/main.cpp \
           src/meta.cpp \
           src/windowManager.cpp \
           src/fileManager.cpp
RESOURCES += src/resources/resources.qrc

#FileSystem
HEADERS += src/FileSystem/ExtensionChecker.hpp
SOURCES += src/FileSystem/ExtensionChecker.cpp

#Viewer
HEADERS += src/viewer/colorManager.h \
           src/viewer/imageCache.h \
           src/viewer/imageViewer.h \
           src/viewer/Orientation.hpp \
           src/viewer/ZoomBox.hpp \
           src/viewer/qrect_extras.h
SOURCES += src/viewer/colorManager.cpp \
           src/viewer/imageCache.cpp \
           src/viewer/imageViewer.cpp \
           src/viewer/ZoomBox.cpp \
           src/viewer/qrect_extras.cpp

#ImageReader
HEADERS += src/ImageReader/ImageReader.hpp \
           src/ImageReader/AReader.hpp \
           src/ImageReader/ReaderJpeg.hpp \
           src/ImageReader/ReaderPng.hpp \
           src/ImageReader/ReaderQt.hpp
SOURCES += src/ImageReader/ImageReader.cpp \
           src/ImageReader/AReader.cpp \
           src/ImageReader/ReaderJpeg.cpp \
           src/ImageReader/ReaderPng.cpp \
           src/ImageReader/ReaderQt.cpp

#Application icons
ICON = src/resources/appicon.icns
RC_FILE = src/resources/icon.rc


# Generate both debug and release on Linux (disabled)
CONFIG += debug_and_release

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
