# 4-Channel PT100 Temperature Board — Modbus RTU

STM32F103RCT6 firmware so a **Modbus RTU master** can read four PT100 temperatures over **RS485**.

## Defaults

| Setting | Value |
|---------|-------|
| Slave ID | 1 |
| Serial | 9600 8N1 |
| Sensors | PT100, ~1 mA excitation, gain 21 |
| Protocol | Modbus RTU (FC03 / FC04 / FC06 / FC10) |

## Quick start — master read

```bash
pip install 'pymodbus>=3.0.0' pyserial
python3 tools/modbus_master_read.py --port /dev/ttyUSB0 --slave 1 --baud 9600
```

Temperatures are in **input registers 0–3** as `int16` with **0.1 °C** resolution (`250` → `25.0 °C`). Fault = `0x7FFF`.

Full map: [docs/MODBUS_MAP.md](docs/MODBUS_MAP.md)

## Build & flash firmware

```bash
cd firmware
pio run
pio run -t upload   # ST-Link on H1: 3V3, GND, SWIO, SWCLK
```

Requires [PlatformIO](https://platformio.org/).

## Hardware wiring (from schematic)

| Function | MCU pin | Net |
|----------|---------|-----|
| ADC | PA4 | ADC_IN |
| Mux S0/S1 | PB0 / PB1 | S0 / S1 |
| RS485 TX/RX | PA2 / PA3 | UART_TX01 / UART_RX01 |
| RS485 DE | PA1 | RS485_R/W |
| EEPROM I²C | PB10 / PB11 | EEPR_SCL / EEPR_SDA |

Connect PT100s to **CN1** (T0/T1) and **CN2** (T2/T3). RS485 A+/B− and DC power on **CN3**.

## Project layout

```
firmware/     PlatformIO app (acquisition + Modbus slave)
tools/        Python Modbus RTU master example
docs/         Register map
```
