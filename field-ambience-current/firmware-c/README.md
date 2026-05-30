# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 4 of 12 — 4× EC11 encoders alive. Build produces a UF2
that runs Step 1–3 plus a 1 kHz hardware timer that quadrature-decodes the
4 EC11s on GP10–GP21 and debounces their push switches. The OLED shows
each encoder's accumulated position + push-state alongside the cells +
modifiers from Step 3; USB CDC logs each rotate / push event. No audio yet.

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
4. The 256×64 SSD1322 OLED shows banner + encoder state + button state:

   ```
               FIELD AMBIENCE
               V0.9 STEP 4

   DRIV  .   BRIT  .   DISP  .   VOL   .
   +0000     +0000     +0000     +0000

   C . . . . .   M . . . . .         J
   ```

   - Twist any encoder → the matching position updates in real time.
   - Press an encoder shaft → its dot brightens.
   - Cells / modifiers / jack from Step 3 still respond.

5. USB CDC traces every event:

   ```
   ENC 3 rotate +1  pos=4
   ENC 1 push   DOWN
   MCP state 0xDFFE   (CELL1 pressed)
   ```

   Heartbeat every 2 s:

   ```
   field-ambience native v0.9-dev step4 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 4. **If a knob feels backwards on real
hardware**, flip its `dir` field in `src/encoders.c` (defs table) from `+1`
to `-1` — that one-bit knob-by-knob fix is the only quirk EC11s ever throw.

Step 5 brings I²S DMA + the first audible sine through PCM5102A → PAM8403.

## What's next

See `../NATIVE_PORT_PLAN.md`. Step 5 = I²S DMA + first audible sine tone.
Step 6 = the schematic update that lets the Pi come out of the BOM.
