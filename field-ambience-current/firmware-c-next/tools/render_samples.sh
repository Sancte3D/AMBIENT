#!/usr/bin/env bash
# Render one WAV per setting into samples/ so each control can be A/B'd by ear.
# Usage: ./tools/render_samples.sh [out_dir]   # default: samples/ (gitignored)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$here/.."
out="${1:-$root/samples}"
mkdir -p "$out"

CC="${CC:-cc}"
bin="$(dirname "$out")/.render_samples_bin"
"$CC" -std=c11 -O2 -Wall -I"$root/include" \
    "$here/render_samples.c" \
    "$root/src/dsp.c" "$root/src/pad.c" "$root/src/padsynth.c" "$root/src/texture.c" \
    "$root/src/bass.c" "$root/src/drone.c" "$root/src/reverb.c" \
    "$root/src/reverb_presets.c" "$root/src/brain.c" \
    "$root/src/generative.c" "$root/src/engine.c" "$root/src/body.c" "$root/src/pluck.c" \
    -lm -o "$bin"
"$bin" "$out"
rm -f "$bin"
echo "→ $out/"
