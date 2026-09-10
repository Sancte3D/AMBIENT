#!/usr/bin/env python3
"""Real firmware render contract regression. Pass a built host .so path."""
import sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from render_musical_review import Instrument
from render_tuning_review import render_pcm

inst = Instrument(sys.argv[1])
inst.call('set_synth', 2)
inst.call('set_fx_mode', 0)
render_pcm(inst, 44100)
inst.note(0, 60, .4)
audio = render_pcm(inst, 44100 + 37)
assert audio.shape == (44137, 2)
assert np.isfinite(audio).all() and np.max(np.abs(audio)) < 1
# Sustain must reach every later window, including a non-block-aligned end.
for start in range(4410, 40000, 4410):
    assert np.sqrt(np.mean(audio[start:start+4410]**2)) > .005, start
assert np.max(np.abs(audio[-37:])) > .001
print('PASS: real PCM fills the full second and partial final block')
