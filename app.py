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

app = Flask(__name__)
socketio = SocketIO(app)

# Queue to hold only the latest frame (maxsize=1)
frame_queue = Queue(maxsize=1)  # Only store the latest frame
face_detector = cv2.FaceDetectorYN_create("face_detection_yunet_2023mar.onnx", "", (320, 320))
RC = RobotController()

@socketio.on('video_frame')
def handle_video_frame(data):
    start_time = time.time()
    frame_data = base64.b64decode(data[22:])
    np_frame = np.frombuffer(frame_data, dtype=np.uint8)
    frame = cv2.imdecode(np_frame, cv2.IMREAD_COLOR)
    # Always replace the queue contents with the latest frame
    try:
        # If queue is full, remove the old frame first
        if frame_queue.full():
            frame_queue.get_nowait()  # Discard the old frame
            frame_queue.task_done()
        frame_queue.put(frame, block=False)
        end_time = time.time()
        print(f"Frame decoding time: {1000*(end_time-start_time)} ms")
    except queue.Full:
        print("Unexpected queue issue (should not happen with maxsize=1)")

def process_latest_frame():
    while True:
        # Get the latest frame (blocks until a frame is available)
        frame = frame_queue.get()
        start_time = time.time()
        faces = process_frame(frame, face_detector)
        RC.process(faces)
        if faces != (0, 0, 0, 0, 0):
            RC.printData()
        end_time = time.time()
        print(f"Frame processing time: {1000*(end_time-start_time)} ms")
        frame_queue.task_done()  # Mark the frame as processed

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == "__main__":
    # Start the processing thread
    rtsp_thread = threading.Thread(target=process_latest_frame, daemon=True)
    rtsp_thread.start()
    # Run Flask with SSL
    socketio.run(app, host='0.0.0.0', port=5000, debug=True,
                 ssl_context=(r'C:\Users\mccab\Desktop\siteVlogus\cert.pem',
                              r'C:\Users\mccab\Desktop\siteVlogus\key.pem'))