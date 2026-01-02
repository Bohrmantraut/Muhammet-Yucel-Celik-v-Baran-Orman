import numpy as np
import librosa
import os

def create_mfcc_features(recordings_list, FFTSize, sample_rate, numOfMelFilters, numOfDctOutputs, window):
    """
    Ses dosyalarından MFCC özniteliklerini çıkarır.
    Kitaptaki mfcc_func.py dosyasının işlevini librosa ile simüle eder.
    """
    features = []
    labels = []
    
    # Tüm ses dosyalarını sabit bir uzunluğa (örneğin 1 saniye = 8000 örnek) sabitleyelim.
    # Bu, Yapay Sinir Ağına (MLP) giren verilerin boyutunun eşit olmasını sağlar.
    fixed_audio_length = sample_rate * 1  # 1 saniye (8000 sample)

    for record in recordings_list:
        # Kayıt listesi (klasör, dosya_adı) formatındaysa
        if isinstance(record, tuple) or isinstance(record, list):
            folder, filename = record
            full_path = os.path.join(folder, filename)
        else:
            # Sadece dosya yoluysa
            full_path = record
            filename = os.path.basename(full_path)

        # Ses dosyasını yükle (8kHz olarak)
        y, sr = librosa.load(full_path, sr=sample_rate)

        # Ses dosyasını sabit uzunluğa getir (Padding veya Truncating)
        if len(y) < fixed_audio_length:
            y = np.pad(y, (0, fixed_audio_length - len(y)), mode='constant')
        else:
            y = y[:fixed_audio_length]

        # MFCC Hesapla
        # Kitapta FFTSize pencere boyutu olarak kullanılıyor, hop_length genelde yarısıdır.
        mfcc = librosa.feature.mfcc(
            y=y, 
            sr=sr, 
            n_fft=FFTSize, 
            hop_length=FFTSize // 2, 
            n_mels=numOfMelFilters, 
            n_mfcc=numOfDctOutputs,
            window=window
        )
        
        # MFCC çıktısı (n_mfcc, zaman) şeklindedir. Bunu (zaman, n_mfcc) yapalım.
        mfcc = mfcc.T
        
        # Veriyi düzleştir (Flatten) - MLP girişi için tek boyutlu vektör haline getir
        features.append(mfcc.flatten())
        
        # Etiketi dosya isminden çıkar (Örn: 0_jackson_0.wav -> Etiket: 0)
        label = int(filename.split('_')[0])
        labels.append(label)

    return np.array(features), np.array(labels)