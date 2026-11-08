#!/usr/bin/env python3
"""Native Resonant/Keys/Pluck vs Ensemble, engine.so report.json.

Five notes x three velocities x four cores, dry path, master .6. Compare
four-second gated/released phrases AND first-400-ms attacks; a pluck's silence
must not determine its output gain. The reference body window is 1..1.4 s.
"""
import json
import sys
import tempfile
from pathlib import Path
import numpy as np
from calibrate_core_levels import probe, NOTES, VELOCITIES
from package_musical_review import measure, write

CORES = [('Resonant', 1), ('Keys', 2), ('Ensemble', 3), ('Pluck', 6)]

def window(path, data):
    if not np.any(data):
        return dict(rms_dbfs=None, integrated_lufs=None, true_peak_dbfs=None,
                    pcm_silent=True)
    write(path, data)
    return dict(rms_dbfs=float(20*np.log10(np.sqrt(np.mean(data**2)))),
                **measure(path))

def summarize(rows):
    summary = {}
    for name, _ in CORES:
        own = [r for r in rows if r['core']==name]
        offsets = []
        for r in own:
            ref = next(q for q in rows if q['core']=='Ensemble' and
                       q['midi']==r['midi'] and q['velocity']==r['velocity'])
            # Pluck is an event: compare its attack to the established bed.
            offsets.append((r['attack']['integrated_lufs']-ref['body']['integrated_lufs'])
                           if name=='Pluck' else
                           r['full']['integrated_lufs']-ref['full']['integrated_lufs'])
        summary[name] = dict(median_offset_lu=float(np.median(offsets)),
                             min_offset_lu=min(offsets), max_offset_lu=max(offsets),
                             max_true_peak_dbfs=max(r['full']['true_peak_dbfs'] for r in own))
    return summary

if __name__ == '__main__':
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)/'probe.wav'
        for name, core in CORES:
            for midi in NOTES:
                for vel in VELOCITIES:
                    with np.errstate(divide='ignore'):
                        data, steady = probe(sys.argv[1], core, midi, vel)
                    rows.append(dict(core=name, device_id=core, midi=midi, velocity=vel,
                                     steady_rms_dbfs=steady if np.isfinite(steady) else None,
                                     full=window(path,data),
                                     attack=window(path,data[:17640]),
                                     body=window(path,data[44100:61740])))
            print(name, 'measured', flush=True)
    summary = summarize(rows)
    Path(sys.argv[2]).write_text(json.dumps(dict(rows=rows, summary=summary), indent=2)+'\n')
    print(json.dumps(summary, indent=2))
