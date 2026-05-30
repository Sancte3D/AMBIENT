# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 2 of 12 — OLED alive. The build produces a UF2 that
initialises the SSD1322, draws a static banner ("FIELD AMBIENCE / V0.9
STEP 2"), keeps the STATUS-LED heartbeat, and continues to print a USB
CDC line every 2 seconds. No audio, no menu logic yet.

The MicroPython firmware in `../firmware/` remains the working firmware
on the device during the transition. Nothing is being removed yet.

## Prerequisites

- **Pico SDK 2.x** installed somewhere on disk
- `PICO_SDK_PATH` exported to its location
- ARM GCC toolchain (`arm-none-eabi-gcc`) on `PATH`
- `cmake >= 3.13`, `make`

Standard SDK setup — if `pico-examples` builds on your machine, this builds.

## Build

```bash
cd field-ambience-current/firmware-c
mkdir -p build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
```

Output (in `build/`):
- `field_ambience_native.uf2` — drop this onto the Pico in BOOTSEL mode
- `field_ambience_native.elf` — for `picotool` / OpenOCD debugging
- `field_ambience_native.bin`, `.hex`, `.map` — for diagnostics

## Flash + verify

1. Hold BOOTSEL on the Pico 2, plug in USB → `RPI-RP2` mass-storage appears
2. Drag `field_ambience_native.uf2` onto it
3. Pico reboots; the STATUS LED (GP26) starts blinking at 1 Hz
4. The 256×64 SSD1322 OLED shows two lines:

   ```
                  FIELD AMBIENCE
                  V0.9 STEP 2
   ```

5. Open a serial monitor on the Pico's USB CDC (e.g. `picocom /dev/ttyACM0
   115200`). Every 2 s you should see:

   ```
   field-ambience native v0.9-dev step2 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 2. Step 3 will bring the I²C bus + the
MCP23017 + the 10 switches up.

## What's next

See `../NATIVE_PORT_PLAN.md`. Step 3 adds the I²C bus + MCP23017 + the 10
switches; subsequent steps add encoders, I²S DMA, and the engine — one PR
per step so the user can review and redirect at any time.
