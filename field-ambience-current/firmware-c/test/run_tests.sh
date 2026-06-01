#!/usr/bin/env bash
# Host-side unit tests for the hardware-independent firmware code (dsp.c,
# voices.c). No Pico SDK / ARM toolchain required — these compile natively.
#
# Usage: ./run_tests.sh   (run from anywhere)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$here/.."
out="$(mktemp -d)/fam_test"

CC="${CC:-cc}"
"$CC" -std=c11 -O2 -Wall -Wextra \
    -I"$src/include" \
    "$here/test_dsp_voices.c" \
    "$src/src/dsp.c" \
    "$src/src/voices.c" \
    -lm -o "$out"

"$out"
