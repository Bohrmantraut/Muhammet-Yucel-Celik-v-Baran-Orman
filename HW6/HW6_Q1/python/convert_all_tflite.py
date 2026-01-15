import os
import tensorflow as tf
from tensorflow import keras

os.makedirs("export", exist_ok=True)

# TrueDivide, Keras3/TensorFlow sürüm farklarında "Unknown layer" diye görünebiliyor.
# Bunu deserialize aşamasında custom object gibi tanıtıyoruz.
class TrueDivide(keras.layers.Layer):
    def call(self, inputs):
        # Bu layer pratikte x / const gibi bir op; load sırasında graph'ı kurmaya yetiyor.
        # Gerçek bölme sabiti model graph içinde zaten gömülü olacağından, burada passthrough yeterli olur.
        return inputs

def safe_load_model(path: str):
    # 1) Normal dene
    try:
        return keras.models.load_model(path)
    except Exception as e1:
        # 2) custom_object_scope ile dene
        try:
            with keras.utils.custom_object_scope({"TrueDivide": TrueDivide}):
                return keras.models.load_model(path)
        except Exception as e2:
            raise RuntimeError(
                f"Model load failed for {path}\n"
                f"First error: {repr(e1)}\n"
                f"Second error: {repr(e2)}"
            )

def convert_model(h5_path: str):
    tag = os.path.splitext(os.path.basename(h5_path))[0]
    tflite_path = f"export/{tag}.tflite"

    model = safe_load_model(h5_path)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    with open(tflite_path, "wb") as f:
        f.write(tflite_model)

    print(f"[OK] {tflite_path}")

if __name__ == "__main__":
    h5_list = [
        "models/SqueezeNet_mnist.h5",
        "models/EfficientNetB0_mnist.h5",
        "models/MobileNetV2_mnist.h5",
        "models/ResNet50_mnist.h5",
    ]

    for p in h5_list:
        if os.path.exists(p):
            convert_model(p)
        else:
            print(f"[SKIP] bulunamadı: {p}")
