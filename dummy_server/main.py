# -*- coding: utf-8 -*-

import os
import sys

import socket
import sys

import cv2

import numpy as np
# Create a TCP/IP socket
from pandas.core.reshape import wide_to_long

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('localhost', 10000)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

width, height, dtype = None, None, None

try:

    while True:
        # Wait for a connection
        print >>sys.stderr, 'waiting for a connection'
        connection, client_address = sock.accept()

        try:
            print >>sys.stderr, 'connection from', client_address

            data = connection.recv(1024)
            if data == "HELLO":
                connection.sendall("HELLO")
            else:
                raise Exception("HELLO expected, but got '%s' instead" % data)

            data = connection.recv(1024)
            if data and data.startswith("WIDTH"):
                _, width = data.split(" ")
                width = int(width)
                print "OK, expecting image width", width
                connection.sendall("OK")
            else:
                raise Exception("WIDTH expected, but got '%s' instead" % data)

            data = connection.recv(1024)
            if data and data.startswith("HEIGHT"):
                _, height = data.split(" ")
                height = int(height)
                print "OK, expecting image height", height
                connection.sendall("OK")
            else:
                raise Exception("HEIGHT expected, but got '%s' instead" % data)

            data = connection.recv(1024)
            if data and data.startswith("DTYPE"):
                _, dtype = data.split(" ")
                dtype = np.dtype(dtype)
                print "OK, expecting image dtype", dtype
                connection.sendall("OK")
            else:
                raise Exception("DTYPE expected, but got '%s' instead" % data)

            data = connection.recv(1024)
            if data == "DATA":
                print "OK, now waiting for image data .. expected bytesize", width*height*dtype.itemsize
                connection.sendall("OK")
            else:
                raise Exception("DATA expected, but got '%s' instead" % data)

            img_bytesize = width*height*dtype.itemsize


            while True:
                remaining_bytes = img_bytesize

                buffer = []
                while remaining_bytes > 0:
                    data = connection.recv(8192)
                    if data:
                        buffer.append(data)
                        remaining_bytes -= len(data)
                    else:
                        print "Expected some data, but got none ... remaining_bytes: %d" % remaining_bytes
                        raise Exception()

                byte_img = "".join(buffer)
                img = np.fromstring(byte_img, dtype=dtype).reshape((height, width))

                cv2.imshow("Received image", img)
                cv2.waitKey(1)

            cv2.destroyAllWindows()

        finally:
            # Clean up the connection
            connection.close()

finally:
    sock.close()