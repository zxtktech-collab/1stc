/**
 * After CubeMX Generate Code, open Core/Src/stm32f1xx_it.c and apply:
 *
 * 1) USER CODE BEGIN Includes
 * 2) Inside USART2_IRQHandler → USER CODE BEGIN USART2_IRQn 0
 */

#if 0 /* reference only */

/* USER CODE BEGIN Includes */
#include "main.h"
#include "app_temp.h"
extern UART_HandleTypeDef huart2;
/* USER CODE END Includes */

void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
  if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET) {
    uint8_t b = (uint8_t)(huart2.Instance->DR & 0xFFU);
    App_OnUartRxByte(b);
  }
  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */
  /* USER CODE END USART2_IRQn 1 */
}

#endif
