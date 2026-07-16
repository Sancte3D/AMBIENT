#!/bin/sh
set -eu

mkdir -p build/color_ui_frames ui/previews/color

render() {
    visual="$1"
    palette="$2"
    slug="$3"
    frame_dir="build/color_ui_frames/$slug"
    rm -rf "$frame_dir"
    mkdir -p "$frame_dir"
    ./build/render_ui "$visual" "$frame_dir" 96 "$palette"
    python3 tools/encode_color_gif.py \
        "$frame_dir/${slug}_*.ppm" \
        "ui/previews/color/$slug.gif" \
        "ui/previews/color/$slug.png"
}

# Three strongly different colour directions for each visual system.
render 0 0  resonant-garden-nacre-dawn
render 0 6  resonant-garden-acid-petal
render 0 11 resonant-garden-biolume

render 1 3  orbital-loom-ion-violet
render 1 5  orbital-loom-arctic-bloom
render 1 9  orbital-loom-ghost-orchid

render 2 1  spectral-canyon-tidal-prism
render 2 2  spectral-canyon-ember-moss
render 2 10 spectral-canyon-solar-ink

render 3 7  rain-memory-copper-rain
render 3 8  rain-memory-deep-coral
render 3 3  rain-memory-ion-violet

render 4 11 dream-topography-biolume
render 4 4  dream-topography-lunar-peach
render 4 1  dream-topography-tidal-prism

python3 tools/make_color_contact_sheet.py
