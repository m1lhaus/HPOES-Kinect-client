# Kinect/Qt module

- written in C++
- grabs data from Kinect v2 sensor (libfreenect2)
- converts raw data to OpenCV Mat
- applies hand hand segmentation algorithms
- sends segmented hand patch (window) to Caffe module through TC

## Components:

Module is split into four sub-modules:
- *mainwindow* - Qt GUI application form (main thread)
- *serverclient* - handles whole routine in separate thread (frame -> segment -> send)
- *kinecthandler* - handles Kinect setup and frame grabbing and conversion
- *segmentation* - module where hand segmentation class is stored

## Requirements:

Whole module should be platform independent. Tested on Linux, but libfreenect2, OpenCV and Qt works under Windows and MacOS too.

- [Qt5 with Qt Creator](http://www.qt.io/download) - to run Qt app from source (Qt4 should work too)
- [libfreenect2](https://github.com/OpenKinect/libfreenect2) - to connect Kinect v2
- [OpenCV2](http://opencv.org/downloads.html)

## How to:

Use Qt Creator to run `main.cpp` from source or use `qmake` to build your own binaries. Set Caffe module server IP and port and connect. To stop and quit just exit client GUI window.  
