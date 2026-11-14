# STM32CubeIDE — 4-ch PT100 Modbus RTU slave

This folder is for **STM32CubeIDE** (HAL), not PlatformIO.

## 1. Create the project from the `.ioc`

1. Open **STM32CubeIDE**
2. **File → New → STM32 Project from an Existing STM32CubeMX Configuration File (.ioc)**
3. Select:
   `cubeide/TempAcq_RTU485/TempAcq_RTU485.ioc`
4. Choose a workspace location (or keep this folder)
5. Finish, then click **Generate Code** if prompted

Configured peripherals:

| Peripheral | Pins / settings |
|------------|-----------------|
| HSE 8 MHz → PLL 72 MHz | PD0 / PD1 |
| ADC1 IN4 | PA4 (`ADC_IN`) |
| USART2 9600 8N1 | PA2 TX, PA3 RX (+ NVIC) |
| I2C2 | PB10 SCL, PB11 SDA |
| GPIO out | PA1 RS485 DE, PB0/PB1 mux, PA0/PA6/PA7 LEDs |
| SWD | PA13 / PA14 |

## 2. Add application sources

In CubeIDE Project Explorer:

1. Right-click project → **New → Source Folder** → name it `App` (if not already present)
2. Copy into the project (already in this repo):
   - `App/Inc/*.h`
   - `App/Src/*.c`
3. Right-click project → **Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths**
4. Add:
   - `../App/Inc`
   - or `${ProjDirPath}/App/Inc`
5. **Project → Build All** should compile `App/Src/*.c` automatically if they are under the project tree

If CubeIDE created the project elsewhere, copy the whole `App/` directory next to `Core/`.

## 3. Wire app into generated `main.c` / IRQ

Open `CubeIDE_USER_CODE_SNIPPETS.c` and paste the marked fragments into:

- `Core/Inc/main.h` → `#include "app_temp.h"`
- `Core/Src/main.c` → `App_Init(...)` after MX inits, `App_Loop()` in `while(1)`
- `Core/Src/stm32f1xx_it.c` → RX byte hook in `USART2_IRQHandler`

Keep edits inside `USER CODE BEGIN/END` blocks.

## 4. Build & flash

1. **Project → Build All**
2. Connect ST-Link to **H1** (3V3, GND, SWIO, SWCLK)
3. **Run → Debug** (or Run)

## 5. Master read (RS485)

Defaults: **slave ID 1**, **9600 8N1**

```bash
pip install -r requirements.txt
python3 tools/modbus_master_read.py --port COMx --slave 1 --baud 9600
```

Temperatures: input registers **0–3**, unit **0.1 °C** (`250` = 25.0 °C).  
Register map: `docs/MODBUS_MAP.md`
