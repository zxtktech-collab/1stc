#include "config_store.h"
#include "modbus_map.h"
#include <string.h>

#define CFG_MAGIC     0xA55AU
/* AT24C02 7-bit address 0x50 → HAL 8-bit write address 0xA0 */
#define EEPROM_ADDR   0xA0U
#define EEPROM_OFFSET 0U

static uint16_t cfg_crc(const BoardConfig *cfg)
{
  const uint8_t *p = (const uint8_t *)cfg;
  const size_t n = sizeof(BoardConfig) - sizeof(uint16_t);
  uint16_t c = 0xFFFFU;
  for (size_t i = 0U; i < n; i++) {
    c ^= p[i];
    for (uint8_t b = 0U; b < 8U; b++) {
      if ((c & 1U) != 0U) {
        c = (uint16_t)((c >> 1) ^ 0xA001U);
      } else {
        c >>= 1;
      }
    }
  }
  return c;
}

void config_set_defaults(BoardConfig *cfg)
{
  (void)memset(cfg, 0, sizeof(*cfg));
  cfg->magic = CFG_MAGIC;
  cfg->slave_id = MB_DEFAULT_SLAVE_ID;
  cfg->baud_code = 0U;
  cfg->sample_ms = 200U;
  for (int i = 0; i < 4; i++) {
    cfg->offset_x10[i] = 0;
  }
  cfg->crc = cfg_crc(cfg);
}

static bool eeprom_write_bytes(I2C_HandleTypeDef *hi2c, uint8_t mem_addr,
                               const uint8_t *data, uint8_t len)
{
  uint8_t offset = 0U;
  while (offset < len) {
    uint8_t chunk = (uint8_t)(len - offset);
    uint8_t page_left = (uint8_t)(8U - ((mem_addr + offset) & 0x07U));
    if (chunk > page_left) {
      chunk = page_left;
    }
    if (HAL_I2C_Mem_Write(hi2c, EEPROM_ADDR, (uint16_t)(mem_addr + offset),
                          I2C_MEMADD_SIZE_8BIT, (uint8_t *)&data[offset], chunk,
                          100U) != HAL_OK) {
      return false;
    }
    HAL_Delay(6U);
    offset = (uint8_t)(offset + chunk);
  }
  return true;
}

static bool eeprom_read_bytes(I2C_HandleTypeDef *hi2c, uint8_t mem_addr,
                              uint8_t *data, uint8_t len)
{
  return HAL_I2C_Mem_Read(hi2c, EEPROM_ADDR, mem_addr, I2C_MEMADD_SIZE_8BIT,
                          data, len, 100U) == HAL_OK;
}

bool config_load(I2C_HandleTypeDef *hi2c, BoardConfig *cfg)
{
  BoardConfig tmp;
  if (!eeprom_read_bytes(hi2c, EEPROM_OFFSET, (uint8_t *)&tmp, sizeof(tmp))) {
    return false;
  }
  if (tmp.magic != CFG_MAGIC) {
    return false;
  }
  if (cfg_crc(&tmp) != tmp.crc) {
    return false;
  }
  if ((tmp.slave_id < 1U) || (tmp.slave_id > 247U) || (tmp.baud_code > 4U) ||
      (tmp.sample_ms < 50U) || (tmp.sample_ms > 2000U)) {
    return false;
  }
  *cfg = tmp;
  return true;
}

bool config_save(I2C_HandleTypeDef *hi2c, const BoardConfig *cfg)
{
  BoardConfig tmp = *cfg;
  tmp.magic = CFG_MAGIC;
  tmp.crc = cfg_crc(&tmp);
  return eeprom_write_bytes(hi2c, EEPROM_OFFSET, (const uint8_t *)&tmp, sizeof(tmp));
}

uint32_t baud_from_code(uint16_t code)
{
  switch (code) {
    case 1U:
      return 19200U;
    case 2U:
      return 38400U;
    case 3U:
      return 57600U;
    case 4U:
      return 115200U;
    default:
      return 9600U;
  }
}
