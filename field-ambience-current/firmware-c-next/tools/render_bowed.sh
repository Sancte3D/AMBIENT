#!/usr/bin/env bash
# render_bowed.sh — audition the bowed lyra voice (r19.46) standalone.
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_bowed.c" \
  "$here"/src/bowed.c "$here"/src/reverb.c "$here"/src/dsp.c -lm -o /tmp/render_bowed
/tmp/render_bowed "${1:-/tmp/bowed.wav}"
