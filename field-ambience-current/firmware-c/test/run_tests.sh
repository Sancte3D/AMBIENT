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
