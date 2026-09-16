cmake_minimum_required(VERSION 3.20)

project(pacman C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

add_executable(pacman
    src/game.c
    src/console.c
    src/main.c
)

target_include_directories(pacman PRIVATE include)

add_executable(pacman_movement_test
    src/game.c
    tests/test_movement.c
)

target_include_directories(pacman_movement_test PRIVATE include)
add_test(NAME pacman_movement_test COMMAND pacman_movement_test)

add_executable(pacman_turn_buffer_test
    src/game.c
    tests/test_turn_buffer.c
)

target_include_directories(pacman_turn_buffer_test PRIVATE include)
add_test(NAME pacman_turn_buffer_test COMMAND pacman_turn_buffer_test)

add_executable(stm32_led_test
    src/APP/led_test.c
    src/MCAL/RCC/RCC_prg.c
    src/MCAL/GPIO/GPIO_prg.c
)

target_include_directories(stm32_led_test PRIVATE src)

if(CMAKE_C_COMPILER MATCHES "arm-none-eabi")
    enable_language(ASM)

    add_executable(stm32_led_firmware
        src/APP/led_test.c
        src/MCAL/RCC/RCC_prg.c
        src/MCAL/GPIO/GPIO_prg.c
        src/startup_stm32f103.s
    )

    target_include_directories(stm32_led_firmware PRIVATE src)
    target_compile_options(stm32_led_firmware PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -ffunction-sections
        -fdata-sections
        -ffreestanding
    )
    target_link_options(stm32_led_firmware PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -nostdlib
        -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f103c8.ld
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/stm32_led_firmware.map
    )

    add_custom_command(TARGET stm32_led_firmware POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32_led_firmware> ${CMAKE_CURRENT_BINARY_DIR}/stm32_led_firmware.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32_led_firmware> ${CMAKE_CURRENT_BINARY_DIR}/stm32_led_firmware.bin
    )

    add_executable(stm32_pacman_tft
        src/APP/pacman_tft.c
        src/game.c
        src/MCAL/RCC/RCC_prg.c
        src/MCAL/GPIO/GPIO_prg.c
        src/MCAL/AFIO/AFIO_prg.c
        src/MCAL/EXTI/EXTI_prg.c
        src/MCAL/NVIC/NVIC_prg.c
        src/MCAL/SPI/SPI_prg.c
        src/MCAL/SYSTICK/SYSTICK_prg.c
        src/HAL/ST7735/ST7735_prg.c
        src/HAL/IR/IR_prg.c
        src/HAL/DAC/DAC_prg.c
        src/platform_runtime.c
        src/startup_stm32f103.s
    )

    target_include_directories(stm32_pacman_tft PRIVATE src include)
    target_compile_options(stm32_pacman_tft PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -ffunction-sections
        -fdata-sections
        -ffreestanding
    )
    target_link_options(stm32_pacman_tft PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -nostdlib
        -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f103c8.ld
        -lgcc
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/stm32_pacman_tft.map
    )

    add_custom_command(TARGET stm32_pacman_tft POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32_pacman_tft> ${CMAKE_CURRENT_BINARY_DIR}/stm32_pacman_tft.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32_pacman_tft> ${CMAKE_CURRENT_BINARY_DIR}/stm32_pacman_tft.bin
    )

    add_executable(stm32_tft_test
        src/APP/tft_test.c
        src/MCAL/RCC/RCC_prg.c
        src/MCAL/GPIO/GPIO_prg.c
        src/MCAL/SPI/SPI_prg.c
        src/MCAL/SYSTICK/SYSTICK_prg.c
        src/HAL/ST7735/ST7735_prg.c
        src/platform_runtime.c
        src/startup_stm32f103.s
    )

    target_include_directories(stm32_tft_test PRIVATE src)
    target_compile_options(stm32_tft_test PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -ffunction-sections
        -fdata-sections
        -ffreestanding
    )
    target_link_options(stm32_tft_test PRIVATE
        -mcpu=cortex-m3
        -mthumb
        -nostdlib
        -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f103c8.ld
        -lgcc
        -Wl,--gc-sections
    )

    add_custom_command(TARGET stm32_tft_test POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32_tft_test> ${CMAKE_CURRENT_BINARY_DIR}/stm32_tft_test.bin
    )
endif()

if(MSVC)
    target_compile_options(pacman PRIVATE /W4)
    target_compile_options(pacman_movement_test PRIVATE /W4)
    target_compile_options(pacman_turn_buffer_test PRIVATE /W4)
else()
    target_compile_options(pacman PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pacman_movement_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pacman_turn_buffer_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
