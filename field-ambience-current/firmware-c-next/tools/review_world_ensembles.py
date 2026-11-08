#!/usr/bin/env python3
"""Distinct seeded WOODLAND/HIGHLANDS host algorithms using real Product DSP.

Production admission, tail ownership and macro integration remain separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import shutil
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure, read
from review_coast_components import ROOT, SR


def plan(world, seed):
    rng = random.Random(seed)
    events = []
    if world == 'WOODLAND':
        # Three related figures: call, upper answer, quieter harmonic return.
        # Same recognizable rhythm, unequal inter-note gaps and natural rings.
        shift = rng.choice((2, 0, -2))
        rhythm = (0, 2.0 + rng.randrange(21)/100, 5.0 + rng.randrange(11)/100)
        figures = ((0, (62, 66, 69)), (8, (74, 71, 66)), (16, (59, 62, 66)))
        for start, notes in figures:
            for step, midi in enumerate(notes):
                owner = len(events) % 2
                velocity = (.66, .50, .59)[step] - (0.08 if start == 16 else 0)
                events.append((round(start+rhythm[step], 2), 1, owner,
                               midi+shift, velocity, .45 + rng.randrange(11)/100))
        seconds, voices, release = 30, 2, .60
        # Reusing an owner must not reset a ringing Karplus-Strong line.
        for owner in range(voices):
            owned = [e for e in events if e[2] == owner]
            assert all(round(b[0]-a[0], 2) >= 4.9 for a, b in zip(owned, owned[1:]))
        assert [e[3]-shift for e in events[:3]] == [62, 66, 69]
        assert [round(e[0]-8, 2) for e in events[3:6]] == [e[0] for e in events[:3]]
    else:
        assert world == 'HIGHLANDS'
        # D major opens into B minor while F#4 stays in the same real voice.
        # A generous silence precedes the wide D-major reprise.
        shift = rng.choice((2, 0, -2))
        lag = rng.randrange(21)/100
        notes = ((0, 0, 50, 6, .57), (.8, 1, 69, 7, .43),
                 (1.6, 2, 66, 17, .47), (10+lag, 0, 59, 7, .50),
                 (11+lag, 1, 62, 7, .52), (28, 0, 50, 7.5, .54),
                 (29, 1, 69, 7.2, .40), (30, 2, 66, 7.2, .46))
        for onset, owner, midi, hold, velocity in notes:
            events.extend(((onset, 1, owner, midi+shift, velocity, .55),
                           (round(onset+hold, 2), 0, owner, midi+shift, 0, 0)))
        seconds, voices, release = 44, 3, 0
        events.sort(key=lambda e: (e[0], e[1], e[2]))
        for owner in range(voices):
            owned = [e for e in events if e[2] == owner]
            assert [e[1] for e in owned] == [1, 0]*(len(owned)//2)
            assert all(on[0]-off[0] >= 3 for off, on in zip(owned[1::2], owned[2::2]))
        assert any(e[1] and e[2] == 2 and e[0] == 1.6 for e in events)
        assert any(not e[1] and e[2] == 2 and e[0] == 18.6 for e in events)
    return dict(world=world, seed=seed, transpose=shift, seconds=seconds,
                voices=voices, release=release, events=events)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def render(out, world, seed):
    chosen = plan(world, seed)
    for other in range(100):
        assert plan(world, other) == plan(world, other)
    assert any(plan(world, other)['events'] != chosen['events'] for other in (1, 42, 91267))
    template = ROOT/'tools/render_ensemble_sketch.c'
    driver = template.read_text()
    score = 'static const event_t score[]={\n' + ''.join(
        '    {%.2ff,%d,%d,%d,%.3ff,%.3ff},\n' % e for e in chosen['events']) + '};'
    driver, count = re.subn(r'static const event_t score\[\]=\{.*?\n\};',
                           lambda _: score, driver, count=1, flags=re.S)
    assert count == 1
    source = 'pluck' if world == 'WOODLAND' else 'horn'
    starts = sum(e[1] for e in chosen['events'])
    replacements = {'SECONDS=27': f'SECONDS={chosen["seconds"]}',
                    'starts==10': f'starts=={starts}',
                    'sources<=3': f'sources<={chosen["voices"]}',
                    'max_sources==3': f'max_sources=={chosen["voices"]}',
                    'shape_set_release(0)': f'shape_set_release({chosen["release"]:.2f}f)'}
    for old, new in replacements.items():
        assert driver.count(old) == 1
        driver = driver.replace(old, new)
    # The event trace must retain the actual direct release used by this source.
    driver = driver.replace('%.3f,%.3f,0\\n',
                            f'%.3f,%.3f,{chosen["release"]:.2f}\\n')
    driver = driver.replace('bowed', source)
    if source == 'pluck':
        driver = driver.replace('pluck_try_note_on', 'pluck_note_on').replace(
            'pluck_render_mix(l,r,sl,sr,n,.35f)', 'pluck_render_mix(l,r,sl,sr,n)')
    sources = [ROOT/'src'/f'{name}.c' for name in (source, 'ambient_room', 'shape', 'dsp')]
    manifest = dict(plan=chosen, plan_checks=100,
        scope='Seeded host algorithm with unchanged actual Product source/Room C DSP; not production generator integration.',
        limits='Direct SHAPE values and register extension bypass Product macro/admission limits. No claim of final hearing or device acceptance.',
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in
                       [*sources, template, Path(__file__), ROOT/'tools/package_musical_review.py',
                        *sorted((ROOT/'include').glob('*.h'))]},
        driver_sha256=hashlib.sha256(driver.encode()).hexdigest(), files=[])
    with tempfile.TemporaryDirectory(prefix='ambient-worlds-') as temp:
        tmp = Path(temp)
        c = tmp/'driver.c'; c.write_text(driver)
        binary = tmp/'render'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include'), str(c),
                        *map(str, sources), '-lm', '-o', str(binary)], check=True)
        for mode in ('dry', 'room'):
            raw = out/f'AMBIENT_{world}_{mode}_raw_{chosen["seconds"]}s.wav'
            report = subprocess.run([str(binary), mode, '512', str(raw)],
                                    check=True, capture_output=True, text=True).stderr
            control = tmp/'control.wav'
            subprocess.run([str(binary), mode, '64', str(control)], check=True, capture_output=True)
            assert raw.read_bytes() == control.read_bytes()
            assert Path(str(raw)+'.events.csv').read_bytes() == Path(str(control)+'.events.csv').read_bytes()
            x = read(raw)
            assert x.shape == (chosen['seconds']*SR, 2) and np.isfinite(x).all()
            mono = float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2))
            dc = x.mean(axis=0).tolist()
            assert mono > .65 and max(map(abs, dc)) < .0002
            level = measure(raw)
            assert level['true_peak_dbfs'] <= -6
            gain = min(18., -23-level['integrated_lufs'], -6-level['true_peak_dbfs'])
            listen = out/raw.name.replace('_raw_', '_listen_')
            subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', str(raw), '-af',
                            f'volume={gain:.8f}dB', '-c:a', 'pcm_s16le', str(listen)], check=True)
            checked = measure(listen)
            assert checked['true_peak_dbfs'] <= -5.9
            manifest['files'].append(dict(mode=mode, listen_file=listen.name,
                raw=level, listen=checked, constant_gain_db=gain, mono_energy_ratio=mono,
                dc_lr=dc, pcm_64_512_identical=True, renderer_report=report.strip(),
                raw_sha256=sha(raw), listen_sha256=sha(listen)))
            print(world, mode, checked, report.strip(), flush=True)
    (out/f'{world}_ENSEMBLE_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--seed', type=int, default=1234)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert shutil.which('cc') and shutil.which('ffmpeg')
    for world in ('WOODLAND', 'HIGHLANDS'):
        render(out, world, args.seed)
    print('WORLD ENSEMBLES PASS: distinct plans, strict actual voice admission, natural source ends, 64/512 exact; production integration/hearing open.')


if __name__ == '__main__':
    main()
