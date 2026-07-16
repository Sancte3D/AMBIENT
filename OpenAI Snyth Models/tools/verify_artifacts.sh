#!/bin/sh
set -eu

audio_slugs="acid-rain fm-glass chorus-mist ion-storm glass-orbit bamboo-circuit nacre-horizon tideglass lumen-swarm hollow-choir"
visual_slugs="resonant-garden orbital-loom spectral-canyon rain-memory dream-topography"
color_slugs="resonant-garden-nacre-dawn resonant-garden-acid-petal resonant-garden-biolume orbital-loom-ion-violet orbital-loom-arctic-bloom orbital-loom-ghost-orchid spectral-canyon-tidal-prism spectral-canyon-ember-moss spectral-canyon-solar-ink rain-memory-copper-rain rain-memory-deep-coral rain-memory-ion-violet dream-topography-biolume dream-topography-lunar-peach dream-topography-tidal-prism"
motion_slugs="focus-rail-ghost-orchid focus-rail-solar-ink prism-veins-ion-violet prism-veins-tidal-prism crystal-choir-arctic-bloom crystal-choir-nacre-dawn radiant-gate-deep-coral radiant-gate-ion-violet lumen-ribbon-biolume lumen-ribbon-acid-petal glyph-relay-tidal-prism glyph-relay-lunar-peach particle-current-ghost-orchid particle-current-arctic-bloom signal-chamber-solar-ink signal-chamber-biolume resonance-orb-nacre-dawn resonance-orb-ion-violet glitch-halo-acid-petal glitch-halo-tidal-prism twin-pulse-deep-coral twin-pulse-arctic-bloom chroma-fall-acid-petal chroma-fall-biolume softburst-lunar-peach softburst-tidal-prism"

audio_count="$(find audio/previews -maxdepth 1 -type f -name '*.mp3' | wc -l | tr -d ' ')"
gif_count="$(find ui/previews -maxdepth 1 -type f -name '*.gif' | wc -l | tr -d ' ')"
png_count="$(find ui/previews -maxdepth 1 -type f -name '*.png' | wc -l | tr -d ' ')"
color_gif_count="$(find ui/previews/color -maxdepth 1 -type f -name '*.gif' | wc -l | tr -d ' ')"
color_png_count="$(find ui/previews/color -maxdepth 1 -type f -name '*.png' | wc -l | tr -d ' ')"
motion_gif_count="$(find ui/previews/motion-studies -maxdepth 1 -type f -name '*.gif' | wc -l | tr -d ' ')"
motion_png_count="$(find ui/previews/motion-studies -maxdepth 1 -type f -name '*.png' | wc -l | tr -d ' ')"
[ "$audio_count" = 10 ] || { echo "expected 10 MP3 files, found $audio_count" >&2; exit 1; }
[ "$gif_count" = 5 ] || { echo "expected 5 GIF files, found $gif_count" >&2; exit 1; }
[ "$png_count" = 9 ] || { echo "expected 9 overview/base PNG files, found $png_count" >&2; exit 1; }
[ "$color_gif_count" = 15 ] || { echo "expected 15 colour GIF files, found $color_gif_count" >&2; exit 1; }
[ "$color_png_count" = 15 ] || { echo "expected 15 colour PNG files, found $color_png_count" >&2; exit 1; }
[ "$motion_gif_count" = 26 ] || { echo "expected 26 motion GIF files, found $motion_gif_count" >&2; exit 1; }
[ "$motion_png_count" = 26 ] || { echo "expected 26 motion PNG files, found $motion_png_count" >&2; exit 1; }

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

for slug in $color_slugs; do
    for extension in gif png; do
        file="ui/previews/color/$slug.$extension"
        geometry="$(ffprobe -v error -select_streams v:0 \
            -show_entries stream=width,height -of csv=p=0:s=x "$file")"
        [ "$geometry" = "320x170" ] || {
            echo "$file has unexpected geometry $geometry" >&2
            exit 1
        }
    done
    frames="$(ffprobe -v error -count_frames -select_streams v:0 \
        -show_entries stream=nb_read_frames -of default=noprint_wrappers=1:nokey=1 \
        "ui/previews/color/$slug.gif")"
    [ "$frames" = 96 ] || {
        echo "ui/previews/color/$slug.gif has $frames frames, expected 96" >&2
        exit 1
    }
done

for slug in $motion_slugs; do
    for extension in gif png; do
        file="ui/previews/motion-studies/$slug.$extension"
        geometry="$(ffprobe -v error -select_streams v:0 \
            -show_entries stream=width,height -of csv=p=0:s=x "$file")"
        [ "$geometry" = "320x170" ] || {
            echo "$file has unexpected geometry $geometry" >&2
            exit 1
        }
    done
    frames="$(ffprobe -v error -count_frames -select_streams v:0 \
        -show_entries stream=nb_read_frames -of default=noprint_wrappers=1:nokey=1 \
        "ui/previews/motion-studies/$slug.gif")"
    [ "$frames" = 96 ] || {
        echo "ui/previews/motion-studies/$slug.gif has $frames frames, expected 96" >&2
        exit 1
    }
done

echo "artifact verification: 10 stereo MP3s, 5 base GIFs, 15 colour GIFs, and 26 motion GIFs passed"
