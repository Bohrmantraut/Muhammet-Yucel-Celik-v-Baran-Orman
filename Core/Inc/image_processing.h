/*
 * image_processing.h
 *
 * Created on: Nov 2, 2025
 * Author: muham
 */

#ifndef INC_IMAGE_PROCESSING_H_
#define INC_IMAGE_PROCESSING_H_

#include "stdint.h" // uint8_t, uint16_t vb. için gerekli

/* Görüntü Formatı Sabitleri */
#define IMAGE_FORMAT_GRAYSCALE  0   // 8-bit Gri tonlamalı (Piksel başına 1 byte)
#define IMAGE_FORMAT_RGB565     1   // 16-bit Renkli (Piksel başına 2 byte)
#define IMAGE_FORMAT_YUV422     2   // 16-bit Renkli (Piksel başına 2 byte)

/* Görüntü Çözünürlük Sabitleri */
#define IMAGE_RES_QQVGA         0   // 160x120
#define IMAGE_RES_QVGA          1   // 320x240

/* Çözünürlük Boyutları (Dahili kullanım için) */
#define QQVGA_WIDTH             160
#define QQVGA_HEIGHT            120
#define QVGA_WIDTH              320
#define QVGA_HEIGHT             240

/**
 * @brief Görüntü verisi ve özelliklerini tutan ana yapı
 */
typedef struct _image{
    uint8_t *pData;     // Görüntü piksellerinin tutulduğu dizinin pointer'ı
    uint16_t width;     // Görüntü genişliği
    uint16_t height;    // Görüntü yüksekliği
    uint32_t size;      // Görüntü boyutu (byte cinsinden)
    uint8_t  format;    // Görüntü formatı (IMAGE_FORMAT_... sabitleri)
} ImageTypeDef;


/**
 * @brief RAM'de yeni bir görüntü için yer ayırır ve ImageTypeDef yapısını başlatır.
 * @param Pimg: Başlatılacak olan ImageTypeDef yapısının pointer'ı
 * @param res: Çözünürlük (IMAGE_RES_QVGA veya IMAGE_RES_QQVGA)
 * @param format: Görüntü formatı (IMAGE_FORMAT_GRAYSCALE, IMAGE_FORMAT_RGB565, vb.)
 */
void IMAGE_init(ImageTypeDef *Pimg, uint8_t res, uint8_t format);
/**
 * @brief  Bir görüntünün negatifini alır (255 - piksel).
 * @note   Sadece GRAYSCALE formatı destekler.
 * @param  pSrcImg: Kaynak görüntü (Örn: Flash'taki)
 * @param  pDstImg: Hedef görüntü (Örn: RAM'deki)
 */
void IMAGE_apply_negative(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg);

/**
 * @brief  Bir görüntüye eşikleme uygular.
 * @note   Sadece GRAYSCALE formatı destekler.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 * @param  threshold: Eşik değeri (0-255). Bu değerin altı 0, üstü 255 olur.
 */
void IMAGE_apply_threshold(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, uint8_t threshold);

/**
 * @brief  Bir görüntüye gama düzeltmesi uygular.
 * @note   Sadece GRAYSCALE formatı destekler. FPU (float) işlemleri kullanır.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 * @param  gamma: Uygulanacak gama değeri (örn: 3.0 veya 1.0/3.0)
 */
void IMAGE_apply_gamma(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, float gamma);

/**
 * @brief  Görüntüye parçalı doğrusal dönüşüm (kontrast germe) uygular.
 * @note   Sadece GRAYSCALE formatı destekler.
 * @param  pSrcImg: Kaynak görüntü
 * @param  pDstImg: Hedef görüntü
 * @param  r1: Alt giriş eşiği (bu değerin altı 0 olur)
 * @param  r2: Üst giriş eşiği (bu değerin üstü 255 olur)
 */
void IMAGE_apply_piecewise_linear(ImageTypeDef *pSrcImg, ImageTypeDef *pDstImg, uint8_t r1, uint8_t r2);

#endif /* INC_IMAGE_PROCESSING_H_ */
