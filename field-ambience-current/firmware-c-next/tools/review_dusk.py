#!/usr/bin/env python3
"""Dusk native dry grid. Args: engine.so report.json. Each probe is 4 s.
Five notes x three velocities; report body, onset, loudness and true peak.
"""
import json
import sys
import tempfile
from pathlib import Path
import numpy as np
from calibrate_core_levels import probe, NOTES, VELOCITIES
from package_musical_review import measure, write

if __name__ == '__main__':
    rows=[]
    with tempfile.TemporaryDirectory() as tmp:
        path=Path(tmp)/'probe.wav'
        for midi in NOTES:
            for velocity in VELOCITIES:
                data,steady=probe(sys.argv[1],1,midi,velocity)
                write(path,data)
                def rms(a,b):
                    return float(20*np.log10(np.sqrt(np.mean(data[round(a*44100):round(b*44100)]**2))))
                rows.append(dict(midi=midi,velocity=velocity,steady_rms_dbfs=steady,
                                 body_rms_dbfs=rms(1,1.4),onset_rms_dbfs=rms(0,.01),
                                 **measure(path)))
    spread={str(v):max(r['body_rms_dbfs'] for r in rows if r['velocity']==v)-
                    min(r['body_rms_dbfs'] for r in rows if r['velocity']==v) for v in VELOCITIES}
    summary=dict(body_spread_db=spread,max_true_peak_dbfs=max(r['true_peak_dbfs'] for r in rows))
    Path(sys.argv[2]).write_text(json.dumps(dict(rows=rows,summary=summary),indent=2)+'\n')
    print(summary)
