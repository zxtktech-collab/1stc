#pragma once

#include <Arduino.h>

// Pin map from EasyEDA netlist (STM32F103RCT6)

// Analog front-end
static const uint32_t PIN_ADC_IN = PA4;   // amplified PT100 voltage
static const uint32_t PIN_MUX_S0 = PB0;   // 74HCT4052 S0
static const uint32_t PIN_MUX_S1 = PB1;   // 74HCT4052 S1

// RS485 (USART2)
static const uint32_t PIN_RS485_TX = PA2;
static const uint32_t PIN_RS485_RX = PA3;
static const uint32_t PIN_RS485_DE = PA1;  // DE and /RE tied together

// Status LEDs (active LOW)
static const uint32_t PIN_LED_RUN  = PA0;
static const uint32_t PIN_LED_EEPR = PA6;
static const uint32_t PIN_LED_ERR  = PA7;

// I2C2 EEPROM AT24C02
static const uint32_t PIN_EEPROM_SCL = PB10;
static const uint32_t PIN_EEPROM_SDA = PB11;
