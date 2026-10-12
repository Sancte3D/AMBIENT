#!/usr/bin/env python3
"""SD33: captured Release, physical slot retirement and matched real ensemble tails."""
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

BASE = '4cd07cd63c82b251320bce3be8676473a2ee9e3e'
TREE = 'cc526a6e3e965302ccb6d33383ea91046688558b'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
VALUES = (0., .5, 1.)
SECONDS, STOP, BEGIN, CLIP = 112, 80, 2., 12.5
MATCHED_SECONDS, MATCHED_STOP = 32, 4
MASK_COLUMNS = ('source_mask_before_tick', 'source_mask_after_tick')


def trace(path):
    with path.open() as file:
        return list(csv.DictReader(file))


def on_score(events):
    return [(r['owner'], r['hz'], r['velocity'], r['kind']) for r in events if r['on'] == '1']


def physical_ownership(events, states):
    """Correlate acknowledged starts with fresh reservations of actual DSP slots.

    Masks include queued starts and releases. A new reservation must follow the
    previous owner's physical retirement, rather than just its NoteOff hook.
    This is a deterministic host observation at 10-ms ticks, not an ISR test.
    """
    by_frame, current, tickets = {}, {}, []
    for event in events:
        by_frame.setdefault(int(event['ack_frame']), []).append(event)
    for state in states:
        frame = int(state['frame'])
        before, after = (int(state[key]) for key in MASK_COLUMNS)
        for owner, ticket in current.items():
            if ticket['retired_frame'] is None and frame > ticket['reserved_frame'] and not before & (1 << owner):
                ticket['retired_frame'] = frame
        for event in by_frame.pop(frame, []):
            owner = int(event['owner'])
            ticket = current[owner]
            key = 'on_frame' if event['on'] == '1' else 'off_frame'
            assert ticket[key] is None, 'Duplicate hook or unreserved held-source restart'
            if key == 'on_frame':
                assert ticket['reserved_frame'] == frame-441, 'Start lacks a fresh prior-tick reservation'
            else:
                assert ticket['on_frame'] is not None
            ticket[key] = frame
        assert not before & ~after, 'Control tick truncated a physical source'
        for owner in (6, 7, 15):
            bit = 1 << owner
            if after & bit and not before & bit:
                previous = current.get(owner)
                if previous:
                    assert previous['retired_frame'] is not None and previous['retired_frame'] <= frame
                    assert previous['off_frame'] is not None
                ticket = dict(owner=owner, reserved_frame=frame, on_frame=None, off_frame=None, retired_frame=None)
                current[owner] = ticket
                tickets.append(ticket)
    assert not by_frame and tickets
    for ticket in tickets:
        assert all(ticket[key] is not None for key in ('on_frame', 'off_frame', 'retired_frame'))
        assert ticket['on_frame'] <= ticket['off_frame'] <= ticket['retired_frame']
    assert len(tickets) == len(on_score(events))
    occupied = [(t['retired_frame']-t['off_frame'])/SR for t in tickets]
    return dict(fresh_physical_reservations=len(tickets), every_ack_has_fresh_prior_tick_reservation=True,
        every_reused_owner_physically_retired_before_reservation=True, control_tick_truncated_sources=False,
        source_slot_occupied_after_note_off_seconds_range=[min(occupied), max(occupied)],
        physical_tickets=tickets)


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
        assert (repo/relative).read_bytes() == data, f'Changed PR159 compilation input: {relative}'
        original[relative] = hashlib.sha256(data).hexdigest()
    sources = [ROOT/'src'/f'{n}.c' for n in SOURCES]
    inputs = [*sources, *sorted((ROOT/'include').glob('*.h')), Path(__file__),
        ROOT/'tools/render_release_comparison.c', ROOT/'tools/render_nature_comparison.c',
        ROOT/'tools/review_activity_comparison.py', ROOT/'tools/review_room_comparison.py',
        ROOT/'tools/package_musical_review.py', ROOT/'test/test_product_sound.c', ROOT/'tools/check_release_ownership.c']
    manifest = dict(scope='Spec0.10 unchanged actual Product engine; Release0/0.5/1 and live controls. '
        'Full autonomous performances measure real retirement and musical handovers. '
        'Separate matched opening/Stop probes isolate actual ensemble tails. No substitute soundtrack.',
        baseline_commit=BASE, baseline_tree=TREE, compilation_inputs_unchanged_from_pr159=True,
        compilation_input_count=len(compiled), firmware_changed=False, hearing_accepted=False,
        range_usefulness_accepted=False, full_pcm_retained=False,
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        parameters=dict(release_values=list(VALUES), default_release=.5, activity=.5, room=.5, nature=0,
            color=.5, attack=.5, volume=.6, seeds=[1234, 42], key='D', collection='major', tuning='equal'),
        raw_seconds=SECONDS, generate_stop_seconds=STOP, block_sizes=[64, 512],
        matched_raw_seconds=MATCHED_SECONDS, matched_generate_stop_seconds=MATCHED_STOP,
        silent_settling_seconds=1, control_tick_ms=10, worlds=[], live=[])
    with tempfile.TemporaryDirectory(prefix='release-ensemble-review-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(ROOT/'tools/render_release_comparison.c'), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        for key, source in [('product_contract_regression', ROOT/'test/test_product_sound.c'),
                            ('prepared_held_releasing_and_future_note_contract', ROOT/'tools/check_release_ownership.c')]:
            check = tmp/key
            subprocess.run([*flags, str(source), *map(str, sources), '-lm', '-o', str(check)], check=True)
            manifest[key] = subprocess.run([str(check)], check=True, capture_output=True, text=True).stdout.strip()
            print(manifest[key], flush=True)
        baseline_driver = tmp/'baseline-render'
        subprocess.run([*flags, str(ROOT/'tools/render_nature_comparison.c'), *map(str, sources), '-lm', '-o', str(baseline_driver)], check=True)
        sealed, files, scores = [], {}, {}

        def render(world, seed, release, live, matched=False):
            name = WORLDS[world]
            seconds, stop = (MATCHED_SECONDS, MATCHED_STOP) if matched else (SECONDS, STOP)
            suffix = 'live' if live else f'release{round(release*100):03d}'
            if matched:
                suffix += '_matched_stop'
            raw = captures/f'{name}_seed{seed}_{suffix}_raw_{seconds}s.wav'
            command = [str(binary), str(world), str(seed), str(release), str(int(live))]
            report = subprocess.run([*command, '512', str(int(matched)), str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([*command, '64', str(int(matched)), str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Full PCM differs at64/512'
            hashes = {}
            for extension in ('.events.csv', '.state.csv'):
                path = Path(str(raw)+extension)
                assert sha(path) == sha(Path(str(control)+extension)), 'Full trace differs at64/512'
                hashes[extension] = sha(path)
            events, states = (trace(Path(str(raw)+extension)) for extension in ('.events.csv', '.state.csv'))
            assert len(states) == seconds*100
            limit = 2 if world == 1 else 3
            assert max(int(r['active_sources']) for r in states) <= limit
            assert max(int(r[key]).bit_count() for r in states for key in MASK_COLUMNS) <= limit
            assert all(r['active_sources'] == '0' for r in states[-100:])
            ownership = physical_ownership(events, states)
            owners, durations = {}, []
            for event in events:
                owner, frame = int(event['owner']), int(event['ack_frame'])
                if event['on'] == '1':
                    assert owner not in owners
                    owners[owner] = frame
                else:
                    duration = (frame-owners.pop(owner))/SR
                    if frame < stop*SR:
                        durations.append(duration)
            assert not owners
            pcm = complete_pcm(raw, seconds)
            assert not np.any(pcm[-SR:]), 'Stop did not become exactly silent'
            metrics, level = pcm_metrics(pcm), measure(raw)
            assert level['true_peak_dbfs'] <= -6 and metrics['mono_energy_ratio'] > .65
            assert max(map(abs, metrics['dc'])) < .0002
            scale = float(re.search(r'initial_release_scale=([0-9.]+)', report).group(1))
            first_free = next(int(r['frame'])/SR for r in states[stop*100:] if r['active_sources'] == '0')
            tail = np.asarray(pcm[stop*SR:], dtype=np.float64)/32768
            row = dict(seed=seed, initial_release=release, live_changes=live, matched_stop=matched,
                renderer_report=report, raw_level=level, **metrics, initial_shape_release_scale=scale,
                unchanged_attack_scale=1., starts=len(on_score(events)),
                completed_phrases=int(states[-1]['completed_phrases']), memory_returns=int(states[-1]['memory_returns']),
                maximum_actual_sources=max(int(r['active_sources']) for r in states),
                full_owner_retirement_after_stop_seconds=first_free,
                stop_to_all_owner_retirement_seconds=first_free-stop,
                stereo_energy_after_generate_stop=float(np.sum(tail*tail)),
                acknowledged_hold_seconds_range=[min(durations), max(durations)] if durations else None,
                raw_sha256=sha(raw), events_sha256=hashes['.events.csv'], state_sha256=hashes['.state.csv'],
                pcm_events_state_64_512_identical=True, final_second_exact_silence=True,
                physical_ownership=ownership)
            if world == 1:
                row['nominal_feedback_T60_seconds_not_measured_decay'] = 36*scale
                row['generated_source_soft_stop_seconds'] = 2*scale
            else:
                row['nominal_source_release_time_constant_seconds_not_tail_length'] = (.9 if world == 0 else .7)*scale
            if live:
                row['changes'] = []
                for second, target in [(10, 0), (20, 1), (32, .5), (48, 0), (64, 1), (72, .5)]:
                    center = second*SR
                    segment = np.asarray(pcm[center-SR//50:center+SR//50], dtype=np.float64)/32768
                    step = float(np.max(np.abs(np.diff(segment, axis=0))))
                    assert step < .05, 'Unbounded sample step near Release control change'
                    row['changes'].append(dict(seconds=second, target_release=target, maximum_adjacent_sample_step=step))
                row['setter_caused_immediate_note_hook'] = False
            key = world, seed, release, live, matched
            files[key], scores[key] = raw, on_score(events)
            sealed.append((raw, row, seconds))
            print(name, seed, suffix, f"{row['starts']} starts;64/512 exact; fresh physical slots; Stop silent", flush=True)
            return row

        for world, name in enumerate(WORLDS):
            cases = []
            for seed in (1234, 42):
                for release in VALUES:
                    cases.append(render(world, seed, release, False))
                sequence = [scores[world, seed, release, False, False] for release in VALUES]
                shared_count = min(map(len, sequence))
                assert all(s[:shared_count] == sequence[0][:shared_count] for s in sequence), 'Release changed ordered shared musical prefix'
                for row in cases[-3:]:
                    row['all_release_values_ordered_shared_on_prefix_identical_count'] = shared_count
                default = files[world, seed, .5, False, False]
                before = tmp/'baseline-default.wav'
                subprocess.run([str(baseline_driver), 'engine', str(world), str(seed), '0', '.5', '512', str(before)], check=True, capture_output=True)
                complete_pcm(before, SECONDS)
                assert sha(default) == sha(before), 'New driver changed normal default audio'
                assert sha(Path(str(default)+'.events.csv')) == sha(Path(str(before)+'.events.csv'))
                after_state = trace(Path(str(default)+'.state.csv'))
                for state in after_state:
                    for key in ('memory_returns', *MASK_COLUMNS):
                        state.pop(key)
                assert after_state == trace(Path(str(before)+'.state.csv'))
                cases[-2]['default_pcm_events_shared_state_equal_existing_driver'] = True
            matched = [render(world, 1234, release, False, True) for release in VALUES]
            assert len({r['events_sha256'] for r in matched}) == 1, 'Matched Stop has different notes or holds'
            retirement = [r['stop_to_all_owner_retirement_seconds'] for r in matched]
            energy = [r['stereo_energy_after_generate_stop'] for r in matched]
            assert retirement[0] < retirement[1] < retirement[2], 'Matched source retirement is not ordered'
            assert energy[0] < energy[1] < energy[2], 'Matched actual tail energy is not ordered'
            segments = [np.asarray(complete_pcm(files[world, 1234, release, False, True], MATCHED_SECONDS)
                [round(BEGIN*SR):round((BEGIN+CLIP)*SR)], dtype=np.float64)/32768 for release in VALUES]
            levels, edge = [], round(.04*SR)
            for index, segment in enumerate(segments):
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                path = tmp/f'part-{index}.wav'
                write(path, segment)
                levels.append(measure(path))
            gain = min(18., -23-max(r['integrated_lufs'] for r in levels), -6-max(r['true_peak_dbfs'] for r in levels))
            parts, sections = [], []
            for index, (release, segment) in enumerate(zip(VALUES, segments)):
                segment *= 10**(gain/20)
                path = tmp/f'gain-{index}.wav'
                write(path, segment)
                level = measure(path)
                assert level['true_peak_dbfs'] <= -5.9
                if parts:
                    parts.append(np.zeros((SR//2, 2)))
                parts.append(segment)
                sections.append(dict(release=release, output_seconds=[index*(CLIP+.5), index*(CLIP+.5)+CLIP],
                    source_seconds=[BEGIN, BEGIN+CLIP], generate_stop_in_section_seconds=MATCHED_STOP-BEGIN, delivered_level=level))
            listen = captures/f'AMBIENT_{name}_Release_0_50_100_38p5s.wav'
            write(listen, np.concatenate(parts))
            complete_pcm(listen, 38.5)
            manifest['worlds'].append(dict(world=name, cases=cases, matched_stop_cases=matched,
                matched_complete_events_identical_all_release_values=True,
                matched_stop_to_all_owner_retirement_seconds=retirement, matched_stereo_tail_energy=energy,
                listen_file=listen.name, listen_sha256=sha(listen), listen_level=measure(listen), sections=sections,
                identical_fixed_gain_db_all_sections=gain, comparison_gap_seconds=.5, excerpt_edge_fade_ms=40,
                note='Release legitimately changes capacity/handovers and keeper holds. Full autonomous state/event timing '
                     'is not asserted equal across Release. Matched notes/holds/Stop are exact. No hearing acceptance.'))
            row = render(world, 1234, .5, True)
            live_score, normal_score = scores[world, 1234, .5, True, False], scores[world, 1234, .5, False, False]
            shared_count = min(len(live_score), len(normal_score))
            assert live_score[:shared_count] == normal_score[:shared_count], 'Live Release changed ordered shared musical prefix'
            row.update(world=name, live_default_ordered_shared_on_prefix_identical_count=shared_count)
            manifest['live'].append(row)
            print(name, 'matched Release tails packaged with one fixed gain', flush=True)
        for path, row, seconds in sealed:
            complete_pcm(path, seconds)
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
            if path.name.endswith(('.events.csv', '.state.csv')) or '_Release_0_50_100_' in path.name:
                target = out/path.name
                shutil.copyfile(path, target)
                assert sha(path) == sha(target), 'Incomplete export'
        for row in manifest['worlds']:
            complete_pcm(out/row['listen_file'], 38.5)
    manifest.update(actual_long_release_render_count=42, existing_default_driver_control_render_count=6,
        actual_matched_stop_render_count=18, total_complete_wav_render_count=66,
        total_full_render_minutes=(48*SECONDS+18*MATCHED_SECONDS)/60)
    (out/'SD33_RELEASE_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('RELEASE PASS:18 fixed +3 live complete performances at64/512, six existing-driver controls, '
          'nine matched Stop cases at64/512, physical source retirement, three genuine tail WAVs. Hearing remains open.')


if __name__ == '__main__':
    main()
