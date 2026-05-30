# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 3 of 12 — I²C + MCP23017 + 10 switches + jack-detect alive.
Build produces a UF2 that initialises SPI0 (OLED), I²C1 (MCP23017), and
GP22 (INTA falling-edge IRQ). The OLED now renders live state for the 5
cells, 5 modifiers, and the jack-detect line; the USB CDC logs every
button-change event. STATUS-LED heartbeat preserved. No audio yet.

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
4. The 256×64 SSD1322 OLED shows the banner + live state rows:

   ```
                  FIELD AMBIENCE
                  V0.9 STEP 3

   CELL  1  2  3  4  5    JACK
         .  .  .  .  .       J
   MOD   S  H  D  G  C
         .  .  .  .  .
   ```

   Press a cell or modifier → the matching `.` becomes a bright `O`. Plug
   into J8 → the `J` brightens. Release → back to dim.

5. Open a serial monitor on the Pico's USB CDC (e.g. `picocom /dev/ttyACM0
   115200`). On every button change you see one `MCP state ...` line, and
   every 2 s the heartbeat:

   ```
   field-ambience native v0.9-dev step3 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 3. Step 4 brings the 4× EC11 encoders
up; Step 5 will bring I²S DMA + the first audible sine.

## What's next

See `../NATIVE_PORT_PLAN.md`. Step 4 adds the 4× EC11 encoder quadrature
decoder; Step 5 brings I²S DMA + first sine tone — then the schematic
update (Step 6) lets the Pi come out of the BOM.
