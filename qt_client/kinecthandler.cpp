#include "kinecthandler.h"


KinectHandler::KinectHandler(int deviceId, int depthProcessor) :
    listener(libfreenect2::Frame::Color | libfreenect2::Frame::Depth | libfreenect2::Frame::Ir),
    undistorted(512, 424, 4), registered(512, 424, 4), depth2rgb(1920, 1080 + 2, 4)
{
    this->deviceId = deviceId;
    this->depthProcessor = depthProcessor;
}

KinectHandler::~KinectHandler()
{
    delete registration;
}


bool KinectHandler::initKinnect()
{
    //! [discovery]
    if (freenect2.enumerateDevices() == 0) {
        qCritical() << "No device connected/found!";
        return false;
    }
    std::string serial = freenect2.getDefaultDeviceSerialNumber();
    std::cout << "SERIAL: " << serial << std::endl;

    //! [discovery]

    if (depthProcessor == Processor::cpu) {
        if (!pipeline)
            pipeline = new libfreenect2::CpuPacketPipeline();
    } else if (depthProcessor == Processor::gl) {
#ifdef LIBFREENECT2_WITH_OPENGL_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::OpenGLPacketPipeline();
#else
        std::cout << "OpenGL pipeline is not supported!" << std::endl;
#endif
    } else if (depthProcessor == Processor::cl) {
#ifdef LIBFREENECT2_WITH_OPENCL_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::OpenCLPacketPipeline(deviceId);
#else
        std::cout << "OpenCL pipeline is not supported!" << std::endl;
#endif
    } else if (depthProcessor == Processor::cuda) {
#ifdef LIBFREENECT2_WITH_CUDA_SUPPORT
        if (!pipeline)
            pipeline = new libfreenect2::CudaPacketPipeline(deviceId);
#else
        std::cout << "CUDA pipeline is not supported!" << std::endl;
#endif
    }

    if (pipeline) {
        //! [open]
        dev = freenect2.openDevice(serial, pipeline);
        //! [open]
    } else {
        dev = freenect2.openDevice(serial);
    }

    if (dev == 0) {
        qCritical() << "Failure when opening device!";
        return false;
    }

    //! [listeners]
    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);
    //! [listeners]

    qDebug() << "Kinect initialized successfully";
    return true;
}

void KinectHandler::startKinect()
{
    qDebug() << "Starting Kinnect recording...";
    dev->start();
    std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
    std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;

    registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
    this->isRunning = true;
}

void KinectHandler::stopKinnect()
{
    qDebug() << "Stopping Kinnect recording...";
    dev->stop();
    dev->close();
}

void KinectHandler::grabFrame(cv::Mat &outputDepth, cv::Mat &outputRegistered)
{
    listener.waitForNewFrame(frames);
    libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
    //libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
    libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

    registration->apply(rgb, depth, &undistorted, &registered, true, &depth2rgb);

    cv::Mat(undistorted.height, undistorted.width, CV_32FC1, undistorted.data).copyTo(outputDepth);
    cv::Mat(registered.height, registered.width, CV_8UC4, registered.data).copyTo(outputRegistered);

    // norm depth to 0..65535
    outputDepth.convertTo(outputDepth, CV_32FC1);
    outputDepth /= 4500.0f;
    outputDepth *= 65535;
    outputDepth.convertTo(outputDepth, CV_16UC1);
    //cv::cvtColor(rgbd, rgbd, CV_RGBA2RGB);      // trim alpha channel

    listener.release(frames);
}
