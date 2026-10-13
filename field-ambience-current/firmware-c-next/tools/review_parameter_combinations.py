#!/usr/bin/env python3
"""SD35: directed reachable-control combinations, actual slots and whole PCM."""
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
from review_release_comparison import trace, on_score, physical_ownership
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = '9bde4de2646bafb027f1e76d6b7b683c32a56798'
TREE = '9308e26869589f4d4e30a53b433c158780a9ae5c'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
PROFILES = (
    ('reference', (.5, .5, .5, .5, .5, 0, .6)),
    ('fast_short_bright_full', (1, 0, 0, 1, 1, 1, 1)),
    ('dense_long_bright_full', (1, 1, 1, 1, 1, 1, 1)),
    ('slow_long_dark_full', (0, 1, 1, 0, 1, 1, 1)),
    ('fast_attack_long_release_dark_full', (1, 0, 1, 0, 1, 1, 1)),
    ('slow_attack_short_release_bright_room', (0, 1, 0, 1, 1, 0, 1)),
    ('fast_long_dry_dark', (1, 0, 1, 0, 0, 0, 1)),
    ('slow_short_dry_bright', (0, 1, 0, 1, 0, 0, 1)),
)
CONTROL_NAMES = ('activity', 'attack', 'release', 'color', 'room', 'nature', 'volume')
FAMILIES = ('COAST3', 'WOODLAND2', 'HIGHLANDS3', 'WOODLAND_WOODLAND_COAST', 'COAST_WOODLAND_HIGHLANDS')


def manual_ownership(events, states):
    on = [e for e in events if e['on'] == '1']
    tickets = []
    for e in on:
        owner, ack = int(e['owner']), int(e['ack_frame'])
        bit = 1 << owner
        reservation = next(s for s in states if int(s['frame']) == ack-441)
        assert not int(reservation['source_mask_before_tick']) & bit
        assert int(reservation['source_mask_after_tick']) & bit
        off = next(e for e in events if int(e['owner']) == owner and e['on'] == '0')
        off_frame = int(off['ack_frame'])
        assert ack < off_frame == 4*SR
        end = next(int(s['frame']) for s in states if int(s['frame']) > ack and
                   not int(s['source_mask_before_tick']) & bit)
        assert end >= off_frame
        assert all(int(s['source_mask_after_tick']) & bit for s in states
                   if ack <= int(s['frame']) < end)
        tickets.append(dict(owner=owner, reserved_frame=ack-441, on_frame=ack,
                            off_frame=off_frame, retired_frame=end))
    assert len(events) == 2*len(tickets)
    return dict(fresh_physical_reservations=len(tickets), physical_tickets=tickets,
                every_ack_has_fresh_prior_tick_reservation=True,
                every_source_remains_owned_until_actual_retirement=True)


