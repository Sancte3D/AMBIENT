# Field Ambience — native C firmware for RP2350 (Pico 2).
#
# Step 1 = bare-bones skeleton: blinks the STATUS LED on GP26 and prints a
# heartbeat on USB CDC every 2 seconds. No audio, no UI, no menu yet — just
# proof that the C codebase + Pico SDK toolchain are wired up and produce
# a working UF2.
#
# Build (with Pico SDK 2.x installed and PICO_SDK_PATH set):
#   mkdir build && cd build
#   cmake .. -DPICO_BOARD=pico2
#   make -j

cmake_minimum_required(VERSION 3.13)

# Pull in the SDK (one-liner standard-boilerplate file next to this CMakeLists).
include(pico_sdk_import.cmake)

# v0.9-dev native port. Bump on each major step.
project(field_ambience_native C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Default to the Pico 2 board (RP2350). Caller can override on the command
# line with -DPICO_BOARD=pico for an RP2040 dev test if needed.
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2)
endif()

pico_sdk_init()

add_executable(field_ambience_native
    src/main.c
    src/oled.c
    src/font_8x8.c
    src/mcp23017.c
    src/encoders.c
    src/audio.c
    src/dsp.c       # Step 7/9: sine LUT, midi→Hz, polyBLEP osc, SVF, smoothing
    src/voices.c    # Step 7: sine voice pool (superseded as the live renderer
                    #         in Step 9; kept + unit-tested for now)
    src/pad.c       # Step 9: famPadCore voice pool
    src/texture.c   # Step 10: famTexture noise bed
    src/reverb.c    # Step 11: famReverbMaster (Freeverb-style algorithmic)
    src/engine.c    # Step 11: mix-bus (pad + texture → dry+send, reverb, clip)
)

target_include_directories(field_ambience_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}        # generated audio_i2s.pio.h lives here
)

# Step 5: assemble the I²S PIO program at build time.
pico_generate_pio_header(field_ambience_native
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_i2s.pio
)

target_link_libraries(field_ambience_native
    pico_stdlib
    pico_time       # Step 4: repeating_timer for 1 kHz encoder sampling
    hardware_spi    # Step 2: SSD1322 over SPI0
    hardware_i2c    # Step 3: MCP23017 over I2C1
    hardware_gpio   # Step 3: GP22 IRQ for MCP INTA
    hardware_pio    # Step 5: I²S TX state machine
    hardware_dma    # Step 5: ping-pong DMA into PIO FIFO
    hardware_clocks # Step 5: PIO clk divider sizing
    hardware_irq    # Step 5: DMA completion IRQ
)

# USB CDC for the v0.9-dev heartbeat — easier to inspect than UART on a
# device that has no other obvious console. UART stays off; we'll bring it
# back if we need it for a hardware diagnostic later.
pico_enable_stdio_usb(field_ambience_native 1)
pico_enable_stdio_uart(field_ambience_native 0)

# Emit the UF2 + map + bin so the user can drop the UF2 onto the Pico in
# BOOTSEL mode without further tooling.
pico_add_extra_outputs(field_ambience_native)
