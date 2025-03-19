from flask import Flask, render_template
from flask_socketio import SocketIO
import cv2
import base64
import numpy as np
from ProcessData import RobotController
from rtsp import process_frame

app = Flask(__name__)
socketio = SocketIO(app)

# Global variable to store the latest frame
latest_frame = None
frame_count = 0

@socketio.on('video_frame')
def handle_video_frame(data):
    global frame_count
    frame_count+=1
    if frame_count>=2:
        global latest_frame
        # Load the Haar cascades
        face_cascade = cv2.CascadeClassifier(r'C:\Users\mccab\Desktop\siteVlogus\haarcascade_frontalface_default.xml')
        if face_cascade.empty():
            print("Error: Could not load haarcascade_frontalface_default.xml")
            return
        profile_cascade = cv2.CascadeClassifier(r'C:\Users\mccab\Desktop\siteVlogus\haarcascade_profileface.xml')
        if profile_cascade.empty():
            print("Error: Could not load haarcascade_profileface.xml")
            return

        RC = RobotController()
        # Decode base64 frame from the browser
        frame_data = base64.b64decode(data.split(',')[1])  # Remove "data:image/jpeg;base64," prefix
        np_frame = np.frombuffer(frame_data, dtype=np.uint8)
        frame = cv2.imdecode(np_frame, cv2.IMREAD_COLOR)
        faces = process_frame(frame, face_cascade, profile_cascade)
        RC.process(faces)
        if faces!=(0,0,0,0,0):
            RC.printData()
        latest_frame = frame
        frame_count = 0

@app.route('/')
def index():
    return render_template('index.html')



if __name__ == "__main__":
    # Run Flask with SSL
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, 
                 ssl_context=(r'C:\Users\mccab\Desktop\siteVlogus\cert.pem', 
                              r'C:\Users\mccab\Desktop\siteVlogus\key.pem'))