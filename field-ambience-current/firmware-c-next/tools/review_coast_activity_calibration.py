#!/usr/bin/env python3
"""SD33: calibrate real COAST retirement spacing against the exact PR155 core."""
import argparse
import csv
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

import numpy as np
import review_activity_comparison as prior
from package_musical_review import measure
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = '4de325a0d4c6255909801e02e0cfe62889a6777e'
TREE = 'f515c8c68420ceec81d4ff26f80b3e40c8b8a69b'
SECONDS, STOP, CLIP, BEGIN = 600, 568, 12.5, 8.
ACTIVITY = (0., .5, 1.)
CHANGED = {'src/engine_product.c', 'src/world_grammar.c', 'include/world_grammar.h'}
prior.SECONDS, prior.STOP = SECONDS, STOP
sha, complete_pcm, pcm_metrics = prior.sha, prior.complete_pcm, prior.pcm_metrics


def retained(a, b):
    length = min(len(a[0]), len(b[0]))
    assert a[0][:length] == b[0][:length], 'Changed ordered heard pitch/role/velocity/memory'
    shared = set(a[1]) & set(b[1])
    assert len(shared) >= 24
    delta = max(abs(a[1][i]-b[1][i]) for i in shared)
    assert delta < .011, 'Shortened a held outer role'
    return dict(matched_heard_prefix_starts=length, unchanged_held_outer_roles=len(shared),
                maximum_held_role_difference_seconds=delta)


