#!/bin/sh
set -eu

mkdir -p build/ui_frames ui/previews

render() {
    index="$1"
    slug="$2"
    frame_dir="build/ui_frames/$slug"
    rm -rf "$frame_dir"
    mkdir -p "$frame_dir"
    ./build/render_ui "$index" "$frame_dir" 72
    ffmpeg -y -loglevel error -framerate 24 -i "$frame_dir/${slug}_%03d.pgm" \
        -vf "fps=24,scale=320:170:flags=neighbor" -loop 0 "ui/previews/$slug.gif"
    ffmpeg -y -loglevel error -i "$frame_dir/${slug}_036.pgm" \
        -frames:v 1 "ui/previews/$slug.png"
}

render 0 resonant-garden
render 1 orbital-loom
render 2 spectral-canyon
render 3 rain-memory
render 4 dream-topography
