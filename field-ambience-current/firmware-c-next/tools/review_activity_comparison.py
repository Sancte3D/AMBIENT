#!/usr/bin/env python3
"""SD33: actual Activity ranges, retained long tones and fair short comparisons."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import tempfile
import wave

import numpy as np
from package_musical_review import measure
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = '9f9aafc3fc44ba53a8496f6ec8e501e6efdd4def'
TREE = '31a3a0a1511c63d637ce12227bca133cae7086cd'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
ACTIVITY = (0., .5, 1.)
SECONDS, STOP = 360, 328
EXCERPT_BEGIN = (8., 8., 34.)
CLIP = 12.5
LISTEN_SECONDS = 3*CLIP+1


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for block in iter(lambda: file.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def complete_pcm(path, seconds):
    frames = round(seconds*SR)
    assert path.stat().st_size == 44+frames*4, f'Incomplete capture: {path}'
    with wave.open(str(path), 'rb') as wav:
        assert wav.getparams()[:4] == (2, 2, SR, frames)
        assert wav.getcomptype() == 'NONE'
    return np.memmap(path, dtype='<i2', mode='r', offset=44, shape=(frames, 2))


def pcm_metrics(pcm):
    energy, mono, dc = 0., 0., np.zeros(2)
    for start in range(0, len(pcm), 2*SR):
        part = np.asarray(pcm[start:start+2*SR], dtype=np.float64)/32768
        energy += float(np.sum(part*part))
        mono += float(np.sum(part.mean(axis=1)**2))
        dc += part.sum(axis=0)
    return dict(mono_energy_ratio=mono/(energy/2), dc=(dc/len(pcm)).tolist())


def excerpt_level(path, segment):
    # Keep a genuine pause at the same gain. Exact quantized silence has no
    # finite LUFS or true peak; do not manufacture either or normalise it up.
    if not np.any(np.rint(segment*32768)):
        return dict(integrated_lufs=None, true_peak_dbfs=None, exact_silence=True)
    return measure(path)


def trace_metrics(raw, world):
    with Path(str(raw)+'.events.csv').open() as file:
        events = list(csv.DictReader(file))
    with Path(str(raw)+'.state.csv').open() as file:
        states = list(csv.DictReader(file))
    assert len(states) == SECONDS*100
    on = [row for row in events if row['on'] == '1']
    assert len(on) >= (6 if world == 1 else 8)*4
    score = [(row['owner'], row['hz'], row['velocity'], row['kind']) for row in on]
    owners, holds = {}, {}
    index = 0
    for row in events:
        owner, frame = int(row['owner']), int(row['ack_frame'])
        assert owner in (6, 7, 15)
        if row['on'] == '1':
            assert owner not in owners
            hz = float(row['hz'])
            midi = round(69+12*math.log2(hz/440))
            assert (midi-2) % 12 in (0, 2, 4, 7, 9)
            assert abs(1200*math.log2(hz/(440*2**((midi-69)/12)))) < .01
            owners[owner] = (index, frame)
            index += 1
        else:
            i, start = owners.pop(owner)
            if frame < STOP*SR and owner != 15:
                holds[i] = (frame-start)/SR
    assert not owners
    openings = []
    for row in on:
        if not openings or int(row['completed_phrases']) != openings[-1]['episode']:
            openings.append(dict(episode=int(row['completed_phrases']), kind=int(row['kind']),
                                 seconds=int(row['ack_frame'])/SR))
    assert [row['kind'] for row in openings[:4]] == [0, 1, 2, 3]
    assert any(row['kind'] == 3 for row in openings)
    assert min(holds.values()) >= (2.68 if world == 0 else 5.75)
    times = np.array([int(row['ack_frame'])/SR for row in on])
    assert np.diff(times).max() < 90
    occupied = [int(row['active_sources']) for row in states[:STOP*100]]
    assert max(occupied) <= (2 if world == 1 else 3)
    free_runs, start = [], None
    for row in states[:STOP*100]:
        frame, count = int(row['frame']), int(row['active_sources'])
        if not count and start is None:
            start = frame
        if count and start is not None:
            free_runs.append([start/SR, frame/SR])
            start = None
    if start is not None:
        free_runs.append([start/SR, float(STOP)])
    longest = max((b-a for a, b in free_runs), default=0.)
    if world == 2:
        assert longest >= 8
    first_free_after_stop = next(int(row['frame'])/SR for row in states[STOP*100:]
                                if row['active_sources'] == '0')
    assert all(row['active_sources'] == '0' for row in states[-100:])
    assert int(states[-1]['completed_phrases']) >= 4 and int(states[-1]['memory_returns']) >= 1
    return dict(starts=len(on), starts_per_minute=len(on)/(STOP/60),
        completed_phrases=int(states[-1]['completed_phrases']), memory_returns=int(states[-1]['memory_returns']),
        maximum_actual_sources=max(occupied), mean_actual_sources=float(np.mean(occupied)),
        source_free_seconds=sum(count == 0 for count in occupied)/100,
        longest_actual_source_free_interval_seconds=longest,
        largest_actual_onset_gap_seconds=float(np.diff(times).max()),
        full_owner_retirement_after_stop_seconds=first_free_after_stop,
        held_role_on_off_seconds_range=[min(holds.values()), max(holds.values())],
        phrase_openings=openings, actual_source_free_intervals_seconds=free_runs), score, holds


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT/'src'/f'{name}.c' for name in SOURCES]
    driver = ROOT/'tools/render_activity_comparison.c'
    regression = ROOT/'test/test_activity_ranges.c'
    inputs = [*sources, driver, regression, Path(__file__), ROOT/'tools/review_room_comparison.py',
              ROOT/'tools/package_musical_review.py', *sorted((ROOT/'include').glob('*.h'))]
    reference = json.loads((ROOT.parent/'docs/audio/SD19_ROOM_CALIBRATED_AUDIT.json').read_text())
    for path, expected in reference['source_sha256'].items():
        assert sha(ROOT/path) == expected, f'Changed published sound input: {path}'
    assert subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=ROOT, text=True).strip() == TREE
    manifest = dict(scope='SD33 Activity only. Normal actual Product Engine and musical Worlds. '
        'Activity changes upcoming spacing and genuine pauses; active hold intentions, pitches, '
        'velocity and heard phrase memory are retained. Actual admission/retirement can limit density.',
        baseline_commit=BASE, baseline_tree=TREE, sound_inputs_unchanged_from_pr153=True,
        source_sha256={str(path.relative_to(ROOT)): sha(path) for path in inputs},
        parameters=dict(activity_values=list(ACTIVITY), default_activity=.5, color=.5, room=.5,
                        nature=0, attack=.5, release=.5, volume=.6, key='D', collection='major',
                        tuning='equal', seeds=[1234, 42]), raw_seconds=SECONDS,
        generate_stop_seconds=STOP, silent_settling_seconds=1, control_tick_ms=10,
        block_sizes=[64, 512], hearing_accepted=False, range_usefulness_accepted=False, firmware_changed=False,
        full_pcm_retained=False, worlds=[], live=[])
    with tempfile.TemporaryDirectory(prefix='activity-review-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
                 '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(driver), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        check = tmp/'activity-ranges'
        subprocess.run([*flags, str(regression), str(ROOT/'src/world_grammar.c'), '-o', str(check)], check=True)
        manifest['pure_regression'] = subprocess.run([str(check)], check=True, capture_output=True, text=True).stdout.strip()
        print(manifest['pure_regression'], flush=True)
        sealed = []

        def render(world, seed, activity, live):
            name = WORLDS[world]
            suffix = 'live' if live else f'activity{round(activity*100):03d}'
            raw = captures/f'{name}_seed{seed}_{suffix}_raw_360s.wav'
            command = [str(binary), str(world), str(seed), str(activity), str(int(live))]
            report = subprocess.run([*command, '512', str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([*command, '64', str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Actual PCM differs at 64/512'
            hashes = {}
            for extension in ('.events.csv', '.state.csv'):
                path = Path(str(raw)+extension)
                assert sha(path) == sha(Path(str(control)+extension)), 'Actual musical trace differs at 64/512'
                hashes[extension] = sha(path)
            pcm = complete_pcm(raw, SECONDS)
            assert not np.any(pcm[-SR:]), 'Stop failed to become exactly silent'
            metrics = pcm_metrics(pcm)
            level = measure(raw)
            assert level['true_peak_dbfs'] <= -6 and metrics['mono_energy_ratio'] > .65
            assert max(map(abs, metrics['dc'])) < .0002
            traces, score, holds = trace_metrics(raw, world)
            row = dict(seed=seed, initial_activity=activity, live_changes=live, renderer_report=report,
                       raw_level=level, **metrics, **traces, raw_sha256=sha(raw),
                       events_sha256=hashes['.events.csv'], state_sha256=hashes['.state.csv'],
                       pcm_events_state_64_512_identical=True, final_second_exact_silence=True)
            sealed.append((raw, row))
            print(name, seed, suffix, f"{row['starts']} starts; {row['completed_phrases']} phrases; "
                  'full PCM/score/state matched, Stop silent', flush=True)
            return raw, row, score, holds

        for world, name in enumerate(WORLDS):
            cases, listening = [], {}
            default_score, default_holds = None, None
            for seed in (1234, 42):
                scores, durations = [], []
                for activity in ACTIVITY:
                    raw, row, score, holds = render(world, seed, activity, False)
                    cases.append(row)
                    scores.append(score)
                    durations.append(holds)
                    if seed == 1234:
                        listening[activity] = raw
                        if activity == .5:
                            default_score, default_holds = score, holds
                length = min(map(len, scores))
                assert scores[0][:length] == scores[1][:length] == scores[2][:length], 'Activity rewrote heard musical sequence'
                shared = set(durations[0]) & set(durations[1]) & set(durations[2])
                assert len(shared) >= 16
                for index in shared:
                    values = [duration[index] for duration in durations]
                    assert max(values)-min(values) < .011, 'Activity shortened a held role'
                for row in cases[-3:]:
                    row['all_activity_values_ordered_heard_prefix_identical'] = True
                    row['matched_heard_prefix_start_count'] = length
                    row['unchanged_actual_held_roles_checked'] = len(shared)
            segments, original_levels = [], []
            begin = EXCERPT_BEGIN[world]
            for i, activity in enumerate(ACTIVITY):
                pcm = complete_pcm(listening[activity], SECONDS)
                segment = np.asarray(pcm[round(begin*SR):round((begin+CLIP)*SR)], dtype=np.float64)/32768
                edge = round(.04*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                path = tmp/f'part-{i}.wav'
                write(path, segment)
                original_levels.append(excerpt_level(path, segment))
                segments.append(segment)
            # One gain across min/default/max. Preserve real density and energy
            # differences instead of separately normalising each Activity.
            audible = [row for row in original_levels if row['integrated_lufs'] is not None]
            assert audible
            gain = min(18., -23-max(row['integrated_lufs'] for row in audible),
                       -6-max(row['true_peak_dbfs'] for row in audible))
            parts, sections = [], []
            for i, (activity, segment) in enumerate(zip(ACTIVITY, segments)):
                segment *= 10**(gain/20)
                path = tmp/f'gain-{i}.wav'
                write(path, segment)
                level = excerpt_level(path, segment)
                assert level['true_peak_dbfs'] is None or level['true_peak_dbfs'] <= -5.9
                if parts:
                    parts.append(np.zeros((SR//2, 2)))
                parts.append(segment)
                sections.append(dict(activity=activity, output_seconds=[i*(CLIP+.5), i*(CLIP+.5)+CLIP],
                    source_seconds=[begin, begin+CLIP], delivered_level=level))
            listen = captures/f'AMBIENT_{name}_Activity_0_50_100_38p5s.wav'
            write(listen, np.concatenate(parts))
            complete_pcm(listen, LISTEN_SECONDS)
            manifest['worlds'].append(dict(world=name, cases=cases, listen_file=listen.name,
                listen_sha256=sha(listen), listen=measure(listen), identical_fixed_gain_db_all_sections=gain,
                sections=sections, comparison_gap_seconds=.5, cut_edge_fade_ms=40,
                coast_weak_activity_range_requires_calibration=world == 0))
            print(name, 'three actual Activity excerpts packaged with identical gain', flush=True)
            raw, row, score, holds = render(world, 1234, .5, True)
            length = min(len(score), len(default_score))
            assert score[:length] == default_score[:length], 'Live Activity rewrote the heard sequence'
            shared = set(holds) & set(default_holds)
            difference = max(abs(holds[index]-default_holds[index]) for index in shared)
            assert len(shared) >= 16 and difference < .011, 'Live Activity shortened a held role'
            row['live_ordered_heard_prefix_starts'] = length
            row['live_unchanged_actual_held_roles'] = len(shared)
            row['live_maximum_held_duration_difference_seconds'] = difference
            row['world'] = name
            row['target_changes_seconds_values'] = [[10, 0], [32, 1], [64, .5], [96, 0], [128, 1], [160, .5]]
            row['setter_caused_immediate_note_hook'] = False
            manifest['live'].append(row)
        # Keep all long performance bytes temporary. Seal every measured capture
        # and complete trace before exporting the three compact listening files.
        for path, row in sealed:
            complete_pcm(path, SECONDS)
            assert sha(path) == row['raw_sha256'], 'Capture changed after measurement'
            assert sha(Path(str(path)+'.events.csv')) == row['events_sha256']
            assert sha(Path(str(path)+'.state.csv')) == row['state_sha256']
        for row in manifest['worlds']:
            complete_pcm(captures/row['listen_file'], LISTEN_SECONDS)
            assert sha(captures/row['listen_file']) == row['listen_sha256']
        for path in sorted(captures.iterdir()):
            if path.name.endswith(('.events.csv', '.state.csv')) or '_Activity_0_50_100_' in path.name:
                target = out/path.name
                shutil.copyfile(path, target)
                assert sha(target) == sha(path), 'Incomplete export'
        for row in manifest['worlds']:
            complete_pcm(out/row['listen_file'], LISTEN_SECONDS)
    (out/'SD33_ACTIVITY_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('SD33 ACTIVITY PASS: 21 complete six-minute actual performances at both block sizes; '
          'held roles, score/memory, bounded sources, genuine pauses and Stop; three short WAVs. Hearing remains open.')


if __name__ == '__main__':
    main()
