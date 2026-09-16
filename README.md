# 🎮 GameHub — STM32 Bare-Metal Arcade Console

> **ITI Graduation Project** — a from-scratch, register-level (no HAL) embedded games console running on an **STM32F401xC** (ARM Cortex-M4).
> Everything below `main()` is hand-written C: GPIO, clocks, SysTick, SPI, EXTI, NVIC, DMA, USART, an ST7735 TFT driver, an NEC IR remote decoder, a software scheduler, and the games themselves.

## 📑 Table of Contents

1. [Elevator Pitch](#1-elevator-pitch-30-seconds)
2. [Key Features / Demo Checklist](#2-key-features--demo-checklist)
3. [Hardware Setup & Pin Map](#3-hardware-setup--pin-map)
4. [System Architecture](#4-system-architecture)
5. [Repository Layout](#5-repository-layout)
6. [Coding Conventions & Naming Rules](#6-coding-conventions--naming-rules)
7. [LIB Layer](#7-lib-layer)
8. [MCAL Layer — Driver by Driver](#8-mcal-layer--driver-by-driver)
9. [HAL Layer — Driver by Driver](#9-hal-layer--driver-by-driver)
10. [RTOS Layer + Vendored FreeRTOS](#10-rtos-layer-custom-scheduler--vendored-freertos)
11. [APP Layer — State Machine & Games](#11-app-layer--state-machine--games)
12. [Deep-Dive: TFT Pixel Pipeline](#12-deep-dive-how-the-tft-pixel-pipeline-works)
13. [Deep-Dive: NEC IR Decoding](#13-deep-dive-nec-ir-remote-decoding-interrupt-driven)
14. [Deep-Dive: XO Game Logic](#14-deep-dive-the-xo-game-logic)
15. [Build, Flash & Run](#15-build-flash--run)
16. [Live Demo Script](#16-live-demo-script-presentation-flow)
17. [Anticipated Q&A](#17-anticipated-qa-examiner-questions)
18. [Known Limitations & Bugs](#18-known-limitations--bugs-honest-list)
19. [Roadmap & Branch Map](#19-roadmap--branch-map)
20. [Glossary](#20-glossary)
21. [Credits & Repository Notes](#21-credits--repository-notes)

---

## 1. Elevator Pitch (30 seconds)

**GameHub** turns a bare STM32F401xC microcontroller into a small arcade machine:

* An **IR remote** is the game controller.
* A **1.8" ST7735 SPI TFT** (128×160, RGB565) is the display.
* A **menu state machine** hosts the games — the finished one is **XO (Tic-Tac-Toe)**; Snake and Pac-Man live on separate branches.
* Every peripheral is driven by **our own drivers** written straight against the reference manual (base addresses, register structs, bit numbers). There is **no ST HAL and no CubeMX-generated code in the application path** — the `system/` folder ships ST's HAL sources, but our `src/` never calls them.

**One-sentence pitch for the jury:**

> *"We built a layered bare-metal driver stack (MCAL → HAL → APP) plus a software real-time scheduler, and proved it works by shipping a playable IR-controlled arcade console on an STM32F401."*

---

## 2. Key Features / Demo Checklist

| # | Feature | Status | Where |
|---|---------|--------|-------|
| 1 | Register-level GPIO driver (Ports A/B/C, AF, pull, speed, atomic BSRR writes) | ✅ Working | `MCAL/GPIO` |
| 2 | RCC clock control (HSE 25 MHz, per-peripheral gating, 4 buses) | ✅ Working | `MCAL/RCC` |
| 3 | SysTick driver: blocking µs/ms delays + single/multi-shot callbacks | ✅ Working | `MCAL/SYSTICK` |
| 4 | SPI1 master driver (8-bit, MSB-first, CPOL=0/CPHA=0) | ✅ Working | `MCAL/SPI` |
| 5 | EXTI driver with per-line callbacks + IRQ handlers | ✅ Working | `MCAL/EXTI` |
| 6 | NVIC driver: enable/disable/pending/priority grouping | ✅ Working | `MCAL/NVIC` |
| 7 | SYSCFG driver (EXTI line → port muxing) | ✅ Working | `MCAL/SYSCFG` |
| 8 | ST7735 TFT HAL: init, screen fill, rectangles, 5×7 font, strings | ✅ Working | `HAL/TFT` |
| 9 | NEC IR remote HAL: interrupt-timed pulse capture + bit decode | ✅ Working | `HAL/IR` |
| 10 | Main menu + XO game (cursor + numpad input, win detection) | ✅ Working | `APP` |
| 11 | BUZZER HAL: tone generation, SFX (button click, victory), melodies | ✅ Implemented, not wired to `main()` | `HAL/BUZZER` |
| 12 | 8-bit R-2R DAC HAL + 79,314-byte voice sample (`voice.h`) | ✅ Implemented, not wired | `HAL/DAC`, `APP/voice.h` |
| 13 | 8×8 LED matrix HAL over cascaded 74HC595 (S2P) | ✅ Implemented, not wired | `HAL/LEDMATRIX`, `HAL/S2P` |
| 14 | Seven-segment HAL (common cathode/anode, digits 0–9) | ✅ Implemented, not wired | `HAL/SEVENSEGMENT` |
| 15 | DMA2 driver (memory-to-memory, stream ISR + callback) | ✅ Implemented, not wired | `MCAL/DMA` |
| 16 | USART1 driver (polled + interrupt TX/RX with callback) | ✅ Implemented, not wired | `MCAL/USART` |
| 17 | Custom periodic-task RTOS (task table, priority, suspend/resume/delete) | ✅ Implemented | `RTOS/` |
| 18 | FreeRTOS V202112.00 kernel vendored + configured for this MCU | 📦 Vendored, unused | `FreeRTOS/` |
| 19 | Multi-game build (Snake + Pac-Man + high scores + parental control) | 🌿 On `main` / `PacMan` / `snake` branches | [§19](#19-roadmap--branch-map) |

---

## 3. Hardware Setup & Pin Map

### 3.1 Target board / MCU

| Item | Value | Evidence in the code |
|------|-------|----------------------|
| MCU | **STM32F401xC** (Cortex-M4 with FPU) | `.cproject` define `STM32F401xC`; `system/include/cmsis/stm32f401xc.h` |
| System clock | **25 MHz HSE** (external crystal, not bypassed) | `RCC_cfg.h`: `RCC_SYS_CLK = HSE_CLK`, `HSE_BYPASS = MECHANICAL_CLK`; `RCC_prg.c` turns HSE on then waits for `HSERDY` |
| SYSCLK source | HSE directly — **no PLL** (the PLL branch of `MRCC_vInit()` is intentionally empty) | `RCC_prg.c` lines 35–36: `CFGR` SW = `01` (HSE) |
| SysTick clock | **AHB/8 = 3.125 MHz** → 1 tick = 0.32 µs → **3125 ticks per ms** | `SYSTICK_prg.c`: `delay_ms * 3125.0`, `delay_us * 3.125` |
| Remote protocol | **NEC**, 38 kHz carrier, "Car MP3 Mini" remote command map | `IR_int.h`, `IR_prg.c` |
| Toolchain | `arm-none-eabi-` Cross ARM GCC (Thumb), Debug profile | `Drivers/.project`, `Drivers/.cproject` (Eclipse CDT managed build) |
| Linker scripts | `mem.ld`, `sections.ld`, `libs.ld` | `WorkSpace/Drivers/ldscripts/` |
| Optional debug UART | USART1 — 9600 baud, 8 data bits, no parity, 1 stop, oversampling ×16 | `USART_prg.c`: `BRR = (162 << 4) | 13` |

### 3.2 Pin map (as configured by `main()` and the HAL drivers)

| Pin | Signal | Configuration | Notes |
|-----|--------|---------------|-------|
| **PA0** | IR receiver OUT | `GPIO_MODE_INPUT`, EXTI line 0, **falling edge** | NVIC position **6** (`EXTI0_IRQn`) — from `IR_cfg.h` |
| **PA1** | TFT `A0` / `DC` (data-command select) | GPIO output, push-pull, low speed | `0` = command byte, `1` = data byte |
| **PA2** | TFT `RST` | GPIO output, push-pull, low speed | Driven by `Reset_Seq()` |
| **PA5** | SPI1 `SCK` | Alternate function **AF5** | SPI1 `BR = 0` → APB2/2 bit clock |
| **PA7** | SPI1 `MOSI` (TFT `SDA`) | Alternate function **AF5** | MSB-first, 8-bit frames |
| PA13 / PA14 / PA15 | — | **blocked by the GPIO driver** | SWD/JTAG pins — deliberately refused |
| PB3 / PB4 | — | **blocked by the GPIO driver** | JTAG pins — deliberately refused |

**Clocks enabled in `main()`:**

```c
MRCC_vInit();                              // HSE 25 MHz, SYSCLK = HSE
MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);     // AHB1ENR bit 0  -> GPIOA
MRCC_vEnableCLK(RCC_APB2, 14);             // APB2ENR bit 14 -> SYSCFG
MRCC_vEnableCLK(RCC_APB2, 12);             // APB2ENR bit 12 -> SPI1
```

**Driver-ready but not yet wired in `main()`:**

| Module | Pins needed | Configuration point |
|--------|-------------|---------------------|
| BUZZER | 1 digital output | `BUZZER_Config_t { Port, Pin }` |
| DAC (R-2R, 8-bit) | 8 **consecutive** pins on one port | `HDAC_vInit(GPIOx_PinConfig_t*, pinCount)` |
| LED MATRIX (2× 74HC595) | DATA, SHIFT_CLK, LATCH_CLK | `S2P_Init_t`, `S2P_NO_OF_SHIFT_REG = 2` |
| 7-SEGMENT | 7 **consecutive** pins | `SevenSegment_t { Port, StartPin, DisplayType }` |

### 3.3 Bill of Materials for the demo table

1. STM32F401 board (Nucleo-F401RE / Blackpill-class)
2. 1.8" ST7735 SPI TFT module (128×160, RGB565)
3. IR receiver module (VS1838B / TSOP-style, 38 kHz) + "Car MP3 Mini" NEC remote
4. Jumper wires + breadboard
5. *(optional, extended demo)* buzzer, 8-resistor R-2R ladder + speaker, 8×8 LED matrix + 2× 74HC595, 7-segment display

---

## 4. System Architecture

```
                   ┌──────────────────────────────────────────────┐
                   │  APP   application / game logic              │
                   │  main.c · state machine · XO.c · voice.h     │
                   └───────────────┬──────────────────────────────┘
                                   │ includes ONLY *_int.h interfaces
                   ┌───────────────▼──────────────────────────────┐
                   │  HAL   board-level abstractions              │
                   │  TFT · IR · BUZZER · DAC · LEDMATRIX · S2P   │
                   │  SEVENSEGMENT                                │
                   └───────────────┬──────────────────────────────┘
                                   │
                   ┌───────────────▼──────────────────────────────┐
                   │  MCAL  peripheral drivers, register-level    │
                   │  RCC · GPIO · SYSTICK · SPI · EXTI · NVIC    │
                   │  SYSCFG · DMA · USART                        │
                   └───────────────┬──────────────────────────────┘
                                   │ memory-mapped register structs
                   ┌───────────────▼──────────────────────────────┐
                   │  STM32F401xC registers / NVIC / SysTick      │
                   └──────────────────────────────────────────────┘

  Cross-cutting:  LIB (STD_TYPES.h, BIT_MATH.h)
                  RTOS (custom tick scheduler)  ·  FreeRTOS (vendored)
```

**Design rules the code actually follows**

| Rule | How it is enforced |
|------|--------------------|
| Upper layers never touch registers | `APP`/`HAL` include only `*_int.h`; register structs live in `*_prv.h` |
| Configuration separated from implementation | `*_cfg.h` (`IR_cfg.h`, `RCC_cfg.h`, `S2P_cfg.h`, `LEDMATRIX_cfg.h`, `RTOS_cfg.h`) |
| Every module exposes init + action APIs | `MXxx_vInit()` (MCAL) / `HXxx_vInit()` (HAL) |
| Interrupts use callbacks, not loose globals | `MEXTI_vSetCallBack`, `MDMA2_vCallBack`, `MUSART_vUSARTCallBack`, `MSYSTICK_vSetInterval*` |
| Error codes instead of silent failure | `RTOS_*` return `1 = RESERVED_PRIORITY_ERROR`, `2 = OUT_OF_SYSTASK_RANGE_ERROR` |
| Illegal hardware states refused early | GPIO driver ignores PA13/14/15 and PB3/4 (debug pins) |

---

## 5. Repository Layout

```
GameHub/
├── README.md                     ← this document
├── .gitignore                    ← tracks ONLY *.c / *.h / .gitignore (build output ignored)
├── WorkSpace/
│   ├── Drivers/                  ★ the Eclipse CDT project ("Drivers", Debug config)
│   │   ├── .project / .cproject  STM32F401xC, Cross ARM GCC, defines & include paths
│   │   ├── ldscripts/            mem.ld, sections.ld, libs.ld
│   │   ├── Debug/                Eclipse build output (generated, not tracked)
│   │   ├── system/               CMSIS core + STM32F4 HAL sources (NOT used by our drivers)
│   │   └── src/
│   │       ├── APP/              main.c, XO.c + XO_int.h, voice.h (79,314-byte PCM sample)
│   │       ├── LIB/              STD_TYPES.h, BIT_MATH.h
│   │       ├── MCAL/             RCC, GPIO, SYSTICK, SPI, EXTI, NVIC, SYSCFG, DMA, USART
│   │       ├── HAL/              TFT, IR, BUZZER, DAC, LEDMATRIX, S2P, SEVENSEGMENT
│   │       └── RTOS/             RTOS_cfg.h, RTOS_int.h, RTOS_prg.c, RTOS_prv.h
│   └── FreeRTOS/                 FreeRTOS V202112.00 + FreeRTOSConfig.h (configured, unused)
├── build/  build-arm/  build-mingw/  build-pc/
│   └── ⚠️ STALE ARTIFACTS — CMake/Ninja leftovers from an unrelated local Pac-Man
│       checkout (their CMakeCache points to "D:/Pac-Man (C language)/Pac-Man-C-language-").
│       NOT part of this project, NOT tracked by git. Safe to delete.
└── tools/                        empty
```

**File-naming convention inside every driver folder**

| Suffix | Meaning | Example |
|--------|---------|---------|
| `*_int.h` | public interface — the only header upper layers may include | `TFT_int.h` |
| `*_cfg.h` | compile-time configuration | `IR_cfg.h` |
| `*_prv.h` | private: base addresses, register structs, bit numbers, const tables | `SPI_prv.h`, `SEVENSEGMENT_prv.h` |
| `*_prg.c` | implementation | `SPI_prg.c` |

---

## 6. Coding Conventions & Naming Rules

| Convention | Example | Meaning |
|-----------|---------|---------|
| `u8 / s8 / u16 / u32 / f32 / f64` | `STD_TYPES.h` | project fixed-width typedefs instead of `uint8_t` |
| `M<Periph>_` prefix | `MGPIO_vPinInit`, `MRCC_vEnableCLK` | MCAL function |
| `H<Module>_` prefix | `HTFT_vInit`, `HIR_u8GetKey` | HAL function |
| `_v` / `_u8` suffix | `HTFT_vDrawString`, `HIR_u8GetKey` | return type (`void`, `u8`) |
| `A_` prefix | `A_u8Key` | function parameter |
| `L_` prefix | `L_u8Ticks` | local variable |
| `G_` prefix | `G_u8NewKeyReady` | global / file-scope static |
| `K_` prefix | `K_u8SevSegPatterns` | constant lookup table |
| `volatile` on ISR-shared data | `static volatile u8 G_u8NewKeyReady` | written in ISR, read in main loop |
| Register access via struct pointers | `SPI1->CR1`, `GPIOA->MODER` | no raw addresses inside logic code |

**Core bit macros (`LIB/BIT_MATH.h`)**

```c
SET_BIT(reg, n)      // reg |=  (1U << n)
CLR_BIT(reg, n)      // reg &= ~(1U << n)
TOG_BIT(reg, n)      // reg ^=  (1U << n)
GET_BIT(reg, n)      // (reg >> n) & 1U
SET_BYTE(reg, value) // reg = value
```


---

## 7. LIB Layer

Two header-only modules at the bottom of the stack — no `.c` file, no cost at runtime.

### `LIB/STD_TYPES.h` — project-wide fixed-width types

| Type | Definition | Typical use in this project |
|------|------------|-----------------------------|
| `u8` / `s8` | `unsigned char` / `signed char` | keys, register bytes, colours low byte |
| `u16` / `s16` | `unsigned short int` / `signed short int` | RGB565 colours, image arrays |
| `u32` / `s32` | `unsigned long int` / `signed long int` | SysTick load values, register accesses |
| `u64` | `unsigned long long int` | (reserved) |
| `f32` / `f64` | `float` / `double` | delay maths (`3125.0`, `3.125`), tone periods |
| `NULL` | `((void*)0)` (guarded by `#ifndef`) | callback-pointer checks in every ISR |

### `LIB/BIT_MATH.h` — bit manipulation macros

`SET_BIT`, `CLR_BIT`, `TOG_BIT`, `GET_BIT`, `SET_BYTE` (see [§6](#6-coding-conventions--naming-rules)). Every driver writes its registers through these macros, e.g. `SET_BIT(SPI1->CR1, SPE)`.

---

## 8. MCAL Layer — Driver by Driver

Nine peripheral drivers. Each one = `*_int.h` (public API) + `*_prv.h` (base address, register struct, bit numbers) + `*_prg.c`.

**MCAL summary table**

| Driver | Files | Base address | Purpose | Public API (short) |
|--------|-------|--------------|---------|--------------------|
| **RCC** | `RCC_int.h`, `RCC_cfg.h`, `RCC_prv.h`, `RCC_prg.c` | `0x40023800` | Select system clock, gate peripheral clocks | `MRCC_vInit`, `MRCC_vEnableCLK`, `MRCC_vDisableCLK` |
| **GPIO** | `GPIO_int.h`, `GPIO_prv.h`, `GPIO_prg.c` | A `0x40020000`, B `0x40020400`, C `0x40020800` | Pin mode/type/speed/pull/AF, read/write | `MGPIO_vPinInit`, `MGPIO_vSetPinValue`, `MGPIO_vSetPinValueAtomic`, `MGPIO_vTogPinValue`, `MGPIO_u8GetPinValue`, +5 setters |
| **SYSTICK** | `SYSTICK_int.h`, `SYSTICK_cfg.h`, `SYSTICK_prv.h`, `SYSTICK_prg.c` | `0xE000E010` | µs/ms delays, periodic/single-shot callbacks | `MSYSTICK_vInit`, `MSYSTICK_vSetDelay_ms/us`, `MSYSTICK_vSetIntervalSingle/Multi`, `MSYSTICK_u32GetElapsedTime_SingleShot` |
| **SPI** | `SPI_int.h`, `SPI_prv.h`, `SPI_prg.c` | SPI1 `0x40013000` | Master serial transfer to the TFT | `MSPI_vInit`, `MSPI_vTransceive` |
| **EXTI** | `EXTI_int.h`, `EXTI_prv.h`, `EXTI_prg.c` | `0x40013C00` | Edge-detect external interrupts + callbacks | `MEXTI_vInit`, `MEXTI_vEnableINT`, `MEXTI_vSetTrigger`, `MEXTI_vSetCallBack`, `MEXTI_vClearPendingFlag` |
| **NVIC** | `NVIC_int.h`, `NVIC_prv.h`, `NVIC_prg.c` | `0xE000E100` | Enable/disable/pend IRQs, priority grouping | `MNVIC_vEnable_Peripheral_INT`, `MNVIC_SetGroupPriority`, `MNVIC_vSetPeripheralPriority` |
| **SYSCFG** | `SYSCFG_int.h`, `SYSCFG_prv.h`, `SYSCFG_prg.c` | `0x40013800` | Route EXTI line `n` to a port | `MSYSCFG_vSetEXTIPort` |
| **DMA** | `DMA_int.h`, `DMA_prv.h`, `DMA_prg.c` | DMA2 `0x40026400` | Memory-to-memory transfers with callback | `MDMA2_vInit`, `MDMA2_vSetStreamCfg`, `MDMA2_vEnableStream`, `MDMA2_vCallBack` |
| **USART** | `USART_int.h`, `USART_prv.h`, `USART_prg.c` | USART1 `0x40011000` | Serial TX/RX, polled + interrupt modes | `MUSART_vInit`, `MUSART_vSendData`, `MUSART_u8ReceiveData`, `MUSART_vSendString`, `MUSART_vSendStringAsync`, `MUSART_vUSARTCallBack` |

### 8.1 RCC — Reset & Clock Control

**What it does**

* `MRCC_vInit()` — implemented branches: `HSE_CLK`, `HSI_CLK`, and a **compile-time `#error "invalid option"`** for anything else. PLL is a placeholder (empty branch). HSE bypass is configurable (`MECHANICAL_CLK` = crystal, `RC_CLK` = external clock source → sets `HSEBYP`).
* `MRCC_vEnableCLK(bus, peripheralBit)` / `MRCC_vDisableCLK(...)` — switch over `RCC_AHB1`, `RCC_AHB2`, `RCC_APB1`, `RCC_APB2` and `SET/CLR` the corresponding `AHB1ENR / AHB2ENR / APB1ENR / APB2ENR` bit.

**Why it matters for the project:** the application decides exactly which peripheral gets a clock — GPIOA, SYSCFG (bit 14) and SPI1 (bit 12) — so the whole TFT + IR stack is clock-gated on demand.

### 8.2 GPIO — General Purpose I/O

**API & behaviour**

| Function | Registers touched |
|----------|-------------------|
| `MGPIO_vSetMode` | `MODER` (2 bits/pin) |
| `MGPIO_vSetOutputType` | `OTYPER` (push-pull / open-drain) |
| `MGPIO_vSetOutputSpeed` | `OSPEEDR` (low/medium/high/very-high) |
| `MGPIO_vSetPullType` | `PUPDR` (none / pull-up / pull-down) |
| `MGPIO_vSetAlt` | `AFRL` (pins 0–7) or `AFRH` (pins 8–15), 4 bits/pin |
| `MGPIO_vSetPinValue` | `ODR` |
| `MGPIO_vSetPinValueAtomic` | `BSRR` (atomic set/reset — cannot be corrupted by an IRQ) |
| `MGPIO_vTogPinValue` / `MGPIO_u8GetPinValue` | `ODR` / `IDR` |
| `MGPIO_vPinInit` | convenience: applies mode + type + speed + pull + AF from one `GPIOx_PinConfig_t` struct |

**Guards built in:** every setter silently ignores **PA13/PA14/PA15** and **PB3/PB4** (SWD/JTAG pins) so firmware can never brick the debug port.

**Config struct used everywhere in the app:**

```c
GPIOx_PinConfig_t pin = { .Port = GPIO_PORTA, .Pin = GPIO_PIN5,
                          .Mode = GPIO_MODE_ALF, .AltFunc = GPIO_AF5 };
MGPIO_vPinInit(&pin);
```

### 8.3 SYSTICK — the timing backbone

* `MSYSTICK_vInit(cfg)` programs `CTRL`: interrupt enable (`TICKINT`), clock source (`CLKSOURCE`) and stops the timer first.
* **Blocking delays:** `MSYSTICK_vSetDelay_ms/us()` compute ticks from the fixed 25 MHz/8 = 3.125 MHz clock (3125 ticks/ms), load `LOAD`, clear `VAL`, then **poll `COUNTFLAG`** and stop. Used by the TFT reset sequence and the buzzer tone loops.
* **Callbacks:** `vSetIntervalSingle(ms, fptr)` / `vSetIntervalMulti(ms, fptr)` store the callback in `G_Fptr` and a "single-shot" flag; `SysTick_Handler()` calls the callback and, for single-shot, stops the timer. This is how the **IR decoder** schedules its sampling and how the **RTOS** gets its tick.
* `u32GetElapsedTime_SingleShot()` = `LOAD - VAL` and `u32GetRemainingTime_SingleShot()` = `VAL` — the two functions the IR driver uses as a stopwatch.


### 8.4 SPI — Serial Peripheral Interface (SPI1 master)

`MSPI_vInit()` configures SPI1 in the exact mode the ST7735 needs:

| Setting | Register/bit | Value |
|---------|--------------|-------|
| Software slave management | `CR1.SSM` (bit 9) = 1 | keeps NSS internal (no CS pin used) |
| Internal NSS high | `CR1.SSI` (bit 8) = 1 | prevents master-mode fault |
| 8-bit frames | `CR1.DFF` (bit 11) = 0 | byte transfers |
| MSB first | `CR1.LSBFIRST` (bit 7) = 0 | ST7735 expects MSB first |
| Master | `CR1.MSTR` (bit 2) = 1 | MCU drives SCK |
| Mode 0 | `CR1.CPOL`=0, `CR1.CPHA`=0 | idle-low clock, sample on first edge |
| Enable | `CR1.SPE` (bit 6) = 1 | turn the peripheral on |

`MSPI_vTransceive(byte)` is a classic blocking full-duplex transfer: wait for `SR.TXE`, write `DR`, wait for `SR.RXNE`, read `DR`. Every TFT command and every RGB565 pixel byte goes through this one function.

### 8.5 EXTI — External Interrupts

* `MEXTI_vInit()` clears all pending flags (`PR = 0xFFFFFFFF`).
* `MEXTI_vEnableINT/DisableINT(line)` → `IMR` bit.
* `MEXTI_vSetTrigger(line, edge)` → `RTSR` (rising), `FTSR` (falling) or both (on-change).
* `MEXTI_vSetCallBack(fptr, line)` stores the handler in a 16-entry function-pointer table.
* `MEXTI_vClearPendingFlag(line)` → writes 1 to `PR` (this is the **explicit flag clear added to `EXTI0_IRQHandler`** in the latest commit, so a new IR frame can't be missed).
* ISRs implemented: `EXTI0..EXTI4_IRQHandler` and `EXTI9_5_IRQHandler` (checks lines 5 and 6 individually via `PR`).

**Used by:** the IR receiver on line 0 (PA0).

### 8.6 NVIC — Nested Vectored Interrupt Controller

| Function | Effect |
|----------|--------|
| `MNVIC_vEnable_Peripheral_INT(pos)` | `ISER[pos/32] |= 1 << (pos%32)` (read-modify-write) |
| `MNVIC_vDisable_Peripheral_INT(pos)` | `ICER[...]` |
| `MNVIC_vSetPendingFlag` / `MNVIC_vCLRPendingFlag` | `ISPR` / `ICPR` |
| `MNVIC_vGetFlagStatus` | `IAPR` (which IRQ is currently active) |
| `MNVIC_SetGroupPriority(NVIC_Group_t)` | writes `SCB_AIRCR = VECTKEY | (group << 8)`; enum values 3–7 map to Cortex `PRIGROUP` (16/0, 8/2, 4/4, 2/8, 0/16) |
| `MNVIC_vSetPeripheralPriority(pos, group, sub)` | builds `IPRx` according to the selected grouping |

**Used by:** enabling IRQ position **6** (`EXTI0_IRQn`) for the IR receiver.

### 8.7 SYSCFG — system configuration / EXTI mux

`MSYSCFG_vSetEXTIPort(line, port)` is the piece of glue that connects a GPIO port to an EXTI line:

```c
u8 reg = line / 4;                 // EXTICR1..4
u8 shift = (line % 4) * 4;         // 4 bits per line
SYSCFG->EXTICRx[reg] &= ~(0xF << shift);
SYSCFG->EXTICRx[reg] |=  (port << shift);
```

Because of this, PA0 (not PB0 or PC0) becomes the source of `EXTI0`. Requires the SYSCFG clock (`APB2ENR` bit 14) — which `main()` enables.

### 8.8 DMA — DMA2 streams

* `MDMA2_vInit(stream)` — disables the stream, selects **memory-to-memory**, enables **source and destination address increment**, enables the transfer-complete interrupt.
* `MDMA2_vSetStreamCfg(stream, src, dst, PSIZE, MSIZE, blockSize, FIFO threshold)` — writes `PAR` (source), `M0AR` (destination), the `CR` size fields, `NDTR` (block size) and the `FCR` FIFO threshold.
* `MDMA2_vEnableStream(stream)` — sets `CR.EN`.
* `MDMA2_vCallBack(stream, fptr)` + `DMA2_Stream0_IRQHandler()` — invokes the callback and clears all five `LIFCR` flags for stream 0.

Status: complete driver, not yet used by the games (it is the natural upgrade path for pushing TFT frames and DAC audio samples without CPU copies).

### 8.9 USART — USART1 serial

* `MUSART_vInit()` — oversampling ×16, 8 data bits, no parity, 1 stop bit, **9600 baud** (`BRR = (162<<4)|13`), TX + RX enabled, USART enabled.
* Polled API: `MUSART_vSendData`, `MUSART_u8ReceiveData`, `MUSART_vSendString`, `MUSART_u8ptrReceiveString` (accumulates into `G_Buffer[50]` until `\r`/`\n`).
* Interrupt API: `MUSART_vEnable_TX/TC/RX_Interrupt`, `MUSART_vSendStringAsync(buffer, length)` (drives an index-based TX ring in the ISR) and `MUSART_vUSARTCallBack(usartNo, fptr)` for RX events.
* `USART1_IRQHandler` handles three sources: **TXE** (feed next byte, or switch to TC when finished), **TC** (clear + disable), **RXNE** (call the registered callback).

Status: complete driver, useful for a serial debug console / remote logging during the demo.

---

## 9. HAL Layer — Driver by Driver

### 9.1 TFT — ST7735 128×160 SPI display (`HAL/TFT`)

**Interface (`TFT_int.h`)**

```c
void HTFT_vInit(void);
void HTFT_vShowImage(const u16 A_u16ImgArr[], u16 A_u16ImgSize);
void HTFT_vSetXPos(u16 xStart, u16 xEnd);
void HTFT_vSetYPos(u16 yStart, u16 yEnd);
void HTFT_vFillBackgroundColor(u16 color);
void HTFT_vFillRectangle(u16 color);      // declared — see limitations §18
void HTFT_vDrawPixel(u16 x, u16 y, u16 color);
void HTFT_vDrawRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void HTFT_vDrawChar(u16 x, u16 y, char c, u16 color, u16 bg);
void HTFT_vDrawString(u16 x, u16 y, const char* s, u16 color, u16 bg);
```

**Pins owned by this driver:** `PA2` = RST, `PA1` = A0/DC (both declared as `GPIOx_PinConfig_t` file-scope structs). SCK/MOSI come from SPI1 (configured by `main()`), and `MSPI_vInit()` is called *inside* `HTFT_vInit()`.

**Low-level write primitives**

```c
Write_CMD(c)  -> A0 = 0 ; SPI(c)      // command
Write_Data(d) -> A0 = 1 ; SPI(d)      // parameter / pixel
```

**Initialisation sequence**

1. `Reset_Seq()` — the real ST7735 power-on timing: RST high → 100 ms → low → 1 µs → high → 100 µs → low → 100 µs → high → 120 ms.
2. SysTick configured **without** interrupt (`INT_DISABLE`) for pure busy-wait delays.
3. `MSPI_vInit()`.
4. `0x11` SLPOUT, wait 15 ms.
5. `0x3A 0x05` COLMOD → **16 bits/pixel (RGB565)**.
6. `0x29` DISPON.


**Drawing layer**

| Function | How it works | Notes / limits |
|----------|--------------|----------------|
| `HTFT_vSetXPos(a,b)` | `0x2A` + start/end as 2 bytes each | column window |
| `HTFT_vSetYPos(a,b)` | `0x2B` + start/end as 2 bytes each | row window |
| `HTFT_vFillBackgroundColor(c)` | sets the full 128×160 window, `0x2C`, then streams `128*160 = 20,480` pixels | ~40 KB over SPI per full redraw |
| `HTFT_vShowImage(img, size)` | fixed window `0..0x7F` × `0..0x9F`, then writes the `u16` array | used for splash/menu art |
| `HTFT_vDrawRect(x,y,w,h,c)` | window `x..x+w-1`, `y..y+h-1`, `0x2C`, `w*h` pixels | used for the XO grid lines |
| `HTFT_vDrawPixel(x,y,c)` | single-pixel window | bounds-checked (`x>=128 || y>=160` → return) |
| `HTFT_vDrawChar(x,y,ch,fg,bg)` | 5×7 bitmap font, 5 columns × 7 rows, draws fg pixel and bg pixel | **only ASCII `' '` (0x20) to `'Z'` (0x5A)** — lowercase is rejected, no wrapping |
| `HTFT_vDrawString(x,y,s,fg,bg)` | loops `DrawChar` with a **6-pixel advance** (5 + 1 spacing) | strings in the app use UPPERCASE |

**Font data:** `static const u8 Font5x7[][5]` — **59 entries** covering ASCII `' '` (0x20) to `'Z'` (0x5A), each 5 column-bytes, LSB = top row. Index = `char - ' '`. Example: `'A'` = `{0x7E, 0x11, 0x11, 0x11, 0x7E}`.

**Colour constants used by the app (RGB565):**

| Name | Value |
|------|-------|
| `ST7735_BLACK` | `0x0000` |
| `ST7735_WHITE` | `0xFFFF` |
| `ST7735_GREEN` | `0x07E0` |
| `ST7735_CYAN` | `0x07FF` |
| `ST7735_YELLOW` | `0xFFE0` |
| `ST7735_RED`, `ST7735_MAGENTA`, `ST7735_DARKGRAY` | used on the `main` branch UI (`0xF800`, `0xF81F`, `0x39E7`) |

### 9.2 IR — NEC infrared remote (`HAL/IR`)

**Interface**

```c
void HIR_vInit(void);
u8   HIR_u8GetKey(void);     // returns IR_KEY_NONE (0xFF) when no new key
```

**Key code map (`IR_int.h`)** — “Car MP3 Mini” NEC remote:

| Button | Code | Button | Code |
|--------|------|--------|------|
| POWER | `0x45` | 0 | `0x16` |
| MODE | `0x46` | 1 | `0x0C` |
| MUTE | `0x47` | 2 | `0x18` |
| PLAY/PAUSE | `0x44` | 3 | `0x5E` |
| PREV | `0x40` | 4 | `0x08` |
| NEXT | `0x43` | 5 | `0x1C` |
| EQ | `0x07` | 6 | `0x5A` |
| VOL− | `0x15` | 7 | `0x42` |
| VOL+ | `0x09` | 8 | `0x52` |
| RPT | `0x19` | 9 | `0x4A` |
| USD | `0x0D` | — | — |

**Logical aliases used by the games:** `IR_NAV_UP = VOL+`, `IR_NAV_DOWN = VOL−`, `IR_NAV_LEFT = PREV`, `IR_NAV_RIGHT = NEXT`, `IR_NAV_OK = PLAY/PAUSE`, `IR_NAV_EXIT = POWER`.

**Configuration (`IR_cfg.h`):** `IR_PORT = GPIO_PORTA`, `IR_PIN = GPIO_PIN0`, `IR_EXTI_LINE = EXTI_LINE0`, `IR_NVIC_POS = 6`.

**Init steps:** pin → EXTI (init, enable line 0, falling edge, callback = `HIR_vGetPulseTime`) → NVIC enable position 6 → SysTick with interrupt enabled at AHB/8.

**State machine (see [§13](#13-deep-dive-nec-ir-remote-decoding-interrupt-driven) for the full walk-through):** first falling edge arms a **15 ms single-shot** timer; every later edge records `elapsed/3.125` into a 50-slot timing buffer and re-arms a **4 ms single-shot**; when that 4 ms expires (frame finished), `HIR_vDecodeBits()` classifies 8 pulse widths into bits `17..24` and publishes the byte. `HIR_u8GetKey()` is a non-blocking "pop the decoded key" call for the main loop.

### 9.3 BUZZER — tone, SFX and melodies (`HAL/BUZZER`)

**Interface**

```c
void HBUZZER_vInit(const BUZZER_Config_t* cfg);
void HBUZZER_vPlayTone(const BUZZER_Config_t* cfg, u32 freqHz, u32 durationMs);
void HBUZZER_vPlaySFX(const BUZZER_Config_t* cfg, BUZZER_SFX_t sfx);
void HBUZZER_vPlayMelody(const BUZZER_Config_t* cfg, const Note_t* melody, u16 length);
```

* Config is a simple `{ Port, Pin }` struct; init makes the pin a push-pull output and drives it low.
* `PlayTone()` generates a **software square wave**: half-period `= 500000 / freq` µs, cycle count `= freq * duration / 1000`, toggling the GPIO with `MSYSTICK_vSetDelay_us()`. `freq == 0` is treated as a rest/pause.
* **Note table** included: `NOTE_C4 (262) … NOTE_C6 (1047)`, `NOTE_REST (0)`.
* **Predefined SFX:** `SFX_BUTTON_CLICK` = C6 for 20 ms; `SFX_VICTORY` = C5 → E5 → G5 → C6 (100/100/100/300 ms) — a rising arpeggio.

Status: compiled and ready; to enable game sounds, call `HBUZZER_vInit()` in `main()` and play `SFX_VICTORY` from `CheckWin()`.


### 9.4 DAC — 8-bit R-2R audio output (`HAL/DAC`)

```c
void HDAC_vInit(GPIOx_PinConfig_t *A_xPins, u8 A_u8PinsNo);
void HDAC_vSendSample(const u8* A_u8Ptr, u32 A_u32Index);
```

* `HDAC_vInit()` stores the port from `A_xPins[0]`, initialises **N** consecutive pins as push-pull outputs, then configures SysTick **with** interrupt.
* `HDAC_vSendSample(buf, index)` writes bit *i* of `buf[index]` onto pin *i* — i.e. it pushes one 8-bit audio sample onto an external **R-2R resistor ladder**, which is the classic “poor man's DAC”.
* The sample data lives in `APP/voice.h`: `const u8 voice_raw[79314]` — ~79 KB of raw 8-bit PCM (a voice/melody recording), played back by sweeping `index` and calling `HDAC_vSendSample()` at a fixed rate.

Status: implementation ready; the header (`DAC_int.h`) even relies on `GPIOx_PinConfig_t` from the GPIO driver. To enable audio, wire 8 pins + ladder + speaker and call the two functions from a periodic task.

### 9.5 S2P — Serial-to-Parallel 74HC595 shifter (`HAL/S2P`)

```c
void HS2P_vInit(S2P_Init_t* cfg);
void HS2P_vSendData(S2P_Init_t* cfg, u32 byte);   // LSB-first
```

* `S2P_Init_t` holds three pin pairs: **DataPort/DataPin**, **ShiftCLKPort/ShiftCLKPin**, **LatchCLKPort/LatchCLKPin**.
* `HS2P_vSendData()` loops `8 * S2P_NO_OF_SHIFT_REG` times (**2 registers = 16 bits**), putting bit *i* on the data pin (LSB first), pulsing the shift clock (`1 µs` high), then pulses the latch to copy the shift register to the outputs.
* This is the standard way to drive many outputs (LED matrix rows/columns) from 3 MCU pins.

### 9.6 LED MATRIX — 8×8 display via 2× 74HC595 (`HAL/LEDMATRIX`)

```c
void HLEDMATRIX_vInit(S2P_Init_t* cfg);
void HLEDMATRIX_vDisplayFrame(u8 frame[], u32 frameDelay);
```

* `HLEDMATRIX_vDisplayFrame()` implements **multiplexed scanning**: for each row `i` it packs
  `byte1 (bits 8-15) = ~(1 << i)` → active-LOW row select, and `byte0 (bits 0-7) = frame[i]` → the row's LED pattern,
  shifts the 16-bit word into the cascaded '595s, waits `SCAN_TIME` (**2.5 ms**, `LEDMATRIX_cfg.h`) and then writes `0xFF00` as an **anti-ghosting blank**.
* `frameDelay` = how many full 8-row scans to repeat a frame (frame rate control).

Status: driver complete, not yet wired in `main()`.

### 9.7 SEVEN SEGMENT (`HAL/SEVENSEGMENT`)

```c
void HSEVSEG_vInit(const SevenSegment_t* cfg);                  // { Port, StartPin, DisplayType }
void HSEVSEG_vDisplayNumber(const SevenSegment_t* cfg, u8 num); // 0..9
```

* Init configures **7 consecutive pins** (`StartPin + 0..6`) as push-pull outputs.
* The digit patterns are in `SEVENSEGMENT_prv.h` as `K_u8SevSegPatterns[10]` (common-cathode encoding, e.g. `0b00111111` = 0).
* For **common-anode** displays the driver inverts each bit at runtime (`DisplayType == SEVSEG_COMMON_ANODE`), so one driver supports both parts.
* Numbers above 9 are ignored (single digit).

---

## 10. RTOS Layer (custom scheduler) + Vendored FreeRTOS

### 10.1 `RTOS/` — hand-written periodic task scheduler

**Interface (`RTOS_int.h`)**

```c
void OS_vStart(void);
u8 RTOS_vCreateTask(void (*TaskFunction)(void), u32 Periodicity, u8 Priority);
void RTOS_vScheduler(void);
u8 RTOS_DeleteTask(u8 Priority);
u8 RTOS_ResumeTask(u8 Priority);
u8 RTOS_SuspendTask(u8 Priority);
```

**Design**

| Element | Detail |
|---------|--------|
| Task table | `Task_t SystemTasks[MAX_SYSTASK_SIZE]` with `MAX_SYSTASK_SIZE = 10` (`RTOS_cfg.h`) |
| Task record | `{ void (*TaskFunction)(void); u32 Periodicity; TASK_States_t state; }` |
| States | `READY`, `RUNNING`, `SUSPENDED` (`RTOS_prv.h`) |
| Time base | SysTick configured with interrupt, `MSYSTICK_vSetIntervalMulti(TICKTIME, RTOS_vScheduler)`, `TICKTIME = 100` → **tick every 100 ms** |
| Scheduling | each tick, every READY task counts down its own `TimingArray[i]`; when it hits 0 the task function runs and its counter reloads to `Periodicity - 1` |
| Priority | literally the array index — **priority 0 runs first**, and a duplicate priority is refused |
| Errors | `1 = RESERVED_PRIORITY_ERROR` (slot already used), `2 = OUT_OF_SYSTASK_RANGE_ERROR` (index ≥ 10) |
| Deletion | sets the slot's function pointer back to `NULL`, state `SUSPENDED`, counters to 0 |

**Pattern to use it**

```c
void MyTask(void);
OS_vStart();                          // start SysTick + attach scheduler
RTOS_vCreateTask(MyTask, 5, 0);       // run every 5 ticks (500 ms), highest priority
while (1) { }                         // main becomes the idle loop
```

Status: works, and it is the intended home for the future game loop (input polling, animation, audio). The current `main.c` runs a plain super-loop instead.

### 10.2 `FreeRTOS/` — vendored kernel (not used by the app)

Full **FreeRTOS V202112.00** sources are committed (`tasks.c`, `queue.c`, `timers.c`, `event_groups.c`, `stream_buffer.c`, `heap_4.c`, `port.c`, CMSIS-RTOS-independent headers) together with a `FreeRTOSConfig.h` that is already tuned for this board:

| Setting | Value |
|---------|-------|
| `configCPU_CLOCK_HZ` | `25 000 000` (matches HSE) |
| `configTICK_RATE_HZ` | `1000` → **1 ms tick** |
| `configUSE_PREEMPTION` | 1 |
| `configMAX_PRIORITIES` | 5 |
| `configMINIMAL_STACK_SIZE` | 128 words |
| `configTOTAL_HEAP_SIZE` | `20 * 1024` (heap_4) |
| `configSUPPORT_DYNAMIC_ALLOCATION` | 1 (`xTaskCreate`) |
| `configUSE_MUTEXES` / `configUSE_COUNTING_SEMAPHORES` / `configUSE_TIMERS` | 1 / 1 / 1 |
| Priority bits | `configPRIO_BITS = 4`, kernel = 15, max syscall = 5 |
| Handler mapping | `vPortSVCHandler → SVC_Handler`, `xPortPendSVHandler → PendSV_Handler`, `xPortSysTickHandler → SysTick_Handler` |

⚠️ Note: FreeRTOS **also** maps `SysTick_Handler`, which collides with `MCAL/SYSTICK/SYSTICK_prg.c`. If both are ever enabled together, only one `SysTick_Handler` may be compiled — a clean integration must route the tick through the FreeRTOS port. For that reason the application currently stays on the custom RTOS/super-loop.


---

## 11. APP Layer — State Machine & Games

### 11.1 `main.c` — system init and the top-level state machine

**Init sequence (exact order)**

```c
MRCC_vInit();                                        // 1. HSE 25 MHz, SYSCLK = HSE
MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);               //    GPIOA clock
MRCC_vEnableCLK(RCC_APB2, 14);                       //    SYSCFG clock (EXTI mux)
MRCC_vEnableCLK(RCC_APB2, 12);                       //    SPI1 clock

GPIOx_PinConfig_t MOSI = { PORTA, PIN7, MODE_ALF, AltFunc = AF5 };
MGPIO_vPinInit(&MOSI);                               // 2. SPI pins
GPIOx_PinConfig_t SCK  = { PORTA, PIN5, MODE_ALF, AltFunc = AF5 };
MGPIO_vPinInit(&SCK);

HTFT_vInit();                                        // 3. display (SPI1 + reset + ST7735 cmds)
HIR_vInit();                                         //    IR (EXTI0 + NVIC + SysTick)

HTFT_vFillBackgroundColor(ST7735_BLACK);             // 4. clear screen
DrawMenu();                                          //    draw the menu
while (1) { ... }                                    // 5. super-loop: poll IR, dispatch
```

**State machine**

```c
typedef enum { STATE_MAIN_MENU, STATE_XO_GAME } SystemState_t;
static SystemState_t CurrentState = STATE_MAIN_MENU;
static u8 MenuSelection = 0;             // 0 = "1. XO GAME", 1 = "2. FUTURE GAME"
```

| State | Event | Action |
|-------|-------|--------|
| `STATE_MAIN_MENU` | UP/DOWN or `2`/`8` | toggle `MenuSelection` (`^= 1`) and redraw the menu |
| `STATE_MAIN_MENU` | OK / `1` / `5` with `MenuSelection == 0` | switch to `STATE_XO_GAME` and call `XO_vInit()` |
| `STATE_MAIN_MENU` | OK with `MenuSelection == 1` | nothing (reserved for the next game) |
| `STATE_XO_GAME` | any key | forward to `XO_u8HandleInput(key)` |
| `STATE_XO_GAME` | `XO_u8HandleInput` returned `XO_STATE_EXIT` | back to `STATE_MAIN_MENU`, clear screen, `DrawMenu()` |

**Menu drawing (`DrawMenu()`)** — title `"ARCADE MENU"` at (20,20) in yellow; the two entries at (15,60) and (15,80); the selected entry is prefixed with `"> "` and drawn in **green**, the unselected in white with a `"  "` prefix (same text width, so nothing shifts).

**Input polling design:** the loop is *fully event-driven on a non-blocking pop* — `HIR_u8GetKey()` returns `IR_KEY_NONE` when nothing new has been decoded, so the CPU only redraws when a real key arrives. No key repeat/delay handling is implemented (see §18).

### 11.2 `XO.c` / `XO_int.h` — Tic-Tac-Toe module

**Interface**

```c
#define XO_STATE_CONTINUE  1
#define XO_STATE_EXIT      0
void XO_vInit(void);
u8   XO_u8HandleInput(u8 A_u8Key);
```

**Internal state**

```c
static char Board[3][3];                  // ' ', 'X' or 'O'
static u8   CursorX, CursorY;             // 0..2
static char CurrentPlayer;                // starts 'X'
static u8   GameActive;                   // 0 after a win
```

**Flow:** `XO_vInit()` clears the board, resets the cursor to (0,0), sets `'X'` to move, clears the screen and draws the grid + all cells. `XO_u8HandleInput(key)` is called once per key press and returns `XO_STATE_EXIT` (on POWER/MODE) or `XO_STATE_CONTINUE`. Full logic walk-through in [§14](#14-deep-dive-the-xo-game-logic).

### 11.3 `voice.h` — audio asset

`const u8 voice_raw[79314]` — a 79,314-byte 8-bit PCM sample (the largest single file in the repo). It is the playback source for the `HAL/DAC` driver and currently costs nothing at runtime because nothing references it yet (the linker will drop it unless it is used).

---

## 12. Deep-Dive: How the TFT Pixel Pipeline Works

**Goal:** show that displaying one character is a well-understood chain of register writes — a great slide for the presentation.

```
HTFT_vDrawString(20, 20, "ARCADE MENU", YELLOW, BLACK)
        │
        ├─ for each character  -> HTFT_vDrawChar(x, y, c, fg, bg)   ; x advances by 6
        │        │
        │        ├─ fontIdx = c - ' '                    ; 5x7 bitmap lookup
        │        └─ for col 0..4, row 0..6:
        │                HTFT_vDrawPixel(x+col, y+row, bit ? fg : bg)
        │                     │
        │                     ├─ bounds check  (x >= 128 || y >= 160 -> return)
        │                     ├─ HTFT_vSetXPos(x,x)  ->  CMD 0x2A + 4 data bytes
        │                     ├─ HTFT_vSetYPos(y,y)  ->  CMD 0x2B + 4 data bytes
        │                     └─ CMD 0x2C (memory write)
        │                        Write_Data(color >> 8); Write_Data(color & 0xFF)
        │                             │
        │                             ├─ A0/DC pin = 1  (data)
        │                             └─ MSPI_vTransceive(byte)
        │                                   ├─ wait SR.TXE, write SPI1->DR
        │                                   └─ wait SR.RXNE, read  SPI1->DR
        │                                        └─ SCK/MOSI on PA5/PA7 (AF5) shift the bits out
        │
        └─ ST7735 stores the pixel into its internal GRAM at (x,y)
```

**Performance reality check (useful Q&A answer):** every single pixel costs **13 blocking SPI byte transfers** — `0x2A` + 4 address bytes, `0x2B` + 4 address bytes, `0x2C`, then the 2 colour bytes. So one character (35 pixels) ≈ **455 transfers**, and a full-screen fill (20,480 pixels) ≈ **41,000 transfers** (`0x2A`/`0x2B`/`0x2C` are sent once, then 40,960 colour bytes). That is exactly why the application repaints **only the cells/menu lines that changed** instead of the whole screen, and why DMA + a dirty-rectangle strategy are the planned optimisation.


---

## 13. Deep-Dive: NEC IR Remote Decoding (interrupt-driven)

### 13.1 The NEC frame on the wire

| Element | Timing |
|---------|--------|
| AGC / leader burst | 9 ms carrier + 4.5 ms space |
| Logical **0** | 562 µs carrier + 562 µs space → **~1125 µs between falling edges** |
| Logical **1** | 562 µs carrier + 1687 µs space → **~2250 µs between falling edges** |
| Frame content | 8-bit address, 8-bit inverted address, 8-bit command, 8-bit inverted command |
| Stop bit | final 562 µs burst |
| Repeat | same frame repeated every ~108 ms while a key is held (not handled here) |

### 13.2 How our driver measures it

The receiver output idles high and goes low during each burst, so **every falling edge is an event**. `IR_prg.c` uses EXTI0 + SysTick as a stopwatch:

```c
static void HIR_vGetPulseTime(void)                 // EXTI0 IRQ
{
    if (G_u8StartingFlag == 0) {                    // first edge of a frame
        G_u8StartingFlag = 1;
        MSYSTICK_vSetIntervalSingle(15, HIR_vDecodeBits);   // skip 9 ms burst + 4.5 ms space
    } else {
        G_u32Arr[G_u8Counter++] = MSYSTICK_u32GetElapsedTime_SingleShot() / 3.125; // ticks -> µs
        MSYSTICK_vSetIntervalSingle(4, HIR_vDecodeBits);     // frame timed out -> decode
    }
}
```

* **`/3.125`** converts SysTick counts into microseconds (3.125 counts per µs at AHB/8 = 3.125 MHz).
* **15 ms single-shot** marks “frame starting”; when it expires we know the leader burst ended.
* **4 ms single-shot** is re-armed on every edge — if 4 ms pass with no edge, the frame is finished and `HIR_vDecodeBits()` runs.
* `G_u32Arr[50]` stores up to 50 widths, `G_u8Counter` is the write index, `G_u8StartingFlag` arms/disarms a frame.

### 13.3 Bit classification

```c
for (i = 0; i < 8; i++) {
    if      (G_u32Arr[17+i] >= 1000 && G_u32Arr[17+i] <= 1250) CLR_BIT(temp, i);  // '0'
    else if (G_u32Arr[17+i] >= 2000 && G_u32Arr[17+i] <= 2450) SET_BIT(temp, i);  // '1'
}
G_u8DecodedValue = temp;   G_u8NewKeyReady = 1;   // publish the key to the application
G_u8StartingFlag = 0;      G_u8Counter = 0;       // reset for the next frame
for (i = 0; i < 50; i++) G_u32Arr[i] = 0;         // clear the timing buffer
```

The two windows (1000–1250 µs and 2000–2450 µs) bracket the theoretical 1125 µs / 2250 µs widths with roughly ±10 % tolerance, and buffer slots `17..24` are the 8 bits of the **command byte** — exactly the codes declared in `IR_int.h`.

### 13.4 Consumer side

```c
u8 Key = HIR_u8GetKey();          // non-blocking: returns IR_KEY_NONE if nothing new
if (Key != IR_KEY_NONE) { ... }   // main.c dispatches to the menu or the active game
```

**Robustness features actually implemented:** pending-flag clear at the top of `EXTI0_IRQHandler` (commit `317ecda`) prevents a stuck interrupt; `volatile` on every ISR-shared variable; the key is published only after all 8 bits are classified; the ISR performs **no drawing** — rendering happens in the main loop.


---

## 14. Deep-Dive: The XO Game Logic

### 14.1 Board rendering

```c
static void DrawXOBoard(void)
{
    HTFT_vDrawRect(50, 20, 2, 120, WHITE);   // vertical line 1
    HTFT_vDrawRect(86, 20, 2, 120, WHITE);   // vertical line 2
    HTFT_vDrawRect(14, 60, 100, 2, WHITE);   // horizontal line 1
    HTFT_vDrawRect(14, 100, 100, 2, WHITE);  // horizontal line 2
    for (r = 0..2) for (c = 0..2) DrawCell(r, c);
}
```

### 14.2 Cell rendering — the "single cell repaint" trick

```c
static void DrawCell(u8 r, u8 c)
{
    u16 x = 18 + c * 36;                    // column pitch = 36 px
    u16 y = 24 + r * 40;                    // row pitch    = 40 px
    u16 color = (r == CursorY && c == CursorX) ? YELLOW : CYAN;   // cursor highlight
    char str[2] = { Board[r][c], '\0' };
    if (str[0] == ' ') str[0] = (cursor here) ? '.' : ' ';        // empty cell = '.' or blank
    HTFT_vDrawString(x + 12, y + 12, str, color, BLACK);          // centred glyph
}
```

When the cursor moves, only the **two affected cells** (previous and new position) are repainted — the rest of the screen is never re-sent. This is what keeps the UI responsive over a blocking SPI link.

### 14.3 Input handling (`XO_u8HandleInput`)

| Key(s) | Effect |
|--------|--------|
| `IR_NAV_EXIT` (POWER) or `IR_KEY_MODE` | return `XO_STATE_EXIT` (leave the game) |
| UP / `2` | `CursorY--` (blocked at 0) |
| DOWN / `8` | `CursorY++` (blocked at 2) |
| LEFT / `4` | `CursorX--` (blocked at 0) |
| RIGHT / `6` | `CursorX++` (blocked at 2) |
| OK (PLAY/PAUSE) on an **empty** cell | place `CurrentPlayer`, then `CheckWin()` |
| any other key | `ProcessNumpadDirectSelection(key)` → keys `1..9` place a mark directly |

**Numpad → cell mapping (phone-keypad rule):**

| Key | Cell | Key | Cell | Key | Cell |
|-----|------|-----|------|-----|------|
| 1 | (0,0) | 2 | (0,1) | 3 | (0,2) |
| 4 | (1,0) | 5 | (1,1) | 6 | (1,2) |
| 7 | (2,0) | 8 | (2,1) | 9 | (2,2) |

Direct selection also **moves the cursor** to that cell (repainting old and new cells) and refuses moves into occupied cells.

### 14.4 Win detection

```c
for (i = 0; i < 3; i++) {
    if (Board[i][0]!=' ' && Board[i][0]==Board[i][1] && Board[i][1]==Board[i][2]) win = Board[i][0]; // rows
    if (Board[0][i]!=' ' && Board[0][i]==Board[1][i] && Board[1][i]==Board[2][i]) win = Board[0][i]; // columns
}
if (Board[0][0]!=' ' && Board[0][0]==Board[1][1] && Board[1][1]==Board[2][2]) win = Board[0][0];     // main diagonal
if (Board[0][2]!=' ' && Board[0][2]==Board[1][1] && Board[1][1]==Board[2][0]) win = Board[0][2];     // anti-diagonal

if (win != ' ') { GameActive = 0;
    HTFT_vDrawString(20, 145, (win=='X') ? "X WINS!" : "O WINS!", GREEN, BLACK); }
```

* The `!= ' '` guards stop an all-empty line from "winning".
* `GameActive = 0` freezes the board; the turn is swapped only `if (GameActive)` after each move, so the winner never plays again.
* The result is printed below the grid at (20,145) in green.

### 14.5 Turn structure per key press

```
key arrives
   ├─ movement key -> cursor moves (old cell + new cell repainted)
   └─ placement    -> Board[r][c] = CurrentPlayer
                      DrawCell(r,c)                       (repaint that cell)
                      CheckWin()                          (may end the game)
                      if (still active) CurrentPlayer = (CurrentPlayer=='X') ? 'O' : 'X'
```


---

## 15. Build, Flash & Run

### 15.1 What the project is (build-system wise)

* The firmware is an **Eclipse CDT managed-build project** named `Drivers`, located at `WorkSpace/Drivers/`.
  * `.project` → nature `org.eclipse.cdt.managedbuilder.core.managedBuildNature` (+ C/C++ natures).
  * `.cproject` → configuration **Debug**, toolchain **Cross ARM GCC** (`arm-none-eabi-`), optimization = **debug**, `-ffunction-sections/-fdata-sections`, debug level max, `createflash = true` (objcopy → `.hex`).
  * Preprocessor define: **`STM32F401xC`**; include paths include `../system/include`, `../system/include/cmsis`, `../system/include/cortexm`, `../system/include/diag`, `../system/include/stm32f4-hal`.
  * Linker scripts: `ldscripts/mem.ld` (memory layout), `sections.ld` (section placement), `libs.ld`.
* ⚠️ There is **no committed CMakeLists.txt or Makefile**. The `build/`, `build-arm/`, `build-mingw/`, `build-pc/` folders are stale CMake/Ninja outputs from an unrelated local Pac-Man checkout (their `CMakeCache.txt` points to `D:/Pac-Man (C language)/…`) and are ignored by `.gitignore` — do not use them.

### 15.2 Build steps (IDE — recommended)

1. Install **STM32CubeIDE** (Eclipse + GNU ARM toolchain + ST-LINK driver). *Eclipse + the GNU ARM Eclipse plug-in works too — the project files were generated by plug-in id `ilg.gnuarmeclipse`.*
2. `File ▸ Import ▸ General ▸ Existing Projects into Workspace` and select the folder `GameHub/WorkSpace/Drivers`.
3. Ensure the build configuration is **Debug** (`Project ▸ Build Configurations ▸ Set Active ▸ Debug`).
4. `Project ▸ Build Project` (Ctrl+B) → produces `Debug/Drivers.elf` and the flash image.
5. Connect an **ST-LINK** (SWD: SWCLK/SWDIO/GND/3V3), then `Run ▸ Debug` (or `Run`) to flash and start.

> The GPIO driver deliberately refuses PA13/PA14/PA15 (SWD) — that guarantee is what makes re-flashing safe even if the firmware misbehaves.

### 15.3 Build steps (command line, if the toolchain is on PATH)

```bat
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -DSTM32F401xC ^
  -I WorkSpace\Drivers\system\include -I WorkSpace\Drivers\system\include\cmsis ^
  -I WorkSpace\Drivers\system\include\cortexm ^
  -I WorkSpace\Drivers\src\MCAL\RCC -I WorkSpace\Drivers\src\MCAL\GPIO ... ^
  -c WorkSpace\Drivers\src\**\*.c
arm-none-eabi-gcc -T WorkSpace\Drivers\ldscripts\mem.ld  ^
                    -T WorkSpace\Drivers\ldscripts\sections.ld ^
                    -T WorkSpace\Drivers\ldscripts\libs.ld -nostartfiles -o Drivers.elf *.o
arm-none-eabi-objcopy -O ihex Drivers.elf Drivers.hex
```

*(You must list every `src/**` folder on the include path — the drivers include each other with relative paths such as `../../MCAL/GPIO/GPIO_int.h`.)*

### 15.4 Pre-demo hardware checklist

- [ ] TFT wired: VCC/GND, SCK→PA5, SDA→PA7, RST→PA2, DC/A0→PA1, CS→GND, BL→3V3 (with series resistor).
- [ ] IR receiver OUT→PA0, VCC 3V3, GND; remote battery OK and the remote is **NEC / Car MP3 Mini** type.
- [ ] Common ground between board and all modules.
- [ ] Board flashed and power LED on; screen shows `ARCADE MENU`.

---

## 16. Live Demo Script (presentation flow)

**Suggested 4-minute live sequence**

| Step | Presenter action | On screen / expected result | Talking point |
|------|------------------|-----------------------------|---------------|
| 1 | Power the board | Screen clears to black, `ARCADE MENU` in yellow, `> 1. XO GAME` in green, `  2. FUTURE GAME` in white | “Init sequence: clocks → GPIO AF → SPI → TFT reset → ST7735 commands → IR + EXTI0” |
| 2 | Press **VOL− / 8** then **VOL+ / 2** | The green highlight jumps between the two menu entries | “The main loop is a non-blocking key pop; only the changed text is repainted” |
| 3 | Press **PLAY/PAUSE** (OK) on `1. XO GAME` | Screen redraws: 3×3 white grid, yellow `.` cursor at the top-left cell | “State machine transition MANU → XO, and `XO_vInit()` resets the board array” |
| 4 | Use **VOL+/VOL−/PREV/NEXT** | Yellow cursor moves cell by cell and the old cell turns cyan | “Two cells repainted per move — we minimise SPI traffic” |
| 5 | Press **1**, **5**, **9** on the numpad (or OK) | `X` / `O` marks appear directly in the selected cells, alternating players | “Two input idioms: cursor+confirm, and direct numpad mapping” |
| 6 | Let X complete a line | `X WINS!` printed in green under the board; board freezes | “Win check: 3 rows + 3 columns + 2 diagonals, with empty-cell guards” |
| 7 | Press **POWER** | Back to `ARCADE MENU` | “`XO_STATE_EXIT` returns control to the menu state” |
| 8 | (Optional) Show the code on screen | `main.c` state machine, `IR_prg.c` decode loop, `TFT_prg.c` font renderer | “Three layers, one convention, 100 % our own register-level drivers” |

**Slide order that matches this README:** Hardware & pin map → Architecture → MCAL → HAL → IR deep-dive → XO logic → Demo → Limitations/Roadmap.


---

## 17. Anticipated Q&A (examiner questions)

**Q1. Why write your own drivers instead of using the ST HAL / CubeMX?**
Because understanding the peripheral is the point of the project: every driver sets the exact bits from the reference manual (`MODER`, `CR1`, `IMR`, `RTSR`, `BRR`, …) and documents them in `*_prv.h`. It also produces a tiny, fully predictable binary — no HAL layers, no hidden state machines, and no CubeMX `.ioc` regeneration risk.

**Q2. FreeRTOS is in the repo — why is the app using your own scheduler?**
Because we had to build the scheduler to learn how one works, and our custom RTOS (task table + per-task periodicity + priorities on a SysTick tick) is enough for a game loop. FreeRTOS is vendored and configured (`configCPU_CLOCK_HZ = 25 MHz`, 1 ms tick, heap_4, 20 KB heap, `configPRIO_BITS = 4`) as the next step. Note the conflict we identified: FreeRTOS defines `xPortSysTickHandler = SysTick_Handler`, and `MCAL/SYSTICK/SYSTICK_prg.c` already defines `SysTick_Handler` — so integrating it requires routing our tick through the FreeRTOS port.

**Q3. How do you avoid missing IR edges?**
The EXTI0 ISR clears its pending flag **first** (`MEXTI_vClearPendingFlag(EXTI_LINE0)` — commit `317ecda`), before running the callback, so a new edge can be latched while the handler works. The handler itself is tiny (read SysTick elapsed, store, re-arm a 4 ms single-shot), and all shared variables are `volatile`. There is no drawing or string processing inside the ISR.

**Q4. How is the display updated fast enough with blocking SPI?**
We never repaint the whole screen during gameplay. Moving the XO cursor repaints 2 cells (2 glyphs = 70 pixels). The full-screen fill is used only on state transitions. The next optimisation is the DMA driver plus a partial/dirty-rectangle refresh.

**Q5. What happens if the master clock changes?**
Two places must be updated: the `3125` / `3.125` tick constants in `SYSTICK_prg.c` (they assume AHB/8 = 3.125 MHz) and `configCPU_CLOCK_HZ` in `FreeRTOSConfig.h`. Everything else (TFT timings, IR thresholds in µs) is clock-independent because it goes through the SysTick driver.

**Q6. How much memory does this use?**
State is tiny: the XO board is 9 bytes, the IR timing buffer is 50 × `u32` = 200 bytes, the task table is 10 records. The big item is the audio asset `voice.h` — 79,314 bytes of 8-bit PCM that would live in flash once it is referenced (F401xC parts in this family have 128–256 KB flash and 64 KB SRAM, hence the `configTOTAL_HEAP_SIZE = 20 KB` choice for FreeRTOS).

**Q7. Why does the GPIO driver refuse PA13/14/15 and PB3/4?**
Those are the SWD/JTAG debug pins. Silently ignoring writes to them means no firmware bug can disable the debug port, so the board can always be re-flashed.

**Q8. What is the difference between `MGPIO_vSetPinValue` and `MGPIO_vSetPinValueAtomic`?**
The first does a read-modify-write on `ODR` (safe in one thread, but an interrupt that also writes `ODR` can corrupt the byte); the second writes `BSRR` — a single 32-bit write that sets (bits 0-15) or resets (bits 16-31) only the target pin, with no read step. Bit patterns (buzzer, LED matrix, DAC) should use `BSRR`.

**Q9. How is the remote proven to be NEC?**
Empirically and structurally: the decoder measures inter-edge widths and classifies them as ~1125 µs (0) or ~2250 µs (1), which is the NEC definition; the command codes in `IR_int.h` match the documented Car MP3 Mini NEC map. The driver does **not** validate the inverted command byte or the address bytes, and it ignores NEC repeat frames.

**Q10. What is "2. FUTURE GAME" in the menu?**
A deliberate placeholder: the menu already draws two rows and routes the selection, so adding a game means adding a state, an entry and a module with `X_vInit()` / `X_u8HandleInput()` — exactly how Snake and Pac-Man are implemented on the other branches.

**Q11. How does the buzzer / DAC produce sound?**
The buzzer is a software square wave (GPIO toggling with µs delays); the DAC writes 8 bits per sample onto an R-2R ladder. A PWM/DMA-based audio path would be the proper next step to avoid busy-wait CPU usage.

**Q12. Why do some drivers exist but are unused in `main()`?**
They were developed as part of the driver-stack milestone and are kept ready (and documented in §2/§9) so the games can consume them later. Unused code is dropped by the linker, so it costs nothing today.


---

## 18. Known Limitations & Bugs (honest list)

> Presenting these openly is a strength: each item is a known, understood trade-off with a named next step.

| # | Area | Limitation | Impact / next step |
|---|------|------------|--------------------|
| 1 | XO game | **No draw/tie detection** — if the board fills with no winner, nothing is displayed | Add a `moves == 9` counter → print `"DRAW!"` |
| 2 | XO game | After a win there is no "play again"; only POWER exits | Add a RESET key or auto-restart after 2 s |
| 3 | TFT | `HTFT_vFillRectangle()` is **declared in `TFT_int.h` but not implemented** in `TFT_prg.c` | Implement it (mirror `vDrawRect` over the whole screen) or remove the prototype — calling it now fails at link time |
| 4 | TFT font | Only ASCII `' '`–`'Z'` (**59 glyphs**, indices 0x20–0x5A); lowercase is rejected, no wrapping, fixed 6 px advance | Extend `Font5x7` with a lowercase table |
| 5 | IR | `G_u8Counter` is **not bounds-checked** against the 50-entry buffer, so a long/unknown pulse train (different remote, sunlight, noisy sensor) can overrun it; address and inverted-command bytes are not validated; NEC **repeat frames are ignored** (no key auto-repeat) | Add `if (G_u8Counter < 50)`, verify address + inverted bytes, handle repeat frames |
| 6 | Timing | All delays are **busy-wait** loops and SPI transfers are blocking, so keys are only polled between redraws | Move to SysTick/RTOS tasks + DMA |
| 7 | Menu | Entry `2. FUTURE GAME` is a placeholder and does nothing when confirmed | Point it at Snake / Pac-Man |
| 8 | USART | API mismatch: the header declares `MUSART_vSendStringAsync()` while the implementation defines `MUSART_vSendStringAsynch()`; `MUSART_vReceive_synch()` has no prototype | Align the names (the async API is unusable exactly as declared) |
| 9 | RTOS | Task bodies execute **inside the SysTick ISR**, so they share its stack and any long task delays the tick; the `RUNNING` state is never set; periodicity is in ticks (100 ms by default) | Move to a real context switch or adopt FreeRTOS |
| 10 | FreeRTOS | Both `FreeRTOSConfig.h` (`xPortSysTickHandler`) and `SYSTICK_prg.c` define `SysTick_Handler` | Route the tick through a single owner before enabling both |
| 11 | Audio | DAC playback is a blocking sample sweep with no hardware-paced rate; the buzzer uses CPU delay loops | Use a timer interrupt / DMA for the sample rate |
| 12 | Assets | `APP/voice.h` is a 79 KB / 6,612-line header with a `const` array | Move to `voice.c` + `extern` declaration |
| 13 | Build | The `build*/` folders belong to a different local project and confuse newcomers; no Makefile/CMake is committed | Delete them; optionally add CMake + toolchain file |
| 14 | RCC | PLL is unimplemented; `MRCC_vInit()` raises `#error "invalid option"` for unsupported clock choices | Implement PLL (needed for higher clocks and SPI speeds) |
| 15 | Tree size | The large `system/src/stm32f4-hal/*` tree is part of the project even though our drivers never call it | Exclude it to cut build time (`.cproject` already excludes many unused HAL sources) |
| 16 | GPIO | Only ports A, B and C are supported | Extend the memory-map table for larger packages |
| 17 | Input | No debouncing, no long-press/key-repeat behaviour; every decoded frame is treated as a fresh press | Add a simple time-based repeat in `HIR_u8GetKey()` |


---

## 19. Roadmap & Branch Map

| Branch | Tip commit | What is on it |
|--------|-----------|---------------|
| **`LetsTry`** *(current checkout)* | `317ecda` — *"Added MEXTI_vClearPendingFlag to execute in IRQ"* | The driver stack (MCAL + HAL), the menu, the XO game, the IR driver with an explicit pending-flag clear, the BUZZER driver and the `voice.h` audio asset. The local `main` branch points at the same commit. |
| `origin/main` | `74e6390` — *"Final BUILD for now"* (4 commits ahead) | Multi-game system: **XO + SNAKE + PACMAN modules**, a new UI, a **high-score system**, **parental control** (PIN lock + play-time limit), and **flash persistence** (`FLASH_SAVE_ADDR 0x08020000`, default PIN `1234`), plus new `MCAL/FMI` (flash memory interface) and `MCAL/TIM2` drivers. `BUZZER`/`voice.h` are not present on that branch. |
| `origin/PacMan` | `e6a74b7` — *"uploaded pacman"* | Flattened layout (`src/APP/pacman_tft.c`, `src/HAL/ST7735`, `src/MCAL/*`) with a **CMake build** (`build-pc`, `build-arm`, `cmake/arm-none-eabi.cmake`) and **unit tests** (`tests/test_movement.c`, `tests/test_turn_buffer.c`). This branch is where the stale `build*/` folders in the working tree came from. |
| `origin/snake` | `a792119` — *"snakeGameWith_instructionMenu"* | Snake game with an instruction menu, a NEC IR driver, an `ESP8266` (Wi-Fi) driver and an **STM32F1 (std-periph)** base. |

**Suggested next milestones (in order)**

1. Merge the multi-game menu (Snake + Pac-Man) into the `LetsTry` driver stack, keeping a single `SystemState_t`.
2. Add draw detection and "play again" to XO, and wire `HBUZZER_vPlaySFX()` into placement / win / cursor events.
3. Use `MDMA2` for TFT frame streaming and for DAC sample playback — removes the blocking transfers.
4. Port the game loop onto FreeRTOS (one `SysTick_Handler` owner) or extend the custom RTOS so tasks run outside the ISR.
5. Expose `MUSART_*` as a 9600 8N1 serial debug console for on-stage diagnostics.
6. Implement the PLL in RCC, then raise the SPI clock for faster redraws.

---

## 20. Glossary

| Term | Meaning in this project |
|------|-------------------------|
| **LIB / MCAL / HAL / RTOS / APP** | The five layers of `WorkSpace/Drivers/src` |
| **MCAL** | Microcontroller Abstraction Layer — drivers that talk directly to silicon registers |
| **HAL** | Hardware Abstraction Layer — board-level devices (TFT, IR, buzzer, DAC, matrix, 7-seg) |
| **RCC** | Reset & Clock Control — system clock selection + per-peripheral clock gating |
| **SysTick** | Cortex-M 24-bit down counter, used here for delays, stopwatch and callbacks |
| **EXTI** | External interrupt controller (edge detection on GPIO lines) |
| **NVIC** | Nested Vectored Interrupt Controller (enable, pending, priorities) |
| **SYSCFG** | System configuration block that muxes a GPIO port onto an EXTI line |
| **SPI** | Serial Peripheral Interface — full-duplex synchronous bus used for the TFT |
| **DMA** | Direct Memory Access — data movement without CPU involvement |
| **USART** | Universal synchronous/asynchronous receiver-transmitter (serial console) |
| **BSRR / ODR / IDR** | GPIO set-reset register / output data register / input data register |
| **NEC** | IR remote protocol decoded by `HAL/IR`: 9 ms AGC burst then 562 µs/1690 µs bit coding |
| **RGB565** | 16-bit colour format (5 red, 6 green, 5 blue) used by the ST7735 |
| **ST7735** | The 128×160 TFT controller driven over SPI |
| **GRAM** | The display controller's internal frame memory |
| **S2P / 74HC595** | Serial-to-parallel shift register used for the LED matrix |
| **R-2R ladder** | Resistor network that converts 8 GPIO bits into an analog audio level |
| **IRQ / ISR** | Interrupt request / interrupt service routine |
| **HSE / HSI / PLL** | High-speed external clock (crystal) / internal clock / phase-locked loop |
| **AHB / APB** | The two peripheral bus domains (APB2 hosts SPI1 and SYSCFG) |
| **SWD / JTAG** | Debug interfaces occupying PA13/PA14/PA15 (and PB3/PB4) |
| **PCM** | Raw digital audio samples — the format of `voice.h` |
| **AFRL / AFRH** | GPIO alternate-function registers for pins 0–7 and 8–15 |


---

## 21. Credits & Repository Notes

* **Project:** GameHub — ITI graduation project (the original one-line `README.md` said *“iti project”*).
* **Author credits recorded in the source file headers:** *Hager Adel* (RCC, GPIO, SYSTICK, SPI, EXTI, NVIC, SYSCFG, S2P) and *bigor* (TFT, DMA, BUZZER, PACMAN, SNAKE); several files still carry the placeholder `Author: ??` (USART, LEDMATRIX, SEVENSEGMENT).
* **Repository:** [github.com/Ali8allam/GameHub](https://github.com/Ali8allam/GameHub) — default branch `main`, active development branch `LetsTry`.
* **Tracked files:** 693 files in git; `.gitignore` deliberately tracks **only** `*.c`, `*.h` and `.gitignore` itself (`*` is ignored first, then `!*/`, `!*.c`, `!*.h` re-allow the sources) — build outputs, IDE metadata and binaries never enter the repository.
* **Reviewer / demo tip:** read the code in this order — `APP/main.c` (application flow) → `HAL/TFT/TFT_prg.c` (device-driver example) → `MCAL/GPIO/GPIO_prg.c` (pure register work) → `HAL/IR/IR_prg.c` (interrupt + timing) → `RTOS/RTOS_prg.c` (scheduling).

### One-slide summary (copy/paste for the last slide)

> **GameHub** — an IR-remote-controlled arcade console on an STM32F401xC (Cortex-M4 @ 25 MHz).
> **9 MCAL drivers + 7 HAL drivers written from scratch at register level**, an interrupt-timed NEC IR decoder,
> an ST7735 RGB565 graphics driver with a 5×7 font, a menu state machine and a complete XO game —
> plus a custom periodic scheduler and a vendored FreeRTOS for the next iteration.
> **Zero ST HAL calls in the application path.**

<!-- END OF README -->
