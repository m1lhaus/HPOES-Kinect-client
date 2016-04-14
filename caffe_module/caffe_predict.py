# -*- coding: utf-8 -*-

import os
import sys
import time
import socket
import json

import cv2
import caffe
import numpy as np

SERVER_HOST = "localhost"
SERVER_PORT = 10000
BLENDER_HOST = "localhost"
BLENDER_PORT = 10100

DEPLOY = "/home/milan/_HPOES_/nn_deploy.proto"
MODEL = "/home/milan/_HPOES_/model_iter_20000.caffemodel"
GPU = True

VISUALIZE = True
BONES = ["finger1joint1", "finger1joint2", "finger1joint3", "finger2joint1", "finger2joint2", "finger2joint3",
         "finger3joint1", "finger3joint2", "finger3joint3", "finger4joint1", "finger4joint2", "finger4joint3",
         "finger5joint1", "finger5joint2", "finger5joint3", "root"]


def init_server(hostname='localhost', port=10000):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (hostname, port)
    print 'starting up server on %s port %s' % server_address
    s.bind(server_address)
    return s


def init_client(hostname='localhost', port=10000):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (hostname, port)
    print "Connecting to Blender server on", server_address
    s.connect(server_address)
    return s


def kinect_hello_protocol(kinect_socket):
    data = kinect_socket.recv(1024)
    if data == "HELLO":
        kinect_socket.sendall("HELLO")
    else:
        raise Exception("HELLO expected, but got '%s' instead" % data)

    data = kinect_socket.recv(1024)
    if data and data.startswith("WIDTH"):
        _, width = data.split(" ")
        width = int(width)
        print "OK, expecting image width", width
        kinect_socket.sendall("OK")
    else:
        raise Exception("WIDTH expected, but got '%s' instead" % data)

    data = kinect_socket.recv(1024)
    if data and data.startswith("HEIGHT"):
        _, height = data.split(" ")
        height = int(height)
        print "OK, expecting image height", height
        kinect_socket.sendall("OK")
    else:
        raise Exception("HEIGHT expected, but got '%s' instead" % data)

    data = kinect_socket.recv(1024)
    if data and data.startswith("DTYPE"):
        _, dtype = data.split(" ")
        dtype = np.dtype(dtype)
        print "OK, expecting image dtype", dtype
        kinect_socket.sendall("OK")
    else:
        raise Exception("DTYPE expected, but got '%s' instead" % data)

    data = kinect_socket.recv(1024)
    if data == "DATA":
        print "OK, now waiting for image data .. expected bytesize", width*height*dtype.itemsize
        kinect_socket.sendall("OK")
    else:
        raise Exception("DATA expected, but got '%s' instead" % data)

    return width, height, dtype


def blender_hello_protocol():
    # --- handshake ---
    print("Sending HELLO to Blender ...")
    blender_socket.sendall("HELLO")
    data = blender_socket.recv(1024)
    if data != "HELLO":
        raise Exception("HELLO expected, but got '%s' instead" % data)
    # -----------------

    # --- send list of bones ---
    print("Sending bone names to Blender ...")
    data = json.dumps(BONES)
    blender_socket.sendall(data)
    data = blender_socket.recv(1024)
    if data != "OK":
        raise Exception("OK expected, but got '%s' instead" % data)
    # ---------------------------

    # --- send info about shape ---
    print("Sending data shape to Blender ...")
    data = "SHAPE " + str(len(BONES)*4)
    blender_socket.sendall(data)
    data = blender_socket.recv(1024)
    if data != "OK":
        raise Exception("OK expected, but got '%s' instead" % data)
    # ---------------------------

    # --- send info about dtype ---
    print("Sending dtype info to Blender ...")
    data = "DTYPE float32"
    blender_socket.sendall(data)
    data = blender_socket.recv(1024)
    if data != "OK":
        raise Exception("OK expected, but got '%s' instead" % data)
    # ---------------------------

    # --- send data confirmation flag ---
    print("Sending data flag to Blender ...")
    blender_socket.sendall("DATA")
    data = blender_socket.recv(1024)
    if data != "OK":
        raise Exception("OK expected, but got '%s' instead" % data)
    # ---------------------------

    print("Blender hello protocol completed")


def send_to_blender(pred_data):
    pred_data = pred_data.tostring()
    blender_socket.sendall(pred_data)
    data = blender_socket.recv(1024)
    if data != "OK":
        raise Exception("OK expected, but got '%s' instead" % data)


def handle_kinect_client():
    print 'waiting for a connection'

    kinect_socket, client_address = server_socket.accept()
    print 'connection from', client_address
    try:
        width, height, dtype = kinect_hello_protocol(kinect_socket)
        img_byte_size = width*height*dtype.itemsize      # image bytesize

        while True:
            # grab image
            remaining_bytes = img_byte_size
            recv_buffer = []
            while remaining_bytes:
                data = kinect_socket.recv(min(8192, remaining_bytes))
                if data:
                    recv_buffer.append(data)
                    remaining_bytes -= len(data)
                else:
                    raise Exception("Expected some data, but got none ... remaining_bytes: %d" % remaining_bytes)
            image = np.fromstring("".join(recv_buffer), dtype=dtype).reshape((1, height, width))
            kinect_socket.sendall("OK")
            print "Image received from Kinect client"

            # predict hand params
            # t0 = time.time()
            pred_data = net.forward(data=image)['fc6']
            # t1 = time.time()
            # print pred_data.shape, (t1-t0)*1000
            print "Rotation data predicated"

            send_to_blender(pred_data)
            print "Data sent to Blender"

            if VISUALIZE:
                cv2.imshow("Received image", image.squeeze())
                cv2.waitKey(5)      # has to be non-zero

    finally:
        kinect_socket.close()       # Clean up the connection
        cv2.destroyAllWindows()


if __name__ == '__main__':
    if GPU:
        caffe.set_mode_gpu()
    else:
        caffe.set_mode_cpu()
    net = caffe.Net(DEPLOY, MODEL, caffe.TEST)

    blender_socket = init_client(BLENDER_HOST, BLENDER_PORT)
    try:
        blender_hello_protocol()

        server_socket = init_server(SERVER_HOST, SERVER_PORT)
        try:
            server_socket.listen(1)         # start listening for incoming connections
            handle_kinect_client()
        finally:
            server_socket.close()
    finally:
        blender_socket.close()
