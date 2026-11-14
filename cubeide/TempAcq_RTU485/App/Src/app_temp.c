#include "app_temp.h"
#include "board_config.h"
#include "modbus_map.h"
#include "modbus_rtu.h"
#include "pt100.h"
#include "config_store.h"
#include <math.h>
#include <string.h>

/* GPIO labels from .ioc */
#ifndef LED_RUN_Pin
#define LED_RUN_Pin        GPIO_PIN_0
#define LED_RUN_GPIO_Port  GPIOA
#define RS485_DE_Pin       GPIO_PIN_1
#define RS485_DE_GPIO_Port GPIOA
#define LED_EEPR_Pin       GPIO_PIN_6
#define LED_EEPR_GPIO_Port GPIOA
#define LED_ERR_Pin        GPIO_PIN_7
#define LED_ERR_GPIO_Port  GPIOA
#define MUX_S0_Pin         GPIO_PIN_0
#define MUX_S0_GPIO_Port   GPIOB
#define MUX_S1_Pin         GPIO_PIN_1
#define MUX_S1_GPIO_Port   GPIOB
#endif

static ADC_HandleTypeDef *g_hadc = NULL;
static UART_HandleTypeDef *g_huart = NULL;
static I2C_HandleTypeDef *g_hi2c = NULL;

static BoardConfig g_cfg;
static ModbusSlave g_mb;
static Pt100Sample g_ch[NUM_CHANNELS];
static uint16_t g_status = 0U;
static bool g_apply_pending = false;
static bool g_save_pending = false;
static uint32_t g_last_acq_ms = 0U;
static bool g_run_blink = true;

