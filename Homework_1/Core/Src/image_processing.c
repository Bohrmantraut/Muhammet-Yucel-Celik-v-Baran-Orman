/*
 * image_processing.c
 *
 * Created on: Nov 2, 2025
 * Author: muham
 */

#include "image_processing.h"
#include <stdlib.h>     // malloc fonksiyonu için gerekli
#include <string.h>     // NULL tanımı için gerekli (veya stdlib.h)
#include <math.h>
/**
 * @brief RAM'de yeni bir görüntü için yer ayırır ve ImageTypeDef yapısını başlatır.
 */
void IMAGE_init(ImageTypeDef *Pimg, uint8_t res, uint8_t format)
{
    // 1. Görüntü boyutlarını ayarla
    if (res == IMAGE_RES_QVGA) {
        Pimg->width = QVGA_WIDTH;
        Pimg->height = QVGA_HEIGHT;
    } else if (res == IMAGE_RES_QQVGA) {
        Pimg->width = QQVGA_WIDTH;
        Pimg->height = QQVGA_HEIGHT;
    } else {
        // Bilinmeyen çözünürlük, hata durumu
        Pimg->width = 0;
        Pimg->height = 0;
    }

    // 2. Görüntü formatını ayarla
    Pimg->format = format;

    // 3. Piksel başına byte (bpp) ve toplam boyutu (size) hesapla
    uint8_t bpp = 0; // Bit Per Pixel değil, Byte Per Pixel
    if (format == IMAGE_FORMAT_GRAYSCALE) {
        bpp = 1;
    } else if (format == IMAGE_FORMAT_RGB565 || format == IMAGE_FORMAT_YUV422) {
        bpp = 2;
    }

    Pimg->size = Pimg->width * Pimg->height * bpp;

    // 4. RAM'den görüntü boyutu kadar yer ayır (Heap)
    if (Pimg->size > 0) {
        Pimg->pData = (uint8_t*)malloc(Pimg->size);
    } else {
        Pimg->pData = NULL;
    }

    // (İsteğe bağlı) Hata ayıklama için:
    // if (Pimg->pData == NULL) {
    //    // RAM'de yer ayrılamadı!
    //    // Burada bir hata LED'i yakılabilir.
    // }
}

/**
 * @brief  Bir görüntünün negatifini alır (255 - piksel).
 * @note   Sadece GRAYSCALE formatı destekler.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 */
void IMAGE_apply_negative(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg)
{
    // 1. Güvenlik Kontrolleri
    // Görüntü pointer'ları geçerli mi?
    if (pSrcImg == NULL || pDstImg == NULL) return;
    // Görüntü verilerinin pointer'ları geçerli mi?
    if (pSrcImg->pData == NULL || pDstImg->pData == NULL) return;
    // Görüntü boyutları aynı mı?
    if (pSrcImg->size != pDstImg->size) return;
    // Görüntü formatları GRAYSCALE mi? (Bu formül sadece 1-byte'lık pikseller içindir)
    if (pSrcImg->format != IMAGE_FORMAT_GRAYSCALE || pDstImg->format != IMAGE_FORMAT_GRAYSCALE)
    {
        return;
    }

    // 2. Asıl İşlem
    // Kaynak görüntüdeki her pikseli tek tek döngüye al
    for (uint32_t i = 0; i < pSrcImg->size; i++)
    {
        // Kaynak pikseli oku
        uint8_t old_pixel = pSrcImg->pData[i];

        // Negatifini hesapla
        uint8_t new_pixel = 255 - old_pixel;

        // Yeni pikseli hedef görüntüye yaz
        pDstImg->pData[i] = new_pixel;
    }
}

/**
 * @brief  Bir görüntüye eşikleme uygular.
 * @note   Sadece GRAYSCALE formatı destekler.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 * @param  threshold: Eşik değeri (0-255).
 */
void IMAGE_apply_threshold(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, uint8_t threshold)
{
    // 1. Güvenlik Kontrolleri
    if (pSrcImg == NULL || pDstImg == NULL) return;
    if (pSrcImg->pData == NULL || pDstImg->pData == NULL) return;
    if (pSrcImg->size != pDstImg->size) return;
    if (pSrcImg->format != IMAGE_FORMAT_GRAYSCALE || pDstImg->format != IMAGE_FORMAT_GRAYSCALE)
    {
        return;
    }

    // 2. Asıl İşlem
    uint8_t new_pixel;
    for (uint32_t i = 0; i < pSrcImg->size; i++)
    {
        // Kaynak pikseli oku
        uint8_t old_pixel = pSrcImg->pData[i];

        // Eşikleme kuralını uygula
        if (old_pixel > threshold)
        {
            new_pixel = 255; // Beyaz
        }
        else
        {
            new_pixel = 0;   // Siyah
        }

        // Yeni pikseli hedef görüntüye yaz
        pDstImg->pData[i] = new_pixel;
    }
}

