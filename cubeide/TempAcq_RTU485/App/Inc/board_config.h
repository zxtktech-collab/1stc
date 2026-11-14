#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define NUM_CHANNELS     4
/* Iexc = (VCC/2) / R1 = 2.5V / 2.5k = 1.0 mA when VCC = 5 V */
#define PT100_IEXC_A     0.001f
/* Gain = 1 + R6/R7 = 1 + 20k/1k = 21 */
#define PT100_GAIN       21.0f
#define ADC_VREF_V       3.3f
#define ADC_MAX_COUNTS   4095.0f
#define PT100_R0_OHM     100.0f

#endif /* BOARD_CONFIG_H */
