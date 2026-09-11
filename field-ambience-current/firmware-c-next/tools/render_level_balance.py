#!/usr/bin/env python3
"""25 s firmware level-calibration A/B. Args: before.so after.so output.wav.
Each half: Storm/Mist/Orbit, 4 s each. A single gain applies to the whole WAV.
"""
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write

def motif(library, core):
    inst = Instrument(library)
    for key, value in [('set_synth', core), ('set_tuning', 1), ('set_key', 60),
                       ('set_fx_mode', 1), ('set_space', .3), ('set_master_volume', .4)]:
        inst.call(key, value)
    render_pcm(inst, 44100)
    parts = []
    for hz, release_frames in [(261.625565, 13230), (327.031956, 57330)]:
        inst.lib.engine_note_on(0, hz, .5)
        parts.append(render_pcm(inst, 52920))
        inst.call('note_off', 0)
        parts.append(render_pcm(inst, release_frames))
    data = np.concatenate(parts)
    assert data.shape == (4*44100, 2)
    data[-11025:] *= np.linspace(1, 0, 11025)[:, None]
    return data

if __name__ == '__main__':
    halves = [np.concatenate([motif(lib, core) for core in (4,3,5)]) for lib in sys.argv[1:3]]
    data = np.concatenate([halves[0], np.zeros((44100, 2)), halves[1]])
    assert len(data)==25*44100
    with tempfile.TemporaryDirectory() as tmp:
        path=Path(tmp)/'raw.wav';write(path, data);raw=measure(path)
        gain=min(0, -25-raw['integrated_lufs'], -6-raw['true_peak_dbfs'])
        write(sys.argv[3], data*10**(gain/20))
        print('Common gain dB',gain,'output',measure(sys.argv[3]))
