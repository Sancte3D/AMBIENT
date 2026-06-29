# Field Ambience — native C firmware build.
#
# Target platform: STM32H743VIT6 (LQFP-100) as the product MCU. The repo
# still keeps the Pico-2 bench tooling (display_hw_test) because the breadboard
# is the only place where the menu + ST7789 + encoder can be physically driven
# until the STM32 prototype exists.
#
# Build targets (-DFAM_TARGET=...):
#   h743  → STM32 firmware (the product). Needs the STM32 ARM-GCC toolchain +
#           CubeH7 HAL. Skeleton stubs live in `src/hal_h743/`; ST-HAL
#           integration is Step 13.3.
#   pico  → bench-only Pico-2 firmware (display_hw_test bring-up tool, not a
#           product build). Uses `src/hal_pico/`.
#   host  → host-only DSP/engine library for offline rendering
#           (tools/render_performance.c, render_wav.c). No HAL.
#
# NB1: the selector variable is FAM_TARGET, *not* TARGET. `TARGET` is a
# reserved keyword in CMake's if() command (`if(TARGET <name>)`), so a cache
# variable named TARGET makes `if(TARGET STREQUAL "h743")` mis-parse and abort
# configuration with "Unknown arguments specified".
#
# NB2: the Pico SDK injects its ARM cross-toolchain through
# pico_sdk_import.cmake, which MUST be included BEFORE project() — otherwise
# project() enables the C/CXX/ASM languages with the host x86 compiler and the
# SDK's ARM boot2 assembly is fed to /usr/bin/cc (boot2_w25q080.S → "expected
# (%rsi)"). So target selection + the SDK import happen above project().
#
# Convenience: when -DPICO_BOARD is passed (the Pico SDK CI path) and the user
# did not name a target, FAM_TARGET auto-selects `pico` — the only thing a
# Pico toolchain can build is the bench tool.
#
# Host tests (the `test/` suite) ignore CMake — see `test/run_tests.sh`. They
# compile pure-C DSP + logic modules directly with `cc -O2 -lm`.

cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# FAM_TARGET selector — resolved BEFORE project() so the Pico SDK can set up
# its ARM toolchain ahead of language enablement. (Named FAM_TARGET, never
# TARGET — see NB1 above.)
# ---------------------------------------------------------------------------
if(DEFINED PICO_BOARD AND NOT DEFINED FAM_TARGET)
    set(_fam_default "pico")        # Pico SDK in play → bench tool only
else()
    set(_fam_default "h743")        # default product target
endif()
set(FAM_TARGET "${_fam_default}" CACHE STRING "Build target: h743 | pico | host")
set_property(CACHE FAM_TARGET PROPERTY STRINGS h743 pico host)

# ---------------------------------------------------------------------------
# Panel selection (ADR-0015 prep). Default = current 1.9" hardware (170×320).
# Set FAM_LCD_PANEL=2_0inch when the proposed 2" 240×320 module is verified
# (BOM_MASTER §5 UNVERIFIED row, SKU/pin-order/mech-dims). Until then this
# defaults safely to the 1.9" build. The flag propagates into the firmware
# via FAM_LCD_PANEL_2_0 in oled.h.
# ---------------------------------------------------------------------------
set(FAM_LCD_PANEL "1_9inch" CACHE STRING "LCD panel: 1_9inch (current) | 2_0inch (proposed)")
set_property(CACHE FAM_LCD_PANEL PROPERTY STRINGS 1_9inch 2_0inch)

# Pico target: pull in the SDK BEFORE project() (see NB2). host/h743 do not
# touch the Pico SDK at all.
if(FAM_TARGET STREQUAL "pico")
    include(pico_sdk_import.cmake)
    if(NOT DEFINED PICO_BOARD)
        set(PICO_BOARD pico2)
    endif()
endif()

