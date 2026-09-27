#!/usr/bin/env python3
"""18-second FM Glass phrase, native defaults + light room; no reference tone.
Usage: python tools/render_short_phrase.py engine.so output.wav
Only constant attenuation is applied after rendering, no added EQ/limiter.
"""
import sys
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write

inst = Instrument(sys.argv[1])
for key, value in [('set_synth', 2), ('set_tuning', 1), ('set_key', 60),
                   ('set_fx_mode', 1), ('set_space', .3), ('set_send', .25),
                   ('set_master_volume', .4), ('set_attack', .7), ('set_release', .6)]:
    inst.call(key, value)
render_pcm(inst, 44100)
parts = []
# Just C4 / E4 / G3 / D4; explicit Hz so the demonstration respects Just.
for hz, velocity in [(261.625565, .35), (327.031956, .30),
                     (196.219174, .35), (294.328761, .25)]:
    inst.lib.engine_note_on(0, hz, velocity)
    parts.append(render_pcm(inst, 3 * 44100))
    inst.call('note_off', 0)
    parts.append(render_pcm(inst, 44100))
parts.append(render_pcm(inst, 2 * 44100))
data = np.concatenate(parts)
assert len(data) == 18 * 44100
# Fade only the file boundaries, leave phrase attacks/tails intact.
data[:441] *= np.linspace(0, 1, 441)[:, None]
data[-22050:] *= np.linspace(1, 0, 22050)[:, None]
out = Path(sys.argv[2])
write(out, data)
raw = measure(out)
gain_db = min(0, -25 - raw['integrated_lufs'], -6 - raw['true_peak_dbfs'])
write(out, data * 10 ** (gain_db / 20))
print({'duration_s': 18, 'attenuation_db': gain_db, **measure(out)})
