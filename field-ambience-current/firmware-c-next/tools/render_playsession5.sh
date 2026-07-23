#!/usr/bin/env bash
# render_playsession5.sh — a ~5-minute human play session touring all five
# landscape worlds through the real engine. Usage: [out.wav] (default /tmp/playsession5.wav)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-/tmp/playsession5.wav}"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_playsession5.c" \
  "$here"/src/dsp.c "$here"/src/dsp_ladder.c "$here"/src/pad.c "$here"/src/padsynth.c \
  "$here"/src/texture.c "$here"/src/ambience.c "$here"/src/bass.c "$here"/src/reverb.c \
  "$here"/src/reverb_presets.c "$here"/src/engine.c "$here"/src/fx_master.c \
  "$here"/src/ambient_effects.c "$here"/src/brain.c "$here"/src/worlds.c \
  "$here"/src/generative.c "$here"/src/cells.c "$here"/src/drone.c "$here"/src/body.c \
  "$here"/src/composer.c "$here"/src/harmony.c "$here"/src/tuning.c "$here"/src/pluck.c \
  "$here"/src/glass.c "$here"/src/ember.c "$here"/src/bowed.c -lm -o /tmp/render_playsession5
/tmp/render_playsession5 "$out"
