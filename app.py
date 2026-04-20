from flask import Flask, render_template, request
from flask_socketio import SocketIO
import cv2
import base64
import numpy as np
from ProcessData import RobotController
from face_tracking import process_frame
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

EXE_PATH = os.getenv("VLOGUS_EXE_PATH", os.path.join("cinematiqueInverse", "build", "MyProject.exe"))
SSL_CERT_PATH = os.getenv("VLOGUS_SSL_CERT", "cert.pem")
SSL_KEY_PATH = os.getenv("VLOGUS_SSL_KEY", "key.pem")
APP_HOST = os.getenv("VLOGUS_HOST", "0.0.0.0")
APP_PORT = int(os.getenv("VLOGUS_PORT", "5000"))
APP_DEBUG = os.getenv("VLOGUS_DEBUG", "0").lower() in ("1", "true", "yes")


def get_ssl_context():
    """Return SSL context tuple when cert files are available; otherwise run HTTP."""
    if os.path.exists(SSL_CERT_PATH) and os.path.exists(SSL_KEY_PATH):
        return (SSL_CERT_PATH, SSL_KEY_PATH)
    return None

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
        if not isinstance(data, str) or "," not in data:
            return

        frame_data = base64.b64decode(data.split(",", 1)[1])
        np_frame = np.frombuffer(frame_data, dtype=np.uint8)
        frame = cv2.imdecode(np_frame, cv2.IMREAD_COLOR)
        if frame is None:
            return
        
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
    RC.stopped_recording = False
    try:
        if exe_process is None:
            if not os.path.exists(EXE_PATH):
                raise FileNotFoundError(f"Executable not found: {EXE_PATH}")

            # Start the .exe in a new process (detached from Flask)
            creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if hasattr(subprocess, "CREATE_NEW_PROCESS_GROUP") else 0
            exe_process = subprocess.Popen(
                [EXE_PATH],
                creationflags=creationflags
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
    RC.stopped_recording = True
    RC.printData()
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
    ssl_enabled = get_ssl_context() is not None
    return render_template(
        'index.html',
        server_ip=request.host.split(":")[0],
        protocol='https' if ssl_enabled else 'http',
        server_port=APP_PORT,
    )

def handle_shutdown(signum, frame):
    """Cleanup on server shutdown"""
    print("\nShutting down gracefully...")
    RC.stopped_recording = True
    RC.printData()
    cleanup_exe_process()
    RC.close()
    raise SystemExit(0)

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

    ssl_context = get_ssl_context()
    if ssl_context is None:
        print("SSL certificates not found. Starting server without TLS.")

    socketio.run(
        app,
        host=APP_HOST,
        port=APP_PORT,
        debug=APP_DEBUG,
        ssl_context=ssl_context,
    )