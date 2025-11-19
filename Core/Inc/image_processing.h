/*
 * image_processing.h
 *
 *  Created on: Nov 9, 2025
 *      Author: muham
 */

#ifndef INC_IMAGE_PROCESSING_H_
#define INC_IMAGE_PROCESSING_H_

#include "stdint.h" // uint32_t, uint8_t için

/**
 * @brief  Bir grayscale görüntünün histogramını hesaplar.
 * @param  image_data: Görüntü piksellerini içeren diziye pointer.
 * @param  num_pixels: Görüntüdeki toplam piksel sayısı.
 * @param  histogram_output: Sonuçların yazılacağı 256 elemanlı uint32_t dizisi.
 */
void calculate_histogram(const uint8_t* image_data, uint32_t num_pixels, uint32_t* histogram_output);

/**
 * @brief  Histogram eşitleme uygular ve yeni bir görüntü oluşturur. (Q2b)
 * @param  original_image: Kaynak görüntü verisi.
 * @param  equalized_image: Eşitlenmiş görüntünün yazılacağı çıktı dizisi.
 * @param  histogram_input: Kaynak görüntünün calculate_histogram() ile elde edilmiş histogramı.
 * @param  num_pixels: Görüntüdeki toplam piksel sayısı.
 */
void apply_histogram_equalization(const uint8_t* original_image,
                                  uint8_t* equalized_image,
                                  uint32_t* histogram_input,
                                  uint32_t num_pixels);

/**
 * @brief  Bir görüntüye 3x3'lük bir kernel ile 2D konvolüsyon uygular. (Q3a)
 * @param  original_image: Kaynak görüntü (örn: maymun).
 * @param  filtered_image: Filtrelenmiş görüntünün yazılacağı çıktı dizisi.
 * @param  width: Görüntü genişliği (örn: 320).
 * @param  height: Görüntü yüksekliği (örn: 240).
 * @param  kernel: 9 elemanlı (3x3) float dizisi (kernel/maske).
 */
void apply_2d_convolution(const uint8_t* original_image,
                          uint8_t* filtered_image,
                          int width,
                          int height,
                          const float* kernel);

/* ... (apply_2d_convolution prototipi altina) ... */

/**
 * @brief  Bir görüntüye 3x3'lük median filtre uygular. (Q4a)
 * @param  original_image: Kaynak görüntü (örn: maymun).
 * @param  filtered_image: Filtrelenmiş görüntünün yazılacağı çıktı dizisi.
 * @param  width: Görüntü genişliği (örn: 320).
 * @param  height: Görüntü yüksekliği (örn: 240).
 */
void apply_median_filtering(const uint8_t* original_image,
                            uint8_t* filtered_image,
                            int width,
                            int height);

#endif /* INC_IMAGE_PROCESSING_H_ */


