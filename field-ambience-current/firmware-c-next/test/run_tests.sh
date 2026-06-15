#!/usr/bin/env bash
# Host-side unit tests for the hardware-independent firmware code (dsp.c,
# voices.c, pad.c). No Pico SDK / ARM toolchain required — these compile
# natively.
#
# Usage: ./run_tests.sh   (run from anywhere)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$here/.."
tmp="$(mktemp -d)"

CC="${CC:-cc}"
CFLAGS=(-std=c11 -O2 -Wall -Wextra -I"$src/include")

# Step 7: dsp + voice pool
"$CC" "${CFLAGS[@]}" \
    "$here/test_dsp_voices.c" \
    "$src/src/dsp.c" \
    "$src/src/voices.c" \
    -lm -o "$tmp/fam_test"
"$tmp/fam_test"

# Step 9: dsp primitives + famPadCore
"$CC" "${CFLAGS[@]}" \
    "$here/test_pad.c" \
    "$src/src/dsp.c" \
    "$src/src/pad.c" \
    -lm -o "$tmp/pad_test"
"$tmp/pad_test"

# Step 10: famTexture + dsp_svf bandpass
"$CC" "${CFLAGS[@]}" \
    "$here/test_texture.c" \
    "$src/src/dsp.c" \
    "$src/src/texture.c" \
    -lm -o "$tmp/tex_test"
"$tmp/tex_test"

# Step 12a: harmonic brain (pure integer theory)
"$CC" "${CFLAGS[@]}" \
    "$here/test_brain.c" \
    "$src/src/brain.c" \
    -lm -o "$tmp/brain_test"
"$tmp/brain_test"

# ADR-0013: cell-velocity input model (Hall position → velocity → amp)
"$CC" "${CFLAGS[@]}" \
    "$here/test_cells.c" \
    "$src/src/cells.c" \
    -lm -o "$tmp/cells_test"
"$tmp/cells_test"

# ADR-0008 r2: modifier + cell hold-latch state machine (controls.c)
"$CC" "${CFLAGS[@]}" \
    "$here/test_controls.c" \
    "$src/src/controls.c" \
    "$src/src/dsp.c" "$src/src/pad.c" "$src/src/texture.c" "$src/src/bass.c" \
    "$src/src/drone.c" "$src/src/reverb.c" "$src/src/reverb_presets.c" \
    "$src/src/brain.c" "$src/src/generative.c" "$src/src/cells.c" "$src/src/engine.c" \
    -lm -o "$tmp/controls_test"
"$tmp/controls_test"

# Encoder → engine parameter bindings + acceleration (params.c)
"$CC" "${CFLAGS[@]}" \
    "$here/test_params.c" \
    "$src/src/params.c" \
    "$src/src/dsp.c" "$src/src/pad.c" "$src/src/texture.c" "$src/src/bass.c" \
    "$src/src/drone.c" "$src/src/reverb.c" "$src/src/reverb_presets.c" \
    "$src/src/brain.c" "$src/src/generative.c" "$src/src/cells.c" "$src/src/engine.c" \
    -lm -o "$tmp/params_test"
"$tmp/params_test"

# Step 8: famSubBass + famDeepBass + dsp_svf highpass / dsp_tri
"$CC" "${CFLAGS[@]}" \
    "$here/test_bass.c" \
    "$src/src/dsp.c" \
    "$src/src/bass.c" \
    -lm -o "$tmp/bass_test"
"$tmp/bass_test"

# Step 12b #1: reverb mode/vibe presets + space/mood macro
"$CC" "${CFLAGS[@]}" \
    "$here/test_reverb_presets.c" \
    "$src/src/reverb_presets.c" \
    -lm -o "$tmp/rp_test"
"$tmp/rp_test"

# Step 12b #2: drone (portamento root pad)
"$CC" "${CFLAGS[@]}" \
    "$here/test_drone.c" \
    "$src/src/dsp.c" \
    "$src/src/drone.c" \
    -lm -o "$tmp/drone_test"
"$tmp/drone_test"

# Step 12b #4: generative progression sequencer (pure logic)
"$CC" "${CFLAGS[@]}" \
    "$here/test_generative.c" \
    "$src/src/generative.c" \
    -lm -o "$tmp/gen_test"
"$tmp/gen_test"

# Step 12b #6: MIDI message core + FIFO
"$CC" "${CFLAGS[@]}" \
    "$here/test_midi.c" \
    "$src/src/midi.c" \
    -lm -o "$tmp/midi_test"
"$tmp/midi_test"

# Step 12b #7: menu state machine + battery curve
"$CC" "${CFLAGS[@]}" \
    "$here/test_menu_battery.c" \
    "$src/src/menu.c" "$src/src/battery.c" \
    "$src/src/oled_draw.c" "$src/src/baked_font.c" "$src/src/baked_font_data.c" "$src/src/font_8x8.c" \
    -lm -o "$tmp/menu_test"
"$tmp/menu_test"

# Step 11: famReverbMaster + engine mix-bus (engine pulls in pad+texture+bass
# and, from Step-12b #1 on, the reverb_presets + brain modules too)
"$CC" "${CFLAGS[@]}" \
    "$here/test_reverb_engine.c" \
    "$src/src/dsp.c" \
    "$src/src/pad.c" \
    "$src/src/reverb.c" \
    "$src/src/texture.c" \
    "$src/src/bass.c" \
    "$src/src/drone.c" \
    "$src/src/reverb_presets.c" \
    "$src/src/brain.c" \
    "$src/src/generative.c" \
    "$src/src/cells.c" \
    "$src/src/engine.c" \
    -lm -o "$tmp/reverb_test"
"$tmp/reverb_test"

# Bench display tool: animated renderer + tween engine + quadrature decode +
# backlight gamma. Compiles the REAL tools/display_hw_test.c against the fake
# SDK headers in test/pico_stubs/ (host build of a device-only tool).
"$CC" "${CFLAGS[@]}" -I"$here/pico_stubs" -I"$src/tools" \
    "$here/test_display_bench.c" \
    "$src/src/menu.c" "$src/src/battery.c" \
    "$src/src/oled_draw.c" "$src/src/baked_font.c" "$src/src/baked_font_data.c" "$src/src/font_8x8.c" \
    -lm -o "$tmp/bench_test"
"$tmp/bench_test"
