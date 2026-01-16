import numpy as np
import cv2
from pathlib import Path
import os

# =========================
# SETTINGS
# =========================
# Dosyanın tam yolunu buraya yazın veya aynı klasörde olduğundan emin olun
RAW_BIN_PATH = r"C:\Users\muham\Desktop\EE4065\EE4065 Final\Question1\RAW_000000.bin"
WIDTH, HEIGHT = 640, 480
TARGET_PIXELS = 1000

# ESP32 genellikle Big-Endian (>u2) kullanır
RGB565_DTYPE = np.dtype(">u2") 

# Çıktı klasörünü oluştur
OUT_DIR = Path("out")
OUT_DIR.mkdir(exist_ok=True)

# =========================
# RGB565 helpers
# =========================
def rgb565_to_rgb888(u16_img: np.ndarray) -> np.ndarray:
    """RGB565 (uint16) -> RGB888 (uint8 HxWx3)"""
    r5 = (u16_img >> 11) & 0x1F
    g6 = (u16_img >> 5)  & 0x3F
    b5 =  u16_img        & 0x1F

    r8 = (r5 * 255 // 31).astype(np.uint8)
    g8 = (g6 * 255 // 63).astype(np.uint8)
    b8 = (b5 * 255 // 31).astype(np.uint8)

    return np.dstack([r8, g8, b8])



def rgb565_to_gray_exact(u16_img: np.ndarray) -> np.ndarray:
    """
    ESP32 integer math:
      r8 = (r5*527 + 23) >> 6
      g8 = (g6*259 + 33) >> 6
      b8 = (b5*527 + 23) >> 6
      gray = (r8*77 + g8*150 + b8*29) >> 8
    """
    r5 = (u16_img >> 11) & 0x1F
    g6 = (u16_img >> 5)  & 0x3F
    b5 =  u16_img        & 0x1F

    r8 = ((r5.astype(np.uint32) * 527 + 23) >> 6).astype(np.uint32)
    g8 = ((g6.astype(np.uint32) * 259 + 33) >> 6).astype(np.uint32)
    b8 = ((b5.astype(np.uint32) * 527 + 23) >> 6).astype(np.uint32)

    gray = ((r8 * 77 + g8 * 150 + b8 * 29) >> 8).astype(np.uint8)
    return gray

# =========================
# Thresholding
# =========================
def find_threshold_for_target(gray_u8: np.ndarray, target_pixels: int) -> tuple[int, int]:
    """En parlak 1000 pikseli bulmak için threshold hesaplar."""
    hist = np.bincount(gray_u8.ravel(), minlength=256).astype(np.uint32)

    cum = 0
    best_thr = 255
    best_diff = 2**31 - 1
    best_selected = 0

    for t in range(255, -1, -1):
        cum += int(hist[t])
        selected = int(cum)
        diff = abs(selected - target_pixels)
        if diff < best_diff:
            best_diff = diff
            best_thr = t
            best_selected = selected
            if best_diff == 0:
                break

    return best_thr, best_selected

# =========================
# MAIN
# =========================
def main():
    raw_path = Path(RAW_BIN_PATH)
    
    if not raw_path.exists():
        print(f"Hata: '{RAW_BIN_PATH}' dosyası bulunamadı!")
        print(f"Lütfen dosyayı şu klasöre koyun: {os.getcwd()}")
        return

    data = raw_path.read_bytes()
    expected = WIDTH * HEIGHT * 2
    
    if len(data) != expected:
        print(f"Boyut hatası! Beklenen: {expected}, Gelen: {len(data)}")
        return

    # 1) Veriyi oku ve yükle
    u16_img = np.frombuffer(data, dtype=RGB565_DTYPE).reshape((HEIGHT, WIDTH))

    # 2) Orijinali kaydet
    rgb_orig = rgb565_to_rgb888(u16_img)
    cv2.imwrite(str(OUT_DIR / "original.png"), cv2.cvtColor(rgb_orig, cv2.COLOR_RGB2BGR))

    # 3) Eşik değerini bul
    gray = rgb565_to_gray_exact(u16_img)
    thr, selected = find_threshold_for_target(gray, TARGET_PIXELS)
    print(f"--- SONUÇLAR ---")
    print(f"Threshold: {thr}")
    print(f"Bulunan Piksel Sayısı: {selected}")
    print(f"Hedeflenen: {TARGET_PIXELS}")

    # 4) Eşiği uygula ve Binary (0xFFFF / 0x0000) yap
    u16_thr = np.where(gray >= thr, np.uint16(0xFFFF), np.uint16(0x0000))

    # 5) Sonuçları kaydet
    rgb_thr = rgb565_to_rgb888(u16_thr)
    cv2.imwrite(str(OUT_DIR / "thresholded.png"), cv2.cvtColor(rgb_thr, cv2.COLOR_RGB2BGR))
    
    mask = (u16_thr != 0).astype(np.uint8) * 255
    cv2.imwrite(str(OUT_DIR / "mask.png"), mask)

    print("\nİşlem tamamlandı. 'out' klasörüne bakınız.")

if __name__ == "__main__":
    main()