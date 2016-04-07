# -*- coding: utf-8 -*-

"""
BLENDER ONLY (Python 3)

Blender script which listens on given port and waits for handshape data to visualize.
When data is received, handshape bone/joint rotations are updated.
"""


__version__ = "$Id: blender_visualize.py 361 2016-03-31 16:35:38Z herbig $"


import bpy

import socket
import time
import os
import json

import numpy as np

SERVER_HOST = '127.0.0.1'
SERVER_PORT = 10100
DATA_SHAPE = None
BONE_NAMES = []
CLIENT_TIMEOUT = 60      # sec
SERVER_TIMEOUT = 120     # sec


class ModalTimerOperator(bpy.types.Operator):
    """
    Operator which runs its self by a Blender timer
    """
    bl_idname = "wm.modal_timer_operator"
    bl_label = "Modal Timer Operator"

    _timer = None

    def modal(self, context, event):
        if event.type == 'TIMER':
            if not update_handshape():
                return self.cancel(context)

        elif event.type == 'ESC':
            return self.cancel(context)

        return {'PASS_THROUGH'}

    def execute(self, context):
        print("Executing ModalTimerOperator ...")
        wm = context.window_manager
        self._timer = wm.event_timer_add(0.033, context.window)      # 33ms = max 30FPS
        wm.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def cancel(self, context):
        print("Canceling ModalTimerOperator ...")
        context.window_manager.event_timer_remove(self._timer)
        self._timer = None
        close_server()
        return {'CANCELLED'}


def register():
    print("Registering modal timer operator in Blender")
    bpy.utils.register_class(ModalTimerOperator)


def unregister():
    print("Unregistering modal timer operator in Blender")
    bpy.utils.unregister_class(ModalTimerOperator)


def init_hand_model():
    print("Initializing hand model to Quaternions")
    for bone in bpy.data.objects['Armature'].pose.bones:
        bone.rotation_mode = "QUATERNION"


def recv_data():
    """
    Reads data from socket.
    :return: received data as numpy array
    :rtype: np.ndarray
    """
    remaining_bytes = DATA_SHAPE * 4

    recv_buffer = []
    while remaining_bytes:
        recv = predictor_socket.recv(min(1024, remaining_bytes))
        if recv:
            recv_buffer.append(recv)
            remaining_bytes -= len(recv)
        else:
            # raise Exception("Expected some data, but got none ... remaining_bytes: %d" % remaining_bytes)
            print("Expected some data, but got none ... remaining_bytes: %d" % remaining_bytes)
            return None

    # send OK flag back to predictor
    predictor_socket.sendall(b"OK")

    recv_buffer = b"".join(recv_buffer)
    rot_data = np.frombuffer(recv_buffer, dtype=np.float32)
    return rot_data


# CALLED FROM BLENDER TIMER OPERATOR (worker)
def update_handshape():
    """
    Method periodically called from blender timer operator (as worker).
    Method has blocking design -> returns only while data are received => there is something to visualize.
    Method receives data from socket, sets bone/joint rotations and returns.
    :return: False when no data received = client finished
    :rtype: bool
    """
    rot_data = recv_data()
    if rot_data is None:
        print("No data received, client finished or failed -> cancel timer operator")
        return False

    rot_data = rot_data.reshape((DATA_SHAPE/4, 4))

    print("Time: %.3f | Received data, updating pose" % time.time())
    load_handshape(rot_data)
    return True


def load_handshape(rot_data):
    """
    Sets given rotations to corresponding bone/joint.
    :param rot_data: numpy array with bone rotations
    """
    armature = bpy.data.objects['Armature']
    for i, bone_name in enumerate(BONE_NAMES):
        armature.pose.bones[bone_name].rotation_quaternion[:] = rot_data[i, :]


def close_server():
    """
    Called when clients side connection is refused (finished, terminated) to release sockets (ports).
    Needs to be called otherwise ports won't be released until Blender application is terminated!
    """
    try:
        predictor_socket.close()        # conn may not exist
    except Exception():
        print("predictor_socket already closed or not exists!")
    server_socket.close()
    print("server_socked closed!")

if __name__ == '__main__':
    os.system('cls' if os.name == 'nt' else 'clear')            # clear blender console
    init_hand_model()

    # ------------- init listener -------------------------
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((SERVER_HOST, SERVER_PORT))
    server_socket.settimeout(SERVER_TIMEOUT)

    try:
        server_socket.listen(1)

        print("Server is listening on %s:%s ..." % (SERVER_HOST, SERVER_PORT))
        print("\n* WAITING FOR CLIENT CONNECTION *\n")

        # ---  wait for client ---
        predictor_socket, addr = server_socket.accept()
        predictor_socket.settimeout(CLIENT_TIMEOUT)
        print("Connection accepted at address:", addr)
        # ------------------------

        try:
            # --- handshake ---
            data = predictor_socket.recv(1024)
            if data and data.decode() == "HELLO":
                predictor_socket.sendall(b"HELLO")
            else:
                raise Exception("HELLO expected, but got '%s' instead" % data)
            # -----------------

            # --- get list of bone names ---
            print("Waiting for information about bones to visualize ...")

            data = predictor_socket.recv(1024)
            try:
                BONE_NAMES = json.loads(data.decode())
                print("Bones:", BONE_NAMES)
            except Exception:
                raise Exception("Bone names in JSON expected, but got '%s' instead!" % data)
            predictor_socket.sendall(b"OK")
            # ------------------------------

            # --- get shape of the data ---
            print("Waiting for information about data shape ...")
            data = predictor_socket.recv(1024)
            if data and data.decode().startswith("SHAPE"):
                _, DATA_SHAPE = data.decode().split(" ")
                DATA_SHAPE = int(DATA_SHAPE)
                print("OK, expecting data shape (%d,) and dtype float32" % DATA_SHAPE)
            else:
                raise Exception("SHAPE expected, but got %s instead" % data)

            if len(BONE_NAMES) != (DATA_SHAPE / 4):
                raise Exception("Input data are in wrong format. "
                                "len(bone_list) = %s (must be X), "
                                "data_shape = %s (must be X) " % (len(BONE_NAMES), DATA_SHAPE))

            predictor_socket.sendall(b"OK")
            # -------------------------------

            print("Waiting for data confirmation ...")
            data = predictor_socket.recv(1024)
            if not data or data.decode() != "DATA":
                raise Exception("DATA flag expected, but got %s instead" % data)
            predictor_socket.sendall(b"OK")

            # ------- init modal timer operator --------------------
            register()
            bpy.ops.wm.modal_timer_operator()

        except Exception:
            predictor_socket.close()
            raise
    except Exception:
        server_socket.close()
        raise

    print("Done")
