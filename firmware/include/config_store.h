#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t magic;       // 0xA55A
  uint16_t slave_id;
  uint16_t baud_code;
  uint16_t sample_ms;
  int16_t offset_x10[4];
  uint16_t crc;
} BoardConfig;

void config_set_defaults(BoardConfig *cfg);
bool config_load(BoardConfig *cfg);
bool config_save(const BoardConfig *cfg);

#ifdef __cplusplus
}
#endif
