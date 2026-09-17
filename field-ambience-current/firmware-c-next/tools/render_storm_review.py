#!/usr/bin/env python3
"""25 s Storm native-default A/B. Args: before.so after.so output.wav.
Same note/velocity/room sequence; constant loudness matching, no post-EQ.
"""
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_fm_controls_review import phrase
from package_musical_review import measure, write

if __name__ == '__main__':
    data = [phrase(p, core=4) for p in sys.argv[1:3]]
    with tempfile.TemporaryDirectory() as tmp:
        paths = [Path(tmp) / f'{i}.wav' for i in range(2)]
        for path, audio in zip(paths, data):
            write(path, audio)
        stats = [measure(p) for p in paths]
        target = min(-25, *(m['integrated_lufs'] for m in stats),
                     *(m['integrated_lufs'] - 6 - m['true_peak_dbfs'] for m in stats))
        for i, m in enumerate(stats):
            data[i] *= 10 ** ((target - m['integrated_lufs']) / 20)
        audio = np.concatenate([data[0], np.zeros((44100, 2)), data[1]])
        assert len(audio) == 25 * 44100
        write(sys.argv[3], audio)
        print('Raw A/B:', stats)
        print('25 s; after at 13 s;', measure(sys.argv[3]))
