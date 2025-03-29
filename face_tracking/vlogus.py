import cv2
import numpy as np
from flask import Flask, Response, render_template
import socket
import threading
import time

app = Flask(__name__)

# Global variables to store the latest frame and control stream status
frame = None
stream_active = False

def get_ip():
    """Get the local IP address of the machine"""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Doesn't need to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def process_frame(frame):
    """
    Process the frame with OpenCV
    You can add any OpenCV operations here
    """
    # Example: Convert to grayscale and back to BGR for demonstration
    # gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    # return cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    
    # Example: Add a timestamp
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    cv2.putText(frame, timestamp, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
    
    # Example: Detect edges
    # edges = cv2.Canny(frame, 100, 200)
    # return cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR)
    
    return frame

def generate_frames():
    """Generator function that yields processed camera frames"""
    global frame, stream_active
    
    while True:
        if frame is not None and stream_active:
            # Process the frame with OpenCV
            processed_frame = process_frame(frame.copy())
            
            # Convert to JPEG for streaming
            ret, buffer = cv2.imencode('.jpg', processed_frame)
            if not ret:
                continue
                
            # Yield the frame in the format expected by Flask
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
        else:
            # If no frame is available, yield a blank frame
            blank = np.zeros((480, 640, 3), dtype=np.uint8)
            cv2.putText(blank, "No camera feed", (180, 240), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            ret, buffer = cv2.imencode('.jpg', blank)
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
        
        time.sleep(0.03)  # Aim for roughly 30 FPS

def receive_frames():
    """Function to receive frames from connected camera"""
    global frame, stream_active
    
    cap = cv2.VideoCapture(0)  # Use 0 for webcam (will be replaced by IP camera URL)
    
    if not cap.isOpened():
        print("Error: Could not open camera.")
        return
    
    stream_active = True
    
    while stream_active:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to capture frame.")
            break
        
        time.sleep(0.03)  # Limit frame rate
    
    cap.release()
    print("Camera released")

@app.route('/')
def index():
    """Serve the main page"""
    ip = get_ip()
    return render_template('index.html', server_ip=ip)

@app.route('/video_feed')
def video_feed():
    """Route for streaming the processed video feed"""
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/templates/index.html')
def serve_template():
    """Serve the template directly if requested"""
    return render_template('index.html', server_ip=get_ip())

if __name__ == '__main__':
    # Create and start the thread to receive frames
    camera_thread = threading.Thread(target=receive_frames)
    camera_thread.daemon = True
    camera_thread.start()
    
    # Run the Flask app
    print("Server running at http://{}:5000".format(get_ip()))
    app.run(host='0.0.0.0', port=5000, debug=False)