#ifndef PT100_H
#define PT100_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  float resistance_ohm;
  float temperature_c;
  uint16_t adc_raw;
  bool fault;
  bool open_wire;
  bool shorted;
} Pt100Sample;

float pt100_resistance_from_adc(uint16_t adc);
float pt100_temperature_c(float resistance_ohm);
Pt100Sample pt100_from_adc(uint16_t adc);

#endif /* PT100_H */
