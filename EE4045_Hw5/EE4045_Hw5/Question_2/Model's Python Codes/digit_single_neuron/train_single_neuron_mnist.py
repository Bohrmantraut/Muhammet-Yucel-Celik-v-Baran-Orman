import os
import numpy as np
import cv2
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay
import tensorflow as tf
from mnist import load_images, load_labels
from matplotlib import pyplot as plt

# --- Paths (kitaptaki gibi) ---
train_img_path   = os.path.join("MNIST-dataset", "train-images.idx3-ubyte")
train_label_path = os.path.join("MNIST-dataset", "train-labels.idx1-ubyte")
test_img_path    = os.path.join("MNIST-dataset", "t10k-images.idx3-ubyte")
test_label_path  = os.path.join("MNIST-dataset", "t10k-labels.idx1-ubyte")

# --- Load MNIST ---
train_images = load_images(train_img_path)
train_labels = load_labels(train_label_path)
test_images  = load_images(test_img_path)
test_labels  = load_labels(test_label_path)

# --- Feature extraction: Hu moments (7 features) ---
train_huMoments = np.empty((len(train_images), 7), dtype=np.float64)
test_huMoments  = np.empty((len(test_images), 7), dtype=np.float64)

for i, img in enumerate(train_images):
    # binaryImage=True: MNIST'te sıfır olmayan pikselleri "foreground" kabul eder
    m = cv2.moments(img, binaryImage=True)
    train_huMoments[i] = cv2.HuMoments(m).reshape(7)

for i, img in enumerate(test_images):
    m = cv2.moments(img, binaryImage=True)
    test_huMoments[i] = cv2.HuMoments(m).reshape(7)

# --- Standardization (train mean/std ile) ---
features_mean = np.mean(train_huMoments, axis=0)
features_std  = np.std(train_huMoments, axis=0)

# std=0 gibi nadir durumlara karşı küçük güvenlik:
features_std[features_std == 0] = 1.0

train_huMoments = (train_huMoments - features_mean) / features_std
test_huMoments  = (test_huMoments  - features_mean) / features_std

# --- Labels: 0 vs not-0 ---
train_labels = train_labels.copy()
test_labels  = test_labels.copy()
train_labels[train_labels != 0] = 1
test_labels[test_labels != 0]   = 1

# --- Single neuron model ---
model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(1, input_shape=(7,), activation="sigmoid")
])

model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
    loss=tf.keras.losses.BinaryCrossentropy(),
    metrics=[tf.keras.metrics.BinaryAccuracy(name="acc")]
)

# --- Train ---
history = model.fit(
    train_huMoments,
    train_labels,
    batch_size=128,
    epochs=50,
    class_weight={0: 8, 1: 1},  # kitapla aynı: "0" sınıfına daha fazla ağırlık
    verbose=1
)

# --- Evaluate (confusion matrix) ---
probs = model.predict(test_huMoments, verbose=0).reshape(-1)
preds = (probs > 0.5).astype(np.uint8)

cm = confusion_matrix(test_labels, preds, labels=[0, 1])
disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["zero", "not-zero"])
disp.plot(values_format="d")
disp.ax_.set_title("Single Neuron Classifier Confusion Matrix")
plt.show()

# --- Save ---
model.save("mnist_single_neuron.h5")
print("Saved model: mnist_single_neuron.h5")
