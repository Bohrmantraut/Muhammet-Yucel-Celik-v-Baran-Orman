import os
import tensorflow as tf
from tensorflow import keras
from squeezeNet import SqueezeNet

NUM_CLASSES = 10
DATA_SHAPE = (32, 32, 3)

os.makedirs("export", exist_ok=True)

def build_keras_app(backbone_name: str):
    if backbone_name == "EfficientNetB0":
        backbone = tf.keras.applications.EfficientNetB0(
            include_top=False, weights=None, input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.efficientnet.preprocess_input
    elif backbone_name == "MobileNetV2":
        backbone = tf.keras.applications.MobileNetV2(
            include_top=False, weights=None, input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.mobilenet_v2.preprocess_input
    elif backbone_name == "ResNet50":
        backbone = tf.keras.applications.ResNet50(
            include_top=False, weights=None, input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.resnet.preprocess_input
    else:
        raise ValueError("Unknown backbone")

    # (eğitim scriptindekiyle aynı top)
    inputs = keras.Input(shape=DATA_SHAPE, name="input")
    x = preprocess(inputs)
    x = backbone(x, training=False)
    x = keras.layers.GlobalAveragePooling2D()(x)
    x = keras.layers.Dropout(0.2)(x)
    outputs = keras.layers.Dense(NUM_CLASSES, activation="softmax")(x)
    model = keras.Model(inputs, outputs, name=backbone_name)
    return model

def to_tflite(model, out_path: str):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite = converter.convert()
    with open(out_path, "wb") as f:
        f.write(tflite)

def load_weights_safely(model, weights_path: str):
    # load_weights, load_model gibi deserialize yapmadığı için bu hataları bypass eder
    model.load_weights(weights_path)

def convert_squeezenet():
    model = SqueezeNet(input_shape=DATA_SHAPE, weights=None, dropout=0.2, classes=NUM_CLASSES)

    # Öncelik: weights checkpoint varsa onu kullan
    # Yoksa, elindeki .h5 dosyasından weights çekmeyi dene
    candidates = [
        "models/squeezenet_tl_mnist.weights.h5",  # varsa
        "models/SqueezeNet_mnist.weights.h5",     # varsa
        "models/squeezenet_tl_mnist.h5",          # genelde var
        "models/SqueezeNet_mnist.h5",             # genelde var
    ]
    w = next((p for p in candidates if os.path.exists(p)), None)
    if w is None:
        raise FileNotFoundError("SqueezeNet weights bulunamadı (models/ altında .h5 veya .weights.h5 yok).")
    load_weights_safely(model, w)

    out = "export/SqueezeNet_mnist.tflite"
    to_tflite(model, out)
    print(f"[OK] {out}  (weights from: {w})")

def convert_backbone(name: str):
    model = build_keras_app(name)

    # Öncelik: .weights.h5 (senin train scriptin bunu üretiyor olmalı)
    w1 = f"models/{name}_mnist.weights.h5"
    # Alternatif: full model .h5 (bu load_model değil, load_weights olarak kullanılacak)
    w2 = f"models/{name}_mnist.h5"

    if os.path.exists(w1):
        w = w1
    elif os.path.exists(w2):
        w = w2
    else:
        raise FileNotFoundError(f"{name} için weights bulunamadı: {w1} veya {w2} yok.")

    load_weights_safely(model, w)

    out = f"export/{name}_mnist.tflite"
    to_tflite(model, out)
    print(f"[OK] {out}  (weights from: {w})")

if __name__ == "__main__":
    # 1) SqueezeNet
    convert_squeezenet()

    # 2) Diğerleri
    for backbone in ["EfficientNetB0", "MobileNetV2", "ResNet50"]:
        convert_backbone(backbone)
