#!/usr/bin/env bash
# render_ui_light.sh — render the light duo-menu PROPOSAL frames.
# Usage: tools/render_ui_light.sh [out_dir]   (default /tmp/ui_light)
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-/tmp/ui_light}"; mkdir -p "$out"
cc -std=c11 -O2 -I"$here/include" "$here/tools/render_ui_light.c" \
  "$here"/src/oled_draw.c "$here"/src/baked_font.c "$here"/src/baked_font_data.c \
  "$here"/src/font_8x8.c -lm -o /tmp/render_ui_light
/tmp/render_ui_light "$out"
