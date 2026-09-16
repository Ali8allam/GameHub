# Pac-Man in C

A Pac-Man game built step by step in C.

## Project layout

- `include/` - public headers
- `src/` - game source files
- `CMakeLists.txt` - portable build configuration

## Build and run with GCC on Windows

From the project root:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\pacman.exe
```

The current version is a playable console game with:

- WASD or arrow-key movement
- Pellet and power-pellet scoring
- Three lives and ghost collisions
- Ghost release, targeting, and tunnel wrapping
- A level-complete state when all pellets are collected

Press `q` or `Esc` to quit. The game logic is kept in `src/game.c`, separate from the Windows console renderer in `src/console.c`.

## STM32F103 with ST7735 TFT

The ARM target renders the game on a 128x160 ST7735 SPI display and runs Pac-Man automatically:

```powershell
cmake -S . -B build-arm -G Ninja -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/arm-none-eabi.cmake"
cmake --build build-arm --target stm32_pacman_tft
```

Default wiring:

| TFT signal | STM32F103 pin |
| --- | --- |
| SCK | PA5 |
| MOSI | PA7 |
| MISO | PA6 |
| CS | PA4 |
| DC | PB0 |
| RESET | PB1 |
| LED/backlight | PB10 |

Flash it through the connected ST-Link with OpenOCD:

```powershell
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build-arm/stm32_pacman_tft.bin verify reset exit 0x08000000"
```

The display must be an SPI ST7735 module using 3.3 V logic. Some modules use different
column/row offsets; if the image is shifted, adjust the address window in
`src/HAL/ST7735/ST7735_prg.c`.