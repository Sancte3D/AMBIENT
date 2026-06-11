# Firmware Engineer — Onboarding

## Two trees

- **`firmware-c-next/`** — ACTIVE. Pico 2 SDK, host-portable C audio engine, web simulator, bench bring-up. This is what you build.
- **`firmware-c/`** — FROZEN. The listening-test snapshot from Step 11/12a. Don't develop here; only fall back to it if `firmware-c-next` breaks.

## TL;DR — current step

Step 12b is mostly done (#1-#5 + #6 core + #7 OLED menu + bench bring-up).
Display sim is live on GitHub Pages. Bench (Pico 2 + 1.9″ ST7789 + KY-040 + tactile) flashes from CI artifact `display_hw_test.uf2`.

What's next: **Phase 5 profiling on STM32H743 hardware** before any layout
work. That gate is what unblocks PCB design.

## Build + test

### Host tests (no hardware needed)
```bash
cd field-ambience-current/firmware-c-next/test
./run_tests.sh
```
All 12+ suites must pass. The bench display tool has 50+ checks that lock
the animation engine + quadrature decoder + backlight gamma.

### Pico 2 cross-build (needs ARM toolchain + Pico SDK 2.x)
```bash
cd field-ambience-current/firmware-c-next
mkdir -p build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
```
Outputs: `field_ambience_native.uf2` (main engine) + `display_hw_test.uf2` (bench).
Or grab CI artifacts: workflow `firmware-c` → `firmware-c-next-pico2-uf2`.

### Web simulator
Open `firmware-c-next/tools/display_sim.html` in any browser. Mirror of `src/menu.c`. Live on GitHub Pages.

## Code layout

- `src/` — the engine. Audio (`dsp.c`, `voices.c`, `pad.c`, `bass.c`, `texture.c`, `reverb.c`), harmonic brain (`brain.c`, `drone.c`, `generative.c`), engine bus (`engine.c`), I/O (`mcp23017.c`, `encoders.c`, `audio.c`), display (`menu.c`, `oled_draw.c`, `baked_font.c`, `lcd_st7789.c`)
- `include/` — public APIs
- `test/` — host tests + `pico_stubs/` (SDK stubs for host builds of device-only tools)
- `tools/` — `display_sim.html` (web), `display_hw_test.c` (bench), font generation, render harnesses

## Bench bring-up (Pico 2 + ST7789)

Wiring + flashing + controls documented in the header of
`tools/display_hw_test.c`. Real menu on real panel, KY-040 encoder
(GP2/3/4) and one tactile button (GP5/SHIFT) on a breadboard. SPI to ST7789
on GP16-20, backlight PWM on GP22. SHIFT is a toggle latch, backlight readout
is transient (Sim parity).

## The "live parameter rule"

From `NATIVE_PORT_PLAN.md` Step 12b: any global (Key, Mode, Vibe, PadVoice,
etc.) glides smoothly. Held cell notes keep their pitch. The sound never
competes with itself. Tests in `test_reverb_engine.c` exercise this with a
"live param storm" + held note.

## STM32H743 migration

The schematic side is done (r18.6). The firmware side has not started — it's
Phase 4 of `NATIVE_PORT_PLAN.md`. Plan: HAL abstraction in `firmware-c-next`
first (Phase 2), then a second `target=stm32h7` build alongside `target=pico2`.

Phase 5 = profiling gate on real H743 hardware. Until this passes, **don't
let anyone start PCB layout**.

## What not to touch

- `firmware-c/` for active development
- `kicad/` (that's the hardware engineer's tree)
- Anything in `archive/` or `legacy_pico2/`