def windows(pcm, controls):
    rows = []
    for change in controls:
        center = int(change['frame'])
        begin, end = max(0, center-SR//50), min(len(pcm), center+SR//50)
        part = np.asarray(pcm[begin:end], dtype=np.float64)/32768
        step = float(np.max(np.abs(np.diff(part, axis=0))))
        # A high bright three-source waveform can have a large natural slope.
        # Do not call that a setter discontinuity. The C paired-PCM contract
        # measures the additional first-sample change against an unmoved target.
        rows.append(dict(seconds=center/SR, maximum_adjacent_sample_step=step))
    return rows


def body_spectrum(pcm):
    part = np.asarray(pcm[round(1.5*SR):round(2.5*SR)], dtype=np.float64)/32768
    energy = np.abs(np.fft.rfft(part*np.hanning(len(part))[:, None], axis=0))**2
    hz = np.fft.rfftfreq(len(part), 1/SR)
    total = float(energy.sum())
    assert total > 0
    edges = (0, 150, 500, 2000, 6000, SR/2+1)
    return dict(window_seconds=[1.5, 2.5], stereo_rms_dbfs=float(10*np.log10(np.mean(part*part))),
                spectral_energy_fractions=[float(energy[(hz >= a) & (hz < b)].sum()/total)
                                           for a, b in zip(edges, edges[1:])],
                band_edges_hz=list(edges), psychoacoustic_alarm_acceptance=False)


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
        assert (repo/relative).read_bytes() == data, f'Changed PR161 firmware input: {relative}'
        original[relative] = hashlib.sha256(data).hexdigest()
    assert len(compiled) == 251
    sources = [ROOT/'src'/f'{n}.c' for n in SOURCES]
    inputs = [*sources, *sorted((ROOT/'include').glob('*.h')), Path(__file__),
        ROOT/'tools/render_parameter_combinations.c', ROOT/'tools/check_parameter_combinations.c',
        ROOT/'tools/render_nature_comparison.c', ROOT/'tools/review_release_comparison.py',
        ROOT/'tools/review_activity_comparison.py', ROOT/'tools/review_room_comparison.py', ROOT/'tools/package_musical_review.py']
    manifest = dict(scope='Directed SD35 reachable-control combinations through Spec0.10 actual Product. '
        'Eight risk profiles, two musical contexts, three Worlds; five full-pool compositions in two registers. '
        'Not a Cartesian sweep, continuous long-run, hardware ISR or psychoacoustic certification.',
        baseline_commit=BASE, baseline_tree=TREE, firmware_changed=False,
        compilation_inputs_unchanged_from_pr161=True, compilation_input_count=len(compiled),
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        hearing_accepted=False, range_usefulness_accepted=False, full_pcm_retained=False,
        block_sizes=[64, 512], control_tick_ms=10, silent_settling_seconds=1,
        autonomous_raw_seconds=192, autonomous_stop_seconds=160,
        manual_raw_seconds=112, manual_key_up_and_nature_stop_seconds=4,
        source_clear_used=False, profiles=[dict(name=n, **dict(zip(CONTROL_NAMES, v))) for n, v in PROFILES],
        musical_contexts=[dict(seed=1234, key=2, collection='major', tuning='Equal'),
                          dict(seed=42, key=9, collection='minor', tuning='Just')],
        removed_effect_combinations='Excitation/Damp, Resonance/Drive, Width/Chorus/Detune, Age/Blur and '
            'Shimmer are not reachable Product controls. Room feedback is derived from Room, not a free knob.',
        physical_ownership_observation='10-ms deterministic host ticks; no claim about device ISR interleaving.',
        autonomous=[], manual=[], live=[], reference_controls=[])
    with tempfile.TemporaryDirectory(prefix='parameter-combinations-') as value:
        tmp = Path(value)
        captures = tmp/'captures'
        captures.mkdir()
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include')]
        binary = tmp/'render'
        subprocess.run([*flags, str(ROOT/'tools/render_parameter_combinations.c'), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        contract = tmp/'contract'
        subprocess.run([*flags, str(ROOT/'tools/check_parameter_combinations.c'), *map(str, sources), '-lm', '-o', str(contract)], check=True)
        manifest['simultaneous_control_contract'] = subprocess.run([str(contract)], check=True, capture_output=True, text=True).stdout.strip()
        print(manifest['simultaneous_control_contract'], flush=True)
        baseline = tmp/'baseline'
        subprocess.run([*flags, str(ROOT/'tools/render_nature_comparison.c'), *map(str, sources), '-lm', '-o', str(baseline)], check=True)
        files, sealed, rows = {}, [], []

        def render(mode, family, context, profile, live=0):
            name = WORLDS[family] if mode == 'gen' else FAMILIES[family]
            label = f'{mode}_{name}_context{context}_{PROFILES[profile][0]}'+('_live' if live else '')
            seconds, stop = (192, 160) if mode == 'gen' else (112, 4)
            raw = captures/(label+'.wav')
            command = [str(binary), mode, str(family), str(context), str(profile), str(live)]
            report = subprocess.run([*command, '512', str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control.wav'
            subprocess.run([*command, '64', str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Full PCM differs at64/512'
            hashes = {}
            for extension in ('.events.csv', '.state.csv', '.controls.csv'):
                path = Path(str(raw)+extension)
                assert sha(path) == sha(Path(str(control)+extension)), 'Full trace differs at64/512'
                hashes[extension] = sha(path)
                shutil.copy2(path, out/path.name)
            events, states, controls = [trace(Path(str(raw)+e)) for e in ('.events.csv', '.state.csv', '.controls.csv')]
            assert len(states) == seconds*100
            assert all(int(s['active_sources']) <= 3 and int(s['pluck_sources']) <= 2 for s in states)
            assert all(int(s['source_mask_before_tick']).bit_count() <= 3 and
                       int(s['source_mask_after_tick']).bit_count() <= 3 for s in states)
            assert all(s['active_sources'] == '0' and s['source_mask_after_tick'] == '0' for s in states[-100:])
            ownership = physical_ownership(events, states) if mode == 'gen' else manual_ownership(events, states)
            assert not any(int(s['source_mask_before_tick']) & ~int(s['source_mask_after_tick']) for s in states)
            first_free = next(int(s['frame'])/SR for s in states[stop*100:] if s['active_sources'] == '0')
            pcm = complete_pcm(raw, seconds)
            assert not np.any(pcm[-SR:]), 'Natural stop did not become exactly silent'
            metrics, level = pcm_metrics(pcm), measure(raw)
            assert level['true_peak_dbfs'] <= -3, 'Less than3dB full-volume headroom in a directed case'
            assert metrics['mono_energy_ratio'] > .65 and max(map(abs, metrics['dc'])) < .0002
            row = dict(label=label, mode=mode, family=name, context=context, profile=PROFILES[profile][0],
                live=bool(live), seconds=seconds, **level, **metrics,
                maximum_sources=max(int(s['active_sources']) for s in states),
                maximum_pluck_sources=max(int(s['pluck_sources']) for s in states),
                starts=len(on_score(events)), completed_phrases=int(states[-1]['completed_phrases']),
                memory_returns=int(states[-1]['memory_returns']),
                admission_rejections=int(states[-1]['admission_rejections']),
                stop_to_all_owner_retirement_seconds=first_free-stop,
                physical_ownership=ownership, controls_count=len(controls),
                setter_caused_immediate_on_or_off_hook=False,
                full_pcm_events_state_controls_64_512_identical=True,
                final_second_exact_zero=True, nonfinite_samples=0, limited_samples=0,
                raw_sha256=sha(raw), trace_sha256=hashes, renderer_report=report,
                control_transition_windows=windows(pcm, controls) if live else [],
                held_body_spectrum=body_spectrum(pcm))
            row['maximum_whole_capture_adjacent_sample_step'] = int(re.search(r'max_step=(\d+)', report)[1])/32768
            if mode == 'gen':
                assert row['completed_phrases'] >= 1
                row['actual_frequencies_hz'] = sorted({float(e['hz']) for e in events if e['on'] == '1'})
            else:
                assert row['starts'] == (2 if family == 1 else 3) and row['admission_rejections'] >= 1
            files[mode, family, context, profile, live] = raw
            rows.append(row)
            sealed.append((raw, sha(raw), seconds))
            print(label, f"starts={row['starts']}; max={row['maximum_sources']}; retire+{first_free-stop:.2f}s;64/512 exact", flush=True)
            return row

        for world in range(3):
            for context in range(2):
                for profile in range(8):
                    manifest['autonomous'].append(render('gen', world, context, profile))
            manifest['live'].append(render('gen', world, 0, 0, 1))
            current = files['gen', world, 0, 0, 0]
            before = tmp/(WORLDS[world]+'-existing-default.wav')
            subprocess.run([str(baseline), 'engine', str(world), '1234', '0', '.5', '512', str(before)], check=True, capture_output=True)
            a, b = complete_pcm(current, 192), complete_pcm(before, 112)
            assert np.array_equal(a[:80*SR], b[:80*SR])
            def prefix_score(path):
                return [(int(e['ack_frame']), int(e['on']), int(e['owner']), float(e['hz']), round(float(e['velocity']), 5),
                         int(e['kind']), int(e['completed_phrases'])) for e in trace(Path(str(path)+'.events.csv'))
                        if int(e['ack_frame']) < 80*SR]
            assert prefix_score(current) == prefix_score(before)
            manifest['reference_controls'].append(dict(world=WORLDS[world], compared_seconds=80,
                pcm_and_complete_event_prefix_exact_existing_driver=True,
                existing_capture_sha256=sha(before), compared_pcm_sha256=hashlib.sha256(a[:80*SR].tobytes()).hexdigest()))
            sealed.append((before, sha(before), 112))
        for family in range(5):
            for register in range(2):
                for profile in (2, 4):
                    manifest['manual'].append(render('manual', family, register, profile))
        # The renderer moves live manual controls before4s key-up, so rapid
        # changes occur on physically occupied sources.
        for family in range(3):
            for register in range(2):
                manifest['live'].append(render('manual', family, register, 2, 1))

        # Short real stress excerpts; one gain for all Worlds and both states.
        # Master is0.6 in reference and1 in stress. Retain this real level change.
        parts, sections, position = [], [], 0.
        for world in range(3):
            for profile in (0, 2):
                raw = files['gen', world, 0, profile, 0]
                segment = np.asarray(complete_pcm(raw, 192)[2*SR:round(8.25*SR)], dtype=np.float64)/32768
                edge = round(.04*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                parts.append(segment)
                sections.append(dict(world=WORLDS[world], profile=PROFILES[profile][0], begin_seconds=position,
                    end_seconds=position+6.25, raw_excerpt_seconds=[2, 8.25], raw_sha256=sha(raw)))
                position += 6.25
                if len(sections) < 6:
                    parts.append(np.zeros((SR//2, 2)));position += .5
        target = out/'AMBIENT_Parameter_Grenzen_40s.wav'
        joined = np.concatenate(parts)
        write(tmp/'joined.wav', joined)
        pre = measure(tmp/'joined.wav')
        gain = min(12., -24-pre['integrated_lufs'], -6-pre['true_peak_dbfs'])
        write(target, joined*10**(gain/20))
        final = measure(target)
        assert final['true_peak_dbfs'] <= -5.9 and target.stat().st_size == 44+40*SR*4
        manifest['listening'] = dict(file=target.name, seconds=40, sections=sections, constant_gain_db=gain,
            reference_master=.6, stress_master=1, no_agc=True,
            comparison='Different real reachable control/level states, not matched scores or loudness. '
                'No new musical composition or positive range acceptance implied.', **final, sha256=sha(target))
        for raw, expected, seconds in sealed:
            complete_pcm(raw, seconds)
            assert sha(raw) == expected
        manifest['full_wav_render_count'] = 2*len(rows)+3
        manifest['total_rendered_pcm_minutes'] = (2*sum(r['seconds'] for r in rows)+3*112)/60
        manifest['render_minutes_scope'] = 'Separate scenarios, not one continuous session; contract in-memory PCM excluded.'
        fields = ['label', 'mode', 'family', 'context', 'profile', 'live', 'seconds', 'starts',
            'completed_phrases', 'maximum_sources', 'maximum_pluck_sources', 'integrated_lufs',
            'true_peak_dbfs', 'mono_energy_ratio', 'stop_to_all_owner_retirement_seconds',
            'maximum_whole_capture_adjacent_sample_step', 'raw_sha256']
        with (out/'SD35_PARAMETER_CASES.csv').open('w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader()
            writer.writerows({k: r[k] for k in fields} for r in rows)
    manifest['firmware_compilation_sha256_unchanged'] = original
    (out/'SD35_PARAMETER_METRICS.json').write_text(json.dumps(manifest, indent=2, allow_nan=False)+'\n')
    print('SD35 PASS', manifest['full_wav_render_count'], 'WAV renders;',
          manifest['total_rendered_pcm_minutes'], 'min PCM; firmware unchanged; hearing/device open', flush=True)


if __name__ == '__main__':
    main()
