#!/usr/bin/env bash
# Render the engine to a reference WAV on the host (no Pico SDK, no device).
# Listen to the result and A/B against software/webapp/field_ambience_webapp.html — if they
# match, any remaining on-device difference is the hardware chain, not the DSP.
#
# Usage: ./tools/render_wav.sh [out.wav]
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$here/.."
out="${1:-$root/field_ambience_ref.wav}"
CC="${CC:-cc}"

"$CC" -std=c11 -O2 -Wall -I"$root/include" \
    "$here/render_wav.c" \
    "$root/src/dsp.c" "$root/src/pad.c" "$root/src/padsynth.c" "$root/src/texture.c" \
    "$root/src/bass.c" "$root/src/drone.c" "$root/src/reverb.c" \
    "$root/src/reverb_presets.c" "$root/src/brain.c" \
    "$root/src/generative.c" "$root/src/engine.c" "$root/src/body.c" "$root/src/pluck.c" \
    -lm -o "$(dirname "$out")/.render_wav_bin"
"$(dirname "$out")/.render_wav_bin" "$out"
rm -f "$(dirname "$out")/.render_wav_bin"
echo "→ $out"
