#pragma once

#include <stdint.h>

// Modbus RTU slave defaults
#define MB_DEFAULT_SLAVE_ID   1
#define MB_DEFAULT_BAUD       9600
#define MB_RESPONSE_DELAY_US  500

// ---------------------------------------------------------------------------
// Input registers (FC 0x04) — read by master
// ---------------------------------------------------------------------------
// Addr  Name                 Scale / meaning
// 0     TEMP_CH0             int16, 0.1 °C  (250 = 25.0 °C). 0x7FFF = fault
// 1     TEMP_CH1
// 2     TEMP_CH2
// 3     TEMP_CH3
// 4     ADC_CH0              uint16 raw ADC average
// 5     ADC_CH1
// 6     ADC_CH2
// 7     ADC_CH3
// 8     R_CH0                uint16, 0.1 Ω  (1000 = 100.0 Ω)
// 9     R_CH1
// 10    R_CH2
// 11    R_CH3
// 12    STATUS               bit0..3 channel fault; bit8 open; bit9 short
// 13    FW_VERSION           e.g. 0x0100 = v1.0
// 14    BOARD_ID_HI          'PT'
// 15    BOARD_ID_LO          '100'

#define MB_IR_TEMP0       0
#define MB_IR_TEMP1       1
#define MB_IR_TEMP2       2
#define MB_IR_TEMP3       3
#define MB_IR_ADC0        4
#define MB_IR_ADC1        5
#define MB_IR_ADC2        6
#define MB_IR_ADC3        7
#define MB_IR_R0          8
#define MB_IR_R1          9
#define MB_IR_R2          10
#define MB_IR_R3          11
#define MB_IR_STATUS      12
#define MB_IR_FW_VERSION  13
#define MB_IR_BOARD_HI    14
#define MB_IR_BOARD_LO    15
#define MB_IR_COUNT       16

#define MB_TEMP_FAULT     0x7FFF
#define MB_FW_VERSION     0x0100

// ---------------------------------------------------------------------------
// Holding registers (FC 0x03 / 0x06 / 0x10)
// ---------------------------------------------------------------------------
// 0  SLAVE_ID          1..247 (applied after save / reset)
// 1  BAUD_CODE         0=9600, 1=19200, 2=38400, 3=57600, 4=115200
// 2  SAMPLE_MS         acquisition period ms (50..2000)
// 3  TEMP_OFFSET0      int16, 0.1 °C added to CH0
// 4  TEMP_OFFSET1
// 5  TEMP_OFFSET2
// 6  TEMP_OFFSET3
// 7  SAVE_CMD          write 1 to store config to EEPROM
// 8  APPLY_CMD         write 1 to apply slave id / baud now

#define MB_HR_SLAVE_ID    0
#define MB_HR_BAUD_CODE   1
#define MB_HR_SAMPLE_MS   2
#define MB_HR_OFFSET0     3
#define MB_HR_OFFSET1     4
#define MB_HR_OFFSET2     5
#define MB_HR_OFFSET3     6
#define MB_HR_SAVE_CMD    7
#define MB_HR_APPLY_CMD   8
#define MB_HR_COUNT       9

uint32_t baud_from_code(uint16_t code);
