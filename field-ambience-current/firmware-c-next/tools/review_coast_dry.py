#!/usr/bin/env python3
"""Real dry Bowed A/B: fixed levels, root stability and <=30 s audition.
No Body, room, texture, diagnostic tone or adaptive normalisation.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path

import numpy as np
from scipy.signal import butter, hilbert, sosfiltfilt

from package_musical_review import measure, write
from review_bowed_motion import render

SR = 44100


def analyse(x, hz):
    mono = x.mean(axis=1)
    root = sosfiltfilt(butter(4, [hz - 8, hz + 8], btype='bandpass',
                            fs=SR, output='sos'), mono)
    envelope = abs(hilbert(root))[2 * SR:11 * SR]
    windows = envelope[:len(envelope) // 4410 * 4410].reshape(-1, 4410)
    rms = np.sqrt((windows ** 2).mean(axis=1))
    segment = mono[3 * SR:11 * SR]
    power = abs(np.fft.rfft(segment * np.hanning(len(segment)))) ** 2
    bins = np.fft.rfftfreq(len(segment), 1 / SR)
    fundamental = power[abs(bins - hz) < 8].sum()
    off = power[abs(bins - 1.5 * hz) < hz * .04].sum()
    return dict(root_swing_db=float(20 * np.log10(rms.max() / rms.min())),
                root_rms=float(np.sqrt(np.mean(root[2 * SR:11 * SR] ** 2))),
                off_1p5_root_db=float(10 * np.log10((off + 1e-30) / fundamental)),
                pcm_sha256=hashlib.sha256(x.astype('<f4').tobytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['before', 'after', 'output']:
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    libs = [C.CDLL(str(p.resolve())) for p in [args.before, args.after]]
    report = dict(sample_rate=SR, block_frames=256, probes=[])
    for colour in range(2):
        for midi in [50, 57, 62, 69]:
            for amp in [.18, .42, .58]:
                old, new = [analyse(*render(lib, colour, midi, amp, 12)) for lib in libs]
                delta = 20 * np.log10(new['root_rms'] / old['root_rms'])
                passed = bool(new['root_swing_db'] < .15 and abs(delta) < .4
                              and all(np.isfinite(list(v.values())[:-1]).all()
                                      for v in [old, new]))
                report['probes'].append(dict(colour=colour, midi=midi, amp=amp,
                                             before=old, after=new,
                                             root_gain_change_db=float(delta), passed=passed))
    clips, levels = [], []
    for colour in range(2):
        for name, lib in zip(['before', 'after'], libs):
            x, _ = render(lib, colour, 62, .42, 6, 4.0)
            edge = SR // 10
            x[-edge:] *= np.linspace(1, 0, edge)[:, None]
            path = args.output / f'colour{colour}_{name}.wav'
            write(path, x)
            clips.append(x)
            levels.append(measure(path))
            path.unlink()  # Only the user-facing comparison is exported.
    target = min([-26.] + [l['integrated_lufs'] - l['true_peak_dbfs'] - 12
                          for l in levels])
    gains = [target - l['integrated_lufs'] for l in levels]
    parts = []
    for x, gain in zip(clips, gains):
        if parts:
            parts.append(np.zeros((SR, 2)))
        parts.append(x * 10 ** (gain / 20))
    audio = args.output / 'COAST_Dry_AB_27s.wav'
    write(audio, np.concatenate(parts))
    report['audio'] = dict(seconds=27, starts_s=[0, 7, 14, 21],
                           order=['colour 0 before', 'colour 0 after',
                                  'colour 1 before', 'colour 1 after'],
                           fixed_gains_db=gains, **measure(audio))
    report['failures'] = sum(not p['passed'] for p in report['probes'])
    (args.output / 'COAST_DRY_METRICS.json').write_text(json.dumps(report, indent=2) + '\n')
    print('24 probes; failures:', report['failures'])
    print('Worst root swing:', {tag: max(p[tag]['root_swing_db'] for p in report['probes'])
                               for tag in ['before', 'after']})
    print(report['audio'])
    return int(bool(report['failures']))


if __name__ == '__main__':
    raise SystemExit(main())
