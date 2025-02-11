import cv2
import numpy as np
import time
from threading import Thread

param_only_track_one_face = False

class VideoStream:
    def __init__(self, rtsp_url):
        self.cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduce frame buffering
        self.frame = None
        self.stopped = False
        Thread(target=self.update, args=()).start()

    def update(self):
        """Continuously update the latest frame in the background"""
        while not self.stopped:
            ret, frame = self.cap.read()
            if ret:
                self.frame = frame

    def read(self):
        """Return the most recent frame"""
        return self.frame

    def stop(self):
        """Stop the video stream"""
        self.stopped = True
        self.cap.release()


def filter_close_faces(faces, image_width, image_height, threshold=10):
    """Remove faces that are too close to each other (threshold in %)."""
    filtered_faces = []
    min_dist_x = (threshold / 100) * image_width  # 5% of width
    min_dist_y = (threshold / 100) * image_height  # 5% of height

    # Sort faces by size (largest first) to prioritize bigger detections
    #faces = sorted(faces, key=lambda f: f[2] * f[3], reverse=True)

    for new_face in faces:
        cx_new, cy_new, w_new, h_new, orientation_new = new_face
        too_close = False

        for (cx, cy, w, h, orientation) in filtered_faces:
            if abs(cx_new - cx) < min_dist_x and abs(cy_new - cy) < min_dist_y:
                too_close = True
                break

        if not too_close:
            filtered_faces.append(new_face)

    return filtered_faces


def process_frame(video_stream, face_cascade, profile_cascade):
    """Process a single frame and return detected faces"""
    frame = video_stream.read()
    if frame is None:
        return []

    h, w, _ = frame.shape  # Get image dimensions
    gray = cv2.cvtColor(cv2.resize(frame, (0, 0), fx=0.5, fy=0.5), cv2.COLOR_BGR2GRAY)
    flipped_gray = cv2.flip(gray, 1)  # Flip for right-profile detection

    detected_faces = []

    # Detect front faces
    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=4, minSize=(15, 15))
    for (x, y, w_f, h_f) in faces:
        x, y, w_f, h_f = [int(i * 2) for i in (x, y, w_f, h_f)]  # Scale back
        detected_faces.append((x + w_f // 2, y + h_f // 2, w_f, h_f, 1))  # Front-facing

    # Detect left-profile faces
    left_profiles = profile_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=4, minSize=(15, 15))
    for (x, y, w_p, h_p) in left_profiles:
        x, y, w_p, h_p = [int(i * 2) for i in (x, y, w_p, h_p)]  # Scale back
        detected_faces.append((x + w_p // 2, y + h_p // 2, w_p, h_p, 3))  # Left-facing

    # Detect right-profile faces (in flipped image)
    right_profiles = profile_cascade.detectMultiScale(flipped_gray, scaleFactor=1.1, minNeighbors=4, minSize=(15, 15))
    for (x, y, w_p, h_p) in right_profiles:
        x = w - (x * 2 + w_p * 2)  # Adjust x for flipped image
        y, w_p, h_p = [int(i * 2) for i in (y, w_p, h_p)]
        detected_faces.append((x + w_p // 2, y + h_p // 2, w_p, h_p, 2))  # Right-facing

    # Filter out close faces
    filtered_faces = filter_close_faces(detected_faces, w, h)

    # If no faces detected, return None
    if len(filtered_faces) == 0:
        return None

    # If exactly one face detected, return its position and orientation
    if len(filtered_faces) == 1 or param_only_track_one_face:
        cx, cy, w_f, h_f, orientation = filtered_faces[0]
        return round((cx / w) * 100, 2), round((cy / h) * 100, 2), round((w_f / w) * 100, 2), round((h_f / h) * 100, 2), orientation

    # If more than one face is detected, compute the average position and distance
    x1, y1, _, _, _ = filtered_faces[0]
    x2, y2, _, _, _ = filtered_faces[1]

    avg_x = (x1 + x2) / 2
    avg_y = (y1 + y2) / 2
    dist_x = abs(x1 - x2)
    dist_y = abs(y1 - y2)

    return round((avg_x / w) * 100, 2), round((avg_y / h) * 100, 2), round((dist_x / w) * 100, 2), round((dist_y / h) * 100, 2), 1



# Usage Example:
http_url = "http://192.168.137.89/live"
video_stream = VideoStream(http_url)

# Load the Haar cascades
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')
profile_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_profileface.xml')

try:
    while True:
        faces = process_frame(video_stream, face_cascade, profile_cascade)
        print("Detected Faces:", faces)  # Each frame's detected faces

        with open("data.txt", "w") as file:
            file.write(str(faces))



        if cv2.waitKey(50) & 0xFF == ord('q'):  # Wait 50ms and check if 'q' is pressed
            break

except KeyboardInterrupt:
    print("Interrupted by user")

video_stream.stop()
cv2.destroyAllWindows()
