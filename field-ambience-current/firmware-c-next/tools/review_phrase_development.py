#!/usr/bin/env python3
"""Actual ten-minute World traces and short, labelled later-phrase comparisons."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import wave

import numpy as np
from package_musical_review import measure, read
from review_coast_components import ROOT, SR


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as file:
        for block in iter(lambda: file.read(1024*1024), b''):
            h.update(block)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT/'src'/f'{s}.c' for s in ('engine_product', 'world_grammar', 'bowed', 'horn', 'pluck',
        'ambient_room', 'nature', 'dsp', 'shape', 'tuning', 'brain', 'cells')]
    driver = ROOT/'tools/render_phrase_development.c'
    inputs = [*sources, driver, Path(__file__), ROOT/'tools/package_musical_review.py', *sorted((ROOT/'include').glob('*.h'))]
    manifest = dict(scope='Actual Product Main/owner/tail/audio-start paths, 600s per World. Short montage: Home, Related, Return, each 9.5s with two 0.5s labelled comparison gaps. No invented continuous transition.',
        parameters=dict(seed=1234, key='D', collection='major', tuning='equal', activity=.5,
                        color=.5, attack=.5, release=.5, volume=.6, room=.24, nature=0),
        control_tick_ms=10, source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs}, worlds=[])
    with tempfile.TemporaryDirectory(prefix='phrase-engine-') as value:
        tmp = Path(value)
        binary = tmp/'render'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'), str(driver), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        for world, name in enumerate(('COAST', 'WOODLAND', 'HIGHLANDS')):
            raw = out/f'{name}_development_raw_600s.wav'
            report = subprocess.run([str(binary), str(world), '512', '1234', str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([str(binary), str(world), '64', '1234', str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Control-grid PCM differs across block sizes'
            trace = Path(str(raw)+'.events.csv')
            assert sha(trace) == sha(Path(str(control)+'.events.csv'))
            events = list(csv.DictReader(trace.open()))
            on = [e for e in events if int(e['on']) > 0]
            first = {kind: next(e for e in on if int(e['kind']) == kind) for kind in range(4)}
            openings = [e for i,e in enumerate(on) if not i or e['completed_phrases'] != on[i-1]['completed_phrases']]
            assert [int(e['kind']) for e in openings[:4]] == [0, 1, 2, 3]
            size = 6 if world == 1 else 8
            home = on[:size]
            for i, e in enumerate(on):
                if e['kind'] == '3':
                    index = i % size
                    assert (e['owner'], e['hz'], e['velocity']) == (home[index]['owner'], home[index]['hz'], home[index]['velocity'])
            x = read(raw)
            assert x.shape == (600*SR, 2) and np.isfinite(x).all()
            mono = float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2))
            dc = x.mean(axis=0).tolist()
            assert mono > .65 and max(map(abs, dc)) < .0002
            level = measure(raw)
            assert level['true_peak_dbfs'] <= -6
            gain = min(18., -23-level['integrated_lufs'], -6-level['true_peak_dbfs'])
            segments, clip = [], []
            # Match identical position within the initial vs recalled motif.
            # Keep one constant gain across all three excerpts of each World.
            for kind, label in ((0, 'home'), (2, 'related'), (3, 'return')):
                start = max(0, int(first[kind]['ack_frame'])-SR//10)
                n = int(9.5*SR)
                segment = x[start:start+n].copy()
                assert len(segment) == n
                edge = int(.04*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                if clip:
                    clip.append(np.zeros((SR//2, 2)))
                clip.append(segment)
                segments.append(dict(kind=label, start_seconds=start/SR, duration_seconds=9.5,
                    cut_edge_fade_ms=40, first_ack_seconds=int(first[kind]['ack_frame'])/SR))
            joined = np.concatenate(clip)*10**(gain/20)
            listen = out/f'AMBIENT_{name}_Development_compare_29s.wav'
            with wave.open(str(listen), 'wb') as wav:
                wav.setparams((2, 2, SR, 0, 'NONE', 'not compressed'))
                wav.writeframes(np.rint(np.clip(joined, -1, 32767/32768)*32768).astype('<i2').tobytes())
            checked = measure(listen)
            assert checked['true_peak_dbfs'] <= -5.9
            manifest['worlds'].append(dict(world=name, renderer_report=report, raw=level, mono=mono, dc=dc,
                pcm_64_512_identical=True, raw_sha256=sha(raw), trace_sha256=sha(trace),
                phrase_openings=[dict(kind=int(e['kind']), completed=int(e['completed_phrases']), seconds=int(e['ack_frame'])/SR) for e in openings],
                listen_file=listen.name, listen=checked, listen_sha256=sha(listen),
                constant_gain_db=gain, segments=segments, comparison_gap_seconds=.5))
            print(name, report, 'comparison', segments, flush=True)
    (out/'PHRASE_DEVELOPMENT_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('PHRASE ENGINE AUDIO PASS: 30 minutes of actual PCM at each block size, exact recalled notes, three short later-phrase comparisons; hearing/device remain open.')


if __name__ == '__main__':
    main()
