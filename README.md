# 4-Channel PT100 Temperature Board — Modbus RTU

STM32F103RCT6 firmware so a **Modbus RTU master** can read four PT100 temperatures over **RS485**.

## Defaults

| Setting | Value |
|---------|-------|
| Slave ID | 1 |
| Serial | 9600 8N1 |
| Sensors | PT100, ~1 mA excitation, gain 21 |
| Protocol | Modbus RTU (FC03 / FC04 / FC06 / FC10) |

## STM32CubeIDE (recommended)

Step-by-step: **[cubeide/TempAcq_RTU485/README_CUBEIDE.md](cubeide/TempAcq_RTU485/README_CUBEIDE.md)**

1. Open `cubeide/TempAcq_RTU485/TempAcq_RTU485.ioc` in CubeIDE → Generate Code  
2. Keep / add the `App/` sources  
3. Paste USER CODE from `CubeIDE_USER_CODE_SNIPPETS.c` into `main.c` / `stm32f1xx_it.c`  
4. Build & flash via ST-Link  

## Master read (any PC)

```bash
pip install -r requirements.txt
python3 tools/modbus_master_read.py --port /dev/ttyUSB0 --slave 1 --baud 9600
```

Temperatures: input registers **0–3**, **0.1 °C** (`250` → `25.0 °C`). Fault = `0x7FFF`.  
Map: [docs/MODBUS_MAP.md](docs/MODBUS_MAP.md)

## Optional: PlatformIO build

```bash
cd firmware && pio run && pio run -t upload
```

## Hardware

| Function | MCU pin | Net |
|----------|---------|-----|
| ADC | PA4 | ADC_IN |
| Mux S0/S1 | PB0 / PB1 | S0 / S1 |
| RS485 TX/RX | PA2 / PA3 | UART_TX01 / UART_RX01 |
| RS485 DE | PA1 | RS485_R/W |
| EEPROM I²C | PB10 / PB11 | EEPR_SCL / EEPR_SDA |

## Layout

```
cubeide/TempAcq_RTU485/   STM32CubeIDE (.ioc + HAL App sources)
firmware/                 PlatformIO alternative
tools/                    Python Modbus master
docs/                     Register map
```
