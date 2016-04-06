# Segmentation module

- C++ class with worker method
- main object here is to find and segment hand (blob) from depth image

## Components:

Final interface has not been defined yet, but expected interface goes like this:

- class constructor takes some parameters depending on segmentation algorithms
    - i.e. segmented image width and height
- worker method `bool segmentImage(const cv::Mat &colorInput, const cv::Mat &depthInput, cv::Mat &output)` takes color (CV_U8C3) and depth image (CV_32FC1) from Kinect, finds hand ROI and stores segmented hand ROI (patch) in `output` Mat (CV_32FC1) where background has some constant value and hand ROI has pre-defined size (i.e. 96x96 px)
- when there is no hand to be found, method returns `false`, otherwise returns `true` as success

## Current demo segmentation:

Currently implemented segmentation works on simple color thresholding basis. Algorithm finds blue color blob (blue glove) and takes depth values from blob area as hand, and rest of the image is declared as background. Hand center-of-mass is taken as patch (image window) center, image is scaled depending on hand distance (constant hand size) and finally image window sliced is saved to output Mat. Optionally hand window could be normalized for Neural Network.   

## How to:

Segmentation module could be developed separately from Qt/Kinect client. Thats why there is a `main.cpp` file to develop and debug.   
