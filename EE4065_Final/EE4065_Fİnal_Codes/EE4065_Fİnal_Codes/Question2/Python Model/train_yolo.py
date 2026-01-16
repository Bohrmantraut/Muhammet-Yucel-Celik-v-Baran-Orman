# train_yolo.py
from pathlib import Path
import sys

def main():
    # --- 1) Proje kökü (bu scriptin bulunduğu klasör) ---
    ROOT = Path(__file__).resolve().parent

    # --- 2) data.yaml yolu (burayı gerekirse değiştir) ---
    data_yaml = ROOT / "data.yaml"   # aynı klasördeyse
    # data_yaml = Path(r"C:\Users\muham\Desktop\EE4065 Final\data.yaml")  # istersen tam yol

    if not data_yaml.exists():
        print(f"[ERROR] data.yaml bulunamadı: {data_yaml}")
        print("İpucu: data.yaml dosyanı bu scriptin yanına koy veya yukarıdaki tam yolu aç.")
        sys.exit(1)

    # --- 3) GPU kontrol ---
    import torch
    print("Torch:", torch.__version__)
    print("CUDA available:", torch.cuda.is_available())
    if torch.cuda.is_available():
        print("GPU:", torch.cuda.get_device_name(0))
        device = 0
    else:
        print("[WARN] CUDA yok, CPU ile devam edilecek.")
        device = "cpu"

    # --- 4) YOLO train ---
    from ultralytics import YOLO

    model = YOLO("yolov8n.pt")  # pre-trained başlangıç

    runs_dir = ROOT / "runs"  # çıktı klasörü

    results = model.train(
        data=str(data_yaml),
        epochs=5,
        imgsz=640,
        batch=16,        # 1650 Ti 4GB ise gerekirse 8'e düşür
        device=device,
        project=str(runs_dir),
        name="digits_train",
        exist_ok=True,
        workers=8,
        verbose=True
    )

    print("\n✅ Training başladı/bitti. Sonuç klasörü:")
    print(runs_dir / "detect" / "digits_train")
    return results

if __name__ == "__main__":
    main()
