#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "board_config.h"
#include "board_pins.h"
#include "modbus_map.h"
#include "modbus_rtu.h"
#include "pt100.h"
#include "config_store.h"

// USART2 on PA2/PA3 — HardwareSerial for RS485 (RX, TX)
HardwareSerial RS485(PIN_RS485_RX, PIN_RS485_TX);
// AT24C02 on I2C2 (PB11 SDA, PB10 SCL)
TwoWire EepromWire(PIN_EEPROM_SDA, PIN_EEPROM_SCL);

static BoardConfig g_cfg;
static ModbusSlave g_mb;
static Pt100Sample g_ch[NUM_CHANNELS];
static uint16_t g_status = 0;
static bool g_apply_pending = false;
static bool g_save_pending = false;

static void led_write(uint8_t pin, bool on) {
  digitalWrite(pin, on ? LOW : HIGH);  // active low
}

static void select_channel(uint8_t ch) {
  digitalWrite(PIN_MUX_S0, (ch & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_MUX_S1, (ch & 0x02) ? HIGH : LOW);
}

static uint16_t read_adc_averaged(uint8_t samples) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += analogRead(PIN_ADC_IN);
    delayMicroseconds(200);
  }
  return (uint16_t)(sum / samples);
}

static void acquire_all_channels() {
  g_status = 0;
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    select_channel(ch);
    delayMicroseconds(800);  // mux + settle (R8/C2 ~ 2 us, give margin)
    const uint16_t adc = read_adc_averaged(16);
    g_ch[ch] = pt100_from_adc(adc);

    // apply offset (0.1 °C units)
    if (!g_ch[ch].fault && !isnan(g_ch[ch].temperature_c)) {
      g_ch[ch].temperature_c += ((float)g_cfg.offset_x10[ch]) / 10.0f;
    } else {
      g_status |= (1u << ch);
      if (g_ch[ch].open_wire) {
        g_status |= (1u << 8);
      }
      if (g_ch[ch].shorted) {
        g_status |= (1u << 9);
      }
    }
  }
  led_write(PIN_LED_ERR, g_status != 0);
}

static int16_t temp_to_reg(const Pt100Sample &s) {
  if (s.fault || isnan(s.temperature_c)) {
    return (int16_t)MB_TEMP_FAULT;
  }
  float x = s.temperature_c * 10.0f;
  if (x > 32766.0f) {
    x = 32766.0f;
  }
  if (x < -32767.0f) {
    x = -32767.0f;
  }
  return (int16_t)lroundf(x);
}

static uint16_t r_to_reg(const Pt100Sample &s) {
  if (s.fault) {
    return 0xFFFF;
  }
  float x = s.resistance_ohm * 10.0f;
  if (x > 65534.0f) {
    x = 65534.0f;
  }
  if (x < 0.0f) {
    x = 0.0f;
  }
  return (uint16_t)lroundf(x);
}

static uint16_t mb_read_input(uint16_t addr) {
  switch (addr) {
    case MB_IR_TEMP0: return (uint16_t)temp_to_reg(g_ch[0]);
    case MB_IR_TEMP1: return (uint16_t)temp_to_reg(g_ch[1]);
    case MB_IR_TEMP2: return (uint16_t)temp_to_reg(g_ch[2]);
    case MB_IR_TEMP3: return (uint16_t)temp_to_reg(g_ch[3]);
    case MB_IR_ADC0: return g_ch[0].adc_raw;
    case MB_IR_ADC1: return g_ch[1].adc_raw;
    case MB_IR_ADC2: return g_ch[2].adc_raw;
    case MB_IR_ADC3: return g_ch[3].adc_raw;
    case MB_IR_R0: return r_to_reg(g_ch[0]);
    case MB_IR_R1: return r_to_reg(g_ch[1]);
    case MB_IR_R2: return r_to_reg(g_ch[2]);
    case MB_IR_R3: return r_to_reg(g_ch[3]);
    case MB_IR_STATUS: return g_status;
    case MB_IR_FW_VERSION: return MB_FW_VERSION;
    case MB_IR_BOARD_HI: return 0x5054;  // 'PT'
    case MB_IR_BOARD_LO: return 0x3130;  // '10'
    default: return 0;
  }
}

static uint16_t mb_read_holding(uint16_t addr) {
  switch (addr) {
    case MB_HR_SLAVE_ID: return g_cfg.slave_id;
    case MB_HR_BAUD_CODE: return g_cfg.baud_code;
    case MB_HR_SAMPLE_MS: return g_cfg.sample_ms;
    case MB_HR_OFFSET0: return (uint16_t)g_cfg.offset_x10[0];
    case MB_HR_OFFSET1: return (uint16_t)g_cfg.offset_x10[1];
    case MB_HR_OFFSET2: return (uint16_t)g_cfg.offset_x10[2];
    case MB_HR_OFFSET3: return (uint16_t)g_cfg.offset_x10[3];
    case MB_HR_SAVE_CMD: return 0;
    case MB_HR_APPLY_CMD: return 0;
    default: return 0;
  }
}

