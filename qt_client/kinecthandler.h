#ifndef KINECTHANDLER_H
#define KINECTHANDLER_H

#include <QObject>
#include <QDebug>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <time.h>
#include <signal.h>
#include <opencv2/opencv.hpp>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>


enum Processor {
    cl, gl, cpu, cuda
};

class KinectHandler
{
public:
    KinectHandler(int deviceId=-1, int depthProcessor=Processor::cuda);
    ~KinectHandler();
    void sigint_handler(int s);
    bool initKinnect();
    void startKinect();
    void stopKinnect();
    void grabFrame(cv::Mat &outputDepth, cv::Mat &outputRegistered);



    bool isRunning = false;
    int deviceId = -1;
    int depthProcessor = Processor::cuda;

    //! [context]
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = nullptr;
    libfreenect2::PacketPipeline *pipeline = nullptr;
    //! [context]

    libfreenect2::SyncMultiFrameListener listener;
    libfreenect2::FrameMap frames;

    libfreenect2::Registration *registration;
    libfreenect2::Frame undistorted, registered, depth2rgb;


    //cv::Mat rgbmat, depthmat, depthmatUndistorted, irmat, rgbd, rgbd2, normedDepth, final;

    double t0, t1, diff, fps;
    double minVal, maxVal;



};

#endif // KINECTHANDLER_H
