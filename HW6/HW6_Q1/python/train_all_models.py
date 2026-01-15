# train_all_models.py (FIXED - no load_model for checkpoints)
import os
import numpy as np
import tensorflow as tf
from tensorflow import keras
from sklearn.metrics import confusion_matrix, classification_report

from squeezeNet import SqueezeNet

NUM_CLASSES = 10
DATA_SHAPE = (32, 32, 3)

os.makedirs("models", exist_ok=True)
os.makedirs("results", exist_ok=True)

def prepare_tensor(images, out_shape=DATA_SHAPE):
    images = tf.expand_dims(images, axis=-1)     # (H,W,1)
    images = tf.repeat(images, 3, axis=-1)       # (H,W,3)
    images = tf.image.resize(images, out_shape[:2])
    images = tf.cast(images, tf.float32) / 255.0
    return images

def load_mnist():
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    x_train = prepare_tensor(x_train)
    x_test  = prepare_tensor(x_test)
    y_train_oh = tf.keras.utils.to_categorical(y_train, NUM_CLASSES)
    y_test_oh  = tf.keras.utils.to_categorical(y_test, NUM_CLASSES)
    return (x_train, y_train, y_train_oh), (x_test, y_test, y_test_oh)

def eval_and_save(model, x_test, y_test_int, tag):
    y_prob = model.predict(x_test, verbose=0)
    y_pred = np.argmax(y_prob, axis=1)

    cm = confusion_matrix(y_test_int, y_pred)
    rep = classification_report(y_test_int, y_pred, digits=4)

    np.save(f"results/cm_{tag}.npy", cm)
    with open(f"results/report_{tag}.txt", "w", encoding="utf-8") as f:
        f.write(rep)

    acc = float((y_pred == y_test_int).mean())
    return acc, cm, rep

# -----------------------------
# 1) SqueezeNet training (book-style)
# -----------------------------
def train_squeezenet(x_train, y_train_oh):
    # weights="imagenet" linki 404 olabildiği için burada weights=None kullanıyoruz.
    model = SqueezeNet(input_shape=DATA_SHAPE, weights=None, dropout=0.2, classes=NUM_CLASSES)

    # Kitaptaki gibi: layer'ların 1/3'ünü freeze
    num_layers_to_freeze = len(model.layers) // 3
    for layer in model.layers[:num_layers_to_freeze]:
        layer.trainable = False

    model.compile(
        loss="categorical_crossentropy",
        optimizer=keras.optimizers.Adam(1e-3),
        metrics=["accuracy"]
    )

    ckpt_path = "models/squeezenet_tl_mnist.h5"
    ckpt = keras.callbacks.ModelCheckpoint(
        ckpt_path,
        monitor="val_loss",
        save_best_only=True,
        mode="min",
        verbose=1
    )
    es = keras.callbacks.EarlyStopping(patience=3, restore_best_weights=True)

    model.fit(
        x_train, y_train_oh,
        batch_size=128,
        epochs=10,
        validation_split=0.1,
        callbacks=[ckpt, es],
        verbose=1
    )

    # SqueezeNet için .h5 kaydı zaten ckpt ile var.
    # Ayrıca en iyi ağırlıkların yüklü olduğundan emin olmak için tekrar kaydediyoruz:
    model.save("models/SqueezeNet_mnist.h5")
    return model

# -----------------------------
# 2) Keras Applications backbones
# -----------------------------
def build_keras_app(backbone_name):
    if backbone_name == "EfficientNetB0":
        backbone = tf.keras.applications.EfficientNetB0(
            include_top=False, weights="imagenet", input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.efficientnet.preprocess_input
    elif backbone_name == "MobileNetV2":
        backbone = tf.keras.applications.MobileNetV2(
            include_top=False, weights="imagenet", input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.mobilenet_v2.preprocess_input
    elif backbone_name == "ResNet50":
        backbone = tf.keras.applications.ResNet50(
            include_top=False, weights="imagenet", input_shape=DATA_SHAPE
        )
        preprocess = tf.keras.applications.resnet.preprocess_input
    else:
        raise ValueError("Unknown backbone")

    backbone.trainable = False  # Stage-1 freeze

    inputs = keras.Input(shape=DATA_SHAPE, name="input")
    x = preprocess(inputs)
    x = backbone(x, training=False)
    x = keras.layers.GlobalAveragePooling2D()(x)
    x = keras.layers.Dropout(0.2)(x)
    outputs = keras.layers.Dense(NUM_CLASSES, activation="softmax")(x)

    model = keras.Model(inputs, outputs, name=backbone_name)
    return model, backbone

def train_two_stage(model, backbone, x_train, y_train_oh, tag, e1=5, e2=5):
    # IMPORTANT: save weights only to avoid "Unknown layer: TrueDivide" on load_model()
    weights_ckpt_path = f"models/{tag}.weights.h5"

    ckpt = keras.callbacks.ModelCheckpoint(
        weights_ckpt_path,
        save_best_only=True,
        save_weights_only=True,   # <-- FIX
        monitor="val_accuracy",
        mode="max",
        verbose=1
    )
    es = keras.callbacks.EarlyStopping(patience=3, restore_best_weights=True)

    # Stage 1 (frozen)
    model.compile(
        optimizer=keras.optimizers.Adam(1e-3),
        loss="categorical_crossentropy",
        metrics=["accuracy"]
    )
    model.fit(
        x_train, y_train_oh,
        epochs=e1,
        batch_size=128,
        validation_split=0.1,
        callbacks=[ckpt, es],
        verbose=1
    )

    # Stage 2 (fine-tune)
    backbone.trainable = True
    model.compile(
        optimizer=keras.optimizers.Adam(1e-4),
        loss="categorical_crossentropy",
        metrics=["accuracy"]
    )
    model.fit(
        x_train, y_train_oh,
        epochs=e2,
        batch_size=128,
        validation_split=0.1,
        callbacks=[ckpt, es],
        verbose=1
    )

    # Load best weights and save full model as .h5 (for TFLite conversion / STM32 workflow)
    model.load_weights(weights_ckpt_path)
    model.save(f"models/{tag}.h5")
    return model

# -----------------------------
# Main
# -----------------------------
if __name__ == "__main__":
    (x_train, y_train_int, y_train_oh), (x_test, y_test_int, y_test_oh) = load_mnist()

    results = {}

    # 1) SqueezeNet
    sq = train_squeezenet(x_train, y_train_oh)
    acc, cm, rep = eval_and_save(sq, x_test, y_test_int, "SqueezeNet")
    results["SqueezeNet"] = acc
    print(f"[DONE] SqueezeNet test acc = {acc:.4f}")

    # 2) EfficientNet / MobileNet / ResNet
    for name in ["EfficientNetB0", "MobileNetV2", "ResNet50"]:
        model, backbone = build_keras_app(name)
        best = train_two_stage(
            model, backbone, x_train, y_train_oh,
            tag=f"{name}_mnist",
            e1=5, e2=5
        )
        acc, cm, rep = eval_and_save(best, x_test, y_test_int, name)
        results[name] = acc
        print(f"[DONE] {name} test acc = {acc:.4f}")

    # Summary
    with open("results/summary.txt", "w", encoding="utf-8") as f:
        for k, v in results.items():
            f.write(f"{k}: {v:.6f}\n")

    print("All done. Check results/summary.txt and models/*.h5")
