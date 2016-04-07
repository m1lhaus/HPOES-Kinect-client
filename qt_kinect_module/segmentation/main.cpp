//! [headers]
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

#include "segmentation.h"
//! [headers]

using namespace std;

enum Processor {
    cl, gl, cpu, cuda
};

bool protonect_shutdown = false; // Whether the running application should shut down.
int deviceId = -1;
int depthProcessor = Processor::cuda;

void sigint_handler(int s) {
    protonect_shutdown = true;
}

int main() {
    //! [context]
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = nullptr;
    libfreenect2::PacketPipeline *pipeline = nullptr;
    //! [context]

    //! [discovery]
    if (freenect2.enumerateDevices() == 0) {
        cout << "no device connected!" << endl;
        return -1;
    }
    string serial = freenect2.getDefaultDeviceSerialNumber();
    cout << "SERIAL: " << serial << endl;
    //! [discovery]

    if (depthProcessor == Processor::cpu) {
        if (!pipeline)
            pipeline = new libfreenect2::CpuPacketPipeline();
    } else if (depthProcessor == Processor::gl) {
#ifdef LIBFREENECT2_WITH_OPENGL_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::OpenGLPacketPipeline();
#else
        cout << "OpenGL pipeline is not supported!" << endl;
#endif
    } else if (depthProcessor == Processor::cl) {
#ifdef LIBFREENECT2_WITH_OPENCL_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::OpenCLPacketPipeline(deviceId);
#else
        cout << "OpenCL pipeline is not supported!" << endl;
#endif
    } else if (depthProcessor == Processor::cuda) {
#ifdef LIBFREENECT2_WITH_CUDA_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::CudaPacketPipeline(deviceId);
#else
        cout << "CUDA pipeline is not supported!" << endl;
#endif
    }

    if (pipeline) {
        dev = freenect2.openDevice(serial, pipeline);
    } else {
        dev = freenect2.openDevice(serial);
    }
    if (dev == 0) {
        cout << "failure opening device!" << endl;
        return -1;
    }

    signal(SIGINT, sigint_handler);
    protonect_shutdown = false;

    //! [listeners]
    libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color | libfreenect2::Frame::Depth | libfreenect2::Frame::Ir);
    libfreenect2::FrameMap frames;
    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);
    //! [listeners]

    //! [start]
    dev->start();
    cout << "device serial: " << dev->getSerialNumber() << endl;
    cout << "device firmware: " << dev->getFirmwareVersion() << endl;
    //! [start]

    //! [registration setup]
    libfreenect2::Registration *registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
    libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4), depth2rgb(1920, 1080 + 2, 4);
    //! [registration setup]

    cv::Mat rgbMat, irMat, depthMat, depthMatUndistorted, colorToDepthMat, depthToColorMat, segmentedMat;
    Segmentation segmentation(96, 96);

    double t0, t1, exec_time, fps;
    double minVal, maxVal;
    bool seg;

    //! [loop start]
    while (!protonect_shutdown) {
        // wait for new frame
        listener.waitForNewFrame(frames);
        libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
//        libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
        libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

        // convert to OpenCV mat
        cv::Mat((int) rgb->height, (int) rgb->width, CV_8UC4, rgb->data).copyTo(rgbMat);
//        cv::Mat(ir->height, ir->width, CV_32FC1, ir->data).copyTo(irMat);
        cv::Mat((int) depth->height, (int) depth->width, CV_32FC1, depth->data).copyTo(depthMat);

        //! [registration]
        registration->apply(rgb, depth, &undistorted, &registered, true, &depth2rgb);
        //! [registration]

        cv::Mat((int) undistorted.height, (int) undistorted.width, CV_32FC1, undistorted.data).copyTo(depthMatUndistorted);
        cv::Mat((int) registered.height, (int) registered.width, CV_8UC4, registered.data).copyTo(colorToDepthMat);
        cv::Mat((int) depth2rgb.height, (int) depth2rgb.width, CV_32FC1, depth2rgb.data).copyTo(depthToColorMat);

        // norm depth to 0..65535
        depthMatUndistorted.convertTo(depthMatUndistorted, CV_32FC1);
        cvtColor(colorToDepthMat, colorToDepthMat, CV_BGRA2BGR);      // trim alpha channel

//        t0 = (double) cv::getTickCount();
        seg = segmentation.segmentImage(colorToDepthMat, depthMatUndistorted, segmentedMat);
//        exec_time = ((double) cv::getTickCount() - t0) / cv::getTickFrequency() * 1000;
//        cout << "Seg exec time: " << exec_time << "ms" << endl;

        cv::imshow("registered", colorToDepthMat);
        cv::imshow("depth", depthMatUndistorted/4500.f);
        if (seg) {
            cv::imshow("segmented", segmentedMat);
        }


        int key = cv::waitKey(1);
        protonect_shutdown = protonect_shutdown || (key > 0 && ((key & 0xFF) == 27)); // shutdown on escape

        //! [loop end]
        listener.release(frames);
    }
    //! [loop end]

    //! [stop]
    dev->stop();
    dev->close();
    //! [stop]

    delete registration;

    cout << "Goodbye World!" << endl;
    return 0;
}