import cv2
import numpy as np
from threading import Thread

param_only_track_one_face = False

class VideoStream:
    def __init__(self, rtsp_url):
        self.cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduce frame buffering
        self.frame = None
        self.stopped = False
        Thread(target=self.update, daemon=True).start()  # Daemon thread for cleaner shutdown

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

def filter_close_faces(faces, image_width, min_dist_x):
    """Remove faces too close in x-direction (optimized for speed)."""
    if not faces:
        return []
    
    # Sort by x-coordinate for efficient filtering
    faces = sorted(faces, key=lambda f: f[0])  # Sort by cx
    filtered_faces = [faces[0]]
    
    for new_face in faces[1:]:
        cx_new = new_face[0]
        if abs(cx_new - filtered_faces[-1][0]) >= min_dist_x:
            filtered_faces.append(new_face)
    
    return filtered_faces

def process_frame(frame, face_cascade, profile_cascade):
    """Process a single frame and return detected faces (optimized)."""
    if frame is None:
        return (0, 0, 0, 0, 0)

    # Downscale frame once at the start
    h, w = frame.shape[:2]
    scale_factor = 0.5
    small_frame = cv2.resize(frame, (0, 0), fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    gray = cv2.cvtColor(small_frame, cv2.COLOR_BGR2GRAY)

    # Precompute flipped gray for right-profile detection
    flipped_gray = cv2.flip(gray, 1) if not param_only_track_one_face else None

    # Detection parameters
    scale_back = 1 / scale_factor
    min_size = (15, 15)
    detect_params = {'scaleFactor': 1.2, 'minNeighbors': 3, 'minSize': min_size}

    detected_faces = []

    # Front face detection
    faces = face_cascade.detectMultiScale(gray, **detect_params)
    for (x, y, w_f, h_f) in faces:
        x, y, w_f, h_f = [int(v * scale_back) for v in (x, y, w_f, h_f)]
        detected_faces.append((x + w_f // 2, y + h_f // 2, w_f, h_f, 1))

    if not param_only_track_one_face:
        # Left-profile detection
        left_profiles = profile_cascade.detectMultiScale(gray, **detect_params)
        for (x, y, w_p, h_p) in left_profiles:
            x, y, w_p, h_p = [int(v * scale_back) for v in (x, y, w_p, h_p)]
            detected_faces.append((x + w_p // 2, y + h_p // 2, w_p, h_p, 3))

        # Right-profile detection (flipped image)
        right_profiles = profile_cascade.detectMultiScale(flipped_gray, **detect_params)
        for (x, y, w_p, h_p) in right_profiles:
            x = w - int((x + w_p) * scale_back)  # Simplified adjustment
            y, w_p, h_p = [int(v * scale_back) for v in (y, w_p, h_p)]
            detected_faces.append((x + w_p // 2, y + h_p // 2, w_p, h_p, 2))

    # Filter close faces (optimized for x-direction only)
    min_dist_x = int(0.05 * w)  # 5% of width
    filtered_faces = filter_close_faces(detected_faces, w, min_dist_x)

    if not filtered_faces:
        return (0, 0, 0, 0, 0)

    if len(filtered_faces) == 1 or param_only_track_one_face:
        cx, cy, w_f, h_f, orientation = filtered_faces[0]
        return (round((cx / w) * 100, 2), round((cy / h) * 100, 2),
                round((w_f / w) * 100, 2), round((h_f / h) * 100, 2), orientation)

    # Average two largest faces (if multiple detected)
    filtered_faces.sort(key=lambda f: f[2] * f[3], reverse=True)  # Sort by area
    x1, y1, _, _, _ = filtered_faces[0]
    x2, y2, _, _, _ = filtered_faces[1] if len(filtered_faces) > 1 else filtered_faces[0]
    avg_x, avg_y = (x1 + x2) / 2, (y1 + y2) / 2
    dist_x, dist_y = abs(x1 - x2), abs(y1 - y2)

    return (round((avg_x / w) * 100, 2), round((avg_y / h) * 100, 2),
            round((dist_x / w) * 100, 2), round((dist_y / h) * 100, 2), 1)