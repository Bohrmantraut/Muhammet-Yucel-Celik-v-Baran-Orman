/*
 * app_homeworks.c
 */

#include "app_homeworks.h"
#include "image_processing.h" // Algoritmalarımız burada

// --- Soru 1: Grayscale Otsu ---
void App_Run_Q1_OtsuGray(IMAGE_HandleTypeDef* hImage, uint8_t* pGray, uint8_t* pProcess, uint32_t numPixels)
{
    // 1. PC'den veri bekle (Blocking)
    if (LIB_SERIAL_IMG_Receive(hImage) == SERIAL_OK)
    {
        // Dönüştür
        Convert_RGB565_To_Grayscale8bit((uint16_t*)hImage->pData, pGray, numPixels);

        // İşle
        apply_otsu_threshold_gray(pGray, pProcess, numPixels);

        // Geri Dönüştür
        Convert_Grayscale8bit_To_RGB565(pProcess, (uint16_t*)hImage->pData, numPixels);

        // Gönder
        LIB_SERIAL_IMG_Transmit(hImage);
    }
}

// --- Soru 2: Renkli Otsu ---
void App_Run_Q2_OtsuColor(IMAGE_HandleTypeDef* hImage, uint16_t* pRGB, uint8_t* pTemp1, uint8_t* pTemp2, uint32_t numPixels)
{
    if (LIB_SERIAL_IMG_Receive(hImage) == SERIAL_OK)
    {
        // Direkt işlem yap (Fonksiyonumuz zaten pRGB üzerinde çalışıyor)
        Apply_Otsu_On_ColorImage_RGB565(pRGB, numPixels, pTemp1, pTemp2);

        // Gönder
        LIB_SERIAL_IMG_Transmit(hImage);
    }
}

// --- Soru 3: Morfolojik İşlemler (Dilation, Erosion, vs.) ---
// operationType: 0=Dilation, 1=Erosion, 2=Opening, 3=Closing
void App_Run_Q3_Morph(IMAGE_HandleTypeDef* hImage, uint8_t* pGray, uint8_t* pProcess, uint32_t width, uint32_t height, uint8_t operationType)
{
    if (LIB_SERIAL_IMG_Receive(hImage) == SERIAL_OK)
    {
        uint32_t numPixels = width * height;

        // RGB -> Gray
        Convert_RGB565_To_Grayscale8bit((uint16_t*)hImage->pData, pGray, numPixels);

        // İstenen işlemi seç
        switch(operationType)
        {
            case 0: apply_dilation_gray_3x3(pGray, pProcess, width, height); break;
            case 1: apply_erosion_gray_3x3(pGray, pProcess, width, height); break;
            case 2: apply_opening_gray_3x3(pGray, pProcess, width, height); break;
            case 3: apply_closing_gray_3x3(pGray, pProcess, width, height); break;
        }

        // Gray -> RGB
        Convert_Grayscale8bit_To_RGB565(pProcess, (uint16_t*)hImage->pData, numPixels);

        // Gönder
        LIB_SERIAL_IMG_Transmit(hImage);
    }
}