project(field_ambience_native C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(FAM_TARGET STREQUAL "pico")
    pico_sdk_init()
endif()

# ---------------------------------------------------------------------------
# Shared DSP / logic sources (target-independent, pure C, host-portable).
# These are the files exercised by test/run_tests.sh — never touch the
# hardware directly. Single source for all three target builds.
# ---------------------------------------------------------------------------
set(DSP_SOURCES
    src/dsp.c
    src/dsp_ladder.c    # 4-pole Huovilainen Moog ladder (MIT port, real analog filter)
    src/voices.c
    src/pad.c
    src/texture.c
    src/ambience.c      # ADR-0017 Phase 2: per-world atmospheric layer
    src/tape.c          # ADR-0017 Phase 3: hiss + warm-tanh saturation (master)
    src/echo.c          # Reddit Echo macro: tape-style stereo delay
    src/blur.c          # Reddit Blur macro: granular cloud
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
    src/leds.c          # controls state → PCA9685 16-channel PWM with fade
    src/oled_draw.c
    src/oled_color.c    # accent-tinted grey→RGB565 LUT (ADR-0015 step 1)
    src/baked_font.c
    src/baked_font_data.c
    src/font_8x8.c
    src/battery.c
    src/menu.c
    src/worlds.c        # ADR-0017 Phase 1: world descriptors as a real module
)

# ---------------------------------------------------------------------------
# Engine V2 — ADR-0014, NEBEN V1 (nicht ersetzt). Wird mit V1 zusammen in
# die h743 + host Targets gelinkt, aber engine_v2_render() wird erst Phase 3
# aktiv per Menü-Toggle aus main_h743.c gerufen. Bis dahin nur über Host-Tests
# + den offline-Renderer in tools/render_v2_wav.c hörbar. (Der pico-Bench-Tool
# linkt V2 nicht — es rendert keinen Sound, sondern testet nur das Display.)
# ---------------------------------------------------------------------------
set(V2_SOURCES
    src/v2/motion.c
    src/v2/harmony_field.c
    src/v2/field_voice.c
    src/v2/material_texture.c
    src/v2/diffuser.c
    src/v2/mod_delay.c
    src/v2/beauty_guard.c
    src/v2/worlds.c
    src/v2/arp.c            # pentatonic bell arpeggio (the melodic layer)
    src/v2/beat.c          # Crystal-Castles synth drum machine
    src/v2/engine_v2.c
    # OP-1-Field-style swappable synth-engine host (FX stay global; the V2
    # ambient stack above remains the FIELD engine, untouched).
    src/v2/synth_host.c
    src/v2/engines/engine_acid.c            # 303-style resonant acid bass
    src/v2/engines/engine_fm_glass.c        # DX7-style 2-op FM key/glass
    src/v2/engines/engine_chorus_mist.c     # Juno detuned-saw pad + chorus
    src/v2/engines/engine_ion_storm.c       # hoover / PWM stack
    src/v2/engines/engine_glass_orbit.c     # wavetable-style morph
    src/v2/engines/engine_bamboo_circuit.c  # west-coast LPG pluck
)

# ---------------------------------------------------------------------------
# Target-specific build dispatch.
# ---------------------------------------------------------------------------
if(FAM_TARGET STREQUAL "h743")
    message(STATUS "Field Ambience: STM32H743 product target (skeleton)")
    # Step 13.2 (this commit) lays the file structure and CMake stub. The
    # actual STM32 toolchain integration (STM32CubeH7 HAL, ARM-GCC, OpenOCD
    # flashing) is Step 13.3. Until then this branch only validates that the
    # HAL files compile syntactically; the SAI/DMA/USB/SPI wiring is TODO.
    # The product target uses the canonical worlds module (src/worlds.c,
    # ADR-0017). Drop the dead v2 duplicate (src/v2/worlds.c) — it defines the
    # same worlds_get/worlds_count and would multiply-define in the executable.
    set(V2_SOURCES_H743 ${V2_SOURCES})
    list(REMOVE_ITEM V2_SOURCES_H743 src/v2/worlds.c)
    add_executable(field_ambience_h743
        ${DSP_SOURCES}
        ${V2_SOURCES_H743}
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
    target_compile_definitions(field_ambience_h743 PRIVATE
        "$<$<STREQUAL:${FAM_LCD_PANEL},2_0inch>:FAM_LCD_PANEL_2_0>"
    )
    # The skeleton links with the HOST compiler (no ARM toolchain yet), so the
    # DSP sources need libm for sinf/cosf/expf/tanhf/etc. This makes the h743
    # branch a real CI syntax+link gate (catches missing prototypes/decls)
    # until the Step 13.3 ARM-GCC toolchain lands.
    target_link_libraries(field_ambience_h743 m)

    # TODO(Step 13.3): ARM-GCC toolchain file, CubeH7 HAL sources +
    # include paths, linker script, startup file. For now this target only
    # builds if the user supplies CMAKE_TOOLCHAIN_FILE manually.

elseif(FAM_TARGET STREQUAL "pico")
    message(STATUS "Field Ambience: Pico-2 bench-tool target")
    # The Pico build only exists to drive the display_hw_test breadboard
    # for menu + ST7789 + encoder bring-up. NOT a product build. The SDK was
    # imported + initialised above project() (see NB2).

    # --- display_hw_test: bench tool that drives a real ST7789 + encoder ---
    add_executable(display_hw_test
        tools/display_hw_test.c
        src/hal_pico/lcd_st7789_pico.c
        src/oled_draw.c
        src/oled_color.c
        src/font_8x8.c
        src/baked_font.c
        src/baked_font_data.c
        src/battery.c
        src/menu.c
        src/worlds.c
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
        # Panel selection (ADR-0015 prep). Add FAM_LCD_PANEL_2_0 when the
        # 2.0" module is verified — until then we stay on 1.9".
        "$<$<STREQUAL:${FAM_LCD_PANEL},2_0inch>:FAM_LCD_PANEL_2_0>"
    )
    target_link_libraries(display_hw_test
        pico_stdlib pico_time hardware_spi hardware_gpio hardware_pwm
    )
    pico_enable_stdio_usb(display_hw_test 1)
    pico_enable_stdio_uart(display_hw_test 0)
    pico_add_extra_outputs(display_hw_test)

elseif(FAM_TARGET STREQUAL "host")
    message(STATUS "Field Ambience: host DSP library + offline renderers")
    # Host build: just the DSP + engine compiled to a static library so the
    # offline renderers in tools/ can link it. The actual renderer binaries
    # (render_performance, render_wav) are built ad-hoc with `cc` — see
    # tools/render_performance.c header. This target is mainly for CI smoke.
    add_library(field_ambience_dsp STATIC ${DSP_SOURCES} ${V2_SOURCES})
    target_include_directories(field_ambience_dsp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(field_ambience_dsp PUBLIC m)

else()
    message(FATAL_ERROR "Unknown FAM_TARGET '${FAM_TARGET}' — must be h743, pico, or host")
endif()
