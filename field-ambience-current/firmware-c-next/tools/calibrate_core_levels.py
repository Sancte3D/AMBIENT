#!/usr/bin/env python3
"""Measure default Storm/Mist/Orbit levels. Args: engine.so report.json.
No normalization: five registers x three velocities x three cores, dry device
path. Each probe gates 3 s then releases for 1 s; private measurement clips.
"""
import json
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write

CORES = [('Storm', 4), ('Mist', 3), ('Orbit', 5)]
NOTES = [48, 55, 60, 67, 72]
VELOCITIES = [.2, .5, .85]

def probe(library, core, midi, vel):
    inst = Instrument(library)
    for key, value in [('set_synth', core), ('set_tuning', 0),
                       ('set_fx_mode', 0), ('set_master_volume', .6)]:
        inst.call(key, value)
    render_pcm(inst, 44100)
    inst.note(0, midi, vel)
    held = render_pcm(inst, 3*44100)
    inst.call('note_off', 0)
    tail = render_pcm(inst, 44100)
    data = np.concatenate([held, tail])
    assert np.isfinite(data).all() and np.max(np.abs(data)) < 1
    steady = held[44100:3*44100]
    return data, float(20*np.log10(np.sqrt(np.mean(steady**2))))

if __name__ == '__main__':
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)/'probe.wav'
        for name, core in CORES:
            for midi in NOTES:
                for vel in VELOCITIES:
                    data, rms = probe(sys.argv[1], core, midi, vel)
                    write(path, data)
                    rows.append(dict(core=name, midi=midi, velocity=vel,
                                     steady_rms_dbfs=rms, **measure(path)))
    spreads = []
    for midi in NOTES:
        for vel in VELOCITIES:
            levels = [r['integrated_lufs'] for r in rows if r['midi']==midi and r['velocity']==vel]
            spreads.append(max(levels)-min(levels))
    summary = dict(median_spread_lu=float(np.median(spreads)),
                   max_spread_lu=float(max(spreads)),
                   max_true_peak_dbfs=max(r['true_peak_dbfs'] for r in rows))
    Path(sys.argv[2]).write_text(json.dumps(dict(rows=rows, summary=summary), indent=2)+'\n')
    print(summary)
    for name,_ in CORES:
        print(name, 'median LUFS', np.median([r['integrated_lufs'] for r in rows if r['core']==name]))
