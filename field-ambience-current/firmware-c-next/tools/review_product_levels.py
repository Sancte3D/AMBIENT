#!/usr/bin/env python3
"""SD34: actual fixed source gains, register/velocity levels and admitted sums."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure
from review_activity_comparison import sha, complete_pcm, pcm_metrics
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = 'a13d01949b425f3cec966573544639bf8ec49b5f'
TREE = '1a146116b8c966e6fed511fae149e650b8c60f65'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
VELOCITIES = (.01, .25, .5, .55, .75, 1.)


def trace(path):
    with path.open() as file:
        return list(csv.DictReader(file))


def rms_db(x):
    rms = float(np.sqrt(np.mean(x*x)))
    return (float(20*np.log10(rms)) if rms else None), rms


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
    original = {}
    for relative in compiled:
        data = subprocess.check_output(['git', 'show', f'{BASE}:{relative}'], cwd=ROOT)
        assert (repo/relative).read_bytes() == data, f'Changed PR160 firmware input: {relative}'
        original[relative] = hashlib.sha256(data).hexdigest()
    assert len(compiled) == 251
    sources = [ROOT/'src'/f'{n}.c' for n in SOURCES]
    inputs = [*sources, *sorted((ROOT/'include').glob('*.h')), Path(__file__),
        ROOT/'tools/render_product_levels.c', ROOT/'tools/check_product_gain.c',
        ROOT/'tools/render_nature_comparison.c', ROOT/'tools/review_activity_comparison.py',
        ROOT/'tools/review_room_comparison.py', ROOT/'tools/package_musical_review.py', ROOT/'test/test_product_sound.c']
    manifest = dict(scope='Spec0.10 actual Product admission/source/master PCM; fixed gains unchanged; no product AGC. '
        'Every manual MIDI50..69 at neutral, bounded velocity/sum cases and real autonomous roles.',
        baseline_commit=BASE, baseline_tree=TREE, firmware_changed=False, compilation_input_count=len(compiled),
        compilation_inputs_unchanged_from_pr160=True, hearing_accepted=False, final_gain_selection_accepted=False,
        full_pcm_retained=False, source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        fixed_source_input_gains=dict(COAST=.5, WOODLAND=.22, HIGHLANDS=.5), shared_source_send=.35,
        legacy_cell_amp_max=.22, deprecated_families='Choir/Guembri absent from Product link.',
        block_sizes=[64, 512], control_tick_ms=10, silent_settling_seconds=1,
        manual_raw_seconds=12, manual_clear_seconds=10, body_window_seconds=[1.5, 2.5], late_window_seconds=[8, 9],
        autonomous_raw_seconds=112, autonomous_stop_seconds=80,
        headroom_scope='26 ordered legal family compositions, two admitted D-major voicings and three Colors; '
            'velocity/master/Room/Nature at1. Not all pitches/keys/tunings/SHAPEs.',
        solo=[], autonomous=[], references=[], neutral_register_summary=[], velocity_summary=[])
    with tempfile.TemporaryDirectory(prefix='product-level-review-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(ROOT/'tools/render_product_levels.c'), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        for key, source in [('product_contract_regression', ROOT/'test/test_product_sound.c'),
                            ('actual_gain_api_contract', ROOT/'tools/check_product_gain.c')]:
            check = tmp/key
            subprocess.run([*flags, str(source), *map(str, sources), '-lm', '-o', str(check)], check=True)
            manifest[key] = subprocess.run([str(check)], check=True, capture_output=True, text=True).stdout.strip()
            print(manifest[key], flush=True)
        baseline = tmp/'baseline'
        subprocess.run([*flags, str(ROOT/'tools/render_nature_comparison.c'), *map(str, sources), '-lm', '-o', str(baseline)], check=True)
        rows, sealed, files, recipes = [], [], {}, {}

        def render(label, recipe_text, key=2, minor=0, color=.5, room=0, nature=0, volume=.6, generated=False):
            seconds = 112 if generated else 12
            recipe = captures/(label+'.recipe.csv')
            recipe.write_text(recipe_text)
            raw = captures/(label+f'_raw_{seconds}s.wav')
            command = [str(binary), str(key), str(minor), str(color), str(room), str(nature), str(volume)]
            report = subprocess.run([*command, '512', str(recipe), str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([*command, '64', str(recipe), str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Whole PCM differs at64/512'
            hashes = {}
            for extension in ('.events.csv', '.state.csv'):
                path = Path(str(raw)+extension)
                assert sha(path) == sha(Path(str(control)+extension)), 'Whole trace differs at64/512'
                hashes[extension] = sha(path)
            events, states = (trace(Path(str(raw)+e)) for e in ('.events.csv', '.state.csv'))
            assert len(states) == seconds*100
            starts = [r for r in events if r['on'] == '1']
            expected = None if generated else sum(int(line.split(',')[-1]) for line in recipe_text.splitlines())
            assert generated or len(starts) == expected
            assert max(int(r['active_sources']) for r in states) <= 3
            assert max(int(r['physical_source_mask']).bit_count() for r in states) <= 3
            assert all(r['active_sources'] == '0' and r['physical_source_mask'] == '0' for r in states[-100:])
            pcm = complete_pcm(raw, seconds)
            assert not np.any(pcm[-SR:])
            metrics, level = pcm_metrics(pcm), measure(raw)
            assert level['true_peak_dbfs'] <= -3, 'Less than3dB measured full-volume headroom'
            assert metrics['mono_energy_ratio'] > .65 and max(map(abs, metrics['dc'])) < .0002
            body_db, body_rms = rms_db(np.asarray(pcm[round(1.5*SR):round(2.5*SR)], dtype=np.float64)/32768)
            late_db, late_rms = rms_db(np.asarray(pcm[8*SR:9*SR], dtype=np.float64)/32768)
            assert body_rms > 0
            row = dict(label=label, key=key, minor=minor, color=color, room=room, nature=nature, volume=volume,
                generated=generated, seconds=seconds, body_rms_dbfs=body_db, late_rms_dbfs=late_db,
                late_to_body_rms_ratio=late_rms/body_rms, **level, **metrics,
                maximum_sources=max(int(r['active_sources']) for r in states), starts=len(starts),
                admission_rejections=int(states[-1]['admission_rejections']),
                full_pcm_64_512_identical=True, events_state_64_512_identical=True, final_second_exact_zero=True,
                nonfinite_samples=0, limited_samples=0, raw_sha256=sha(raw), events_sha256=hashes['.events.csv'],
                state_sha256=hashes['.state.csv'], recipe_sha256=sha(recipe), renderer_report=report)
            if generated:
                owners = {}
                for event in events:
                    owner = int(event['owner'])
                    if event['on'] == '1':
                        assert owner not in owners
                        owners[owner] = int(event['ack_frame'])
                    else:
                        assert event['on'] == '0'
                        owners.pop(owner)
                assert not owners and len(starts) >= 6
                row['confirmed_roles'] = {str(owner): dict(starts=sum(int(r['owner']) == owner for r in starts),
                    hz_range=[min(float(r['hz']) for r in starts if int(r['owner']) == owner),
                              max(float(r['hz']) for r in starts if int(r['owner']) == owner)],
                    velocity_range=[min(float(r['velocity']) for r in starts if int(r['owner']) == owner),
                                    max(float(r['velocity']) for r in starts if int(r['owner']) == owner)])
                    for owner in sorted({int(r['owner']) for r in starts})}
                row['completed_phrases'] = int(states[-1]['completed_phrases'])
            rows.append(row)
            sealed.append((raw, recipe, row))
            files[label], recipes[label] = raw, recipe_text
            if len(rows)%20 == 0 or generated or label.startswith('REFERENCE_'):
                print(label, f"{len(rows)} cases; full64/512 PCM/traces exact; peak{level['true_peak_dbfs']:.1f}dBTP", flush=True)
            return row

        solo = {}
        for world, name in enumerate(WORLDS):
            for midi in range(50, 70):
                for velocity in VELOCITIES if midi in (50, 60, 69) else (.5,):
                    row = render(f'{name}_midi{midi}_velocity{round(velocity*100):03d}',
                        f'0,0,{world},{midi},{velocity},1\n', key=midi%12)
                    row.update(world=name, midi=midi, velocity=velocity)
                    manifest['solo'].append(row)
                    solo[world, midi, velocity] = row
            register = [solo[world, m, .5]['body_rms_dbfs'] for m in range(50, 70)]
            assert max(register)-min(register) < 4
            manifest['neutral_register_summary'].append(dict(world=name, midi_range=[50, 69],
                body_rms_dbfs_range=[min(register), max(register)], body_register_spread_db=max(register)-min(register),
                late_to_body_rms_ratios_at_midi50_60_69=[solo[world, m, .5]['late_to_body_rms_ratio'] for m in (50, 60, 69)]))
            for midi in (50, 60, 69):
                levels = [solo[world, midi, v]['body_rms_dbfs'] for v in VELOCITIES]
                assert all(a < b for a, b in zip(levels, levels[1:]))
                delta = solo[world, midi, .55]['body_rms_dbfs']-solo[world, midi, .5]['body_rms_dbfs']
                assert .5 < delta < 1.2
                manifest['velocity_summary'].append(dict(world=name, midi=midi, velocities=list(VELOCITIES),
                    body_rms_dbfs=levels, velocity_0p5_to_0p55_delta_db=delta, strictly_increasing=True))
        spread = [max(solo[w, m, .5]['body_rms_dbfs'] for w in range(3))-
                  min(solo[w, m, .5]['body_rms_dbfs'] for w in range(3)) for m in range(50, 70)]
        assert max(spread) < 4
        manifest['maximum_same_pitch_neutral_body_spread_db'] = max(spread)
        headroom = []
        compositions = [c for c in itertools.product(range(3), repeat=3) if c != (1, 1, 1)]
        assert len(compositions) == 26
        for index, composition in enumerate(compositions):
            for register, mids, extra in [('low', (50, 54, 57), 62), ('high', (62, 66, 69), 50)]:
                recipe = ''.join(f'0,{o},{f},{m},1,1\n' for o, (f, m) in enumerate(zip(composition, mids)))
                recipe += f'10,3,{composition[-1]},{extra},1,0\n'
                for color in (0., .5, 1.):
                    row = render(f'SUM_{index:02d}_{register}_color{round(100*color):03d}', recipe, color=color, room=1, nature=1, volume=1)
                    assert row['maximum_sources'] == 3 and row['starts'] == 3 and row['admission_rejections'] == 1
                    row.update(families='/'.join(WORLDS[w] for w in composition), voicing=register, midi=list(mids),
                        extra_start_declined_without_stealing=True)
                    headroom.append(row)
        for register, mids, extra in [('low', (50, 54), 57), ('high', (62, 66), 69)]:
            recipe = ''.join(f'0,{o},1,{m},1,1\n' for o, m in enumerate(mids))+f'10,2,1,{extra},1,0\n'
            for color in (0., .5, 1.):
                row = render(f'SUM_WOODLAND2_{register}_color{round(100*color):03d}', recipe, color=color, room=1, nature=1, volume=1)
                assert row['maximum_sources'] == 2 and row['starts'] == 2 and row['admission_rejections'] == 1
                row.update(families='WOODLAND/WOODLAND', voicing=register, midi=list(mids), extra_start_declined_without_stealing=True)
                headroom.append(row)
        fields = ['label', 'families', 'voicing', 'midi', 'color', 'room', 'nature', 'volume', 'body_rms_dbfs',
            'late_rms_dbfs', 'late_to_body_rms_ratio', 'integrated_lufs', 'true_peak_dbfs', 'mono_energy_ratio', 'dc',
            'maximum_sources', 'starts', 'admission_rejections', 'nonfinite_samples', 'limited_samples', 'raw_sha256',
            'events_sha256', 'state_sha256', 'recipe_sha256', 'full_pcm_64_512_identical', 'events_state_64_512_identical', 'final_second_exact_zero']
        table = captures/'SD34_GAIN_SUM_CASES.csv'
        with table.open('w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=fields, extrasaction='ignore')
            writer.writeheader()
            writer.writerows(headroom)
        worst = max(headroom, key=lambda r: r['true_peak_dbfs'])
        manifest['headroom_summary'] = dict(cases=len(headroom), ordered_three_family_compositions=26,
            true_peak_dbfs_range=[min(r['true_peak_dbfs'] for r in headroom), worst['true_peak_dbfs']],
            minimum_measured_full_volume_headroom_db=-worst['true_peak_dbfs'], worst_case=worst,
            all_source_budgets_respected=True, all_extra_starts_declined=True, cases_file=table.name, cases_sha256=sha(table))
        selections = {'global': worst}
        for name in WORLDS:
            selections[name] = max((r for r in headroom if set(r['families'].split('/')) == {name}), key=lambda r: r['true_peak_dbfs'])
        for name, full in selections.items():
            row = render('REFERENCE_'+name, recipes[full['label']], color=full['color'], room=1, nature=1, volume=.6)
            assert row['true_peak_dbfs'] <= -6
            assert row['events_sha256'] == full['events_sha256'] and row['state_sha256'] == full['state_sha256']
            row.update(selection=name, full_volume_case=full['label'], complete_events_state_equal_full_volume_case=True)
            manifest['references'].append(row)
        manifest['minimum_reference_headroom_db'] = -max(r['true_peak_dbfs'] for r in manifest['references'])
        defaults = {}
        for world, name in enumerate(WORLDS):
            for key, minor, seed in [(2, 0, 1234), (2, 0, 42), (8, 0, 1234), (8, 1, 1234), (9, 0, 1234), (9, 1, 1234)]:
                label = f'{name}_GEN_key{key}_minor{minor}_seed{seed}'
                row = render(label, f'generate,{world},{seed}\n', key=key, minor=minor, room=.5, generated=True)
                row['world'] = name
                if key == 2:
                    old = tmp/'baseline-default.wav'
                    subprocess.run([str(baseline), 'engine', str(world), str(seed), '0', '.5', '512', str(old)], check=True, capture_output=True)
                    complete_pcm(old, 112)
                    assert sha(old) == row['raw_sha256']
                    actual = trace(Path(str(files[label])+'.events.csv'))
                    for event in actual:
                        event['velocity'] = f"{float(event['velocity']):.5f}"
                    assert actual == trace(Path(str(old)+'.events.csv'))
                    actual = trace(Path(str(files[label])+'.state.csv'))
                    for state in actual:
                        state.pop('memory_returns')
                        state.pop('physical_source_mask')
                    assert actual == trace(Path(str(old)+'.state.csv'))
                    row['default_full_pcm_events_shared_state_equal_existing_renderer'] = True
                    if seed == 1234:
                        defaults[world] = files[label]
                manifest['autonomous'].append(row)
            for key, minor in [(8, 0), (9, 1)]:
                row = render(f'{name}_GEN_MAX_key{key}_minor{minor}', f'generate,{world},1234\n',
                    key=key, minor=minor, color=1, room=1, nature=1, volume=1, generated=True)
                row['world'] = name
                manifest['autonomous'].append(row)
        parts, levels = [], []
        edge = round(.04*SR)
        for world in range(3):
            x = np.asarray(complete_pcm(defaults[world], 112)[SR:14*SR], dtype=np.float64)/32768
            x[:edge] *= np.linspace(0, 1, edge)[:, None]
            x[-edge:] *= np.linspace(1, 0, edge)[:, None]
            path = tmp/f'listen-part-{world}.wav'
            write(path, x)
            parts.append(x)
            levels.append(measure(path))
        gain = min(18., -23-max(r['integrated_lufs'] for r in levels), -6-max(r['true_peak_dbfs'] for r in levels))
        assembled, sections = [], []
        for world, part in enumerate(parts):
            if assembled:
                assembled.append(np.zeros((SR//2, 2)))
            assembled.append(part*10**(gain/20))
            sections.append(dict(world=WORLDS[world], output_seconds=[world*13.5, world*13.5+13],
                source_seconds=[1, 14], raw_excerpt_level=levels[world]))
        listen = captures/'AMBIENT_World_Pegelvergleich_40s.wav'
        write(listen, np.concatenate(assembled))
        complete_pcm(listen, 40)
        manifest['listening'] = dict(file=listen.name, sha256=sha(listen), level=measure(listen), sections=sections,
            one_fixed_gain_db_across_all_worlds=gain, excerpt_edge_fade_ms=40, gap_seconds=.5,
            note='Actual distinct autonomous openings, not identical scores. No individual normalisation; hearing remains open.')
        assert manifest['listening']['level']['true_peak_dbfs'] <= -5.9
        for raw, recipe, row in sealed:
            complete_pcm(raw, row['seconds'])
            assert sha(raw) == row['raw_sha256'] and sha(recipe) == row['recipe_sha256']
            assert sha(Path(str(raw)+'.events.csv')) == row['events_sha256']
            assert sha(Path(str(raw)+'.state.csv')) == row['state_sha256']
        for relative, expected in original.items():
            assert sha(repo/relative) == expected
        for relative, expected in manifest['source_sha256'].items():
            assert sha(ROOT/relative) == expected
        assert sha(table) == manifest['headroom_summary']['cases_sha256'] and sha(listen) == manifest['listening']['sha256']
        for path in sorted(captures.iterdir()):
            if not path.name.endswith('.wav') or path == listen:
                shutil.copyfile(path, out/path.name)
                assert sha(path) == sha(out/path.name)
        complete_pcm(out/listen.name, 40)
    manual = len(manifest['solo'])+manifest['headroom_summary']['cases']+len(manifest['references'])
    manifest.update(manual_cases_without_reference=manual-4, reference_cases=4, autonomous_cases=len(manifest['autonomous']),
        existing_default_control_renders=6, total_complete_wav_renders=2*(manual+len(manifest['autonomous']))+6,
        total_full_pcm_minutes=(2*manual*12+(2*len(manifest['autonomous'])+6)*112)/60)
    (out/'SD34_GAIN_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (out/'SD34_REFERENCE_HEADROOM.json').write_text(json.dumps(dict(reference_volume=.6,
        minimum_measured_headroom_db=manifest['minimum_reference_headroom_db'], cases=manifest['references']), indent=2)+'\n')
    print('GAIN REVIEW PASS:596 complete WAV renders, source/velocity/register/mixture/reference checks, real roles, '
          'whole64/512 PCM and gain API contract. Hearing remains open.')


if __name__ == '__main__':
    main()
