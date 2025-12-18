/*
 * app_homeworks.h
 */

#ifndef INC_APP_HOMEWORKS_H_
#define INC_APP_HOMEWORKS_H_

#include "main.h"
#include "lib_serialimage.h" // IMAGE_HandleTypeDef için

// Hangi ödevin çalışacağını seçmek için enum
typedef enum {
    MODE_IDLE = 0,
    MODE_Q1_OTSU_GRAY,
    MODE_Q2_OTSU_COLOR,
    MODE_Q3_DILATION,
    MODE_Q3_EROSION,
    MODE_Q3_OPENING,
    MODE_Q3_CLOSING
} AppMode_t;

// Tüm işi yapacak ana fonksiyonlar
void App_Run_Q1_OtsuGray(IMAGE_HandleTypeDef* hImage, uint8_t* pGray, uint8_t* pProcess, uint32_t numPixels);
void App_Run_Q2_OtsuColor(IMAGE_HandleTypeDef* hImage, uint16_t* pRGB, uint8_t* pTemp1, uint8_t* pTemp2, uint32_t numPixels);
void App_Run_Q3_Morph(IMAGE_HandleTypeDef* hImage, uint8_t* pGray, uint8_t* pProcess, uint32_t width, uint32_t height, uint8_t operationType);

#endif /* INC_APP_HOMEWORKS_H_ */
