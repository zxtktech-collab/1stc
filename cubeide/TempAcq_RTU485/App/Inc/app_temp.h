#ifndef APP_TEMP_H
#define APP_TEMP_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void App_Init(ADC_HandleTypeDef *hadc,
              UART_HandleTypeDef *huart,
              I2C_HandleTypeDef *hi2c);
void App_Loop(void);

/* Call from USART2 IRQ when RXNE (byte received) */
void App_OnUartRxByte(uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* APP_TEMP_H */