static bool mb_write_holding(uint16_t addr, uint16_t value) {
  switch (addr) {
    case MB_HR_SLAVE_ID:
      if (value < 1 || value > 247) {
        return false;
      }
      g_cfg.slave_id = value;
      return true;
    case MB_HR_BAUD_CODE:
      if (value > 4) {
        return false;
      }
      g_cfg.baud_code = value;
      return true;
    case MB_HR_SAMPLE_MS:
      if (value < 50 || value > 2000) {
        return false;
      }
      g_cfg.sample_ms = value;
      return true;
    case MB_HR_OFFSET0:
      g_cfg.offset_x10[0] = (int16_t)value;
      return true;
    case MB_HR_OFFSET1:
      g_cfg.offset_x10[1] = (int16_t)value;
      return true;
    case MB_HR_OFFSET2:
      g_cfg.offset_x10[2] = (int16_t)value;
      return true;
    case MB_HR_OFFSET3:
      g_cfg.offset_x10[3] = (int16_t)value;
      return true;
    case MB_HR_SAVE_CMD:
      if (value == 1) {
        g_save_pending = true;
      }
      return true;
    case MB_HR_APPLY_CMD:
      if (value == 1) {
        g_apply_pending = true;
      }
      return true;
    default:
      return false;
  }
}

static void rs485_set_de(bool enable) {
  digitalWrite(PIN_RS485_DE, enable ? HIGH : LOW);
  if (enable) {
    delayMicroseconds(50);
  } else {
    RS485.flush();
    delayMicroseconds(50);
  }
}

static void rs485_tx(const uint8_t *data, size_t len) {
  RS485.write(data, len);
  RS485.flush();
}

static void rs485_begin(uint32_t baud) {
  RS485.end();
  RS485.setRx(PIN_RS485_RX);
  RS485.setTx(PIN_RS485_TX);
  RS485.begin(baud);
  // Frame gap ~3.5 chars; scale with baud
  uint32_t gap = (35000000UL / baud) / 1000UL;  // ms approx
  if (gap < 3) {
    gap = 3;
  }
  if (gap > 20) {
    gap = 20;
  }
  modbus_set_frame_gap_ms(gap);
}

void setup() {
  pinMode(PIN_MUX_S0, OUTPUT);
  pinMode(PIN_MUX_S1, OUTPUT);
  pinMode(PIN_RS485_DE, OUTPUT);
  pinMode(PIN_LED_RUN, OUTPUT);
  pinMode(PIN_LED_EEPR, OUTPUT);
  pinMode(PIN_LED_ERR, OUTPUT);

  led_write(PIN_LED_RUN, false);
  led_write(PIN_LED_EEPR, false);
  led_write(PIN_LED_ERR, false);
  rs485_set_de(false);

  analogReadResolution(12);

  EepromWire.begin();

  config_set_defaults(&g_cfg);
  if (config_load(&g_cfg)) {
    led_write(PIN_LED_EEPR, true);
    delay(100);
    led_write(PIN_LED_EEPR, false);
  }

  g_mb.slave_id = (uint8_t)g_cfg.slave_id;
  g_mb.read_input = mb_read_input;
  g_mb.read_holding = mb_read_holding;
  g_mb.write_holding = mb_write_holding;
  g_mb.input_count = MB_IR_COUNT;
  g_mb.holding_count = MB_HR_COUNT;
  modbus_init(&g_mb);

  rs485_begin(baud_from_code(g_cfg.baud_code));

  acquire_all_channels();
  led_write(PIN_LED_RUN, true);
}

void loop() {
  const uint32_t now = millis();

  // Drain RS485 RX into Modbus framer
  while (RS485.available() > 0) {
    modbus_rx_byte((uint8_t)RS485.read(), now);
  }
  modbus_poll(millis(), rs485_tx, rs485_set_de);

  static uint32_t last_acq = 0;
  if ((now - last_acq) >= g_cfg.sample_ms) {
    last_acq = now;
    acquire_all_channels();
    // heartbeat blink on RUN LED
    static bool run_blink = true;
    run_blink = !run_blink;
    led_write(PIN_LED_RUN, run_blink);
  }

  if (g_save_pending) {
    g_save_pending = false;
    const bool ok = config_save(&g_cfg);
    led_write(PIN_LED_EEPR, ok);
    delay(80);
    led_write(PIN_LED_EEPR, false);
  }

  if (g_apply_pending) {
    g_apply_pending = false;
    modbus_set_slave_id(&g_mb, (uint8_t)g_cfg.slave_id);
    rs485_begin(baud_from_code(g_cfg.baud_code));
  }
}
