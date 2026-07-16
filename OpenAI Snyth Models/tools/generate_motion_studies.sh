#!/bin/sh
set -eu

mkdir -p build/motion_ui_frames ui/previews/motion-studies

render() {
    visual="$1"
    palette="$2"
    slug="$3"
    frame_dir="build/motion_ui_frames/$slug"
    rm -rf "$frame_dir"
    mkdir -p "$frame_dir"
    ./build/render_ui "$visual" "$frame_dir" 96 "$palette"
    python3 tools/encode_color_gif.py \
        "$frame_dir/${slug}_*.ppm" \
        "ui/previews/motion-studies/$slug.gif" \
        "ui/previews/motion-studies/$slug.png"
    rm -rf "$frame_dir"
}

# Two independently authored colour directions for each new motion system.
render 5  9  focus-rail-ghost-orchid
render 5  10 focus-rail-solar-ink
render 6  3  prism-veins-ion-violet
render 6  1  prism-veins-tidal-prism
render 7  5  crystal-choir-arctic-bloom
render 7  0  crystal-choir-nacre-dawn
render 8  8  radiant-gate-deep-coral
render 8  3  radiant-gate-ion-violet
render 9  11 lumen-ribbon-biolume
render 9  6  lumen-ribbon-acid-petal
render 10 1  glyph-relay-tidal-prism
render 10 4  glyph-relay-lunar-peach
render 11 9  particle-current-ghost-orchid
render 11 5  particle-current-arctic-bloom
render 12 10 signal-chamber-solar-ink
render 12 11 signal-chamber-biolume
render 13 0  resonance-orb-nacre-dawn
render 13 3  resonance-orb-ion-violet
render 14 6  glitch-halo-acid-petal
render 14 1  glitch-halo-tidal-prism
render 15 8  twin-pulse-deep-coral
render 15 5  twin-pulse-arctic-bloom
render 16 6  chroma-fall-acid-petal
render 16 11 chroma-fall-biolume
render 17 4  softburst-lunar-peach
render 17 1  softburst-tidal-prism

python3 tools/make_motion_contact_sheets.py
