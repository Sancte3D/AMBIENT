#!/usr/bin/env python3
"""Dry Pluck measurement and a 26 s reference/candidate listening file.
No body, FX, backgrounds or per-note loudness correction. Requires numpy,
ffmpeg and a C compiler. The two excerpts receive one constant gain each.
Usage: python tools/review_woodland_dry.py --reference-source /tmp/pluck-before.c
       --output /tmp/WOODLAND_Dry_AB.wav --metrics /tmp/woodland.json
Reference source may be obtained with git show <revision>:<path/to/pluck.c>.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure, write

SR = 44100
PTR = C.POINTER(C.c_float)
ROOT = Path(__file__).resolve().parents[1]


def build(source, target):
    subprocess.run(['cc', '-shared', '-fPIC', '-O2', '-I'+str(ROOT/'include'),
                    str(source), str(ROOT/'src/shape.c'), str(ROOT/'src/dsp.c'),
                    '-lm', '-o', str(target)], check=True)
    e = C.CDLL(str(target))
    e.pluck_note_on.argtypes = [C.c_uint8, C.c_float, C.c_float]
    e.pluck_note_on.restype = C.c_bool
    e.pluck_set_damp.argtypes = [C.c_float]
    e.pluck_render_mix.argtypes = [PTR, PTR, PTR, PTR, C.c_int]
    e.dsp_init()
    e.shape_init()
    return e


def pcm(e, frames):
    a = np.zeros((4, frames), np.float32)
    e.pluck_render_mix(*[row.ctypes.data_as(PTR) for row in a], frames)
    assert np.isfinite(a).all()
    return a[:2].T.copy()


def audit(e):
    rows = []
    pan_sum = np.cos(.65*np.pi/4) + np.sin(.65*np.pi/4)
    for hz in [60, 110, 146.832, 220, 440, 1760]:
        e.pluck_init()
        e.pluck_set_damp(.42)
        observations = []
        for _ in range(12):
            assert e.pluck_note_on(0, hz, .4)
            x = pcm(e, SR).sum(axis=1)/pan_sum
            segment = x[4410:26460]
            t = np.arange(len(segment))/SR
            windowed = segment*np.hanning(len(segment))
            modes = [abs(np.dot(windowed, np.exp(-2j*np.pi*hz*k*t)))
                     for k in range(1, 6)]
            rms = float(np.sqrt(np.mean(segment**2)))
            observations.append([rms, abs(float(segment.mean()))/rms,
                                 modes[0]/max(modes[1:]), abs(float(x[0])),
                                 float(abs(x).max()),
                                 float(abs(np.diff(x[:2205])).max())])
            e.pluck_all_off()
            pcm(e, 882)
        a = np.asarray(observations)
        rows.append(dict(hz=hz, repeats=12,
                         rms_spread_db=float(20*np.log10(a[:, 0].max()/a[:, 0].min())),
                         max_window_mean_over_rms=float(a[:, 1].max()),
                         min_fundamental_over_strongest_overtone=float(a[:, 2].min()),
                         max_first_sample=float(a[:, 3].max()),
                         max_peak=float(a[:, 4].max()),
                         max_first_50ms_sample_step=float(a[:, 5].max())))
    return rows


def phrase(e):
    e.shape_init()
    e.pluck_init()
    e.pluck_set_damp(.42)
    events = [(round(.12*SR), 0, 146.832383),
              (round(2.6*SR), 1, 220.), (round(5.6*SR), 0, 164.813778)]
    parts, cursor = [], 0
    for frame, owner, hz in events:
        parts.append(pcm(e, frame-cursor))
        assert e.pluck_note_on(owner, hz, .30), 'phrase capacity was exceeded'
        cursor = frame
    parts.append(pcm(e, 12*SR-cursor))
    result = np.concatenate(parts)
    # Preserve source attacks; fade only a residual at the file's end.
    result[-8820:] *= np.linspace(1, 0, 8820)[:, None]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--metrics', type=Path, required=True)
    args = parser.parse_args()
    candidate = ROOT/'src/pluck.c'
    with tempfile.TemporaryDirectory() as directory:
        tmp = Path(directory)
        sources = [args.reference_source.resolve(), candidate]
        engines = [build(source, tmp/f'{i}.so') for i, source in enumerate(sources)]
        audits = [audit(e) for e in engines]
        excerpts = [phrase(e) for e in engines]
        raw = []
        for i, x in enumerate(excerpts):
            path = tmp/f'{i}.wav'
            write(path, x)
            raw.append(measure(path))
        target = min(-26., *(m['integrated_lufs']-m['true_peak_dbfs']-10 for m in raw))
        gain_db = [target-m['integrated_lufs'] for m in raw]
        pieces = [x*10**(g/20) for x, g in zip(excerpts, gain_db)]
        combined = np.concatenate([pieces[0], np.zeros((2*SR, 2)), pieces[1]])
        assert len(combined) == 26*SR and abs(combined).max() < 1
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write(args.output, combined)
        report = dict(duration_s=26, reference_seconds=[0, 12],
                      silence_seconds=[12, 14], candidate_seconds=[14, 26],
                      dry=True, body=False, effects=False, backgrounds=False,
                      software_only=True, damping=.42, shape_attack=.5, shape_release=.5,
                      phrase_hz=[146.832383, 220., 164.813778],
                      excerpt_target_lufs=target, excerpt_gain_db=gain_db,
                      raw_excerpt_measurements=raw, file_measurement=measure(args.output),
                      source_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in sources],
                      reference=audits[0], candidate=audits[1])
        args.metrics.parent.mkdir(parents=True, exist_ok=True)
        args.metrics.write_text(json.dumps(report, indent=2)+'\n')
        print(json.dumps({k: report[k] for k in ['duration_s', 'excerpt_gain_db',
                                              'file_measurement']}))


if __name__ == '__main__':
    main()
