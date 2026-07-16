#!/bin/sh
set -eu

mkdir -p build/audio_wav audio/previews

render() {
    index="$1"
    slug="$2"
    ./build/render_audio "$index" "build/audio_wav/$slug.wav" 14
    ffmpeg -y -loglevel error -i "build/audio_wav/$slug.wav" \
        -af "loudnorm=I=-20:LRA=9:TP=-1.5" \
        -codec:a libmp3lame -b:a 192k -ar 44100 "audio/previews/$slug.mp3"
}

render 0 acid-rain
render 1 fm-glass
render 2 chorus-mist
render 3 ion-storm
render 4 glass-orbit
render 5 bamboo-circuit
render 6 nacre-horizon
render 7 tideglass
render 8 lumen-swarm
render 9 hollow-choir
