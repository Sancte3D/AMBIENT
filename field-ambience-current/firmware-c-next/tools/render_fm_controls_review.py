#!/usr/bin/env python3
"""25 s FM default A/B, same phrase/velocity/room, loudness matched.
Args: before.so after.so output.wav. No reference sine or post-render EQ.
"""
import sys
from pathlib import Path
import tempfile
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write


def phrase(library):
    inst = Instrument(library)
    for key, value in [('set_synth', 2), ('set_tuning', 1), ('set_key', 60),
                       ('set_fx_mode', 1), ('set_space', .3),
                       ('set_master_volume', .4)]:
        inst.call(key, value)
    render_pcm(inst, 44100)
    parts = []
    for hz, velocity in [(261.625565, .4), (327.031956, .45), (392.438348, .35)]:
        inst.lib.engine_note_on(0, hz, velocity)
        parts.append(render_pcm(inst, 110250))
        inst.call('note_off', 0)
        parts.append(render_pcm(inst, 44100))
    parts.append(render_pcm(inst, 66150))
    data = np.concatenate(parts)
    data[-11025:] *= np.linspace(1, 0, 11025)[:, None]
    assert len(data) == 12 * 44100
    return data

if __name__ == '__main__':
    data = [phrase(p) for p in sys.argv[1:3]]
    with tempfile.TemporaryDirectory() as tmp:
        paths = [Path(tmp) / f'{i}.wav' for i in range(2)]
        for p, x in zip(paths, data):
            write(p, x)
        measurements = [measure(p) for p in paths]
        target = min(-25, *(m['integrated_lufs'] for m in measurements),
                     *(m['integrated_lufs'] - 6 - m['true_peak_dbfs'] for m in measurements))
        for i, m in enumerate(measurements):
            data[i] *= 10 ** ((target - m['integrated_lufs']) / 20)
            # First 250 ms: energy distribution, not a subjective quality score.
            x = data[i][:11025].mean(axis=1) * np.hanning(11025)
            f = np.fft.rfftfreq(len(x), 1/44100)
            power = np.abs(np.fft.rfft(x)) ** 2
            share = 100 * power[(f >= 2000) & (f < 6000)].sum() / power.sum()
            print(('before', 'after')[i], m, 'attack_2to6k_percent', round(share, 3))
        write(sys.argv[3], np.concatenate([data[0], np.zeros((44100, 2)), data[1]]))
        print('25 s; after at 13 s;', measure(sys.argv[3]))
