#pragma once

// Analog front-end constants (schematic)
// Iexc = (VCC/2) / R1 = 2.5V / 2.5kΩ = 1.0 mA  (VCC=5 V)
// Gain = 1 + R6/R7 = 1 + 20k/1k = 21
#define NUM_CHANNELS     4
#define PT100_IEXC_A     0.001f
#define PT100_GAIN       21.0f
#define ADC_VREF_V       3.3f
#define ADC_MAX_COUNTS   4095.0f
#define PT100_R0_OHM     100.0f
