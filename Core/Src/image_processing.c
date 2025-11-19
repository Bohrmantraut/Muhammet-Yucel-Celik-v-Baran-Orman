/*
 * image_processing.c
 *
 *  Created on: Nov 9, 2025
 *      Author: muham
 */

#include "image_processing.h"

/**
 * @brief  9 elemanlı bir uint8_t dizisini sıralamak için basit bir
 * "Insertion Sort" (Araya Ekleme Sıralaması) uygular.
 * @param  arr: Sıralanacak 9 elemanlı dizi.
 */
static void sort_9_elements(uint8_t* arr)
{
    int i, j;
    uint8_t key;
    for (i = 1; i < 9; i++) {
        key = arr[i];
        j = i - 1;

        // key'den büyük olan elemanları bir pozisyon ileri kaydır
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

/**
 * @brief  Bir grayscale görüntünün histogramını hesaplar. (Q1a)
 * @param  image_data: Görüntü piksellerini içeren diziye pointer.
 * @param  num_pixels: Görüntüdeki toplam piksel sayısı.
 * @param  histogram_output: Sonuçların yazılacağı 256 elemanlı uint32_t dizisi.
 */
void calculate_histogram(const uint8_t* image_data, uint32_t num_pixels, uint32_t* histogram_output)
{
    // 1. Adım: Histogram dizisini sıfırlama
    // 0-255 arası her piksel değeri için bir sayaç
    for (int i = 0; i < 256; i++)
    {
        histogram_output[i] = 0;
    }

    // 2. Adım: Görüntüyü tara ve sayaçları artır
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        // Mevcut pikselin parlaklık değerini al (0-255)
        uint8_t pixel_value = image_data[i];

        // Bu parlaklık değerine karşılık gelen sayacı artır
        histogram_output[pixel_value]++;
    }
}


/**
 * @brief  Histogram eşitleme uygular ve yeni bir görüntü oluşturur. (Q2b)
 */
void apply_histogram_equalization(const uint8_t* original_image,
                                  uint8_t* equalized_image,
                                  uint32_t* histogram_input,
                                  uint32_t num_pixels)
{
    // Adım 1: Histogramdan CDF (Kümülatif Dağılım Fonksiyonu) hesapla
    // (Teorideki (1/N) * Toplam(n_j) kısmı)
    uint32_t cdf[256];
    cdf[0] = histogram_input[0];
    for (int i = 1; i < 256; i++)
    {
        cdf[i] = cdf[i - 1] + histogram_input[i];
    }

    // Adım 2: Dönüşüm için Arama Tablosu (Lookup Table - LUT) oluştur
    // Bu, Q2a'daki s_k = 255 * (1/N) * CDF(k) formülünü uygular.
    // (Optimizasyon: Ölçekleme faktörünü (255.0f / num_pixels) önceden hesapla)
    uint8_t lookup_table[256];
    float scale_factor = 255.0f / (float)num_pixels;

    for (int i = 0; i < 256; i++)
    {
        // (float)cdf[i] * scale_factor -> Bu, (L-1) * CDF(k) işlemini yapar
        // + 0.5f eklemek, en yakın tam sayıya doğru yuvarlama (rounding) sağlar.
        lookup_table[i] = (uint8_t)( (float)cdf[i] * scale_factor + 0.5f );
    }

    // Adım 3: LUT kullanarak yeni görüntüyü oluştur
    // Orijinal görüntüdeki her pikseli (değer: v) al
    // ve yeni görüntüye lookup_table[v] değerini yaz.
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        equalized_image[i] = lookup_table[ original_image[i] ];
    }
}


/**
 * @brief  Bir görüntüye 3x3'lük bir kernel ile 2D konvolüsyon uygular. (Q3a)
 * Kenarlar 'zero-padding' (sıfır doldurma) yöntemiyle ele alınır.
 */
void apply_2d_convolution(const uint8_t* original_image,
                          uint8_t* filtered_image,
                          int width,
                          int height,
                          const float* kernel)
{
    // Görüntünün her pikseli (satır satır) üzerinde gezin
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Bu (x, y) pikseli için konvolüsyon toplamını hesapla
            float sum = 0.0f;

            // 3x3'lük kernel üzerinde gezin (-1'den +1'e)
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    // Komşu pikselin koordinatını bul
                    int pixel_x = x + kx;
                    int pixel_y = y + ky;

                    // Kernel'in indisini hesapla (0-8)
                    // (ky+1)*3 + (kx+1)
                    // (-1,-1) -> (0*3)+0 = 0
                    // ( 0, 0) -> (1*3)+1 = 4
                    // ( 1, 1) -> (2*3)+2 = 8
                    int kernel_index = (ky + 1) * 3 + (kx + 1);

                    // KENAR KONTROLÜ (Zero Padding)
                    // Eğer komşu piksel görüntü sınırları dışındaysa,
                    // değeri 0 olarak kabul et (toplama etki etme).
                    if (pixel_x >= 0 && pixel_x < width && pixel_y >= 0 && pixel_y < height)
                    {
                        // Komşu pikselin 1D dizideki indisini bul
                        int image_index = pixel_y * width + pixel_x;

                        // Toplama ekle
                        sum += (float)original_image[image_index] * kernel[kernel_index];
                    }
                }
            }

            // Adım 3: Sonucu [0, 255] aralığına sıkıştır (Clamp)
            // High-pass filtreler negatif sonuçlar üretebilir
            // Low-pass filtreler 255'ten büyük sonuçlar üretebilir
            if (sum < 0.0f)   sum = 0.0f;
            if (sum > 255.0f) sum = 255.0f;

            // Sonucu çıktı görüntüsüne yaz (yuvarlama için +0.5f)
            filtered_image[y * width + x] = (uint8_t)(sum + 0.5f);
        }
    }
}

/**
 * @brief  Bir görüntüye 3x3'lük median filtre uygular. (Q4a)
 * Kenarlar 'zero-padding' (sıfır doldurma) yöntemiyle ele alınır.
 */
void apply_median_filtering(const uint8_t* original_image,
                            uint8_t* filtered_image,
                            int width,
                            int height)
{
    // 3x3'lük penceredeki pikselleri geçici olarak tutacak dizi
    uint8_t window[9];

    // Görüntünün her pikseli (satır satır) üzerinde gezin
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // 1. Adım: 3x3'lük pencereyi doldur
            int i = 0; // window dizisinin indeksi
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int pixel_x = x + kx;
                    int pixel_y = y + ky;

                    // KENAR KONTROLÜ (Zero Padding)
                    // Görüntü dışındaysak 0 (siyah) olarak kabul et
                    if (pixel_x >= 0 && pixel_x < width && pixel_y >= 0 && pixel_y < height)
                    {
                        window[i] = original_image[pixel_y * width + pixel_x];
                    }
                    else
                    {
                        window[i] = 0;
                    }
                    i++;
                }
            }

            // 2. Adım: Penceredeki 9 pikseli sırala
            sort_9_elements(window);

            // 3. Adım: Ortanca değeri (median) bul (9 elemanlı dizide 5. eleman)
            // İndeks 0'dan başladığı için [4] olur.
            uint8_t median_value = window[4];

            // 4. Adım: Sonucu çıktı görüntüsüne yaz
            filtered_image[y * width + x] = median_value;
        }
    }
}
