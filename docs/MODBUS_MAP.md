# Modbus RTU register map

Slave defaults: **ID=1**, **9600 8N1**, RS485 half-duplex.

## Input registers (FC 0x04) — temperatures

| Addr | Name | Type | Description |
|-----:|------|------|-------------|
| 0 | TEMP_CH0 | int16 | Channel 0 temperature, **0.1 °C** (250 → 25.0 °C). `0x7FFF` = fault |
| 1 | TEMP_CH1 | int16 | Channel 1 |
| 2 | TEMP_CH2 | int16 | Channel 2 |
| 3 | TEMP_CH3 | int16 | Channel 3 |
| 4–7 | ADC_CHx | uint16 | Raw 12-bit ADC average |
| 8–11 | R_CHx | uint16 | Resistance **0.1 Ω** (1000 → 100.0 Ω). `0xFFFF` = fault |
| 12 | STATUS | uint16 | bit0–3 channel fault; bit8 open; bit9 short |
| 13 | FW_VERSION | uint16 | `0x0100` = v1.0 |
| 14 | BOARD_HI | uint16 | `'PT'` |
| 15 | BOARD_LO | uint16 | `'10'` |

### Master example (read 4 temperatures)

Function **0x04**, start **0**, quantity **4**.

Decode: `temp_c = int16(register) / 10.0` (skip if `0x7FFF`).

## Holding registers (FC 0x03 / 0x06 / 0x10)

| Addr | Name | Description |
|-----:|------|-------------|
| 0 | SLAVE_ID | 1–247 |
| 1 | BAUD_CODE | 0=9600, 1=19200, 2=38400, 3=57600, 4=115200 |
| 2 | SAMPLE_MS | 50–2000 ms acquisition period |
| 3–6 | TEMP_OFFSETx | int16, 0.1 °C calibration offset per channel |
| 7 | SAVE_CMD | write `1` → store config to EEPROM |
| 8 | APPLY_CMD | write `1` → apply slave ID / baud immediately |

After changing ID/baud: write **APPLY_CMD=1**, then reopen the master with new settings. Write **SAVE_CMD=1** to keep settings across power cycles.
