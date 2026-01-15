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
#include "network.h"
#include "stai.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYNC_BYTE   0xBB
#define IMG_SIZE_28 28
#define IMG_SIZE_32 32
#define IMG_BYTES   784
#define IN_SIZE     3072
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Network buffers */
STAI_ALIGNED(8) static uint8_t net_ctx[STAI_NETWORK_CONTEXT_SIZE];
STAI_ALIGNED(4) static uint8_t acts[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];
static stai_ptr act_ptrs[] = {acts};

STAI_ALIGNED(4) static float in_buf[IN_SIZE];
STAI_ALIGNED(4) static float out_buf[10];
static stai_ptr in_ptrs[] = {(stai_ptr)in_buf};
static stai_ptr out_ptrs[] = {(stai_ptr)out_buf};

static uint8_t img_buf[IMG_BYTES];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
static void PrepareInput(uint8_t *img28, float *in32);
static int AI_Init(void);
static uint8_t ArgMax(float *d, int n);
static void uart2_printf(const char *fmt, ...);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static void uart2_printf(const char *fmt, ...)
{
  char buf[128];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0) {
    if (n > (int)sizeof(buf)) n = (int)sizeof(buf);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, n, HAL_MAX_DELAY);
  }
}

/* Bilinear interpolation resize 28x28 -> 32x32, grayscale -> RGB, normalize */
static void PrepareInput(uint8_t *img28, float *in32) {
  int x32, y32, c;
  float scale = 28.0f / 32.0f;

  for (y32 = 0; y32 < IMG_SIZE_32; y32++) {
    for (x32 = 0; x32 < IMG_SIZE_32; x32++) {
      float src_x = x32 * scale;
      float src_y = y32 * scale;

      int x0 = (int)src_x;
      int y0 = (int)src_y;
      int x1 = (x0 < IMG_SIZE_28 - 1) ? x0 + 1 : x0;
      int y1 = (y0 < IMG_SIZE_28 - 1) ? y0 + 1 : y0;

      float dx = src_x - x0;
      float dy = src_y - y0;

      float p00 = img28[y0 * IMG_SIZE_28 + x0];
      float p10 = img28[y0 * IMG_SIZE_28 + x1];
      float p01 = img28[y1 * IMG_SIZE_28 + x0];
      float p11 = img28[y1 * IMG_SIZE_28 + x1];

      float val = p00 * (1 - dx) * (1 - dy) +
                  p10 * dx * (1 - dy) +
                  p01 * (1 - dx) * dy +
                  p11 * dx * dy;

      float norm_val = val / 255.0f;

      int idx = (y32 * IMG_SIZE_32 + x32) * 3;
      for (c = 0; c < 3; c++) {
        in32[idx + c] = norm_val;
      }
    }
  }
}

static int AI_Init(void) {
  if (stai_network_init((stai_network *)net_ctx) != STAI_SUCCESS) return -1;
  if (stai_network_set_activations((stai_network *)net_ctx, act_ptrs, 1) != STAI_SUCCESS) return -1;
  if (stai_network_set_inputs((stai_network *)net_ctx, in_ptrs, 1) != STAI_SUCCESS) return -1;
  if (stai_network_set_outputs((stai_network *)net_ctx, out_ptrs, 1) != STAI_SUCCESS) return -1;
  return 0;
}

static uint8_t ArgMax(float *d, int n) {
  uint8_t m = 0;
  for (int i = 1; i < n; i++)
    if (d[i] > d[m]) m = i;
  return m;
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t sync, res = 0xFF;
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();

  uart2_printf("BOOT OK\r\n");

  if (AI_Init() != 0) {
    uart2_printf("AI INIT FAIL\r\n");
    while (1) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      HAL_Delay(100);
    }
  }
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  uart2_printf("AI READY\r\n");

  while (1)
  {
    /* SYNC + 784 byte bekle */
    if (HAL_UART_Receive(&huart2, &sync, 1, HAL_MAX_DELAY) == HAL_OK &&
        sync == SYNC_BYTE)
    {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

      if (HAL_UART_Receive(&huart2, img_buf, IMG_BYTES, 5000) == HAL_OK)
      {
        PrepareInput(img_buf, in_buf);

        uint32_t t0 = HAL_GetTick();
        if (stai_network_run((stai_network *)net_ctx, STAI_MODE_SYNC) == STAI_SUCCESS)
        {
          uint32_t dt = HAL_GetTick() - t0;
          res = ArgMax(out_buf, 10);

          /* TEK SATIR: Python sadece bunu okuyacak */
          uart2_printf("PRED=%d DT=%lu\r\n", (int)res, (unsigned long)dt);
        }
        else
        {
          uart2_printf("PRED=255 DT=0\r\n");
        }
      }
      else
      {
        /* img alamazsa da tek satır bas */
        uart2_printf("PRED=255 DT=0\r\n");
      }

      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */
