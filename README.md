# HPOES deploy modules:

- Kinect/Qt module
- Caffe module
- Blender module

For more information head to module folder. There is a README file with detailed informations.

## Kinect/Qt module

- written in C++
- grabs data from Kinect v2 sensor (libfreenect2)
- converts raw data to OpenCV Mat
- applies hand hand segmentation algorithms
- sends segmented hand patch (window) to Caffe module through TCP

## Caffe module

- written in Python2
- listens for Kinect client on TCP
- receives segmented hand patch
- makes prediction of hand parameters (Caffe framework)
- sends hand parameters to Blender module through TCP

## Blender module

- written in Python3
- runs as Blender script
- receives hand parameters
- renders estimated pose

## Dummy server

- acts like Caffe server
- to Qt/Kinect client debug purposes
