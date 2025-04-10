import cv2
import numpy as np
from threading import Thread

param_only_track_one_face = False

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

def process_frame(frame, face_detector):
    """Process a single frame and return detected faces using YuNet."""
    if frame is None:
        return (0, 0, 0, 0, 0)

    # Downscale frame once at the start
    h, w = frame.shape[:2]
    scale_factor = 0.5
    small_frame = cv2.resize(frame, (0, 0), fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)

    # Set input size for YuNet
    face_detector.setInputSize((small_frame.shape[1], small_frame.shape[0]))
    _, faces = face_detector.detect(small_frame)

    scale_back = 1 / scale_factor
    detected_faces = []

    # Process detected faces
    if faces is not None:
        for face in faces:
            x, y, w_f, h_f = [int(v * scale_back) for v in face[:4]]  # YuNet returns [x, y, w, h, confidence, ...]
            cx, cy = x + w_f // 2, y + h_f // 2
            detected_faces.append((cx, cy, w_f, h_f, 1))  # Orientation fixed to 1 (YuNet doesn't distinguish profiles)

    # Filter close faces (optimized for x-direction only)
    min_dist_x = int(0.05 * w)  # 5% of width
    filtered_faces = filter_close_faces(detected_faces, w, min_dist_x)

    if not filtered_faces:
        return (0, 0, 0, 0, 0)

    if len(filtered_faces) == 1 or param_only_track_one_face:
        cx, cy, w_f, h_f, orientation = filtered_faces[0]
        return (round((cx / w) * 100, 2), round((cy / h) * 100, 2),
                round((w_f / w) * 100, 2), round((h_f / h) * 100, 2), orientation)

    # If more than one face, calculate average of all faces
    if len(filtered_faces) > 1:
        # Calculate average center coordinates
        avg_cx = sum(face[0] for face in filtered_faces) / len(filtered_faces)
        avg_cy = sum(face[1] for face in filtered_faces) / len(filtered_faces)
        
        # Calculate average width and height
        avg_w = sum(face[2] for face in filtered_faces) / len(filtered_faces)
        avg_h = sum(face[3] for face in filtered_faces) / len(filtered_faces)

        return (round((avg_cx / w) * 100, 2), round((avg_cy / h) * 100, 2),
                round((avg_w / w) * 100, 2), round((avg_h / h) * 100, 2), 1)

def main():
    # Initialize YuNet face detector (download 'face_detection_yunet_2023mar.onnx' first)
    face_detector = cv2.FaceDetectorYN_create("face_detection_yunet_2023mar.onnx", "", (320, 320))
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("Erreur: Impossible d'ouvrir la caméra")
        return

    # Main loop
    while True:
        # Read a frame from the camera
        ret, frame = cap.read()
        if not ret:
            print("Erreur: Impossible de lire la frame")
            break

        # Process the frame to detect faces
        result = process_frame(frame, face_detector)
        cx_percent, cy_percent, w_percent, h_percent, orientation = result

        # Convert percentages to pixels for display
        h, w = frame.shape[:2]
        cx = int(cx_percent * w / 100)
        cy = int(cy_percent * h / 100)
        w_f = int(w_percent * w / 100)
        h_f = int(h_percent * h / 100)

        # Draw a rectangle around the detected face
        if cx_percent != 0 or cy_percent != 0:  # Check if a face is detected
            x1 = max(0, cx - w_f // 2)
            y1 = max(0, cy - h_f // 2)
            x2 = min(w, cx + w_f // 2)
            y2 = min(h, cy + h_f // 2)
            
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            
            # Display coordinates in percentage
            text = f"X: {cx_percent:.1f}%, Y: {cy_percent:.1f}%"
            cv2.putText(frame, text, (x1, y1-10), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

        # Show the frame
        cv2.imshow('Face Detection', frame)

        # Quit with 'q' key
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Release resources
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()