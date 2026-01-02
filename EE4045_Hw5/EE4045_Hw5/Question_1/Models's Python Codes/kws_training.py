import os
import numpy as np
import scipy.signal as sig
from mfcc_func import create_mfcc_features # Bu fonksiyonun bulunduğu dosya proje klasöründe olmalıdır
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay
import tensorflow as tf
from matplotlib import pyplot as plt
from sklearn.preprocessing import OneHotEncoder

# Ses kayıtlarının bulunduğu klasör
RECORDINGS_DIR = "recordings"

# Kayıtları listele
recordings_list = [(RECORDINGS_DIR, recording_path) for recording_path in os.listdir(RECORDINGS_DIR)]

# MFCC Parametreleri
FFTSize = 1024
sample_rate = 8000
numOfMelFilters = 20
numOfDctOutputs = 13
window = sig.get_window("hamming", FFTSize)

# Test ve Eğitim setlerini ayırma (yweweler kullanıcısı test, diğerleri eğitim)
test_list = {record for record in recordings_list if "yweweler" in record[1]}
train_list = set(recordings_list) - test_list

# Eğitim verisi için öznitelik çıkarımı
train_mfcc_features, train_labels = create_mfcc_features(
    train_list, FFTSize, sample_rate, numOfMelFilters, numOfDctOutputs, window)

# Test verisi için öznitelik çıkarımı
test_mfcc_features, test_labels = create_mfcc_features(
    test_list, FFTSize, sample_rate, numOfMelFilters, numOfDctOutputs, window)

# Etiketleri One-Hot Encoding formatına çevirme
ohe = OneHotEncoder()
train_labels_ohe = ohe.fit_transform(train_labels.reshape(-1, 1)).toarray()

# Modelin Oluşturulması (MLP)
# Giriş katmanı boyutu: numOfDctOutputs (13) * zaman boyutu (create_mfcc_features çıktısına bağlı)
# Model yapısı: Giriş -> Dense(100, ReLU) -> Dense(100, ReLU) -> Çıkış(10, Softmax)
model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(100, input_shape=[train_mfcc_features.shape[1]], activation="relu"),
    tf.keras.layers.Dense(100, activation="relu"),
    tf.keras.layers.Dense(10, activation="softmax")
])

# Kategorileri belirleme (0-9 arası rakamlar)
categories, test_labels = np.unique(test_labels, return_inverse=True)

# Modeli Derleme (Adam optimizer ve Categorical Crossentropy kaybı ile)
model.compile(loss=tf.keras.losses.CategoricalCrossentropy(),
              optimizer=tf.keras.optimizers.Adam(1e-3),
              metrics=['accuracy'])

# Modeli Eğitme
model.fit(train_mfcc_features, train_labels_ohe, epochs=200, verbose=1)

# Test verisi ile tahmin yapma ve Karmaşıklık Matrisi (Confusion Matrix) çizdirme
nn_preds = model.predict(test_mfcc_features)
predicted_classes = np.argmax(nn_preds, axis=1)
conf_matrix = confusion_matrix(test_labels, predicted_classes)

cm_display = ConfusionMatrixDisplay(confusion_matrix=conf_matrix, display_labels=categories)
cm_display.plot()
cm_display.ax_.set_title("Neural Network Confusion Matrix")
plt.show()

# Eğitilen modeli kaydetme
model.save("mlp_kws_model.h5")
print("Model 'mlp_kws_model.h5' olarak kaydedildi.")