#!/usr/bin/env python3
"""26 s Storm/Mist/Orbit comparison. Args: engine.so output.wav metrics.json.
Native defaults, identical C4/E4 phrase and light room; constant gain only.
Dry measurements expose source differences without the shared room.
"""
import json
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write

SR = 44100
CORES = [('Storm', 4), ('Mist', 3), ('Orbit', 5)]

def phrase(library, core, wet):
    inst = Instrument(library)
    for key, value in [('set_synth', core), ('set_tuning', 1), ('set_key', 60),
                       ('set_fx_mode', int(wet)), ('set_space', .3),
                       ('set_master_volume', .4)]:
        inst.call(key, value)
    render_pcm(inst, SR)
    parts = []
    for hz in (261.625565, 327.031956):
        inst.lib.engine_note_on(0, hz, .4)
        parts.append(render_pcm(inst, 5*SR//2))
        inst.call('note_off', 0)
        parts.append(render_pcm(inst, SR//2))
    parts.append(render_pcm(inst, 2*SR))
    data = np.concatenate(parts)
    assert data.shape == (8*SR, 2) and np.isfinite(data).all()
    assert np.max(np.abs(data)) < 1
    # The old broken exporter left most of every long request silent.
    assert np.sqrt(np.mean(data[SR:2*SR]**2)) > .0001
    data[-SR//4:] *= np.linspace(1, 0, SR//4)[:, None]
    return data

def source_metrics(data):
    held = data[SR//2:2*SR]
    mono = held.mean(axis=1)
    power = np.abs(np.fft.rfft(mono*np.hanning(len(mono))))**2
    f = np.fft.rfftfreq(len(mono), 1/SR)
    mid = (held[:,0]+held[:,1])/2
    side = (held[:,0]-held[:,1])/2
    early = np.mean(data[:SR//20]**2)
    steady = np.mean(data[SR:2*SR]**2)
    return dict(dry_centroid_hz=float(np.sum(f*power)/power.sum()),
                dry_side_to_mid_db=float(10*np.log10(np.mean(side**2)/np.mean(mid**2)))
                if np.mean(side**2)>1e-16 else None,
                early_50ms_to_steady_db=float(10*np.log10(early/steady)))

if __name__ == '__main__':
    library, output, report = sys.argv[1:4]
    audio = [phrase(library, core, True) for _, core in CORES]
    metrics = [source_metrics(phrase(library, core, False)) for _, core in CORES]
    with tempfile.TemporaryDirectory() as tmp:
        stats = []
        for i, data in enumerate(audio):
            path = Path(tmp)/f'{i}.wav';write(path, data);stats.append(measure(path))
        target = min(-25, *(m['integrated_lufs']-6-m['true_peak_dbfs'] for m in stats))
        rows = []
        for i, ((name, _), data, stats_i) in enumerate(zip(CORES, audio, stats)):
            gain = target-stats_i['integrated_lufs']
            audio[i] = data*10**(gain/20)
            path = Path(tmp)/f'matched{i}.wav';write(path, audio[i])
            matched = measure(path)
            assert abs(matched['integrated_lufs']-target)<.2
            assert matched['true_peak_dbfs']<=-6
            rows.append(dict(name=name, start_s=i*9, raw=stats_i,
                             gain_db=gain, matched=matched, **metrics[i]))
        pause = np.zeros((SR, 2))
        joined = np.concatenate([audio[0], pause, audio[1], pause, audio[2]])
        assert len(joined)==26*SR
        write(output, joined)
        result=dict(duration_s=26, target_lufs=target, cores=rows, output=measure(output))
        Path(report).write_text(json.dumps(result, indent=2, allow_nan=False)+'\n')
        print(json.dumps(result, indent=2))
