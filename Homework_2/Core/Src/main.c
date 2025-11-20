/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "image_processing.h"
#include "lib_image.h"
#include "lib_serialimage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define IMG_WIDTH 128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT) // 16384 olmalı

// Q1 için (Orijinal histogram)
uint32_t histogram_results[256]; // (256 * 4 byte) = 1,024 byte RAM

// Q2c için (Eşitlenmiş histogram)
uint32_t histogram_equalized_results[256]; // (256 * 4 byte) = 1,024 byte RAM

// TOPLAM KULLANIM: 1024 + 76800 + 1024 = 78,848 byte.
// Bu, 128KB limitimizin çok altında ve güvenlidir.
// -------------------------------------------------------------------


// Q3 Filtre Kernelleri (Bunlar Flash'ta saklanır, RAM kullanmaz)
// Q3b: Low-Pass Filtre (Averaging / Box Blur)
const float kernel_low_pass[9] = {
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f
};

// Q3c: High-Pass Filtre (Laplacian - Kenar Tespiti)
const float kernel_high_pass[9] = {
     0.0f, -1.0f,  0.0f,
    -1.0f,  4.0f, -1.0f,
     0.0f, -1.0f,  0.0f
};

// 1. GİRİŞ/ÇIKIŞ BUFFER'LARI (RGB565 formatında)
// Seri porttan gelen ve giden veriler bu yapıyı kullanır (2 Byte/Piksel).
volatile uint8_t pImage_RGB565[IMG_WIDTH * IMG_HEIGHT * 2];

// 2. İŞLEME BUFFER'LARI (8-bit Gri Tonlama formatında)
// Görüntü işleme bu buffer üzerinde yapılır (1 Byte/Piksel).
uint8_t processed_image[NUM_PIXELS];
uint8_t original_gray_image[NUM_PIXELS];
// Diğer histogram ve kernel tanımları burada kalır...

// SERİ İLETİŞİM YAPISI (lib_image.h'den gelmelidir)
IMAGE_HandleTypeDef img; // Tek bir yapıyı hem alma hem gönderme için kullanacağız.
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 128x128 çözünürlük, RGB565 (2 byte) formatı için buffer
volatile uint8_t pImage[128*128*2];
IMAGE_HandleTypeDef img;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
    // Görüntü Yapısını Başlat (Alış/Gönderim Buffer'ı)
    LIB_IMAGE_InitStruct(&img, (uint8_t*)pImage_RGB565, IMG_WIDTH, IMG_HEIGHT, IMAGE_FORMAT_RGB565);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
        // 1. PC'den görüntü gelmesini bekle (Görüntü pImage_RGB565'e yazılır)
    	if (LIB_SERIAL_IMG_Receive(&img) == SERIAL_OK)
    	{
            // 2. GİRİŞ DÖNÜŞÜMÜ: RGB565'ten 8-bit Gri Tonlamaya
            Convert_RGB565_To_Grayscale8bit((uint16_t*)pImage_RGB565,
                                              original_gray_image,
                                              NUM_PIXELS);

            // *** GÖRÜNTÜ İŞLEME BLOKU ***
            // 3. İŞLEME: Median Filtre Uygula (original_gray_image -> processed_image)
            apply_median_filtering(original_gray_image,
                                   processed_image,
                                   IMG_WIDTH,
                                   IMG_HEIGHT);

            // 4. ÇIKIŞ DÖNÜŞÜMÜ: 8-bit Gri Tonlamadan RGB565'e
            // İşlenmiş görüntüyü (processed_image) gönderim buffer'ına (pImage_RGB565) yaz.
            Convert_Grayscale8bit_To_RGB565(processed_image,
                                            (uint16_t*)pImage_RGB565,
                                            NUM_PIXELS);

            // 5. İŞLENMİŞ GÖRÜNTÜYÜ PC'ye Geri Gönder
    	    LIB_SERIAL_IMG_Transmit(&img);
    	}
    }


    /* USER CODE BEGIN WHILE */
    /*while (1)
    {
        if (LIB_SERIAL_IMG_Receive(&img) == SERIAL_OK)
        {
            // 1. GİRİŞ DÖNÜŞÜMÜ: RGB565 -> 8-bit Gri (16-bit'ten 1-byte'a düşüş)
            Convert_RGB565_To_Grayscale8bit((uint16_t*)pImage_RGB565,
                                              original_gray_image,
                                              NUM_PIXELS);

            // *** HİSTOGRAM EŞİTLEME BLOKU ***
            // 2. Histogramı Hesapla
            calculate_histogram(original_gray_image, NUM_PIXELS, histogram_results);

            // 3. Eşitlemeyi Uygula (original_gray_image -> processed_image)
            apply_histogram_equalization(original_gray_image,
                                         processed_image,
                                         histogram_results,
                                         NUM_PIXELS);

            // 4. ÇIKIŞ DÖNÜŞÜMÜ: 8-bit Gri -> RGB565
            Convert_Grayscale8bit_To_RGB565(processed_image,
                                            (uint16_t*)pImage_RGB565,
                                            NUM_PIXELS);

            // 5. İşlenmiş Görüntüyü PC'ye Geri Gönder
            LIB_SERIAL_IMG_Transmit(&img);
        }
    }*/

    /* USER CODE BEGIN WHILE */
    /*while (1)
    {
        if (LIB_SERIAL_IMG_Receive(&img) == SERIAL_OK)
        {
            // 1. GİRİŞ DÖNÜŞÜMÜ: RGB565 -> 8-bit Gri
            Convert_RGB565_To_Grayscale8bit((uint16_t*)pImage_RGB565,
                                              original_gray_image,
                                              NUM_PIXELS);

            // *** HIGH-PASS FİLTRE BLOKU ***
            // 2. 3x3 High-Pass Kernel Uygula (original_gray_image -> processed_image)
            apply_2d_convolution(original_gray_image,
                                 processed_image,
                                 IMG_WIDTH,
                                 IMG_HEIGHT,
                                 kernel_high_pass); // kernel_high_pass dizisi kullanılacak



            // 3. ÇIKIŞ DÖNÜŞÜMÜ: 8-bit Gri -> RGB565
            Convert_Grayscale8bit_To_RGB565(processed_image,
                                            (uint16_t*)pImage_RGB565,
                                            NUM_PIXELS);

            // 4. İşlenmiş Görüntüyü PC'ye Geri Gönder
            LIB_SERIAL_IMG_Transmit(&img);
        }
    }*/

    /* USER CODE BEGIN WHILE */
    /*while (1)
    {
        if (LIB_SERIAL_IMG_Receive(&img) == SERIAL_OK)
        {
            // 1. GİRİŞ DÖNÜŞÜMÜ: RGB565 -> 8-bit Gri
            Convert_RGB565_To_Grayscale8bit((uint16_t*)pImage_RGB565,
                                              original_gray_image,
                                              NUM_PIXELS);

            // *** LOW-PASS FİLTRE BLOKU ***
            // 2. 3x3 Low-Pass Kernel Uygula (original_gray_image -> processed_image)
            apply_2d_convolution(original_gray_image,
                                 processed_image,
                                 IMG_WIDTH,
                                 IMG_HEIGHT,
                                 kernel_low_pass); // kernel_low_pass dizisi kullanılacak

            // 3. ÇIKIŞ DÖNÜŞÜMÜ: 8-bit Gri -> RGB565
            Convert_Grayscale8bit_To_RGB565(processed_image,
                                            (uint16_t*)pImage_RGB565,
                                            NUM_PIXELS);

            // 4. İşlenmiş Görüntüyü PC'ye Geri Gönder
            LIB_SERIAL_IMG_Transmit(&img);
        }
    }*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}
  /* USER CODE END 3 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 2000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  RGB565'ten 8-bit Gri tonlamaya çevirir. (2 Byte -> 1 Byte)
 */
