cmake_minimum_required(VERSION 3.3)
project(kinect_seg)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(/usr/local/include)
include_directories(/home/milan/Programy/libfreenect2/install/include)

link_directories(/home/milan/Programy/libfreenect2/install/lib)
link_directories(/usr/local/lib)

set(SOURCE_FILES main.cpp segmentation.cpp)
add_executable(kinect_seg ${SOURCE_FILES})

# opencv_ml, opencv_video, opencv_features2d, opencv_calib3d, opencv_objdetect,
# opencv_contrib, opencv_legacy, opencv_flann, opencv_nonfree
target_link_libraries(kinect_seg opencv_core opencv_imgproc opencv_highgui freenect2)

