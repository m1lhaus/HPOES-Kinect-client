# Caffe module

- written in Python2
- listens for Kinect client on TCP
- receives segmented hand patch
- makes prediction of hand parameters (Caffe framework)
- sends hand parameters to Blender module through TCP

## Requirements:

- Python2 with Numpy
- [OpenCV2](http://opencv.org/downloads.html)
- [Caffe](https://github.com/BVLC/caffe) with PyCaffe Python wrapper

## How to:

You have to provide your trained Caffe module and deploy proto file. Then run the module.    
