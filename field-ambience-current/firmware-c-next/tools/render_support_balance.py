#!/usr/bin/env python3
"""25 s Resonant/Keys/Pluck A/B, before.so after.so output.wav.
Each half contains the same three four-second motifs. One export gain applies
to the entire file; no per-core matching, reference tones or new effects.
"""
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_level_balance import motif
from package_musical_review import measure, write

if __name__ == '__main__':
    halves = [np.concatenate([motif(lib, core) for core in (1,2,6)])
              for lib in sys.argv[1:3]]
    data = np.concatenate([halves[0], np.zeros((44100,2)), halves[1]])
    assert data.shape==(25*44100,2) and np.isfinite(data).all()
    with tempfile.TemporaryDirectory() as tmp:
        path=Path(tmp)/'raw.wav';write(path,data);raw=measure(path)
        gain=min(0, -25-raw['integrated_lufs'], -6-raw['true_peak_dbfs'])
        write(sys.argv[3],data*10**(gain/20))
        print('Common gain dB',gain,'output',measure(sys.argv[3]))
