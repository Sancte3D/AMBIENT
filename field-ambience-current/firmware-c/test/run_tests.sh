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

# Step 8: famSubBass + famDeepBass + dsp_svf highpass / dsp_tri
"$CC" "${CFLAGS[@]}" \
    "$here/test_bass.c" \
    "$src/src/dsp.c" \
    "$src/src/bass.c" \
    -lm -o "$tmp/bass_test"
"$tmp/bass_test"

# Step 11: famReverbMaster + engine mix-bus (engine pulls in pad+texture+bass)
"$CC" "${CFLAGS[@]}" \
    "$here/test_reverb_engine.c" \
    "$src/src/dsp.c" \
    "$src/src/pad.c" \
    "$src/src/reverb.c" \
    "$src/src/texture.c" \
    "$src/src/bass.c" \
    "$src/src/engine.c" \
    -lm -o "$tmp/reverb_test"
"$tmp/reverb_test"
