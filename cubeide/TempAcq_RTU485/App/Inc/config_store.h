#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

typedef struct {
  uint16_t magic;
  uint16_t slave_id;
  uint16_t baud_code;
  uint16_t sample_ms;
  int16_t offset_x10[4];
  uint16_t crc;
} BoardConfig;

void config_set_defaults(BoardConfig *cfg);
bool config_load(I2C_HandleTypeDef *hi2c, BoardConfig *cfg);
bool config_save(I2C_HandleTypeDef *hi2c, const BoardConfig *cfg);

#endif /* CONFIG_STORE_H */
