#!/usr/bin/env bash
# render_catalog.sh — build the SOUND CATALOG: every sound source of the
# instrument rendered in isolation into ../demos/audio_catalog/, plus an
# INDEX.md listing each sample with a status so we see what works / is missing.
#
# Extensible: add one MANIFEST line (category|name|folder|file|secs|status|desc).
# Usage: tools/render_catalog.sh [out_dir]   (default ../demos/audio_catalog)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
outdir="${1:-$here/../demos/audio_catalog}"
bin=/tmp/render_catalog

echo "building render_catalog…"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_catalog.c" \
  "$here"/src/dsp.c "$here"/src/dsp_ladder.c "$here"/src/pad.c "$here"/src/padsynth.c \
  "$here"/src/texture.c "$here"/src/ambience.c "$here"/src/bass.c "$here"/src/reverb.c \
  "$here"/src/reverb_presets.c "$here"/src/engine.c "$here"/src/fx_master.c \
  "$here"/src/ambient_effects.c "$here"/src/brain.c "$here"/src/worlds.c \
  "$here"/src/generative.c "$here"/src/cells.c "$here"/src/drone.c "$here"/src/body.c \
  "$here"/src/composer.c "$here"/src/harmony.c "$here"/src/tuning.c "$here"/src/pluck.c \
  "$here"/src/glass.c "$here"/src/ember.c "$here"/src/bowed.c "$here"/src/horn.c \
  -lm -o "$bin"

# --- MANIFEST: category | name | folder | file | secs | status | description ---
# status: OK = works & in product · PROTO = exists, not wired into the product ·
#         TODO = placeholder / own voice still to build
MANIFEST="
voice|bowed_opensea|1_voices|voice_bowed_opensea|30|OK|Bowed lyra — Open Sea character voice (warm)
voice|bowed_fjords|1_voices|voice_bowed_fjords|30|OK|Bowed Hardanger — Fjords voice (darker, more sympathetic ring)
voice|horn|1_voices|voice_horn_alps|30|PROTO|Alphorn/brass — Alps voice (prototype, NOT yet wired into the engine)
voice|pluck|1_voices|voice_pluck_string|30|OK|Karplus-Strong pluck/string (VOICE: String)
voice|glass|1_voices|voice_glass|30|OK|FM glass bell (VOICE: Glass)
voice|ember|1_voices|voice_ember|30|OK|Warm subtractive analog (VOICE: Ember)
bed|alps|2_beds|bed_alps|24|OK|PADsynth pad bed — Alps timbre (warm odd harmonics)
bed|opensea|2_beds|bed_opensea|24|OK|PADsynth pad bed — Open Sea (glassy)
bed|fjords|2_beds|bed_fjords|24|OK|PADsynth pad bed — Fjords (darkest rolloff)
bed|moss|2_beds|bed_moss|24|OK|PADsynth pad bed — Moss (dusty)
bed|desert|2_beds|bed_desert|24|OK|PADsynth pad bed — Desert (dark-warm low-mid)
ambience|alps|3_ambience|amb_alps|24|OK|Alps ambience — wind only (clear air)
ambience|opensea|3_ambience|amb_opensea|24|OK|Open Sea ambience — gentle waves + warm Mediterranean sea-hum
ambience|fjords|3_ambience|amb_fjords|24|TODO|Fjords ambience — wind only (fjord-water texture still to build)
ambience|moss|3_ambience|amb_moss|24|OK|Moss ambience — wind + rain
ambience|desert|3_ambience|amb_desert|24|TODO|Desert ambience — wind only (heat-shimmer texture still to build)
bass|root|4_low|bass_root|16|OK|Bass voice — root mode
bass|deep|4_low|bass_deep|16|OK|Bass voice — deep mode
drone|default|4_low|drone|24|OK|Drone voice (bloom in / tail out)
fx|bypass|5_fx|fx_bypass|26|OK|Effects: Bypass (dry reference)
fx|reverb|5_fx|fx_reverb|26|OK|Effects: Reverb (dark FDN hall)
fx|delay|5_fx|fx_delay|26|OK|Effects: filtered ping-pong Delay
fx|chorus|5_fx|fx_chorus|26|OK|Effects: Chorus
fx|tape|5_fx|fx_tape|26|OK|Effects: Tape age (wow/flutter/hiss)
fx|swell|5_fx|fx_swell|26|OK|Effects: reverse Swell
fx|shimmer|5_fx|fx_shimmer|26|OK|Effects: octave Shimmer
fx|blur|5_fx|fx_blur|26|OK|Effects: temporal Blur
fx|dream|5_fx|fx_dream|26|OK|Effects: Dream Chain (boot default — everything)
world|alps|6_worlds|world_alps|40|OK|Full world — Alps (Pad voice; own voice TODO)
world|opensea|6_worlds|world_opensea|40|OK|Full world — Open Sea (bowed lyra + Mediterranean bed)
world|fjords|6_worlds|world_fjords|40|OK|Full world — Fjords (bowed, darker)
world|moss|6_worlds|world_moss|40|OK|Full world — Moss Fields (Pad; own voice TODO)
world|desert|6_worlds|world_desert|40|OK|Full world — Desert (Pad; own voice TODO)
"

mkdir -p "$outdir"
# clean only previously-generated audio — never the tracked .gitignore / INDEX.md
find "$outdir" -type f \( -name '*.wav' -o -name '*.flac' \) -delete 2>/dev/null || true
# self-heal the ignore rule so generated audio never gets committed
printf '# generated audio — regenerate with tools/render_catalog.sh\n*.wav\n*.flac\n' > "$outdir/.gitignore"
idx="$outdir/INDEX.md"
{
  echo "# Field Ambience — Sound Catalog"
  echo
  echo "Every sound source of the instrument, rendered in **isolation** so each"
  echo "voice / bed / ambience layer / effect / world can be auditioned on its own."
  echo "Regenerate: \`tools/render_catalog.sh\`. Extend: add a line to its MANIFEST."
  echo
  echo "Status: **OK** = works & in the product · **PROTO** = exists, not yet wired"
  echo "into the product · **TODO** = placeholder / own voice or layer still to build."
  echo
} > "$idx"

cur_folder=""
count=0
while IFS='|' read -r cat name folder file secs status desc; do
  [ -z "${cat// }" ] && continue
  case "$cat" in \#*) continue;; esac
  mkdir -p "$outdir/$folder"
  "$bin" "$cat" "$name" "$outdir/$folder/$file.wav" "$secs" >/dev/null
  if [ "$folder" != "$cur_folder" ]; then
    printf '\n## %s\n\n| sample | status | what it is |\n|---|---|---|\n' \
      "$(echo "$folder" | sed 's/^[0-9]*_//' | tr a-z A-Z)" >> "$idx"
    cur_folder="$folder"
  fi
  printf '| `%s/%s.wav` | %s | %s |\n' "$folder" "$file" "$status" "$desc" >> "$idx"
  count=$((count+1))
done <<< "$MANIFEST"

echo
echo "wrote $count samples + INDEX.md to $outdir"
