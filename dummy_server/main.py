# -*- coding: utf-8 -*-

import os
import sys
import socket

import cv2
import numpy as np

SERVER_HOST = "localhost"
SERVER_PORT = 10000
BLENDER_HOST = "localhost"
BLENDER_PORT = 10100

VISUALIZE = True


def init_server(hostname='localhost', port=10000):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (hostname, port)
    print 'starting up server on %s port %s' % server_address
    server_socket.bind(server_address)
    return server_socket


def init_client(hostname='localhost', port=10000):
    return None


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
    pass


def nn_forward_pass(image):
    return None


def send_to_blender(data):
    pass


def handle_kinect_client():
    try:
        server_socket.listen(1)                     # Listen for incoming connections
        print 'waiting for a connection'
        kinect_socket, client_address = server_socket.accept()
        print 'connection from', client_address

        try:
            width, height, dtype = kinect_hello_protocol(kinect_socket)
            img_byte_size = width*height*dtype.itemsize      # image bytesize

            while True:
                remaining_bytes = img_byte_size

                recv_buffer = []
                while remaining_bytes:
                    data = kinect_socket.recv(min(8192, remaining_bytes))
                    if data:
                        recv_buffer.append(data)
                        remaining_bytes -= len(data)
                    else:
                        raise Exception("Expected some data, but got none ... remaining_bytes: %d" % remaining_bytes)

                img = np.fromstring("".join(recv_buffer), dtype=dtype).reshape((height, width))
                data = nn_forward_pass(img)
                send_to_blender(data)

                if VISUALIZE:
                    cv2.imshow("Received image", img)
                    cv2.waitKey(1)      # has to be non-zero

        finally:
            kinect_socket.close()       # Clean up the connection
            if VISUALIZE:
                cv2.destroyAllWindows()
    finally:
        server_socket.close()


if __name__ == '__main__':
    server_socket = init_server(SERVER_HOST, SERVER_PORT)
    blender_socket = init_client(BLENDER_HOST, BLENDER_PORT)
    handle_kinect_client()