/**
 * @brief  Bir görüntüye gama düzeltmesi uygular.
 * @note   Sadece GRAYSCALE formatı destekler. FPU (float) işlemleri kullanır.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 * @param  gamma: Uygulanacak gama değeri
 */
void IMAGE_apply_gamma(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, float gamma)
{
    // 1. Güvenlik Kontrolleri
    if (pSrcImg == NULL || pDstImg == NULL) return;
    if (pSrcImg->pData == NULL || pDstImg->pData == NULL) return;
    if (pSrcImg->size != pDstImg->size) return;
    if (pSrcImg->format != IMAGE_FORMAT_GRAYSCALE || pDstImg->format != IMAGE_FORMAT_GRAYSCALE)
    {
        return;
    }

    // 2. Asıl İşlem

    // Gama formülündeki (1 / gamma) kısmını döngüden önce bir kez hesaplayalım.
    // Bu, 76,800 çarpma işleminden tasarruf sağlar.
    float exponent = gamma;

        for (uint32_t i = 0; i < pSrcImg->size; i++)
    {
        // Orijinal pikseli al
        uint8_t old_pixel = pSrcImg->pData[i];

        // Formülü uygula: encoded = ((original / 255) ^ (1 / gamma)) * 255

        // 1. (original / 255): Pikseli 0.0-1.0 arasına normalize et
        //    (float)old_pixel -> 'integer' olan 110'u, 'float' olan 110.0f'a çevirir
        //    Bunu yapmazsak C, 110/255 işlemini 0 (sıfır) olarak hesaplar!
        float norm_pixel = (float)old_pixel / 255.0f;

        // 2. (... ^ (1 / gamma)): 'powf' fonksiyonu ile üs al
        //    powf() -> 'float' versiyonudur, FPU için daha hızlıdır.
        float corrected_pixel = powf(norm_pixel, exponent);

        // 3. (... * 255): Tekrar 0-255 aralığına genişlet ve uint8_t'ye çevir
        uint8_t new_pixel = (uint8_t)(corrected_pixel * 255.0f);

        // Yeni pikseli hedef görüntüye yaz
        pDstImg->pData[i] = new_pixel;
    }
}

/**
 * @brief  Görüntüye parçalı doğrusal dönüşüm (kontrast germe) uygular.
 */
void IMAGE_apply_piecewise_linear(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, uint8_t r1, uint8_t r2)
{
    // 1. Güvenlik Kontrolleri
    if (pSrcImg == NULL || pDstImg == NULL) return;
    if (pSrcImg->pData == NULL || pDstImg->pData == NULL) return;
    if (pSrcImg->size != pDstImg->size) return;
    if (pSrcImg->format != IMAGE_FORMAT_GRAYSCALE || pDstImg->format != IMAGE_FORMAT_GRAYSCALE)
    {
        return;
    }
    // r1'in r2'den küçük olduğundan emin ol
    if (r1 >= r2) return;

    // 2. Asıl İşlem

    // (float)(r2 - r1) işlemini döngüden önce bir kez hesapla
    float denominator = (float)(r2 - r1);

    for (uint32_t i = 0; i < pSrcImg->size; i++)
    {
        uint8_t old_pixel = pSrcImg->pData[i];
        uint8_t new_pixel;

        if (old_pixel < r1)
        {
            new_pixel = 0;
        }
        else if (old_pixel > r2)
        {
            new_pixel = 255;
        }
        else
        {
            // Formül: 255 * ( (old_pixel - r1) / (r2 - r1) )
            // Kayan noktalı (float) hesaplama yapmak zorundayız
            float norm_pixel = (float)(old_pixel - r1) / denominator;
            new_pixel = (uint8_t)(norm_pixel * 255.0f);
        }

        // Yeni pikseli hedef görüntüye yaz
        pDstImg->pData[i] = new_pixel;
    }
}
