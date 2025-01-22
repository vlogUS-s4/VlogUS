import cv2

def display_rtsp_stream_with_face_tracking(rtsp_url):
    # Load the pre-trained Haar Cascade for face detection
    face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

    # Open the RTSP stream
    cap = cv2.VideoCapture(rtsp_url)

    if not cap.isOpened():
        print("Error: Unable to open RTSP stream.")
        return

    print("Press 'q' to exit.")

    frame_skip = 4  # Detect faces every 2 frames
    frame_count = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to read frame from RTSP stream.")
            break

        frame_count += 1

        # Resize frame for faster face detection (e.g., 50% of the original size)
        small_frame = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)

        if frame_count % frame_skip == 0:
            # Convert the frame to grayscale (required for Haar Cascade)
            gray = cv2.cvtColor(small_frame, cv2.COLOR_BGR2GRAY)

            # Detect faces in the frame
            faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(30, 30))
        else:
            faces = []  # Skip detection on this frame

        # Draw rectangles around detected faces
        for (x, y, w, h) in faces:
            # Scale back the coordinates to the original frame size
            x, y, w, h = [int(i * 2) for i in (x, y, w, h)]
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)

        # Display the frame
        cv2.imshow("RTSP Stream with Optimized Face Tracking", frame)

        # Exit the loop when 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Release the capture object and close the display window
    cap.release()
    cv2.destroyAllWindows()


def display_rtsp_stream(rtsp_url):
    # Open the RTSP stream
    cap = cv2.VideoCapture(rtsp_url)

    if not cap.isOpened():
        print("Error: Unable to open RTSP stream.")
        return

    print("Press 'q' to exit.")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to read frame from RTSP stream.")
            break

        # Display the frame
        cv2.imshow("RTSP Stream", frame)

        # Exit the loop when 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Release the capture object and close the display window
    cap.release()
    cv2.destroyAllWindows()
# Replace with your RTSP stream URL
rtsp_url = "rtsp://192.168.137.144:8554/live"
display_rtsp_stream_with_face_tracking(rtsp_url)