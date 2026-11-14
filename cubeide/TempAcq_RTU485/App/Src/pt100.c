#include "pt100.h"
#include "board_config.h"
#include <math.h>

static const float CVD_A = 3.9083e-3f;
static const float CVD_B = -5.775e-7f;

float pt100_resistance_from_adc(uint16_t adc)
{
  const float vadc = ((float)adc * ADC_VREF_V) / ADC_MAX_COUNTS;
  const float v_rt = vadc / PT100_GAIN;
  float r = v_rt / PT100_IEXC_A;
  if (r < 0.0f) {
    r = 0.0f;
  }
  return r;
}

float pt100_temperature_c(float resistance_ohm)
{
  const float r_norm = resistance_ohm / PT100_R0_OHM;
  const float disc = (CVD_A * CVD_A) - (4.0f * CVD_B * (1.0f - r_norm));
  if (disc < 0.0f) {
    return NAN;
  }
  return (-CVD_A + sqrtf(disc)) / (2.0f * CVD_B);
}

Pt100Sample pt100_from_adc(uint16_t adc)
{
  Pt100Sample s;
  s.adc_raw = adc;
  s.resistance_ohm = pt100_resistance_from_adc(adc);
  s.fault = false;
  s.open_wire = false;
  s.shorted = false;

  if (adc >= 4000U || s.resistance_ohm > 320.0f) {
    s.open_wire = true;
    s.fault = true;
    s.temperature_c = NAN;
    return s;
  }
  if (adc < 80U || s.resistance_ohm < 40.0f) {
    s.shorted = true;
    s.fault = true;
    s.temperature_c = NAN;
    return s;
  }

  s.temperature_c = pt100_temperature_c(s.resistance_ohm);
  if (isnan(s.temperature_c) || s.temperature_c < -50.0f || s.temperature_c > 250.0f) {
    s.fault = true;
  }
  return s;
}
