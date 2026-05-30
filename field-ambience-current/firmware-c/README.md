# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 1 of 12 — bare-bones skeleton. The build produces a
working UF2 that blinks the status LED and prints a heartbeat on USB CDC.
No audio, no UI yet.

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
4. Open a serial monitor on the Pico's USB CDC (e.g. `picocom /dev/ttyACM0
   115200`). Every 2 s you should see:

   ```
   field-ambience native v0.9-dev step1 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 1. Step 2 will bring the OLED up.

## What's next

See `../NATIVE_PORT_PLAN.md`. The next steps add OLED, switches, encoders,
I²S audio, and the engine — one PR per step so the user can review and
redirect at any time.
