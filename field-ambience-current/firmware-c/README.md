# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 5 of 12 — **first sound on the speakers.** Build produces
a UF2 that runs Step 1–4 plus a PIO-driven 16-bit/44.1-kHz I²S stream into
the PCM5102A, double-buffered via DMA. The SPEC v0.6 §8 pop-suppression
power sequence is enforced: rails settle → /SHDN HIGH → 50 ms → /MUTE HIGH
+ PCM XSMT HIGH, all while the I²S already pumps silence so the DAC never
sees garbage. After unmute the engine produces a continuous 440 Hz sine at
-20 dBFS for "yes, the audio path works" verification.

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
4. The 256×64 SSD1322 OLED shows banner + audio status + Step 4 encoder /
   button state:

   ```
               FIELD AMBIENCE
               V0.9 STEP 5
   AUDIO 440HZ TEST

   DRIV  .   BRIT  .   DISP  .   VOL   .
   +0000     +0000     +0000     +0000

   C . . . . .   M . . . . .         J
   ```

5. **Listen.** A clean 440 Hz sine is on the speakers and the line-out.
   Volume is fixed at -20 dBFS into the PCM5102A → 23 dB into the PAM8403 →
   ~1.5 Vrms BTL → reasonable on the 40 mm down-firing drivers without
   driving them into excursion. Plugging into J8 should mute the speakers
   (jack-detect via MCP) and leave the sine on the line-out only.

6. USB CDC traces continue from Step 3/4 plus a one-time `audio:` line:

   ```
   audio: I2S pump live, 440 Hz sine at -20 dBFS
   field-ambience native v0.9-dev step5 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 5. **The Pi is functionally redundant
from this point on**: the audio path is Pico → DAC → Amp → Speaker, no
Linux involved. Step 6 cleans up the schematic to reflect that (J2 / D2 /
R_BCK/LRCK/DOUT / the Pi module itself all come out of the BOM).

## What's next

See `../NATIVE_PORT_PLAN.md`. Step 6 = the schematic update that lets the
Pi come out of the BOM (now that audio is proven to flow Pico → DAC → Amp
without any Linux in the path).
