######################################################################
# Automatically generated by qmake (2.01a) s� 10. jun 21:25:03 2012
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += . debug src src/resources
INCLUDEPATH += .
LIBS += -lexif
LIBS += -llcms2

# Input
HEADERS += src/imageCache.h \
           src/imageContainer.h \
           src/imageLoader.h \
           src/imageViewer.h \
           src/meta.h \
           src/qrect_extras.h \
           src/windowManager.h \
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
           src/color.cpp
RESOURCES += src/resources/resources.qrc
