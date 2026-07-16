#!/usr/bin/env bash
# Rendert eine kurze Demo je V2-Sound-Core durch die echte synth_host-API
# in EIN WAV. Aufruf aus firmware-c-next/:  tools/render_synth_samples.sh [out.wav]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-/tmp/field_synths.wav}"
BIN="$(mktemp -d)/render_synth_samples"
cc -O2 -std=c11 -Iinclude \
    tools/render_synth_samples.c \
    src/dsp.c src/dsp_ladder.c src/reverb.c \
    src/v2/beauty_guard.c src/v2/synth_host.c src/v2/engines/*.c \
    -lm -o "$BIN"
"$BIN" "$OUT"
