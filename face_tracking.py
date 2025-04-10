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

    # Average two largest faces (if multiple detected)
    filtered_faces.sort(key=lambda f: f[2] * f[3], reverse=True)  # Sort by area
    x1, y1, _, _, _ = filtered_faces[0]
    x2, y2, _, _, _ = filtered_faces[1] if len(filtered_faces) > 1 else filtered_faces[0]
    avg_x, avg_y = (x1 + x2) / 2, (y1 + y2) / 2
    dist_x, dist_y = abs(x1 - x2), abs(y1 - y2)

    return (round((avg_x / w) * 100, 2), round((avg_y / h) * 100, 2),
            round((dist_x / w) * 100, 2), round((dist_y / h) * 100, 2), 1)


def main():
    # Initialiser la capture vidéo (0 pour la webcam par défaut)
    
    # Initialize YuNet face detector (download 'face_detection_yunet_2023mar.onnx' first)
    face_detector = cv2.FaceDetectorYN_create("face_detection_yunet_2023mar.onnx", "", (320, 320))
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("Erreur: Impossible d'ouvrir la caméra")
        return

    # Boucle principale
    while True:
        # Lire une frame de la caméra
        ret, frame = cap.read()
        if not ret:
            print("Erreur: Impossible de lire la frame")
            break

        # Traiter la frame pour détecter les visages
        result = process_frame(frame, face_detector)
        cx_percent, cy_percent, w_percent, h_percent, orientation = result

        # Convertir les pourcentages en pixels pour l'affichage
        h, w = frame.shape[:2]
        cx = int(cx_percent * w / 100)
        cy = int(cy_percent * h / 100)
        w_f = int(w_percent * w / 100)
        h_f = int(h_percent * h / 100)

        # Dessiner un rectangle autour du visage détecté
        if cx_percent != 0 or cy_percent != 0:  # Vérifier si un visage est détecté
            x1 = max(0, cx - w_f // 2)
            y1 = max(0, cy - h_f // 2)
            x2 = min(w, cx + w_f // 2)
            y2 = min(h, cy + h_f // 2)
            
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            
            # Afficher les coordonnées en pourcentage
            text = f"X: {cx_percent:.1f}%, Y: {cy_percent:.1f}%"
            cv2.putText(frame, text, (x1, y1-10), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

        # Afficher la frame
        cv2.imshow('Face Detection', frame)

        # Quitter avec la touche 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Libérer les ressources
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()