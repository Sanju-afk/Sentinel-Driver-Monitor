import cv2
import os

# CONFIGURATION
BASE_DIR = "../data/dataset"
CATEGORIES = ["alert", "drowsy"]

# Create folders 
for cat in CATEGORIES:
    os.makedirs(os.path.join(BASE_DIR, cat), exist_ok=True)

# Camera Initialization
cap = cv2.VideoCapture(0)
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")

if not cap.isOpened():
    print("ERROR: Could not open webcam.")
    print("Suggest checking: 1. Privacy switch/slider 2. Other apps using camera")
    exit()
# ----------------------------

print("==== DRIVER DATA COLLECTOR ====")
print("Press 'a' to save ALERT example {Eyes open}")
print("Press 'd' to save DROWSY example {Eyes closed}")
print("Press 'q' to QUIT")

while True:
    ret, frame = cap.read()
    if not ret: break
    
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 4)
    
    # Count existing files
    counts = {cat: len(os.listdir(os.path.join(BASE_DIR, cat))) for cat in CATEGORIES}
    
    # Draw UI 
    cv2.putText(frame, f"ALERT : {counts['alert']}", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    
    cv2.putText(frame, f"DROWSY: {counts['drowsy']}", (10, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
    
    # Handle Keyboard Input 
    key = cv2.waitKey(1) & 0xFF
    save_cat = None
    
    if key == ord('a'):
        save_cat = "alert"
    elif key == ord('d'):
        save_cat = "drowsy"
    elif key == ord('q'): 
        break # Use break to exit the while loop cleanly

    # Process Faces
    for (x, y, w, h) in faces:
        cv2.rectangle(frame, (x, y), (x+w, y+h), (255, 255, 0), 2)
        
        if save_cat:
            # Crop and resize to 224x224 (MobileNet Standard)
            face_roi = frame[y:y+h, x:x+w]
            if face_roi.size > 0: # Ensure crop isn't empty
                face_resized = cv2.resize(face_roi, (224, 224))
                
                # Save the file 
                filename = os.path.join(BASE_DIR, save_cat, f"{counts[save_cat]}.jpg")
                cv2.imwrite(filename, face_resized)
                print(f"Saved {save_cat} image to {filename}!")
    
    cv2.imshow("Sentinel Data Collector", frame)

cap.release()
cv2.destroyAllWindows()
