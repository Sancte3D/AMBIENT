# Field Ambience — native C firmware build.
#
# Target platform: STM32H743VIT6 (LQFP-100) as the product MCU. The repo
# still keeps the Pico-2 bench tooling (display_hw_test) because the breadboard
# is the only place where the menu + ST7789 + encoder can be physically driven
# until the STM32 prototype exists.
#
# Build targets:
#   -DTARGET=h743  → STM32 firmware (the product). Needs the STM32 ARM-GCC
#                    toolchain + CubeH7 HAL. Skeleton stubs live in
#                    `src/hal_h743/`; ST-HAL integration is Step 13.3.
#   -DTARGET=pico  → bench-only Pico-2 firmware (display_hw_test bring-up tool,
#                    not a product build). Uses `src/hal_pico/`.
#   -DTARGET=host  → host-only DSP/engine library for offline rendering
#                    (tools/render_performance.c, render_wav.c). No HAL.
#
# Host tests (the `test/` suite) ignore CMake — see `test/run_tests.sh`. They
# compile pure-C DSP + logic modules directly with `cc -O2 -lm`.

cmake_minimum_required(VERSION 3.16)
project(field_ambience_native C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ---------------------------------------------------------------------------
# TARGET selector — single switch that determines which HAL gets linked.
# ---------------------------------------------------------------------------
set(TARGET "h743" CACHE STRING "Build target: h743 | pico | host")
set_property(CACHE TARGET PROPERTY STRINGS h743 pico host)

# ---------------------------------------------------------------------------
# Shared DSP / logic sources (target-independent, pure C, host-portable).
# These are the 19 files exercised by test/run_tests.sh — never touch the
# hardware directly. Single source for all three target builds.
# ---------------------------------------------------------------------------
set(DSP_SOURCES
    src/dsp.c
    src/voices.c
    src/pad.c
    src/texture.c
    src/bass.c
    src/reverb.c
    src/reverb_presets.c
    src/engine.c
    src/brain.c
    src/drone.c
    src/generative.c
    src/midi.c
    src/cells.c
    src/controls.c      # ADR-0008 r2: hold-latch + modifier state machine
    src/params.c        # encoder → engine parameter bindings + acceleration
    src/oled_draw.c
    src/baked_font.c
    src/baked_font_data.c
    src/font_8x8.c
    src/battery.c
    src/menu.c
)

# ---------------------------------------------------------------------------
# Target-specific build dispatch.
# ---------------------------------------------------------------------------
if(TARGET STREQUAL "h743")
    message(STATUS "Field Ambience: STM32H743 product target (skeleton)")
    # Step 13.2 (this commit) lays the file structure and CMake stub. The
    # actual STM32 toolchain integration (STM32CubeH7 HAL, ARM-GCC, OpenOCD
    # flashing) is Step 13.3. Until then this branch only validates that the
    # HAL files compile syntactically; the SAI/DMA/USB/SPI wiring is TODO.
    add_executable(field_ambience_h743
        ${DSP_SOURCES}
        src/hal_h743/main_h743.c
        src/hal_h743/audio_h743.c
        src/hal_h743/encoders_h743.c
        src/hal_h743/lcd_st7789_h743.c
        src/hal_h743/mcp23017_h743.c
        src/hal_h743/midi_uart_h743.c
    )
    target_include_directories(field_ambience_h743 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hal
    )
    # TODO(Step 13.3): ARM-GCC toolchain file, CubeH7 HAL sources +
    # include paths, linker script, startup file. For now this target only
    # builds if the user supplies CMAKE_TOOLCHAIN_FILE manually.

elseif(TARGET STREQUAL "pico")
    message(STATUS "Field Ambience: Pico-2 bench-tool target")
    # The Pico build only exists to drive the display_hw_test breadboard
    # for menu + ST7789 + encoder bring-up. NOT a product build.
    include(pico_sdk_import.cmake)
    if(NOT DEFINED PICO_BOARD)
        set(PICO_BOARD pico2)
    endif()
    pico_sdk_init()

    # --- display_hw_test: bench tool that drives a real ST7789 + encoder ---
    add_executable(display_hw_test
        tools/display_hw_test.c
        src/hal_pico/lcd_st7789_pico.c
        src/oled_draw.c
        src/font_8x8.c
        src/baked_font.c
        src/baked_font_data.c
        src/battery.c
        src/menu.c
    )
    target_include_directories(display_hw_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_definitions(display_hw_test PRIVATE
        LCD_SPI_HZ=24000000
        LCD_PIN_SCK=18
        LCD_PIN_MOSI=19
        LCD_PIN_CS=17
        LCD_PIN_DC=16
        LCD_PIN_RES=20
        PIN_BL=22
        PIN_ENC_CLK=2
        PIN_ENC_DT=3
        PIN_ENC_SW=4
        PIN_SHIFT=5
    )
    target_link_libraries(display_hw_test
        pico_stdlib pico_time hardware_spi hardware_gpio hardware_pwm
    )
    pico_enable_stdio_usb(display_hw_test 1)
    pico_enable_stdio_uart(display_hw_test 0)
    pico_add_extra_outputs(display_hw_test)

elseif(TARGET STREQUAL "host")
    message(STATUS "Field Ambience: host DSP library + offline renderers")
    # Host build: just the DSP + engine compiled to a static library so the
    # offline renderers in tools/ can link it. The actual renderer binaries
    # (render_performance, render_wav) are built ad-hoc with `cc` — see
    # tools/render_performance.c header. This target is mainly for CI smoke.
    add_library(field_ambience_dsp STATIC ${DSP_SOURCES})
    target_include_directories(field_ambience_dsp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(field_ambience_dsp PUBLIC m)

else()
    message(FATAL_ERROR "Unknown TARGET '${TARGET}' — must be h743, pico, or host")
endif()
