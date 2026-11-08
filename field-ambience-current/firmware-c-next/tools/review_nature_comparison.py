#!/usr/bin/env python3
"""SD28: actual engine Nature on/off, matched held and genuine later quiet passages."""
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
from package_musical_review import measure, read
from review_room_comparison import ROOT, SR, SOURCES, mono_ratio, sha, write

BASE = 'e34bcf372637c6e3db239849550d7bd4bb015bdc'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
AMOUNTS = (0., .35, 1.)
SECONDS = 112
STOP = 80
CLIP = 6.5


def quiet_window(states, music):
    """A real source pause, or the quietest later musical transition.

    Continuous Worlds may have no long source pause. Preserve that musical
    behavior instead of fabricating a gap or presenting a release as silence.
    """
    runs = []
    start = None
    for row in states:
        frame = int(row['frame'])
        quiet = 3*SR <= frame < STOP*SR and int(row['active_sources']) == 0
        if quiet and start is None:
            start = frame
        if not quiet and start is not None:
            runs.append((start, frame))
            start = None
    a, b = max(runs, key=lambda pair: pair[1]-pair[0]) if runs else (0, 0)
    length = round(CLIP*SR)
    if b-a >= length:
        begin = a
        selection = 'actual complete source pause; Room may still ring'
    else:
        candidates = np.arange(16*SR, STOP*SR-length+1, SR//100)
        cumulative = np.concatenate(([0.], np.cumsum(np.sum(music*music, axis=1))))
        energies = cumulative[candidates+length]-cumulative[candidates]
        begin = int(candidates[np.argmin(energies)])
        selection = 'quietest actual later 6.5s music transition, not a complete source pause'
    assert begin >= 0 and begin+length <= STOP*SR
    observed = [int(row['active_sources']) for row in states
                if begin <= int(row['frame']) < begin+length]
    assert len(observed) == 650
    return begin, dict(selection=selection, longest_actual_source_free_interval_seconds=[a/SR, b/SR],
        excerpt_seconds=[begin/SR, (begin+length)/SR],
        entire_excerpt_source_free=max(observed) == 0,
        source_free_seconds_in_excerpt=sum(count == 0 for count in observed)/100,
        actual_source_count_range=[min(observed), max(observed)])


def residual_metrics(x):
    energy = float(np.sum(x*x))
    rms = float(np.sqrt(np.mean(x*x)))
    result = dict(rms_dbfs=float(20*np.log10(max(rms, 1e-15))),
        peak=float(np.max(np.abs(x))), mono_energy_ratio=mono_ratio(x),
        dc=x.mean(axis=0).tolist())
    # Diagnostic band energy, not a perceptual masking or soothing rating.
    size = 16384
    segments = x[:len(x)//size*size].reshape(-1, size, 2)
    power = np.sum(np.abs(np.fft.rfft(segments*np.hanning(size)[None, :, None], axis=1))**2,
                   axis=(0, 2))
    frequencies = np.fft.rfftfreq(size, 1/SR)
    total = float(power.sum())
    result['windowed_band_energy_fractions'] = {
        name: float(power[(frequencies >= lo) & (frequencies < hi)].sum()/total)
        for name, lo, hi in [('below_100_hz', 0, 100), ('100_1000_hz', 100, 1000),
                             ('1000_4000_hz', 1000, 4000), ('above_4000_hz', 4000, SR/2+1)]}
    assert energy > 0 and total > 0 and np.isfinite(x).all()
    return result


def excerpt_level(path, segment):
    # An actual pause can be exactly silent; ffmpeg then reports Peak -inf.
    # Retain that fact rather than inventing a loudness or amplifying silence.
    if not np.any(np.rint(segment*32768)):
        return dict(integrated_lufs=None, true_peak_dbfs=None, exact_silence=True)
    return measure(path)


def complete_pcm(path, seconds):
    assert path.stat().st_size == 44+round(seconds*SR)*4, f'Incomplete capture: {path}'
    x = read(path)
    assert x.shape == (round(seconds*SR), 2) and np.isfinite(x).all()
    return x


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT/'src'/f'{s}.c' for s in SOURCES]
    driver = ROOT/'tools/render_nature_comparison.c'
    inputs = [*sources, driver, Path(__file__), ROOT/'tools/review_room_comparison.py',
              ROOT/'tools/package_musical_review.py', ROOT/'test/test_room_nature.c',
              *sorted((ROOT/'include').glob('*.h'))]
    # Verify the published sound rather than rebuilding an invented substitute.
    reference = json.loads((ROOT.parent/'docs/audio/SD19_ROOM_CALIBRATED_AUDIT.json').read_text())
    for path, expected in reference['source_sha256'].items():
        assert sha(ROOT/path) == expected, f'Changed PR153 sound input: {path}'
    baseline = subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=ROOT, text=True).strip()
    assert baseline == 'f43cda115e55bca5e2652b71160bf1685be1c6d9'
    manifest = dict(scope='Normal actual Product Engine; optional Nature changes only. '
        'Same notes, owners, actual source counts and phrase histories. Hearing remains open.',
        baseline_commit=BASE, baseline_tree=baseline, sound_inputs_unchanged_from_pr153=True,
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        parameters=dict(room=.5, nature_candidate=.35, nature_extreme=1., volume=.6,
            activity=.5, color=.5, attack=.5, release=.5, key='D', collection='major',
            tuning='equal', seeds=[1234, 42]), raw_seconds=SECONDS, generate_stop_seconds=STOP,
        silent_settling_seconds=1, control_tick_ms=10, block_sizes=[64, 512],
        hearing_accepted=False, nature_default=0, worlds=[], location=[])
    with tempfile.TemporaryDirectory(prefix='nature-review-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        manifest['full_pcm_retained'] = False
        manifest['export_scope'] = 'Three short WAVs, metrics, complete note and owner/phrase traces. Full PCM verified in temporary storage.'
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
                 '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(driver), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        regression = tmp/'room-nature'
        subprocess.run([*flags, str(ROOT/'test/test_room_nature.c'), *map(str, sources), '-lm',
                        '-o', str(regression)], check=True)
        manifest['room_nature_regression'] = subprocess.run([str(regression)], check=True,
            capture_output=True, text=True).stdout.strip()
        print(manifest['room_nature_regression'], flush=True)
        for world, name in enumerate(WORLDS):
            cases = []
            normal = {}
            for seed in (1234, 42):
                matched = None
                for amount in AMOUNTS:
                    raw = captures/f'{name}_seed{seed}_nature{round(amount*100):03d}_raw_112s.wav'
                    command = [str(binary), 'engine', str(world), str(seed), str(amount), '.5']
                    report = subprocess.run([*command, '512', str(raw)], check=True,
                                            capture_output=True, text=True).stderr.strip()
                    control = tmp/'control.wav'
                    subprocess.run([*command, '64', str(control)], check=True, capture_output=True)
                    assert sha(raw) == sha(control), 'PCM differs at 64/512'
                    fingerprints = []
                    for suffix in ('.events.csv', '.state.csv'):
                        path = Path(str(raw)+suffix)
                        assert sha(path) == sha(Path(str(control)+suffix)), 'Trace differs at 64/512'
                        fingerprints.append(sha(path))
                    if matched is None:
                        matched = fingerprints
                    assert fingerprints == matched, 'Nature changed actual musical score or owners'
                    x = complete_pcm(raw, SECONDS)
                    assert not np.any(x[-SR:]), 'Stop failed to become genuinely silent'
                    level = measure(raw)
                    assert level['true_peak_dbfs'] <= -6 and mono_ratio(x) > .65
                    row = dict(seed=seed, amount=amount, renderer_report=report, raw_level=level,
                        mono_energy_ratio=mono_ratio(x), dc=x.mean(axis=0).tolist(),
                        raw_sha256=sha(raw), events_sha256=fingerprints[0], state_sha256=fingerprints[1],
                        pcm_events_state_64_512_identical=True, all_nature_amounts_score_state_identical=True,
                        final_second_exact_silence=True)
                    assert max(map(abs, row['dc'])) < .0002
                    cases.append(row)
                    if seed == 1234:
                        normal[amount] = raw
                    print(name, seed, amount, 'matched full PCM/score/state; Stop silent', flush=True)
            states = list(csv.DictReader(Path(str(normal[0.])+'.state.csv').open()))
            off, on = complete_pcm(normal[0.], SECONDS), complete_pcm(normal[.35], SECONDS)
            quiet_start, quiet = quiet_window(states, off)
            held_start = 6*SR
            delta = on-off
            held = slice(held_start, held_start+round(CLIP*SR))
            later = slice(quiet_start, quiet_start+round(CLIP*SR))
            signal_rms = float(np.sqrt(np.mean(off[held]**2)))
            nature_rms = float(np.sqrt(np.mean(delta[held]**2)))
            assert signal_rms > 0 and 0 < nature_rms < signal_rms
            segments = [off[held].copy(), on[held].copy(), off[later].copy(), on[later].copy()]
            edge = round(.04*SR)
            levels = []
            for i, segment in enumerate(segments):
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                path = tmp/f'part-{i}.wav'
                write(path, segment)
                levels.append(excerpt_level(path, segment))
            # ONE identical gain for Nature-off/on, held/quiet. Do not make
            # the music quieter when adding Nature, or turn up a pause alone.
            gain = min(-23-levels[0]['integrated_lufs'], 18.,
                       *[-6-level['true_peak_dbfs'] for level in levels
                         if level['true_peak_dbfs'] is not None])
            delivered = []
            parts = []
            for i, segment in enumerate(segments):
                segment *= 10**(gain/20)
                path = tmp/f'gain-{i}.wav'
                write(path, segment)
                level = excerpt_level(path, segment)
                assert level['true_peak_dbfs'] is None or level['true_peak_dbfs'] <= -5.9
                delivered.append(level)
                if parts:
                    parts.append(np.zeros((SR//2, 2)))
                parts.append(segment)
            listen = captures/f'AMBIENT_{name}_Nature_AB_27p5s.wav'
            write(listen, np.concatenate(parts))
            complete_pcm(listen, 27.5)
            manifest['worlds'].append(dict(world=name, cases=cases,
                held_excerpt_seconds=[6., 12.5], quiet_comparison=quiet,
                held_nature_difference_to_music_rms_ratio=nature_rms/signal_rms,
                nature_difference_held=residual_metrics(delta[held]),
                nature_difference_quiet=residual_metrics(delta[later]),
                identical_fixed_gain_db_all_sections=gain, delivered_sections=delivered,
                comparison_gap_seconds=.5, cut_edge_fade_ms=40, listen_file=listen.name,
                listen_sha256=sha(listen), listen=measure(listen)))
            print(name, 'packaged held + actual release/pause:', quiet, flush=True)
            # Nature-only through the actual Engine: no phantom voices and
            # exactly the same output at Dry / maximum Room.
            location = captures/f'{name}_location_raw_32s.wav'
            command = [str(binary), 'location', str(world), '1234', '.35']
            report = subprocess.run([*command, '0', '512', str(location)], check=True,
                                    capture_output=True, text=True).stderr.strip()
            for room, block in (('0', '64'), ('1', '512')):
                check = tmp/'location-check.wav'
                subprocess.run([*command, room, block, str(check)], check=True, capture_output=True)
                assert sha(location) == sha(check), 'Room changed Nature-only actual output'
                for suffix in ('.events.csv', '.state.csv'):
                    assert sha(Path(str(location)+suffix)) == sha(Path(str(check)+suffix))
            manifest['location'].append(dict(world=name, renderer_report=report,
                raw_sha256=sha(location), level=measure(location), room0_1_and_64_512_identical=True,
                phantom_note_count=0, nature_only_metrics=residual_metrics(complete_pcm(location, 32)[6*SR:12*SR])))
        # Seal every complete capture against its previously measured bytes,
        # including files read again to assemble the short listening montages.
        for world in manifest['worlds']:
            for row in world['cases']:
                path = captures/f"{world['world']}_seed{row['seed']}_nature{round(row['amount']*100):03d}_raw_112s.wav"
                complete_pcm(path, SECONDS)
                assert sha(path) == row['raw_sha256'], 'Capture changed after measurement'
                assert sha(Path(str(path)+'.events.csv')) == row['events_sha256']
                assert sha(Path(str(path)+'.state.csv')) == row['state_sha256']
            complete_pcm(captures/world['listen_file'], 27.5)
            assert sha(captures/world['listen_file']) == world['listen_sha256']
        for row in manifest['location']:
            path = captures/f"{row['world']}_location_raw_32s.wav"
            complete_pcm(path, 32)
            assert sha(path) == row['raw_sha256']
        # Export only completed short audio and complete diagnostics. Large
        # raw performances are proof inputs, not user-facing deliverables.
        for path in sorted(captures.iterdir()):
            if path.name.endswith(('.events.csv', '.state.csv')) or '_Nature_AB_' in path.name:
                target = out/path.name
                shutil.copyfile(path, target)
                assert sha(target) == sha(path), 'Incomplete final export'
        for row in manifest['worlds']:
            complete_pcm(out/row['listen_file'], 27.5)
    (out/'SD28_NATURE_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('SD28 PASS: three actual on/off held/quiet WAVs; 18 complete performances, '
          '64/512, matched real musical histories, Nature/Room independence. Hearing remains open.')


if __name__ == '__main__':
    main()
