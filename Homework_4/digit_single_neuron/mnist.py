import numpy as np

def load_images(path: str) -> np.ndarray:
    with open(path, "rb") as f:
        buffer = f.read()[16:]  # skip header
        images = np.frombuffer(buffer, dtype=np.uint8).reshape((-1, 28, 28))
    return images

def load_labels(path: str) -> np.ndarray:
    with open(path, "rb") as f:
        buffer = f.read()[8:]   # skip header
        labels = np.frombuffer(buffer, dtype=np.uint8)
    return labels
