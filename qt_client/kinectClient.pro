#-------------------------------------------------
#
# Project created by QtCreator 2016-03-18T19:29:12
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = kinectClient
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serverclient.cpp \
    kinecthandler.cpp \
    segmentation/segmentation.cpp

HEADERS  += mainwindow.h \
    serverclient.h \\
    kinecthandler.h \
    segmentation/segmentation.h

FORMS    += mainwindow.ui

INCLUDEPATH += /usr/local/include/opencv2
LIBS += -L/usr/local/lib
LIBS += -lopencv_core
LIBS += -lopencv_imgproc
LIBS += -lopencv_highgui
#LIBS += -lopencv_ml
#LIBS += -lopencv_video
#LIBS += -lopencv_features2d
#LIBS += -lopencv_calib3d
#LIBS += -lopencv_objdetect
#LIBS += -lopencv_contrib
#LIBS += -lopencv_legacy
#LIBS += -lopencv_flann
#LIBS += -lopencv_nonfree

INCLUDEPATH += /home/milan/Programy/libfreenect2/install/include
LIBS += -L/home/milan/Programy/libfreenect2/install/lib
LIBS += -lfreenect2
