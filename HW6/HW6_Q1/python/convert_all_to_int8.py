# convert_all_to_int8.py
# ------------------------------------------------------------
# Robust H5 -> INT8 TFLite converter
# - Tries to load full .h5 model
# - If deserialization fails (Keras3 issues like TrueDivide/Stack/GetItem),
#   rebuilds known architectures (MobileNetV2 / ResNet50) and loads weights.
# ------------------------------------------------------------

import os
import re
import numpy as np
import tensorflow as tf

# ---------------------------
# Representative dataset
# ---------------------------
def representative_data_gen(input_shape=(1, 32, 32, 3), n_samples=200):
    # If your training used different normalization, mirror it here.
    for _ in range(n_samples):
        x = np.random.rand(*input_shape).astype(np.float32)  # [0,1]
        yield [x]


# ---------------------------
# Build fallback models
# ---------------------------
def build_mobilenetv2_classifier(input_shape=(32, 32, 3), num_classes=10):
    base = tf.keras.applications.MobileNetV2(
        input_shape=input_shape,
        include_top=False,
        weights=None
    )
    x = tf.keras.layers.GlobalAveragePooling2D()(base.output)
    out = tf.keras.layers.Dense(num_classes, activation="softmax")(x)
    return tf.keras.Model(inputs=base.input, outputs=out, name="MobileNetV2_custom")


def build_resnet50_classifier(input_shape=(32, 32, 3), num_classes=10):
    base = tf.keras.applications.ResNet50(
        input_shape=input_shape,
        include_top=False,
        weights=None
    )
    x = tf.keras.layers.GlobalAveragePooling2D()(base.output)
    out = tf.keras.layers.Dense(num_classes, activation="softmax")(x)
    return tf.keras.Model(inputs=base.input, outputs=out, name="ResNet50_custom")


def guess_arch_from_filename(path: str):
    name = os.path.basename(path).lower()
    if "mobilenet" in name:
        return "mobilenetv2"
    if "resnet" in name:
        return "resnet50"
    return None


# ---------------------------
# Load model robustly
# ---------------------------
def try_load_full_model(h5_path: str):
    # Use tf.keras loader only (avoid standalone keras deserialization differences)
    return tf.keras.models.load_model(h5_path, compile=False, safe_mode=False)


def load_model_with_fallback(h5_path: str, input_shape=(32, 32, 3), num_classes=10):
    # 1) Try loading full model
    try:
        model = try_load_full_model(h5_path)
        # Quick sanity check
        _ = model.output_shape
        return model, "loaded_full_model"
    except Exception as e:
        err = str(e)

    # 2) Fallback rebuild by filename
    arch = guess_arch_from_filename(h5_path)
    if arch == "mobilenetv2":
        model = build_mobilenetv2_classifier(input_shape=input_shape, num_classes=num_classes)
    elif arch == "resnet50":
        model = build_resnet50_classifier(input_shape=input_shape, num_classes=num_classes)
    else:
        raise RuntimeError(
            f"Full model load failed and architecture not recognized from filename.\n"
            f"File: {h5_path}\n"
            f"Error: {err}"
        )

    # 3) Load weights from the .h5
    # This works if your .h5 contains weights (or a full model with weights accessible).
    # If strict load fails, try by_name+skip_mismatch as a last resort.
    try:
        model.load_weights(h5_path)
        return model, f"rebuilt_{arch}_loaded_weights"
    except Exception as e1:
        try:
            model.load_weights(h5_path, by_name=True, skip_mismatch=True)
            return model, f"rebuilt_{arch}_loaded_weights_by_name_skip_mismatch"
        except Exception as e2:
            raise RuntimeError(
                f"Could not load weights into rebuilt {arch}.\n"
                f"First load_weights error: {e1}\n"
                f"Second load_weights(by_name=True, skip_mismatch=True) error: {e2}\n"
                f"NOTE: This usually means your classifier head differs from GAP+Dense({num_classes})."
            )


# ---------------------------
# Convert to INT8 TFLite
# ---------------------------
def convert_model_to_int8_tflite(model: tf.keras.Model, out_tflite_path: str, rep_shape=(1, 32, 32, 3)):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)

    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = lambda: representative_data_gen(rep_shape, n_samples=200)

    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_int8 = converter.convert()

    with open(out_tflite_path, "wb") as f:
        f.write(tflite_int8)


def inspect_tflite(tflite_path: str):
    interpreter = tf.lite.Interpreter(model_path=tflite_path)
    interpreter.allocate_tensors()

    in_details = interpreter.get_input_details()
    out_details = interpreter.get_output_details()

    print(f"\n=== Inspect: {os.path.basename(tflite_path)} ===")
    for i, d in enumerate(in_details):
        print(f"IN {i} {d['name']} shape={d['shape']} dtype={d['dtype']} quant={d.get('quantization', None)}")
    for i, d in enumerate(out_details):
        print(f"OUT {i} {d['name']} shape={d['shape']} dtype={d['dtype']} quant={d.get('quantization', None)}")


# ---------------------------
# MAIN
# ---------------------------
if __name__ == "__main__":
    H5_MODELS = [
        ("EfficientNetB0_mnist.h5", "EfficientNetB0_mnist_int8.tflite"),
        ("MobileNetV2_mnist.h5", "MobileNetV2_mnist_int8.tflite"),
        ("ResNet50_mnist.h5", "ResNet50_mnist_int8.tflite"),
        ("SqueezeNet_mnist.h5", "SqueezeNet_mnist_int8.tflite"),
    ]

    INPUT_SHAPE = (32, 32, 3)          # model input (H,W,C)
    REP_INPUT_SHAPE = (1, *INPUT_SHAPE)
    NUM_CLASSES = 10

    for h5_name, out_name in H5_MODELS:
        print(f"\nConverting: {h5_name}  ->  {out_name}")
        try:
            if not os.path.exists(h5_name):
                print(f"SKIP: {h5_name} bulunamadı.")
                continue

            model, mode = load_model_with_fallback(
                h5_name,
                input_shape=INPUT_SHAPE,
                num_classes=NUM_CLASSES
            )
            print(f"Model load mode: {mode} | output_shape={model.output_shape}")

            convert_model_to_int8_tflite(model, out_name, rep_shape=REP_INPUT_SHAPE)
            print(f"OK: {out_name}")
            inspect_tflite(out_name)

        except Exception as e:
            print(f"\nFAILED: {h5_name} -> {e}")
            continue
