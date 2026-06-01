# Field Ambience — native C firmware (RP2350)

This is the **native port** of the Field Ambience engine and UI, targeting
the Pico 2 (RP2350) directly. The plan is documented in
`../NATIVE_PORT_PLAN.md`. When this firmware is complete, the Raspberry Pi
Zero 2 W will no longer be part of the device — the RP2350 will host both
the audio engine and the UI.

**Status:** Step 7 of 12 — **polyphonic voice pool.** Build runs Steps 1–6
plus an 8-voice pool (sine + click-free ASR envelope) fed by the cell
buttons: tap a cell → a sustained tone blooms in over ~0.8 s; release →
it fades over ~2.5 s. The Step-5 test sine is replaced by `voices_render()`
via the new pluggable `audio_set_renderer()`. Cell→pitch is a placeholder
C-minor-pentatonic until the harmonic brain (Step 12). (Step 6 was the
schematic-only Pi removal — no firmware change.)

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

## Host tests (no device, no SDK)

The hardware-independent audio math (`src/dsp.c`, `src/voices.c`) compiles and
runs natively, so its correctness can be checked without a Pico or the ARM
toolchain — a clean UF2 link proves the build wires up, not that the sine is a
sine or the envelope is click-free.

```bash
cd field-ambience-current/firmware-c/test
./run_tests.sh          # builds with host cc, runs assertions, prints PASS/FAIL
```

Covers: sine-LUT accuracy vs `sinf` (< 1e-3), phase wrap, MIDI→Hz (A4=440,
octave doubling), voice-pool alloc/re-trigger/count, 9th-note voice stealing,
click-free attack/release ramps, attack→sustain peak, and int16 soft-clip
bounds under 8-voice full load.

## Flash + verify

1. Hold BOOTSEL on the Pico 2, plug in USB → `RPI-RP2` mass-storage appears
2. Drag `field_ambience_native.uf2` onto it
3. Pico reboots; the STATUS LED (GP26) starts blinking at 1 Hz
4. The 256×64 SSD1322 OLED shows banner + a live voice count:

   ```
               FIELD AMBIENCE
               V0.9 STEP 7
   TAP A CELL              VOX 0

   DRIV  .   BRIT  .   DISP  .   VOL   .
   +0000     +0000     +0000     +0000

   C . . . . .   M . . . . .         J
   ```

5. **Play.** Tap a cell button → a tone blooms in (~0.8 s attack) and holds;
   the `VOX` count rises. Release → it fades out (~2.5 s). Up to 8 voices
   ring at once; a 9th steals the quietest. Cell pitches are a placeholder
   C-minor pentatonic (C3 Eb3 F3 G3 Bb3). Plugging into J8 mutes the
   speakers and leaves the audio on the line-out.

6. USB CDC traces:

   ```
   cell 1 ON  (midi 48)
   cell 1 OFF
   field-ambience native v0.9-dev step7 — pico2 (RP2350) — heartbeat N
   ```

That's the success signal for Step 7. Step 8 adds the two-layer bass
(famSubBass + famDeepBass) under the voices.

## What's next

See `../NATIVE_PORT_PLAN.md`. Steps 8–12 replace the sine voices with the
real engine timbre: two-layer bass, famPadCore, texture bed, algorithmic
reverb, and finally the harmonic brain + v30 menu + USB-MIDI.
