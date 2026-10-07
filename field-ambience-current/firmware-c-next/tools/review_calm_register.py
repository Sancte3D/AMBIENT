#!/usr/bin/env python3
"""Compare real autonomous World registers; every render/export is <=28 s.

--before baseline.so --after candidate.so --output /tmp/calm-register
Uses the existing actual-scheduler renderer in a fresh process per case.
Exports one 27.5 s A/B: Alps old/new, Open Sea old/new, 6.5 s each.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

import numpy as np
from render_musical_review import SR, worlds
from package_musical_review import read, write, measure


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    renderer = Path(__file__).with_name('review_listening_worlds.py')
    report = {'seed': '0x5EEDBA55', 'render_seconds': 28, 'generate_stop_s': 24}
    for tag, lib in [('before', a.before), ('after', a.after)]:
        report[tag] = []
        for idx, world in enumerate(worlds()):
            folder = a.output / f'{tag}_{idx}'
            subprocess.run([sys.executable, str(renderer), '--engine', str(lib.resolve()),
                            '--world', str(idx), '--output', str(folder)],
                           check=True, stdout=subprocess.DEVNULL)
            data = json.loads((folder/'measurements.json').read_text())
            melody = [e for e in data['events'] if e['on'] and e['source'] == 15]
            for e in melody:
                e['midi'] = round(69 + 12 * np.log2(e['hz'] / 440), 3)
            # A quiet 24-second window may legitimately contain only the bed.
            # Keep this visible; do not force a note or claim register coverage.
            if tag == 'after' and any(not 49.99 <= e['midi'] <= 69.01 for e in melody):
                raise RuntimeError(f'Out-of-register emitted note: {world["name"]}')
            report[tag].append({'world': world['name'], 'melody': melody,
                                'melody_observed': bool(melody),
                                'raw': data['raw'],
                                'clipped_samples': data['raw_clipped_samples']})
    # Real Generate performances, same seed, presets and timestamps. Only the
    # pitch policy differs. Match audition loudness using one FIXED gain per
    # excerpt, preserving its internal dynamics. No limiter or compression.
    excerpts, levels = [], []
    order = [(idx, tag) for idx in [0, 1] for tag in ['before', 'after']]
    for idx, tag in order:
        x = read(a.output/f'{tag}_{idx}'/'raw.wav')[round(20.5*SR):27*SR].copy()
        fade = round(.15*SR)
        x[:fade] *= np.linspace(0, 1, fade)[:, None]
        x[-fade:] *= np.linspace(1, 0, fade)[:, None]
        path = a.output/f'{tag}_{idx}_excerpt.wav'
        write(path, x)
        level = measure(path)
        levels.append(level); excerpts.append(x)
    target = min([-25.] + [-12 - v['true_peak_dbfs'] + v['integrated_lufs'] for v in levels])
    gains = [target - v['integrated_lufs'] for v in levels]
    parts = []
    for x, gain in zip(excerpts, gains):
        if parts: parts.append(np.zeros((SR//2, 2)))
        parts.append(x*10**(gain/20))
    audio = np.concatenate(parts)
    output = a.output/'AMBIENT_Calm_Register_AB_27s5.wav'
    write(output, audio)
    report['comparison'] = {'file': output.name, 'seconds': len(audio)/SR,
                            'order': ['Alps old 0 s', 'Alps new 7 s',
                                      'Open Sea old 14 s', 'Open Sea new 21 s'],
                            'excerpt_start_s': 20.5, 'target_lufs_per_excerpt': target,
                            'fixed_gains_db': gains, **measure(output)}
    (a.output/'measurements.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
