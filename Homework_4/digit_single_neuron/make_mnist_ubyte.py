import os
import struct
import numpy as np
from tensorflow.keras.datasets import mnist

def write_idx_images(path, images):
    # IDX3 magic = 2051
    with open(path, "wb") as f:
        f.write(struct.pack(">IIII", 2051, images.shape[0], images.shape[1], images.shape[2]))
        f.write(images.astype(np.uint8).tobytes())

def write_idx_labels(path, labels):
    # IDX1 magic = 2049
    with open(path, "wb") as f:
        f.write(struct.pack(">II", 2049, labels.shape[0]))
        f.write(labels.astype(np.uint8).tobytes())

def main():
    (train_images, train_labels), (test_images, test_labels) = mnist.load_data()

    outdir = "MNIST-dataset"
    os.makedirs(outdir, exist_ok=True)

    write_idx_images(os.path.join(outdir, "train-images.idx3-ubyte"), train_images)
    write_idx_labels(os.path.join(outdir, "train-labels.idx1-ubyte"), train_labels)
    write_idx_images(os.path.join(outdir, "t10k-images.idx3-ubyte"), test_images)
    write_idx_labels(os.path.join(outdir, "t10k-labels.idx1-ubyte"), test_labels)

    print("Done. Files written to MNIST-dataset/")

if __name__ == "__main__":
    main()
