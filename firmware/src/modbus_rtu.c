#include "modbus_rtu.h"
#include <string.h>

#define MB_RX_BUF_SIZE  256
#define MB_TX_BUF_SIZE  256

static uint32_t g_frame_gap_ms = 5;
static ModbusSlave *g_slave = NULL;
static uint8_t g_rx[MB_RX_BUF_SIZE];
static uint16_t g_rx_len = 0;
static uint32_t g_last_rx_ms = 0;
static bool g_frame_ready = false;
static uint8_t g_tx[MB_TX_BUF_SIZE];

static uint16_t crc16_modbus(const uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void modbus_init(ModbusSlave *slave) {
  g_slave = slave;
  g_rx_len = 0;
  g_frame_ready = false;
}

void modbus_set_slave_id(ModbusSlave *slave, uint8_t id) {
  if (slave) {
    slave->slave_id = id;
  }
}

void modbus_set_frame_gap_ms(uint32_t gap_ms) {
  g_frame_gap_ms = (gap_ms < 2) ? 2 : gap_ms;
}

void modbus_rx_byte(uint8_t b, uint32_t now_ms) {
  if (g_frame_ready) {
    return;
  }
  if (g_rx_len < MB_RX_BUF_SIZE) {
    g_rx[g_rx_len++] = b;
  } else {
    g_rx_len = 0;
  }
  g_last_rx_ms = now_ms;
}

static void send_exception(uint8_t fn, uint8_t code, mb_tx_fn tx, mb_set_de_fn set_de) {
  g_tx[0] = g_slave->slave_id;
  g_tx[1] = (uint8_t)(fn | 0x80);
  g_tx[2] = code;
  const uint16_t crc = crc16_modbus(g_tx, 3);
  g_tx[3] = (uint8_t)(crc & 0xFF);
  g_tx[4] = (uint8_t)(crc >> 8);
  set_de(true);
  tx(g_tx, 5);
  set_de(false);
}

static void handle_frame(mb_tx_fn tx, mb_set_de_fn set_de) {
  if (!g_slave || g_rx_len < 4) {
    g_rx_len = 0;
    g_frame_ready = false;
    return;
  }

  const uint16_t crc_rx =
      (uint16_t)g_rx[g_rx_len - 2] | ((uint16_t)g_rx[g_rx_len - 1] << 8);
  if (crc_rx != crc16_modbus(g_rx, (uint16_t)(g_rx_len - 2))) {
    g_rx_len = 0;
    g_frame_ready = false;
    return;
  }

  const uint8_t addr = g_rx[0];
  if (addr != g_slave->slave_id && addr != 0) {
    g_rx_len = 0;
    g_frame_ready = false;
    return;
  }

  const bool broadcast = (addr == 0);
  const uint8_t fn = g_rx[1];

  if (fn == 0x03 || fn == 0x04) {
    if (broadcast || g_rx_len != 8) {
      g_rx_len = 0;
      g_frame_ready = false;
      return;
    }
    const uint16_t start = ((uint16_t)g_rx[2] << 8) | g_rx[3];
    const uint16_t qty = ((uint16_t)g_rx[4] << 8) | g_rx[5];
    const uint16_t max_count =
        (fn == 0x04) ? g_slave->input_count : g_slave->holding_count;

    if (qty == 0 || qty > 125 || ((uint32_t)start + qty) > max_count) {
      send_exception(fn, 0x02, tx, set_de);
      g_rx_len = 0;
      g_frame_ready = false;
      return;
    }

    g_tx[0] = g_slave->slave_id;
    g_tx[1] = fn;
    g_tx[2] = (uint8_t)(qty * 2);
    for (uint16_t i = 0; i < qty; i++) {
      const uint16_t val = (fn == 0x04) ? g_slave->read_input((uint16_t)(start + i))
                                        : g_slave->read_holding((uint16_t)(start + i));
      g_tx[3 + i * 2] = (uint8_t)(val >> 8);
      g_tx[4 + i * 2] = (uint8_t)(val & 0xFF);
    }
    const uint16_t pdu_len = (uint16_t)(3 + qty * 2);
    const uint16_t crc = crc16_modbus(g_tx, pdu_len);
    g_tx[pdu_len] = (uint8_t)(crc & 0xFF);
    g_tx[pdu_len + 1] = (uint8_t)(crc >> 8);
    set_de(true);
    tx(g_tx, pdu_len + 2);
    set_de(false);
  } else if (fn == 0x06) {
    if (g_rx_len != 8) {
      g_rx_len = 0;
      g_frame_ready = false;
      return;
    }
    const uint16_t reg = ((uint16_t)g_rx[2] << 8) | g_rx[3];
    const uint16_t val = ((uint16_t)g_rx[4] << 8) | g_rx[5];
    if (reg >= g_slave->holding_count) {
      if (!broadcast) {
        send_exception(fn, 0x02, tx, set_de);
      }
    } else if (!g_slave->write_holding(reg, val)) {
      if (!broadcast) {
        send_exception(fn, 0x03, tx, set_de);
      }
    } else if (!broadcast) {
      memcpy(g_tx, g_rx, 8);
      set_de(true);
      tx(g_tx, 8);
      set_de(false);
    }
  } else if (fn == 0x10) {
    if (g_rx_len < 9) {
      g_rx_len = 0;
      g_frame_ready = false;
      return;
    }
    const uint16_t start = ((uint16_t)g_rx[2] << 8) | g_rx[3];
    const uint16_t qty = ((uint16_t)g_rx[4] << 8) | g_rx[5];
    const uint8_t byte_count = g_rx[6];
    if (qty == 0 || qty > 123 || byte_count != qty * 2 ||
        ((uint32_t)start + qty) > g_slave->holding_count ||
        g_rx_len != (uint16_t)(9 + byte_count)) {
      if (!broadcast) {
        send_exception(fn, 0x02, tx, set_de);
      }
    } else {
      bool ok = true;
      for (uint16_t i = 0; i < qty; i++) {
        const uint16_t val =
            ((uint16_t)g_rx[7 + i * 2] << 8) | g_rx[8 + i * 2];
        if (!g_slave->write_holding((uint16_t)(start + i), val)) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        if (!broadcast) {
          send_exception(fn, 0x03, tx, set_de);
        }
      } else if (!broadcast) {
        g_tx[0] = g_slave->slave_id;
        g_tx[1] = fn;
        g_tx[2] = g_rx[2];
        g_tx[3] = g_rx[3];
        g_tx[4] = g_rx[4];
        g_tx[5] = g_rx[5];
        const uint16_t crc = crc16_modbus(g_tx, 6);
        g_tx[6] = (uint8_t)(crc & 0xFF);
        g_tx[7] = (uint8_t)(crc >> 8);
        set_de(true);
        tx(g_tx, 8);
        set_de(false);
      }
    }
  } else if (!broadcast) {
    send_exception(fn, 0x01, tx, set_de);
  }

  g_rx_len = 0;
  g_frame_ready = false;
}

void modbus_poll(uint32_t now_ms, mb_tx_fn tx, mb_set_de_fn set_de) {
  if (g_rx_len > 0 && !g_frame_ready) {
    if ((now_ms - g_last_rx_ms) >= g_frame_gap_ms) {
      g_frame_ready = true;
    }
  }
  if (g_frame_ready) {
    handle_frame(tx, set_de);
  }
}
