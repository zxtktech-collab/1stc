#include "config_store.h"
#include "modbus_map.h"
#include <Wire.h>
#include <string.h>

extern TwoWire EepromWire;

#define CFG_MAGIC 0xA55A
#define EEPROM_ADDR 0x50
#define EEPROM_OFFSET 0

static uint16_t cfg_crc(const BoardConfig *cfg) {
  const uint8_t *p = (const uint8_t *)cfg;
  const size_t n = sizeof(BoardConfig) - sizeof(uint16_t);
  uint16_t c = 0xFFFF;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (c & 1) {
        c = (c >> 1) ^ 0xA001;
      } else {
        c >>= 1;
      }
    }
  }
  return c;
}

void config_set_defaults(BoardConfig *cfg) {
  memset(cfg, 0, sizeof(*cfg));
  cfg->magic = CFG_MAGIC;
  cfg->slave_id = MB_DEFAULT_SLAVE_ID;
  cfg->baud_code = 0;  // 9600
  cfg->sample_ms = 200;
  for (int i = 0; i < 4; i++) {
    cfg->offset_x10[i] = 0;
  }
  cfg->crc = cfg_crc(cfg);
}

static bool eeprom_write_bytes(uint8_t mem_addr, const uint8_t *data, uint8_t len) {
  uint8_t offset = 0;
  while (offset < len) {
    uint8_t chunk = (uint8_t)(len - offset);
    uint8_t page_left = (uint8_t)(8 - ((mem_addr + offset) & 0x07));
    if (chunk > page_left) {
      chunk = page_left;
    }
    EepromWire.beginTransmission(EEPROM_ADDR);
    EepromWire.write((uint8_t)(mem_addr + offset));
    for (uint8_t i = 0; i < chunk; i++) {
      EepromWire.write(data[offset + i]);
    }
    if (EepromWire.endTransmission() != 0) {
      return false;
    }
    delay(6);
    offset = (uint8_t)(offset + chunk);
  }
  return true;
}

static bool eeprom_read_bytes(uint8_t mem_addr, uint8_t *data, uint8_t len) {
  EepromWire.beginTransmission(EEPROM_ADDR);
  EepromWire.write(mem_addr);
  if (EepromWire.endTransmission(false) != 0) {
    return false;
  }
  const uint8_t got = EepromWire.requestFrom((int)EEPROM_ADDR, (int)len);
  if (got != len) {
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    data[i] = EepromWire.read();
  }
  return true;
}

bool config_load(BoardConfig *cfg) {
  BoardConfig tmp;
  if (!eeprom_read_bytes(EEPROM_OFFSET, (uint8_t *)&tmp, sizeof(tmp))) {
    return false;
  }
  if (tmp.magic != CFG_MAGIC) {
    return false;
  }
  if (cfg_crc(&tmp) != tmp.crc) {
    return false;
  }
  if (tmp.slave_id < 1 || tmp.slave_id > 247 || tmp.baud_code > 4 ||
      tmp.sample_ms < 50 || tmp.sample_ms > 2000) {
    return false;
  }
  *cfg = tmp;
  return true;
}

bool config_save(const BoardConfig *cfg) {
  BoardConfig tmp = *cfg;
  tmp.magic = CFG_MAGIC;
  tmp.crc = cfg_crc(&tmp);
  return eeprom_write_bytes(EEPROM_OFFSET, (const uint8_t *)&tmp, sizeof(tmp));
}

uint32_t baud_from_code(uint16_t code) {
  switch (code) {
    case 1:
      return 19200;
    case 2:
      return 38400;
    case 3:
      return 57600;
    case 4:
      return 115200;
    default:
      return 9600;
  }
}