void Convert_RGB565_To_Grayscale8bit(const uint16_t* pIn16bit, uint8_t* pOut8bit, uint32_t num_pixels)
{
    // Ağırlıklı Ortalama (ITU-R BT.601) kullanılır, 8-bit R/G/B varsayılarak.
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint16_t pixel = pIn16bit[i];

        // 16-bit'ten 8-bit R, G, B bileşenlerini çıkar
        uint8_t R8 = ((pixel >> 11) & 0x1F) << 3; // R5 -> R8
        uint8_t G8 = ((pixel >> 5) & 0x3F) << 2;  // G6 -> G8
        uint8_t B8 = (pixel & 0x1F) << 3;         // B5 -> B8

        // Gri Tonlama Hesaplama (77*R + 150*G + 29*B) >> 8
        uint32_t gray8bit = (77 * R8 + 150 * G8 + 29 * B8) >> 8;

        pOut8bit[i] = (uint8_t)gray8bit;
    }
}

/**
 * @brief  8-bit Gri tonlamadan RGB565'e çevirir. (1 Byte -> 2 Byte)
 * (Seri porttan göndermek için gereklidir.)
 */
void Convert_Grayscale8bit_To_RGB565(const uint8_t* pIn8bit, uint16_t* pOut16bit, uint32_t num_pixels)
{
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint8_t gray8bit = pIn8bit[i];

        // 8-bit Gri değeri R, G, B bileşenleri olarak kabul edilir.
        uint16_t R5 = gray8bit >> 3; // 8 bit -> 5 bit R
        uint16_t G6 = gray8bit >> 2; // 8 bit -> 6 bit G
        uint16_t B5 = gray8bit >> 3; // 8 bit -> 5 bit B

        // Yeni RGB565 Gri Pikseli Oluştur
        pOut16bit[i] = (R5 << 11) | (G6 << 5) | B5;
    }
}

// ... (Diğer fonksiyonlar: image_processing.c'den gelenler) ...

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
