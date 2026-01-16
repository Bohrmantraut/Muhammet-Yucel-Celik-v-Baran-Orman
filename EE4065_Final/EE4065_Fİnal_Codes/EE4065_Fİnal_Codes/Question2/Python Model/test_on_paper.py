from ultralytics import YOLO
import cv2
import os

# 1) Model yolu (best.pt)
MODEL_PATH = r"C:\Users\muham\Desktop\EE4065 Final\runs\digits_train\weights\best.pt"

# 2) Test resmi
IMG_PATH = r"C:\Users\muham\Desktop\EE4065 Final\test_images\view (3).jpg"

# 3) Çıktı klasörü
OUT_DIR = r"C:\Users\muham\Desktop\EE4065 Final\paper_test_out_4"
os.makedirs(OUT_DIR, exist_ok=True)

# Load model
model = YOLO(MODEL_PATH)

# Inference
results = model.predict(
    source=IMG_PATH,
    imgsz=640,
    conf=0.25,
    iou=0.45,
    device=0,     # GPU
    verbose=False
)

# Read image for drawing
img = cv2.imread(IMG_PATH)
h, w = img.shape[:2]

r = results[0]
names = r.names

print("\nDetections:")
for box in r.boxes:
    cls_id = int(box.cls.item())
    conf = float(box.conf.item())

    # xyxy in pixels
    x1, y1, x2, y2 = box.xyxy[0].tolist()
    x1, y1, x2, y2 = map(int, [x1, y1, x2, y2])

    bw = x2 - x1
    bh = y2 - y1

    digit = names[cls_id]  # class name (0..9)

    print(f"- digit={digit}, prob={conf:.3f}, "
          f"w={bw}, h={bh}, "
          f"upper_left=({x1},{y1})")

    # Draw
    cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
    cv2.putText(img, f"{digit} {conf:.2f}", (x1, max(0, y1-8)),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,255,0), 2)

# Save visual output
out_path = os.path.join(OUT_DIR, "paper1_pred.jpg")
cv2.imwrite(out_path, img)
print(f"\nSaved annotated image to:\n{out_path}")
