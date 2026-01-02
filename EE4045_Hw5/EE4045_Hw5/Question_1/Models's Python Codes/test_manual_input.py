import re
import numpy as np
import tensorflow as tf

CLASSES = ['zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine']

MODEL_PATH = r"mlp_kws_model.h5"

# ✅ Buraya C array'ini yapıştır (const float test_input_data[] = { ... }; dahil olabilir)
C_ARRAY_TEXT = r"""
const float test_input_data[] = {
    -154.928986f, 23.385689f, 8.458590f, 3.041790f, -11.457752f, -4.804412f, -11.056170f, -9.720474f, -8.489189f, -6.095830f,
    -0.654269f, -2.510599f, -1.922349f, -72.506241f, 61.743332f, 3.410263f, -10.142792f, -9.010780f, 7.435240f, -1.323824f,
    -1.618741f, 2.774549f, 5.589571f, 1.703612f, -4.044213f, 0.641237f, -51.244347f, 77.733192f, -5.894693f, -20.818047f,
    0.900517f, 9.834179f, -5.526544f, -2.445736f, 10.906787f, -2.275260f, 6.051255f, -5.637564f, 1.686063f, -57.486271f,
    69.534073f, -16.685230f, -11.132300f, -0.689024f, 13.448252f, -5.309526f, 4.046693f, 4.403437f, -1.109519f, -0.542012f,
    -1.671165f, 2.626287f, -83.783890f, 49.671143f, -13.036543f, 0.019981f, -0.432177f, 11.990392f, -2.441794f, 11.596338f,
    0.925236f, -1.967037f, 3.005574f, -0.630387f, 2.651869f, -82.745270f, 43.820732f, 5.794074f, 8.875984f, -14.195602f,
    0.357003f, 3.065966f, 12.156353f, 1.064639f, 2.766534f, 1.579679f, -2.470376f, 0.853845f, -138.140091f, 25.213860f,
    15.636736f, 14.833538f, -19.253300f, 0.608680f, 1.020174f, 6.095763f, 2.840493f, 4.467783f, 0.953852f, 0.289363f,
    4.398521f, -153.081512f, 6.004027f, 14.675364f, 15.613729f, -2.998344f, 4.652536f, 1.411593f, 6.424456f, -0.231585f,
    2.611815f, 3.558175f, 1.229811f, 0.173426f, -168.531494f, 9.678284f, 7.140454f, 12.204716f, -3.561239f, 4.583701f,
    -3.549389f, 3.772939f, 1.214527f, -4.059633f, 0.966169f, 3.690001f, 2.088814f, -275.954315f, 7.293756f, 7.287298f,
    10.194265f, -3.813112f, 1.230821f, -3.308901f, -0.691760f, -1.024043f, -0.755463f, -1.182107f, 1.448441f, 2.023072f,
    -308.401428f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000004f, 0.000000f,
    0.000000f, 0.000000f, 0.000003f, -308.401428f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
    0.000000f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000003f, -308.401428f, 0.000000f, 0.000000f, 0.000000f,
    0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000003f, -308.401428f,
    0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000004f, 0.000000f, 0.000000f,
    0.000000f, 0.000003f, -308.401428f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
    0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000003f, -308.401428f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
    0.000000f, 0.000000f, 0.000000f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000003f
};
"""

def parse_c_float_array(text: str) -> np.ndarray:
    # float sayıları yakala: -12.34, 0.000004, 3.0e-5 vs; sonundaki 'f' olsa da olur
    nums = re.findall(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?', text)
    arr = np.array([float(x) for x in nums], dtype=np.float32)
    return arr

def main():
    model = tf.keras.models.load_model(MODEL_PATH, compile=False)
    print("Model başarıyla yüklendi.")
    
    class_order = np.load("class_order.npy")      # örn: [0 1 2 ... 9]
    # İstersen isimli:
    CLASSES = [f"digit_{d}" for d in class_order]  # ["digit_0",...]

    x = parse_c_float_array(C_ARRAY_TEXT)
    print("Örnek sayısı:", x.size)

    # modelin beklediği giriş boyutu (loglarında 208 gördük)
    expected = model.input_shape[-1]
    print("Model input dim:", expected)
    print("Örnek sayısı:", x.size)

    if x.size != expected:
      raise ValueError(f"Input size mismatch: got {x.size}, expected {expected}")

    x = x.reshape(1, expected)
    print("Giriş şekli:", x.shape)

    preds = model.predict(x, verbose=0)
    idx = int(np.argmax(preds))
    conf = float(np.max(preds))

    print("-" * 30)
    if idx < len(CLASSES):
        print(f"Tahmin: {CLASSES[idx]} (index={idx})")
    else:
        print(f"Tahmin index={idx} (CLASSES listesinde yok)")
    print(f"Güven: %{conf * 100:.2f}")
    print("-" * 30)

    print("Tüm olasılıklar:")
    for i, p in enumerate(preds[0]):
        label = CLASSES[i] if i < len(CLASSES) else f"class_{i}"
        print(f"{label:>6}: {p:.6f}")

if __name__ == "__main__":
    main()
