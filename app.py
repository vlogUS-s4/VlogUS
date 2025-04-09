from flask import Flask, render_template
from flask_socketio import SocketIO
import cv2
import base64
import numpy as np
from ProcessData import RobotController
from face_tracking import process_frame
import time
import threading
from queue import Queue
import subprocess
import os
import signal

app = Flask(__name__)
socketio = SocketIO(app)

# Queue to hold only the latest frame (maxsize=1)
frame_queue = Queue(maxsize=1)  # Only store the latest frame
face_detector = cv2.FaceDetectorYN_create("face_detection_yunet_2023mar.onnx", "", (320, 320))
RC = RobotController()
exe_process = None  # Global variable to track the subprocess

# Path to your executable (modify as needed)
EXE_PATH = r'C:\Users\mccab\Desktop\gitProjet\VlogUS\cinematiqueInverse\build\MyProject.exe'

def cleanup_exe_process():
    """Helper function to safely terminate the .exe process"""
    global exe_process
    if exe_process:
        try:
            # Try graceful termination (SIGTERM on Unix, CTRL_C_EVENT on Windows)
            exe_process.terminate()
            exe_process.wait(timeout=2)  # Wait 2 seconds for clean exit
            print("EXE terminated successfully")
        except subprocess.TimeoutExpired:
            print("EXE not responding - killing forcefully")
            exe_process.kill()  # Force kill if not responding
        except Exception as e:
            print(f"Error stopping EXE: {e}")
        finally:
            exe_process = None

@socketio.on('video_frame')
def handle_video_frame(data):
    """Handle incoming video frames from the client"""
    try:
        frame_data = base64.b64decode(data[22:])
        np_frame = np.frombuffer(frame_data, dtype=np.uint8)
        frame = cv2.imdecode(np_frame, cv2.IMREAD_COLOR)
        
        # Update the queue with the latest frame
        if frame_queue.full():
            frame_queue.get_nowait()  # Discard old frame if queue is full
        frame_queue.put(frame, block=False)
    except Exception as e:
        print(f"Frame processing error: {e}")

@socketio.on('start_processing')
def start_processing():
    """Start the external .exe when client connects"""
    global exe_process
    try:
        if exe_process is None:
            # Start the .exe in a new process (detached from Flask)
            exe_process = subprocess.Popen(
                [EXE_PATH],
                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP  # Windows-specific flag
            )
            print(f"Started EXE with PID: {exe_process.pid}")
        else:
            print("EXE is already running")
    except Exception as e:
        print(f"Error starting EXE: {e}")
        socketio.emit('error', {'message': str(e)})

@socketio.on('stop_processing')
def stop_processing():
    """Stop the .exe when client disconnects"""
    cleanup_exe_process()

def process_latest_frame():
    """Background thread for frame processing"""
    while True:
        frame = frame_queue.get()  # Blocks until a frame is available
        try:
            faces = process_frame(frame, face_detector)
            RC.process(faces)
            if faces != (0, 0, 0, 0, 0):
                RC.printData()
        except Exception as e:
            print(f"Error in frame processing: {e}")
        finally:
            frame_queue.task_done()

@app.route('/')
def index():
    return render_template('index.html')

def handle_shutdown(signum, frame):
    """Cleanup on server shutdown"""
    print("\nShutting down gracefully...")
    cleanup_exe_process()
    os._exit(0)

if __name__ == "__main__":
    # Register signal handlers for clean shutdown
    signal.signal(signal.SIGINT, handle_shutdown)
    signal.signal(signal.SIGTERM, handle_shutdown)

    # Start frame processing thread
    processing_thread = threading.Thread(
        target=process_latest_frame,
        daemon=True  # Thread will exit when main program does
    )
    processing_thread.start()

    # Start Flask with SSL
    socketio.run(
        app,
        host='0.0.0.0',
        port=5000,
        debug=True,
        ssl_context=(
            r'C:\Users\mccab\Desktop\siteVlogus\cert.pem',
            r'C:\Users\mccab\Desktop\siteVlogus\key.pem'
        )
    )