def retirement_metrics(raw, activity, calibrated):
    with Path(str(raw)+'.events.csv').open() as file:
        on = [r for r in csv.DictReader(file) if r['on'] == '1']
    with Path(str(raw)+'.state.csv').open() as file:
        states = list(csv.DictReader(file))
    masks = np.array([int(r['dsp_sources']) for r in states])
    outer, all_roles = (1<<6)|(1<<7), (1<<6)|(1<<7)|(1<<15)
    result = []
    for index, row in enumerate(on):
        phase = index % 8
        if phase not in (0, 3, 5, 7) or index == 0:
            continue
        closing = phase == 0
        ack_tick = int(row['ack_frame'])//441
        # This actual engine reserves the DSP slot in Main; the note hook
        # acknowledges its first rendered block on the following 10ms tick.
        # Exclude that newly reserved slot when locating the OLD retirement.
        tick = ack_tick-1
        assert masks[tick] & (1<<int(row['owner']))
        check = all_roles if closing else outer
        busy = np.flatnonzero(masks[:tick] & check)
        assert len(busy)
        retired = int(busy[-1])+1
        elapsed = (tick-retired)/100
        kind = int(on[index-1 if closing else index]['kind'])
        factor = (1., 1.08, 1.12, .96)[kind]
        expected = ((1+9*(1-activity)) if closing else 6*(1-activity))*factor
        if calibrated:
            assert elapsed >= expected-.021, 'Wait did not follow actual DSP retirement'
        assert not np.any(masks[retired:tick] & check)
        if not closing:
            assert np.all(masks[retired:tick] & (1<<15)), 'Lost or reattacked held common tone'
        result.append(dict(next_source_issue_seconds=tick/100, next_ack_seconds=ack_tick/100,
            actual_retirement_seconds=retired/100,
            actual_wait_seconds=elapsed, phase='closing' if closing else str((phase-1)//2),
            nominal_extra_wait_seconds=expected if calibrated else None,
            common_tone_retained=not closing))
    assert len(result) >= 12
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    assert subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=ROOT, text=True).strip() == TREE
    sources = [ROOT/'src'/f'{name}.c' for name in SOURCES]
    headers = sorted((ROOT/'include').glob('*.h'))
    inputs = [*sources, *headers, Path(__file__), ROOT/'tools/render_coast_activity_calibration.c',
              ROOT/'tools/render_nature_comparison.c', ROOT/'test/test_activity_ranges.c',
              ROOT/'tools/review_activity_comparison.py', ROOT/'tools/review_room_comparison.py',
              ROOT/'tools/package_musical_review.py']
    manifest = dict(scope='Spec0.9 COAST Activity only: waits after real outgoing-owner retirement. '
        'Held outer intentions, common tone, ordered heard music and bounded source admission retained.',
        baseline_commit=BASE, baseline_tree=TREE, firmware_changed=True, hearing_accepted=False,
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        parameters=dict(activity_values=list(ACTIVITY), default_activity=.5, color=.5, room=.5,
                        nature=0, attack=.5, release=.5, volume=.6, seeds=[1234, 42],
                        key='D', collection='major', tuning='equal'),
        raw_seconds=SECONDS, generate_stop_seconds=STOP, block_sizes=[64, 512],
        control_tick_ms=10, fixed_cases=[], live=[], other_worlds_unchanged=[], full_pcm_retained=False)
    with tempfile.TemporaryDirectory(prefix='coast-activity-calibration-') as value:
        tmp = Path(value)
        old = tmp/'baseline'
        for path in [*sources, *headers]:
            relative = path.relative_to(ROOT)
            target = old/relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(['git', 'show', f'{BASE}:{prefix/relative}'], cwd=ROOT))
            changed = sha(path) != sha(target)
            assert changed == (str(relative) in CHANGED), f'Unexpected core/header change: {relative}'
        binaries = {}
        for version, core in [('baseline', old), ('calibrated', ROOT)]:
            flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
                     '-I'+str(core/'include')]
            core_sources = [str(core/'src'/f'{name}.c') for name in SOURCES]
            binary = tmp/f'render-{version}'
            subprocess.run([*flags, str(ROOT/'tools/render_coast_activity_calibration.c'),
                            *core_sources, '-lm', '-o', str(binary)], check=True)
            binaries[version] = binary
        pure = tmp/'pure'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'include'),
                        str(ROOT/'test/test_activity_ranges.c'), str(ROOT/'src/world_grammar.c'),
                        '-o', str(pure)], check=True)
        manifest['pure_regression'] = subprocess.check_output([str(pure)], text=True).strip()
        print(manifest['pure_regression'], flush=True)
        captures = tmp/'captures'
        captures.mkdir()
        sealed, actual, listening = [], {}, {}

        def render(version, seed, activity, live):
            suffix = 'live' if live else f'activity{round(activity*100):03d}'
            raw = captures/f'COAST_{version}_seed{seed}_{suffix}_raw_600s.wav'
            command = [str(binaries[version]), str(seed), str(activity), str(int(live))]
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
            assert not np.any(pcm[-SR:])
            metrics, level = pcm_metrics(pcm), measure(raw)
            assert metrics['mono_energy_ratio'] > .65 and level['true_peak_dbfs'] <= -6
            assert max(map(abs, metrics['dc'])) < .0002
            trace, score, holds = prior.trace_metrics(raw, 0)
            row = dict(version=version, seed=seed, initial_activity=activity, live_changes=live,
                       renderer_report=report, raw_level=level, **metrics, **trace, raw_sha256=sha(raw),
                       events_sha256=hashes['.events.csv'], state_sha256=hashes['.state.csv'],
                       pcm_events_state_64_512_identical=True, final_second_exact_silence=True)
            if not live:
                row['actual_post_retirement_waits'] = retirement_metrics(raw, activity, version == 'calibrated')
            sealed.append((raw, row, SECONDS))
            actual[version, seed, activity, live] = (score, holds)
            if version == 'calibrated' and seed == 1234 and not live:
                listening[activity] = raw
            print(version, seed, suffix, f"{row['starts']} starts, {row['completed_phrases']} phrases, "
                  '64/512 PCM + traces equal; Stop silent', flush=True)
            return row

        for seed in (1234, 42):
            rows = []
            for activity in ACTIVITY:
                for version in ('baseline', 'calibrated'):
                    row = render(version, seed, activity, False)
                    manifest['fixed_cases'].append(row)
                    if version == 'calibrated':
                        rows.append(row)
                        row['retained_against_exact_pr155'] = retained(actual['baseline', seed, activity, False],
                                                                      actual[version, seed, activity, False])
            assert rows[0]['starts'] < rows[1]['starts'] < rows[2]['starts'], 'Actual Activity range still inert'
            returns = [next(p['seconds'] for p in row['phrase_openings'] if p['kind'] == 3) for row in rows]
            assert returns[0] > returns[1] > returns[2]
            for activity, row in zip(ACTIVITY, rows):
                row['retained_across_activity'] = retained(actual['calibrated', seed, .5, False],
                                                          actual['calibrated', seed, activity, False])
        for version in ('baseline', 'calibrated'):
            row = render(version, 1234, .5, True)
            row['retained_against_fixed_default'] = retained(actual[version, 1234, .5, False], actual[version, 1234, .5, True])
            row['setter_caused_immediate_note_hook'] = False
            row['target_changes_seconds_values'] = [[10, 0], [32, 1], [64, .5], [96, 0], [128, 1], [160, .5]]
            manifest['live'].append(row)
        manifest['live'][1]['retained_against_exact_pr155_live'] = retained(actual['baseline', 1234, .5, True],
                                                                         actual['calibrated', 1234, .5, True])
        # Prove both other actual Worlds unchanged, rather than infer it solely
        # from the small diff. Use the identical existing production driver.
        other = {}
        for version, core in [('baseline', old), ('calibrated', ROOT)]:
            binary = tmp/f'other-{version}'
            subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
                '-I'+str(core/'include'), str(ROOT/'tools/render_nature_comparison.c'),
                *[str(core/'src'/f'{n}.c') for n in SOURCES], '-lm', '-o', str(binary)], check=True)
            other[version] = binary
        for world in (1, 2):
            for seed in (1234, 42):
                files = []
                for version in ('baseline', 'calibrated'):
                    raw = captures/f'world{world}_{version}_seed{seed}_raw_112s.wav'
                    command = [str(other[version]), 'engine', str(world), str(seed), '0', '.5']
                    subprocess.run([*command, '512', str(raw)], check=True, capture_output=True)
                    control = tmp/'control-other.wav'
                    subprocess.run([*command, '64', str(control)], check=True, capture_output=True)
                    complete_pcm(raw, 112)
                    hashes = {'raw_sha256': sha(raw), 'events_sha256': sha(Path(str(raw)+'.events.csv')),
                              'state_sha256': sha(Path(str(raw)+'.state.csv'))}
                    for extension in ('', '.events.csv', '.state.csv'):
                        assert sha(Path(str(raw)+extension)) == sha(Path(str(control)+extension))
                    sealed.append((raw, hashes, 112))
                    files.append(hashes)
                assert files[0] == files[1], 'COAST change altered another World'
                manifest['other_worlds_unchanged'].append(dict(world=('COAST', 'WOODLAND', 'HIGHLANDS')[world],
                    seed=seed, raw_seconds=112, baseline_candidate_pcm_events_state_identical=True,
                    block_64_512_identical=True, **files[0]))
                print('Unchanged World', world, seed, 'full PCM/events/state identical', flush=True)
        segments, levels = [], []
        for index, activity in enumerate(ACTIVITY):
            pcm = complete_pcm(listening[activity], SECONDS)
            segment = np.asarray(pcm[round(BEGIN*SR):round((BEGIN+CLIP)*SR)], dtype=np.float64)/32768
            edge = round(.04*SR)
            segment[:edge] *= np.linspace(0, 1, edge)[:, None]
            segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
            path = tmp/f'part-{index}.wav'
            write(path, segment)
            levels.append(measure(path))
            segments.append(segment)
        gain = min(18., -23-max(r['integrated_lufs'] for r in levels), -6-max(r['true_peak_dbfs'] for r in levels))
        parts, sections = [], []
        for index, (activity, segment) in enumerate(zip(ACTIVITY, segments)):
            segment *= 10**(gain/20)
            path = tmp/f'gain-{index}.wav'
            write(path, segment)
            level = measure(path)
            assert level['true_peak_dbfs'] <= -5.9
            if parts:
                parts.append(np.zeros((SR//2, 2)))
            parts.append(segment)
            sections.append(dict(activity=activity, output_seconds=[index*(CLIP+.5), index*(CLIP+.5)+CLIP],
                                 source_seconds=[BEGIN, BEGIN+CLIP], delivered_level=level))
        listen = captures/'AMBIENT_COAST_Activity_Calibrated_0_50_100_38p5s.wav'
        write(listen, np.concatenate(parts))
        complete_pcm(listen, 38.5)
        manifest['listening'] = dict(file=listen.name, sha256=sha(listen), level=measure(listen), sections=sections,
            identical_fixed_gain_db_all_sections=gain, comparison_gap_seconds=.5, cut_edge_fade_ms=40)
        for path, row, seconds in sealed:
            complete_pcm(path, seconds)
            assert sha(path) == row['raw_sha256'], 'Capture changed after measurement'
            assert sha(Path(str(path)+'.events.csv')) == row['events_sha256']
            assert sha(Path(str(path)+'.state.csv')) == row['state_sha256']
        assert sha(listen) == manifest['listening']['sha256']
        for path in sorted(captures.iterdir()):
            if path.name.endswith(('.events.csv', '.state.csv')) or path == listen:
                target = out/path.name
                shutil.copyfile(path, target)
                assert sha(target) == sha(path), 'Incomplete export'
        complete_pcm(out/listen.name, 38.5)
    manifest['total_full_render_count'] = 44
    manifest['total_full_render_minutes'] = (28*600+16*112)/60
    (out/'SD33_COAST_ACTIVITY_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('COAST CALIBRATION PASS: actual min/default/max distinguishable at both seeds; 44 full renders, '
          'retained held roles/music, real retirement waits, other Worlds byte-identical, Stop silent. Hearing remains open.')


if __name__ == '__main__':
    main()
