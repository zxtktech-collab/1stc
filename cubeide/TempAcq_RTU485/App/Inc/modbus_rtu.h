#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint16_t (*mb_read_input_fn)(uint16_t address);
typedef uint16_t (*mb_read_holding_fn)(uint16_t address);
typedef bool (*mb_write_holding_fn)(uint16_t address, uint16_t value);
typedef void (*mb_tx_fn)(const uint8_t *data, size_t len);
typedef void (*mb_set_de_fn)(bool drive_enable);

typedef struct {
  uint8_t slave_id;
  mb_read_input_fn read_input;
  mb_read_holding_fn read_holding;
  mb_write_holding_fn write_holding;
  uint16_t input_count;
  uint16_t holding_count;
} ModbusSlave;

void modbus_init(ModbusSlave *slave);
void modbus_set_slave_id(ModbusSlave *slave, uint8_t id);
void modbus_set_frame_gap_ms(uint32_t gap_ms);
void modbus_rx_byte(uint8_t b, uint32_t now_ms);
void modbus_poll(uint32_t now_ms, mb_tx_fn tx, mb_set_de_fn set_de);

#endif /* MODBUS_RTU_H */
