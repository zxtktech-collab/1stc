#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float resistance_ohm;
  float temperature_c;
  uint16_t adc_raw;
  bool fault;       // open / short / out of range
  bool open_wire;
  bool shorted;
} Pt100Sample;

float pt100_resistance_from_adc(uint16_t adc);
float pt100_temperature_c(float resistance_ohm);
Pt100Sample pt100_from_adc(uint16_t adc);

#ifdef __cplusplus
}
#endif
