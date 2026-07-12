/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c (USER CODE reference for CubeIDE)
  * @brief          : After generating from TempAcq_RTU485.ioc, ensure these
  *                   USER CODE sections exist in your Core/Src/main.c
  ******************************************************************************
  */
/* USER CODE END Header */

/* This file is a REFERENCE only. Do not replace the entire Cube-generated
 * main.c with this file. Copy the USER CODE blocks into your generated main.c.
 */

#if 0 /* ---------- BEGIN reference excerpt ---------- */

#include "main.h"
#include "app_temp.h"

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c2;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  App_Init(&hadc1, &huart2, &hi2c2);
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    App_Loop();
    /* USER CODE END 3 */
  }
}

#if 0
#endif
#endif /* ---------- END reference excerpt ---------- */
