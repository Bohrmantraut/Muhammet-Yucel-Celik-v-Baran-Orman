import numpy as np
import librosa
import os

# Parametreler (Eğitimdeki ile birebir aynı olmalı)
FFTSize = 1024
sample_rate = 8000
numOfMelFilters = 20
numOfDctOutputs = 13
fixed_audio_length = 8000 # 1 saniye

def process_my_voice(filename):
    # 1. Dosyayı Yükle (Otomatik olarak 8kHz'e dönüştürür)
    print(f"Dosya isleniyor: {filename}...")
    try:
        y, sr = librosa.load(filename, sr=sample_rate)
    except FileNotFoundError:
        print(f"HATA: '{filename}' dosyasi bulunamadi! Lutfen dosya yolunu kontrol edin.")
        return

    # 2. Boyut Ayarlama (1 saniye / 8000 sample olacak şekilde)
    if len(y) < fixed_audio_length:
        # Kısa ise sonuna sıfır ekle (padding)
        y = np.pad(y, (0, fixed_audio_length - len(y)), mode='constant')
    else:
        # Uzun ise fazlasını kırp
        y = y[:fixed_audio_length]

    # 3. MFCC Özellik Çıkarımı (mfcc_func.py mantığıyla aynı)
    # librosa.feature.mfcc kullanırken parametrelerin eğitimle aynı olması kritik.
    window = np.hamming(FFTSize) # scipy.signal.get_window("hamming", FFTSize) yerine basit numpy
    
    mfcc = librosa.feature.mfcc(
        y=y, 
        sr=sr, 
        n_fft=FFTSize, 
        hop_length=FFTSize // 2, 
        n_mels=numOfMelFilters, 
        n_mfcc=numOfDctOutputs,
        window='hamming' # librosa string kabul eder
    )
    
    # Transpose ve Flatten işlemleri
    mfcc = mfcc.T
    features = mfcc.flatten()
    
    # 4. C Dizisi Oluşturma
    print(f"\n--- '{filename}' ICIN C KODU ---")
    print(f"Veri Boyutu: {features.shape}")
    
    c_array_str = "const float test_input_data[] = {\n    "
    for i, val in enumerate(features):
        c_array_str += f"{val:.6f}f, "
        if (i + 1) % 10 == 0:
            c_array_str += "\n    "
    c_array_str = c_array_str.rstrip(", \n    ") + "\n};"
    
    print("-" * 50)
    print(c_array_str)
    print("-" * 50)
    print("Yukaridaki kodu kopyalayip STM32 main.c dosyaniza yapistirin.")

# --- ÇALIŞTIR ---
# Dosya adınız neyse buraya yazın
process_my_voice("five1.wav")