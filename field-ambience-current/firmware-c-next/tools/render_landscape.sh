#!/usr/bin/env bash
# Render the Landscape cell mode (r19.27) to a WAV through the real engine +
# real landscape.c role machine. Run from firmware-c-next/:
#   tools/render_landscape.sh [out.wav]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-/tmp/landscape.wav}"
BIN="$(mktemp -d)/render_landscape"
cc -O2 -std=c11 -Iinclude tools/render_landscape.c \
    src/dsp.c src/pad.c src/padsynth.c src/texture.c src/ambience.c src/tape.c \
    src/echo.c src/blur.c src/bass.c src/drone.c src/reverb.c src/reverb_presets.c \
    src/brain.c src/worlds.c src/generative.c src/cells.c src/engine.c src/body.c \
    src/composer.c src/harmony.c src/tuning.c src/pluck.c src/glass.c src/shimmer.c \
    src/landscape.c -lm -o "$BIN"
"$BIN" "$OUT"
