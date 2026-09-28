#ifndef MODBUS_MAP_H
#define MODBUS_MAP_H

#include <stdint.h>

#define MB_DEFAULT_SLAVE_ID   1U
#define MB_DEFAULT_BAUD       9600U

/* Input registers (FC 0x04) */
#define MB_IR_TEMP0       0U
#define MB_IR_TEMP1       1U
#define MB_IR_TEMP2       2U
#define MB_IR_TEMP3       3U
#define MB_IR_ADC0        4U
#define MB_IR_ADC1        5U
#define MB_IR_ADC2        6U
#define MB_IR_ADC3        7U
#define MB_IR_R0          8U
#define MB_IR_R1          9U
#define MB_IR_R2          10U
#define MB_IR_R3          11U
#define MB_IR_STATUS      12U
#define MB_IR_FW_VERSION  13U
#define MB_IR_BOARD_HI    14U
#define MB_IR_BOARD_LO    15U
#define MB_IR_COUNT       16U

#define MB_TEMP_FAULT     0x7FFF
#define MB_FW_VERSION     0x0100

/* Holding registers (FC 0x03 / 0x06 / 0x10) */
#define MB_HR_SLAVE_ID    0U
#define MB_HR_BAUD_CODE   1U
#define MB_HR_SAMPLE_MS   2U
#define MB_HR_OFFSET0     3U
#define MB_HR_OFFSET1     4U
#define MB_HR_OFFSET2     5U
#define MB_HR_OFFSET3     6U
#define MB_HR_SAVE_CMD    7U
#define MB_HR_APPLY_CMD   8U
#define MB_HR_COUNT       9U

uint32_t baud_from_code(uint16_t code);

#endif /* MODBUS_MAP_H */
