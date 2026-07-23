#!/usr/bin/env bash
# render_worlds5.sh — audition the five r19.44 landscape worlds through the
# real engine. Writes per-world + combined WAVs to the given prefix.
# Usage: tools/render_worlds5.sh [out_prefix]   (default /tmp/world5_)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-/tmp/world5_}"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_worlds5.c" \
  "$here"/src/dsp.c "$here"/src/dsp_ladder.c "$here"/src/pad.c "$here"/src/padsynth.c \
  "$here"/src/texture.c "$here"/src/ambience.c "$here"/src/bass.c "$here"/src/reverb.c \
  "$here"/src/reverb_presets.c "$here"/src/engine.c "$here"/src/fx_master.c \
  "$here"/src/ambient_effects.c "$here"/src/brain.c "$here"/src/worlds.c \
  "$here"/src/generative.c "$here"/src/cells.c "$here"/src/drone.c "$here"/src/body.c \
  "$here"/src/composer.c "$here"/src/harmony.c "$here"/src/tuning.c "$here"/src/pluck.c \
  "$here"/src/glass.c "$here"/src/ember.c "$here"/src/bowed.c -lm -o /tmp/render_worlds5
/tmp/render_worlds5 "$out"
