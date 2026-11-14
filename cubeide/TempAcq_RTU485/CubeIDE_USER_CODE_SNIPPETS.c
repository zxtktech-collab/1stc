/**
 * Paste these fragments into the CubeMX-generated files AFTER you
 * open TempAcq_RTU485.ioc and click Generate Code.
 *
 * Keep everything inside USER CODE BEGIN / END blocks so regeneration
 * does not wipe your edits.
 */

/* =====================================================================
 * File: Core/Inc/main.h
 * Add inside USER CODE BEGIN Includes
 * ===================================================================== */
#if 0
/* USER CODE BEGIN Includes */
#include "app_temp.h"
/* USER CODE END Includes */
#endif

/* =====================================================================
 * File: Core/Src/main.c
 * 1) Includes
 * 2) After all MX_*_Init() calls, before while(1)
 * 3) Inside while(1)
 * ===================================================================== */
#if 0
/* USER CODE BEGIN Includes */
#include "app_temp.h"
/* USER CODE END Includes */

/* --- after MX_GPIO_Init(); MX_ADC1_Init(); MX_I2C2_Init(); MX_USART2_UART_Init(); --- */
/* USER CODE BEGIN 2 */
  App_Init(&hadc1, &huart2, &hi2c2);
/* USER CODE END 2 */

/* --- inside while (1) --- */
/* USER CODE BEGIN 3 */
    App_Loop();
/* USER CODE END 3 */
#endif

/* =====================================================================
 * File: Core/Src/stm32f1xx_it.c
 * Inside USART2_IRQHandler, USER CODE BEGIN USART2_IRQn 0
 * (or after HAL_UART_IRQHandler if you prefer RXNE check before HAL)
 * ===================================================================== */
#if 0
/* USER CODE BEGIN USART2_IRQn 0 */
  if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET) {
    uint8_t b = (uint8_t)(huart2.Instance->DR & 0xFFU);
    App_OnUartRxByte(b);
  }
/* USER CODE END USART2_IRQn 0 */
#endif

/* Also add near top of stm32f1xx_it.c USER CODE BEGIN Includes: */
#if 0
/* USER CODE BEGIN Includes */
#include "main.h"
#include "app_temp.h"
extern UART_HandleTypeDef huart2;
/* USER CODE END Includes */
#endif
