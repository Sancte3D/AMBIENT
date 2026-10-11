#!/usr/bin/env python3
"""SD33: Color endpoints in actual held ensembles; no firmware calibration."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure
from review_activity_comparison import sha, complete_pcm, pcm_metrics
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = 'c89bc630fec5a9e193f06a4e08d8e6752a16b3b2'
TREE = 'b3d1db9e8d9fe6f5c972147fd4df47d331f69a3d'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
VALUES = (0., .5, 1.)
SECONDS, STOP, BEGIN, CLIP = 112, 80, 3., 12.5


def trace(path):
    with path.open() as file:
        return list(csv.DictReader(file))


def spectrum(x):
    size = 16384
    pieces = x[:len(x)//size*size].reshape(-1, size, 2)
    power = np.sum(np.abs(np.fft.rfft(pieces*np.hanning(size)[None, :, None], axis=1))**2, axis=(0, 2))
    frequency = np.fft.rfftfreq(size, 1/SR)
    total = float(power.sum())
    assert total > 0 and np.isfinite(power).all()
    return dict(power_weighted_centroid_hz=float(np.sum(frequency*power)/total),
        energy_fraction_above_1000_hz=float(power[frequency >= 1000].sum()/total),
        energy_fraction_above_4000_hz=float(power[frequency >= 4000].sum()/total),
        diagnostic_only_not_hearing_acceptance=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    assert subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=ROOT, text=True).strip() == TREE
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')], cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in ('.c', '.h', '.s', '.ld', '.cmake') or Path(p).name == 'CMakeLists.txt']
    assert compiled
    original = {}
    for relative in compiled:
        data = subprocess.check_output(['git', 'show', f'{BASE}:{relative}'], cwd=ROOT)
        assert (repo/relative).read_bytes() == data, f'Changed PR156 compilation input: {relative}'
        original[relative] = hashlib.sha256(data).hexdigest()
    sources = [ROOT/'src'/f'{n}.c' for n in SOURCES]
    inputs = [*sources, *sorted((ROOT/'include').glob('*.h')), Path(__file__),
        ROOT/'tools/render_color_comparison.c', ROOT/'tools/render_nature_comparison.c',
        ROOT/'tools/review_activity_comparison.py', ROOT/'tools/review_room_comparison.py',
        ROOT/'tools/package_musical_review.py', ROOT/'test/test_product_sound.c']
    manifest = dict(scope='Spec0.9 unchanged actual Product engine; Color0/0.5/1 and live controls. '
        'Matched held ensemble, normal sources/score/envelopes/Room, no replacement soundtrack.',
        baseline_commit=BASE, baseline_tree=TREE, compilation_inputs_unchanged_from_pr156=True,
        compilation_input_count=len(compiled), firmware_changed=False, hearing_accepted=False,
        range_usefulness_accepted=False, full_pcm_retained=False,
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        parameters=dict(color_values=list(VALUES), default_color=.5, activity=.5, room=.5, nature=0,
                        attack=.5, release=.5, volume=.6, seeds=[1234, 42], key='D', collection='major', tuning='equal'),
        raw_seconds=SECONDS, generate_stop_seconds=STOP, block_sizes=[64, 512],
        silent_settling_seconds=1, control_tick_ms=10, worlds=[], live=[])
    with tempfile.TemporaryDirectory(prefix='color-ensemble-review-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(ROOT/'tools/render_color_comparison.c'), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        check = tmp/'product-contract'
        subprocess.run([*flags, str(ROOT/'test/test_product_sound.c'), *map(str, sources), '-lm', '-o', str(check)], check=True)
        manifest['product_contract_regression'] = subprocess.run([str(check)], check=True, capture_output=True, text=True).stdout.strip()
        print(manifest['product_contract_regression'], flush=True)
        baseline_driver = tmp/'baseline-render'
        subprocess.run([*flags, str(ROOT/'tools/render_nature_comparison.c'), *map(str, sources), '-lm', '-o', str(baseline_driver)], check=True)
        sealed, files, fixed = [], {}, {}

        def render(world, seed, color, live):
            name = WORLDS[world]
            suffix = 'live' if live else f'color{round(color*100):03d}'
            raw = captures/f'{name}_seed{seed}_{suffix}_raw_112s.wav'
            command = [str(binary), str(world), str(seed), str(color), str(int(live))]
            report = subprocess.run([*command, '512', str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([*command, '64', str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Full PCM differs at64/512'
            hashes = {}
            for extension in ('.events.csv', '.state.csv'):
                path = Path(str(raw)+extension)
                assert sha(path) == sha(Path(str(control)+extension)), 'Full trace differs at64/512'
                hashes[extension] = sha(path)
            events = trace(Path(str(raw)+'.events.csv'))
            states = trace(Path(str(raw)+'.state.csv'))
            assert len(states) == SECONDS*100
            assert max(int(r['active_sources']) for r in states) <= (2 if world == 1 else 3)
            assert all(r['active_sources'] == '0' for r in states[-100:])
            assert int(states[-1]['completed_phrases']) >= 1
            owners, durations = {}, []
            for row in events:
                owner, frame = int(row['owner']), int(row['ack_frame'])
                if row['on'] == '1':
                    assert owner not in owners
                    owners[owner] = frame
                else:
                    duration = (frame-owners.pop(owner))/SR
                    if frame < STOP*SR:
                        durations.append(duration)
            assert not owners and durations
            pcm = complete_pcm(raw, SECONDS)
            assert not np.any(pcm[-SR:]), 'Stop did not become exactly silent'
            metrics, level = pcm_metrics(pcm), measure(raw)
            assert level['true_peak_dbfs'] <= -6 and metrics['mono_energy_ratio'] > .65
            assert max(map(abs, metrics['dc'])) < .0002
            held = np.asarray(pcm[6*SR:round(12.5*SR)], dtype=np.float64)/32768
            first_free = next(int(r['frame'])/SR for r in states[STOP*100:] if r['active_sources'] == '0')
            row = dict(seed=seed, initial_color=color, live_changes=live, renderer_report=report,
                raw_level=level, **metrics, held_spectrum_6_to_12p5_seconds=spectrum(held),
                starts=sum(r['on'] == '1' for r in events), completed_phrases=int(states[-1]['completed_phrases']),
                maximum_actual_sources=max(int(r['active_sources']) for r in states),
                full_owner_retirement_after_stop_seconds=first_free,
                acknowledged_hold_seconds_range=[min(durations), max(durations)],
                raw_sha256=sha(raw), events_sha256=hashes['.events.csv'], state_sha256=hashes['.state.csv'],
                pcm_events_state_64_512_identical=True, final_second_exact_silence=True)
            if live:
                row['changes'] = []
                for second, target in [(10, 0), (20, 1), (32, .5), (48, 0), (64, 1), (72, .5)]:
                    center = second*SR
                    segment = np.asarray(pcm[center-SR//50:center+SR//50], dtype=np.float64)/32768
                    step = float(np.max(np.abs(np.diff(segment, axis=0))))
                    assert step < .05, 'Unbounded sample step near Color control change'
                    row['changes'].append(dict(seconds=second, target_color=target, maximum_adjacent_sample_step=step))
                row['setter_caused_immediate_note_hook'] = False
            files[world, seed, color, live] = raw
            sealed.append((raw, row))
            print(name, seed, suffix, f"{row['starts']} starts; full64/512 PCM and traces equal; Stop silent", flush=True)
            return row

        for world, name in enumerate(WORLDS):
            cases = []
            for seed in (1234, 42):
                matched = None
                for color in VALUES:
                    row = render(world, seed, color, False)
                    hashes = row['events_sha256'], row['state_sha256']
                    if matched is None:
                        matched = hashes
                    assert hashes == matched, 'Color changed actual notes, holds, owners, phrase timing or admission'
                    row['all_color_values_complete_events_state_identical'] = True
                    cases.append(row)
                centroids = [r['held_spectrum_6_to_12p5_seconds']['power_weighted_centroid_hz'] for r in cases[-3:]]
                assert centroids[0] < centroids[1] < centroids[2], 'Color brightness direction reversed'
                default = files[world, seed, .5, False]
                before = tmp/'baseline-default.wav'
                subprocess.run([str(baseline_driver), 'engine', str(world), str(seed), '0', '.5', '512', str(before)], check=True, capture_output=True)
                complete_pcm(before, SECONDS)
                assert sha(default) == sha(before), 'New driver changed normal default audio'
                assert sha(Path(str(default)+'.events.csv')) == sha(Path(str(before)+'.events.csv'))
                after_state = trace(Path(str(default)+'.state.csv'))
                for state in after_state:
                    state.pop('memory_returns')
                assert after_state == trace(Path(str(before)+'.state.csv'))
                cases[-2]['default_pcm_events_shared_state_equal_existing_driver'] = True
            normal = [files[world, 1234, color, False] for color in VALUES]
            segments = [np.asarray(complete_pcm(p, SECONDS)[round(BEGIN*SR):round((BEGIN+CLIP)*SR)], dtype=np.float64)/32768 for p in normal]
            rms = float(np.sqrt(np.mean(segments[1]**2)))
            differences = [float(np.sqrt(np.mean((x-segments[1])**2))/rms) for x in segments]
            centroids = [r['held_spectrum_6_to_12p5_seconds']['power_weighted_centroid_hz'] for r in cases[:3]]
            contrast = (centroids[2]-centroids[0])/centroids[1]
            levels = []
            edge = round(.04*SR)
            for index, x in enumerate(segments):
                x[:edge] *= np.linspace(0, 1, edge)[:, None]
                x[-edge:] *= np.linspace(1, 0, edge)[:, None]
                path = tmp/f'part-{index}.wav'
                write(path, x)
                levels.append(measure(path))
            gain = min(18., -23-max(r['integrated_lufs'] for r in levels), -6-max(r['true_peak_dbfs'] for r in levels))
            parts, sections = [], []
            for index, (color, segment) in enumerate(zip(VALUES, segments)):
                segment *= 10**(gain/20)
                path = tmp/f'gain-{index}.wav'
                write(path, segment)
                level = measure(path)
                assert level['true_peak_dbfs'] <= -5.9
                if parts:
                    parts.append(np.zeros((SR//2, 2)))
                parts.append(segment)
                sections.append(dict(color=color, output_seconds=[index*(CLIP+.5), index*(CLIP+.5)+CLIP],
                    source_seconds=[BEGIN, BEGIN+CLIP], delivered_level=level))
            listen = captures/f'AMBIENT_{name}_Color_0_50_100_38p5s.wav'
            write(listen, np.concatenate(parts))
            complete_pcm(listen, 38.5)
            manifest['worlds'].append(dict(world=name, cases=cases, listen_file=listen.name, listen_sha256=sha(listen),
                listen_level=measure(listen), sections=sections, identical_fixed_gain_db_all_sections=gain,
                comparison_gap_seconds=.5, cut_edge_fade_ms=40,
                raw_rms_difference_to_default_same_held_excerpt_fraction=differences,
                held_power_centroid_min_max_fraction_of_default=contrast,
                measured_weak_woodland_color_range_requires_calibration=world == 1,
                note='RMS/FFT are diagnostics. No perceptual audibility, soothing or musical usefulness is accepted.'))
            if world == 1:
                assert max(differences) < .01 and contrast < .01, 'Published weak-range finding changed; review it'
            row = render(world, 1234, .5, True)
            base = cases[1]
            assert (row['events_sha256'], row['state_sha256']) == (base['events_sha256'], base['state_sha256']), 'Live Color changed actual music or holds'
            row['world'] = name
            row['live_complete_events_state_identical_to_fixed_default'] = True
            manifest['live'].append(row)
            print(name, 'matched Color excerpts packaged with one fixed gain', flush=True)
        for path, row in sealed:
            complete_pcm(path, SECONDS)
            assert sha(path) == row['raw_sha256'], 'Capture changed after measurement'
            assert sha(Path(str(path)+'.events.csv')) == row['events_sha256']
            assert sha(Path(str(path)+'.state.csv')) == row['state_sha256']
        for relative, expected in original.items():
            assert sha(repo/relative) == expected, 'Compilation input changed during capture'
        for relative, expected in manifest['source_sha256'].items():
            assert sha(ROOT/relative) == expected, 'Audit input changed during capture'
        for row in manifest['worlds']:
            path = captures/row['listen_file']
            complete_pcm(path, 38.5)
            assert sha(path) == row['listen_sha256']
        for path in sorted(captures.iterdir()):
            if path.name.endswith(('.events.csv', '.state.csv')) or '_Color_0_50_100_' in path.name:
                target = out/path.name
                shutil.copyfile(path, target)
                assert sha(path) == sha(target), 'Incomplete export'
        for row in manifest['worlds']:
            complete_pcm(out/row['listen_file'], 38.5)
    manifest['actual_color_render_count'] = 42
    manifest['existing_default_driver_control_render_count'] = 6
    manifest['total_full_render_minutes'] = 48*SECONDS/60
    (out/'SD33_COLOR_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('COLOR PASS:18 fixed +3 live complete actual performances at64/512, six existing-driver default controls, '
          'retained full musical traces, three short WAVs. WOODLAND range is weak; hearing and calibration remain open.')


if __name__ == '__main__':
    main()