static void led_write(GPIO_TypeDef *port, uint16_t pin, bool on)
{
  /* Active LOW LEDs on schematic */
  HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void select_channel(uint8_t ch)
{
  HAL_GPIO_WritePin(MUX_S0_GPIO_Port, MUX_S0_Pin,
                    ((ch & 0x01U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_S1_GPIO_Port, MUX_S1_Pin,
                    ((ch & 0x02U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint16_t read_adc_averaged(uint8_t samples)
{
  uint32_t sum = 0U;
  for (uint8_t i = 0U; i < samples; i++) {
    if (HAL_ADC_Start(g_hadc) != HAL_OK) {
      return 0U;
    }
    if (HAL_ADC_PollForConversion(g_hadc, 10U) != HAL_OK) {
      return 0U;
    }
    sum += HAL_ADC_GetValue(g_hadc);
    HAL_ADC_Stop(g_hadc);
    for (volatile uint32_t d = 0U; d < 200U; d++) {
      __NOP();
    }
  }
  return (uint16_t)(sum / samples);
}

static void acquire_all_channels(void)
{
  g_status = 0U;
  for (uint8_t ch = 0U; ch < NUM_CHANNELS; ch++) {
    select_channel(ch);
    HAL_Delay(1U);
    {
      const uint16_t adc = read_adc_averaged(16U);
      g_ch[ch] = pt100_from_adc(adc);
    }
    if ((!g_ch[ch].fault) && (!isnan(g_ch[ch].temperature_c))) {
      g_ch[ch].temperature_c += ((float)g_cfg.offset_x10[ch]) / 10.0f;
    } else {
      g_status |= (uint16_t)(1U << ch);
      if (g_ch[ch].open_wire) {
        g_status |= (1U << 8);
      }
      if (g_ch[ch].shorted) {
        g_status |= (1U << 9);
      }
    }
  }
  led_write(LED_ERR_GPIO_Port, LED_ERR_Pin, g_status != 0U);
}

static int16_t temp_to_reg(const Pt100Sample *s)
{
  float x;
  if (s->fault || isnan(s->temperature_c)) {
    return (int16_t)MB_TEMP_FAULT;
  }
  x = s->temperature_c * 10.0f;
  if (x > 32766.0f) {
    x = 32766.0f;
  }
  if (x < -32767.0f) {
    x = -32767.0f;
  }
  return (int16_t)lroundf(x);
}

static uint16_t r_to_reg(const Pt100Sample *s)
{
  float x;
  if (s->fault) {
    return 0xFFFFU;
  }
  x = s->resistance_ohm * 10.0f;
  if (x > 65534.0f) {
    x = 65534.0f;
  }
  if (x < 0.0f) {
    x = 0.0f;
  }
  return (uint16_t)lroundf(x);
}

static uint16_t mb_read_input(uint16_t addr)
{
  switch (addr) {
    case MB_IR_TEMP0: return (uint16_t)temp_to_reg(&g_ch[0]);
    case MB_IR_TEMP1: return (uint16_t)temp_to_reg(&g_ch[1]);
    case MB_IR_TEMP2: return (uint16_t)temp_to_reg(&g_ch[2]);
    case MB_IR_TEMP3: return (uint16_t)temp_to_reg(&g_ch[3]);
    case MB_IR_ADC0: return g_ch[0].adc_raw;
    case MB_IR_ADC1: return g_ch[1].adc_raw;
    case MB_IR_ADC2: return g_ch[2].adc_raw;
    case MB_IR_ADC3: return g_ch[3].adc_raw;
    case MB_IR_R0: return r_to_reg(&g_ch[0]);
    case MB_IR_R1: return r_to_reg(&g_ch[1]);
    case MB_IR_R2: return r_to_reg(&g_ch[2]);
    case MB_IR_R3: return r_to_reg(&g_ch[3]);
    case MB_IR_STATUS: return g_status;
    case MB_IR_FW_VERSION: return MB_FW_VERSION;
    case MB_IR_BOARD_HI: return 0x5054U;
    case MB_IR_BOARD_LO: return 0x3130U;
    default: return 0U;
  }
}

static uint16_t mb_read_holding(uint16_t addr)
{
  switch (addr) {
    case MB_HR_SLAVE_ID: return g_cfg.slave_id;
    case MB_HR_BAUD_CODE: return g_cfg.baud_code;
    case MB_HR_SAMPLE_MS: return g_cfg.sample_ms;
    case MB_HR_OFFSET0: return (uint16_t)g_cfg.offset_x10[0];
    case MB_HR_OFFSET1: return (uint16_t)g_cfg.offset_x10[1];
    case MB_HR_OFFSET2: return (uint16_t)g_cfg.offset_x10[2];
    case MB_HR_OFFSET3: return (uint16_t)g_cfg.offset_x10[3];
    default: return 0U;
  }
}

static bool mb_write_holding(uint16_t addr, uint16_t value)
{
  switch (addr) {
    case MB_HR_SLAVE_ID:
      if ((value < 1U) || (value > 247U)) {
        return false;
      }
      g_cfg.slave_id = value;
      return true;
    case MB_HR_BAUD_CODE:
      if (value > 4U) {
        return false;
      }
      g_cfg.baud_code = value;
      return true;
    case MB_HR_SAMPLE_MS:
      if ((value < 50U) || (value > 2000U)) {
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
      if (value == 1U) {
        g_save_pending = true;
      }
      return true;
    case MB_HR_APPLY_CMD:
      if (value == 1U) {
        g_apply_pending = true;
      }
      return true;
    default:
      return false;
  }
}

static void rs485_set_de(bool enable)
{
  HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin,
                    enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
  if (enable) {
    for (volatile uint32_t d = 0U; d < 200U; d++) {
      __NOP();
    }
  }
}

static void rs485_tx(const uint8_t *data, size_t len)
{
  (void)HAL_UART_Transmit(g_huart, (uint8_t *)data, (uint16_t)len, 100U);
  for (volatile uint32_t d = 0U; d < 200U; d++) {
    __NOP();
  }
  rs485_set_de(false);
}

static void uart_apply_baud(uint32_t baud)
{
  g_huart->Init.BaudRate = baud;
  (void)HAL_UART_Abort(g_huart);
  (void)HAL_UART_DeInit(g_huart);
  (void)HAL_UART_Init(g_huart);
  /* Enable RXNE interrupt for Modbus RX */
  __HAL_UART_ENABLE_IT(g_huart, UART_IT_RXNE);

  {
    uint32_t gap = 35000000UL / baud / 1000UL;
    if (gap < 3U) {
      gap = 3U;
    }
    if (gap > 20U) {
      gap = 20U;
    }
    modbus_set_frame_gap_ms(gap);
  }
}

void App_OnUartRxByte(uint8_t byte)
{
  modbus_rx_byte(byte, HAL_GetTick());
}

void App_Init(ADC_HandleTypeDef *hadc,
              UART_HandleTypeDef *huart,
              I2C_HandleTypeDef *hi2c)
{
  g_hadc = hadc;
  g_huart = huart;
  g_hi2c = hi2c;

  led_write(LED_RUN_GPIO_Port, LED_RUN_Pin, false);
  led_write(LED_EEPR_GPIO_Port, LED_EEPR_Pin, false);
  led_write(LED_ERR_GPIO_Port, LED_ERR_Pin, false);
  rs485_set_de(false);

  /* ADC calibration (F1) */
  (void)HAL_ADCEx_Calibration_Start(g_hadc);

  config_set_defaults(&g_cfg);
  if (config_load(g_hi2c, &g_cfg)) {
    led_write(LED_EEPR_GPIO_Port, LED_EEPR_Pin, true);
    HAL_Delay(100U);
    led_write(LED_EEPR_GPIO_Port, LED_EEPR_Pin, false);
  }

  g_mb.slave_id = (uint8_t)g_cfg.slave_id;
  g_mb.read_input = mb_read_input;
  g_mb.read_holding = mb_read_holding;
  g_mb.write_holding = mb_write_holding;
  g_mb.input_count = MB_IR_COUNT;
  g_mb.holding_count = MB_HR_COUNT;
  modbus_init(&g_mb);

  uart_apply_baud(baud_from_code(g_cfg.baud_code));
  acquire_all_channels();
  g_last_acq_ms = HAL_GetTick();
  led_write(LED_RUN_GPIO_Port, LED_RUN_Pin, true);
}

void App_Loop(void)
{
  const uint32_t now = HAL_GetTick();

  modbus_poll(now, rs485_tx, rs485_set_de);

  if ((now - g_last_acq_ms) >= g_cfg.sample_ms) {
    g_last_acq_ms = now;
    acquire_all_channels();
    g_run_blink = !g_run_blink;
    led_write(LED_RUN_GPIO_Port, LED_RUN_Pin, g_run_blink);
  }

  if (g_save_pending) {
    bool ok;
    g_save_pending = false;
    ok = config_save(g_hi2c, &g_cfg);
    led_write(LED_EEPR_GPIO_Port, LED_EEPR_Pin, ok);
    HAL_Delay(80U);
    led_write(LED_EEPR_GPIO_Port, LED_EEPR_Pin, false);
  }

  if (g_apply_pending) {
    g_apply_pending = false;
    modbus_set_slave_id(&g_mb, (uint8_t)g_cfg.slave_id);
    uart_apply_baud(baud_from_code(g_cfg.baud_code));
  }
}
