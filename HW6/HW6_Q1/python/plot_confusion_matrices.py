import os
import numpy as np
import matplotlib.pyplot as plt

os.makedirs("results/png", exist_ok=True)

models = [
    "SqueezeNet",
    "EfficientNetB0",
    "MobileNetV2",
    "ResNet50"
]

for model in models:
    cm_path = f"results/cm_{model}.npy"
    if not os.path.exists(cm_path):
        print(f"[SKIP] {cm_path} bulunamadı")
        continue

    cm = np.load(cm_path)

    plt.figure(figsize=(6, 5))
    plt.imshow(cm)
    plt.title(f"Confusion Matrix – {model}")
    plt.xlabel("Predicted Label")
    plt.ylabel("True Label")
    plt.colorbar()

    for i in range(cm.shape[0]):
        for j in range(cm.shape[1]):
            plt.text(j, i, cm[i, j],
                     ha="center", va="center", fontsize=8)

    plt.tight_layout()
    out_path = f"results/png/confusion_matrix_{model}.png"
    plt.savefig(out_path, dpi=300)
    plt.close()

    print(f"[OK] {out_path}")
