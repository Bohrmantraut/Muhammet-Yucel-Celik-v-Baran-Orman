/*
 * image_processing.h
 */

#ifndef INC_IMAGE_PROCESSING_H_
#define INC_IMAGE_PROCESSING_H_

#include "stdint.h"

// --- Temel Otsu Fonksiyonları ---
uint8_t compute_otsu_threshold(const uint8_t* image, uint32_t num_pixels);
void apply_otsu_threshold(const uint8_t* in, uint8_t* out, uint32_t num_pixels);
uint8_t compute_otsu_threshold_gray(const uint8_t* image, uint32_t num_pixels);
void apply_otsu_threshold_gray(const uint8_t* in, uint8_t* out, uint32_t num_pixels);

// --- Renkli Otsu Fonksiyonu ---
// Parametrelerin .c dosyasıyla birebir aynı olduğundan emin olduk
void Apply_Otsu_On_ColorImage_RGB565(uint16_t* pRGB565,
                                     uint32_t num_pixels,
                                     uint8_t* tempImg1,
                                     uint8_t* tempImg2);

// --- Morfolojik İşlemler ---
void apply_dilation_gray_3x3(const uint8_t* in, uint8_t* out, uint32_t width, uint32_t height);
void apply_erosion_gray_3x3(const uint8_t* in, uint8_t* out, uint32_t width, uint32_t height);
void apply_opening_gray_3x3(const uint8_t* in, uint8_t* out, uint32_t width, uint32_t height);
void apply_closing_gray_3x3(const uint8_t* in, uint8_t* out, uint32_t width, uint32_t height);

// --- Yardımcı Dönüştürme Fonksiyonları ---
void Write_Channel_To_RGB565(uint16_t* dst, const uint8_t* src, uint32_t num_pixels, char channel);
void Extract_Channel_From_RGB565(const uint16_t* src, uint8_t* dst, uint32_t num_pixels, char channel);
void Convert_Grayscale8bit_To_RGB565(const uint8_t* pIn8bit, uint16_t* pOut16bit, uint32_t num_pixels);
void Convert_RGB565_To_Grayscale8bit(const uint16_t* pIn16bit, uint8_t* pOut8bit, uint32_t num_pixels);

#endif /* INC_IMAGE_PROCESSING_H_ */
