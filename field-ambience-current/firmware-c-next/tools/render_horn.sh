#!/usr/bin/env bash
# render_horn.sh — audition the alphorn/brass voice (Alps) standalone.
# Usage: tools/render_horn.sh [out.wav]   (default /tmp/horn.wav)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-/tmp/horn.wav}"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_horn.c" \
  "$here"/src/horn.c "$here"/src/dsp.c "$here"/src/reverb.c "$here"/src/reverb_presets.c \
  -lm -o /tmp/render_horn
/tmp/render_horn "$out"
