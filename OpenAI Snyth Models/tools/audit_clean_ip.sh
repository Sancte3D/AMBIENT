#!/bin/sh
set -eu

status=0
if rg -n -i 'minecraft|c418|blind[[:space:]_-]*spots|github\.com|https?://' \
    firmware tools tests CMakeLists.txt Makefile --glob '!audit_clean_ip.sh'; then
    echo "clean-IP audit: external reference found in implementation inputs" >&2
    status=1
fi

if find firmware tools tests -type f \
    \( -iname '*.wav' -o -iname '*.aif' -o -iname '*.aiff' -o -iname '*.flac' \
       -o -iname '*.mp3' -o -iname '*.ogg' -o -iname '*.mid' -o -iname '*.midi' \) \
    | grep -q .; then
    echo "clean-IP audit: source tree unexpectedly contains sample or MIDI media" >&2
    status=1
fi

if rg -n -i 'copied from|adapted from|derived from' firmware tools tests \
    --glob '!audit_clean_ip.sh'; then
    echo "clean-IP audit: derivation marker found in implementation inputs" >&2
    status=1
fi

if [ "$status" -ne 0 ]; then
    exit "$status"
fi
echo "clean-IP audit: passed (no external references, samples, MIDI, or derivation markers)"
