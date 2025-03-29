from flask import Flask, render_template
from flask_socketio import SocketIO
import cv2
import base64
import numpy as np
from ProcessData import RobotController
import time
import threading
import ctypes
import os

app = Flask(__name__)
socketio = SocketIO(app)

# Load the shared library with full path
dll_path = r'C:\Users\mccab\Desktop\gitProjet\VlogUS\face_tracking\build\lib\libface_tracking.dll'
if not os.path.exists(dll_path):
    print(f"Error: {dll_path} does not exist. Please ensure the DLL is built correctly.")
    exit(1)

try:
    face_tracking_lib = ctypes.WinDLL(dll_path)
except OSError as e:
    print(f"Error loading libface_tracking.dll: {e}")
    print("Ensure all dependencies (OpenCV DLLs, MinGW runtime DLLs) are in PATH or in the same directory as the DLL.")
    exit(1)

# Define the ProcessFrameResult struct to match the C++ struct
class ProcessFrameResult(ctypes.Structure):
    _fields_ = [
        ('cx', ctypes.c_float),
        ('cy', ctypes.c_float),
        ('w', ctypes.c_float),
        ('h', ctypes.c_float),
        ('orientation', ctypes.c_int)
    ]

# Set up function signatures
face_tracking_lib.init_classifiers.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
face_tracking_lib.init_classifiers.restype = None
face_tracking_lib.process_frame.argtypes = [ctypes.c_char_p]
face_tracking_lib.process_frame.restype = ProcessFrameResult

# Initialize the classifiers
face_cascade_path = b"C:/Users/mccab/Desktop/siteVlogus/haarcascade_frontalface_default.xml"
profile_cascade_path = b"C:/Users/mccab/Desktop/siteVlogus/haarcascade_profileface.xml"
try:
    face_tracking_lib.init_classifiers(face_cascade_path, profile_cascade_path)
except Exception as e:
    print(f"Error initializing classifiers: {e}")
    exit(1)

# Global variables
latest_frame = None  # Stores the decoded cv2 frame (for potential future use)
latest_encoded_frame = None  # Stores the Base64-encoded frame string
frame_count = 0
RC = RobotController()
frame_lock = threading.Lock()  # For thread safety

@socketio.on('video_frame')
def handle_video_frame(data):
    global frame_count, latest_frame, latest_encoded_frame
    start_time = time.time()
    frame_count += 1
    if frame_count >= 2:
        try:
            # Store the Base64-encoded frame string
            encoded_frame = data  # e.g., "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQE..."
            # Update global variables with thread safety
            with frame_lock:
                latest_encoded_frame = encoded_frame
            frame_count = 0
        except Exception as e:
            print(f"Error decoding frame: {e}")
        finally:
            end_time = time.time()

def process_latest_frame():
    global latest_encoded_frame
    while True:
        with frame_lock:
            if latest_encoded_frame is None:
                time.sleep(0.033)
                continue
            encoded_frame = latest_encoded_frame

        try:
            # Pass the Base64-encoded frame directly to the shared library
            start_time = time.time()
            result = face_tracking_lib.process_frame(encoded_frame.encode('utf-8'))
            faces = (result.cx, result.cy, result.w, result.h, result.orientation)

            # Process the results with RobotController
            RC.process(faces)
            if faces != (0, 0, 0, 0, 0):
                RC.printData()
        except Exception as e:
            print(f"Error processing frame: {e}")
        finally:
            end_time = time.time()
        time.sleep(0.033)

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == "__main__":
    # Start the frame processing thread
    rtsp_thread = threading.Thread(target=process_latest_frame, daemon=True)
    rtsp_thread.start()
    # Run Flask with SSL
    socketio.run(app, host='0.0.0.0', port=5000, debug=True,
                 ssl_context=(r'C:\Users\mccab\Desktop\siteVlogus\cert.pem',
                              r'C:\Users\mccab\Desktop\siteVlogus\key.pem'))