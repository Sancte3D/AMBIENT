#!/bin/sh
set -eu

audio_slugs="acid-rain fm-glass chorus-mist ion-storm glass-orbit bamboo-circuit nacre-horizon tideglass lumen-swarm hollow-choir"
visual_slugs="resonant-garden orbital-loom spectral-canyon rain-memory dream-topography"

audio_count="$(find audio/previews -maxdepth 1 -type f -name '*.mp3' | wc -l | tr -d ' ')"
gif_count="$(find ui/previews -maxdepth 1 -type f -name '*.gif' | wc -l | tr -d ' ')"
png_count="$(find ui/previews -maxdepth 1 -type f -name '*.png' | wc -l | tr -d ' ')"
[ "$audio_count" = 10 ] || { echo "expected 10 MP3 files, found $audio_count" >&2; exit 1; }
[ "$gif_count" = 5 ] || { echo "expected 5 GIF files, found $gif_count" >&2; exit 1; }
[ "$png_count" = 6 ] || { echo "expected 6 PNG files including contact sheet, found $png_count" >&2; exit 1; }

for slug in $audio_slugs; do
    file="audio/previews/$slug.mp3"
    probe="$(ffprobe -v error -select_streams a:0 \
        -show_entries stream=codec_name,sample_rate,channels \
        -show_entries format=duration -of default=noprint_wrappers=1 "$file")"
    echo "$probe" | grep -q '^codec_name=mp3$'
    echo "$probe" | grep -q '^sample_rate=44100$'
    echo "$probe" | grep -q '^channels=2$'
    duration="$(echo "$probe" | sed -n 's/^duration=//p')"
    awk -v value="$duration" 'BEGIN { exit !(value >= 13.9 && value <= 14.2) }'
done

for slug in $visual_slugs; do
    for extension in gif png; do
        file="ui/previews/$slug.$extension"
        geometry="$(ffprobe -v error -select_streams v:0 \
            -show_entries stream=width,height -of csv=p=0:s=x "$file")"
        [ "$geometry" = "320x170" ] || {
            echo "$file has unexpected geometry $geometry" >&2
            exit 1
        }
    done
done

echo "artifact verification: 10 stereo MP3 previews and 5 animated + 5 still UI previews passed"
