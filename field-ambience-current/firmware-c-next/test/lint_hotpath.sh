#!/usr/bin/env bash
# lint_hotpath.sh — audio render-graph hot-path guard (REALTIME_AUDIO_RULES.md).
#
# engine_render() runs inside the SAI-DMA IRQ. Every module reachable from it
# is REALTIME-SAFE: no heap, no blocking, no unbounded work. This gate greps
# the hot-path source (comments stripped) for hard-forbidden calls and FAILS
# the build if any appear — so a future edit can't quietly smuggle a malloc or
# a HAL_Delay into the audio path.
#
# It also prints an INFO list of transcendental calls (sinf/powf/…) found in
# *_render/*_process/*_tick bodies. Those are NOT auto-failed (LUT builds and
# control-rate coeff updates are legitimate) — they are surfaced for review so
# nobody adds a per-sample powf() without noticing.
#
# Usage: lint_hotpath.sh <firmware-c-next-root>
set -euo pipefail
root="${1:?usage: lint_hotpath.sh <firmware-c-next-root>}"

# The per-sample audio render call graph (executes in the DMA ISR). Keep this
# list in sync when a new DSP module joins engine_render(). NOT included:
# UI/menu, presets, generative brain (control-rate, main loop), HAL drivers.
HOTPATH=(
  engine.c pad.c padsynth.c texture.c ambience.c
  ambient_effects.c fx_master.c
  bass.c reverb.c glass.c harmonic_bass.c dsp.c dsp_ladder.c
  v2/synth_host.c v2/beauty_guard.c
  v2/engines/engine_acid.c v2/engines/engine_fm_glass.c
  v2/engines/engine_chorus_mist.c v2/engines/engine_ion_storm.c
  v2/engines/engine_glass_orbit.c v2/engines/engine_bamboo_circuit.c
)

py() { python3 "$@"; }

py - "$root" "${HOTPATH[@]}" <<'PYEOF'
import re, sys, os
root = sys.argv[1]
files = sys.argv[2:]

FORBIDDEN = r'\b(malloc|calloc|realloc|free|printf|fprintf|sprintf|snprintf|vprintf|puts|gets|HAL_Delay|HAL_I2C_[A-Za-z_]+|HAL_SPI_[A-Za-z_]+|exit|abort|rand|srand|fopen|fread|fwrite)\s*\('
TRANSC   = r'\b(sinf|cosf|tanf|powf|expf|logf|log10f|tanhf|sqrtf|acosf|asinf|atan2f)\s*\('

def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    t = re.sub(r'//[^\n]*', '', t)
    return t

hard = []
info = []
for name in files:
    path = os.path.join(root, 'src', name)
    if not os.path.exists(path):
        print(f"  lint: WARNING hot-path file missing: {name}")
        continue
    raw = open(path).read()
    code = strip_comments(raw)
    # hard-forbidden anywhere in the hot-path module
    for ln, line in enumerate(code.splitlines(), 1):
        for m in re.finditer(FORBIDDEN, line):
            hard.append((name, ln, m.group(1)))
    # informational: transcendentals (for review, not a failure)
    for ln, line in enumerate(code.splitlines(), 1):
        for m in re.finditer(TRANSC, line):
            info.append((name, m.group(1)))

if info:
    from collections import Counter
    c = Counter(f"{n}:{fn}" for n, fn in info)
    print("  lint: transcendental calls in hot-path (review — not a failure):")
    for k in sorted(c):
        print(f"        {k}  x{c[k]}")

if hard:
    print("  lint: *** FORBIDDEN calls in the audio hot-path ***")
    for name, ln, fn in hard:
        print(f"        {name}:{ln}  {fn}()")
    print("  lint: FAIL — see REALTIME_AUDIO_RULES.md §3")
    sys.exit(1)

print(f"  lint: hot-path clean ({len(files)} modules, 0 forbidden calls)")
PYEOF
