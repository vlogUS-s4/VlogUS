import cv2
import numpy as np
from threading import Thread, Lock


class VideoStream:
    def __init__(self, rtsp_url):
        self.cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduce frame buffering
        self.frame = None
        self.stopped = False
        Thread(target=self.update, args=()).start()

    def update(self):
        while not self.stopped:
            ret, frame = self.cap.read()
            if ret:
                self.frame = frame

    def read(self):
        return self.frame

    def stop(self):
        self.stopped = True
        self.cap.release()


# Global variable for detected faces
side_faces = []
lock = Lock()


def detect_side_faces(frame):
    """Detects left and right profile faces."""
    global side_faces
    side_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_profileface.xml')

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Detect left-profile faces
    left_faces = side_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(10, 10))

    # Flip the image to detect right-profile faces
    flipped_gray = cv2.flip(gray, 1)
    right_faces = side_cascade.detectMultiScale(flipped_gray, scaleFactor=1.1, minNeighbors=5, minSize=(20, 20))

    # Convert right-profile coordinates back to the original frame
    frame_width = gray.shape[1]
    right_faces_corrected = [(frame_width - x - w, y, w, h) for (x, y, w, h) in right_faces]

    with lock:
        side_faces = list(left_faces) + right_faces_corrected  # Combine left & right profiles


def display_rtsp_stream_with_face_tracking(rtsp_url):
    video_stream = VideoStream(rtsp_url)

    # Load Haar Cascade for front-face detection
    face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

    print("Press 'q' to exit.")

    frame_skip = 8  # Adjust for lag reduction
    frame_count = 0
    detected_faces = []

    while True:
        frame = video_stream.read()
        if frame is None:
            continue

        frame_count += 1
        small_frame = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)

        if frame_count % frame_skip == 0:
            gray = cv2.cvtColor(small_frame, cv2.COLOR_BGR2GRAY)

            # Try detecting front faces first
            faces = face_cascade.detectMultiScale(gray, scaleFactor=1.3, minNeighbors=5, minSize=(30, 30))

            if len(faces) > 0:
                detected_faces = faces  # Use front faces
            else:
                # Start a separate thread for side-profile detection
                Thread(target=detect_side_faces, args=(small_frame,)).start()

                # Use side faces only if front faces are not detected
                with lock:
                    detected_faces = side_faces

        # Draw rectangles for detected faces (either front or profile)
        for (x, y, w, h) in detected_faces:
            x, y, w, h = [int(i * 2) for i in (x, y, w, h)]  # Scale back
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)  # Green = detected face

        # Show frame
        cv2.imshow("RTSP Stream with Face Tracking", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    video_stream.stop()
    cv2.destroyAllWindows()


# Replace with your RTSP stream URL
rtsp_url = "rtsp://192.168.137.144:8554/live"
display_rtsp_stream_with_face_tracking(rtsp_url)
