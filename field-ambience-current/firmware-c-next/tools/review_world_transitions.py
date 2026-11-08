#!/usr/bin/env python3
"""SD37: actual World ownership, latest targets, surviving Room and PCM."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from check_prepared_scenes import compare_inputs

import numpy as np
from package_musical_review import measure
from review_activity_comparison import sha, complete_pcm, pcm_metrics
from review_release_comparison import trace
from review_room_comparison import ROOT, SR, SOURCES, write

BASE = '4c784f64de8f27686be6539b228be578d147620e'
TREE = 'ae4e183c6a88ab5193158e6784cc4e6e677b222e'
WORLDS = ('COAST', 'WOODLAND', 'HIGHLANDS')
MODES = ('held', 'pending', 'releasing', 'batch', 'rapid_audio', 'manual')
FAMILY_COLUMNS = ('bowed_mask', 'pluck_mask', 'horn_mask')


def physical_tickets(events, states):
    by_frame, current, tickets = {}, {}, []
    for event in events:
        by_frame.setdefault(int(event['ack_frame']), []).append(event)
    for state in states:
        frame, before, after = (int(state[k]) for k in ('frame', 'before_mask', 'after_mask'))
        for owner, ticket in current.items():
            if ticket['retired_frame'] is None and frame > ticket['reserved_frame'] and not before & (1 << owner):
                ticket['retired_frame'] = frame
        for event in by_frame.pop(frame, []):
            owner = int(event['owner'])
            ticket = current[owner]
            assert ticket['family'] == int(event['family'])
            field = 'on_frame' if event['on'] == '1' else 'off_frame'
            assert ticket[field] is None
            if field == 'on_frame':
                assert ticket['reserved_frame'] == frame-441
            else:
                assert ticket['on_frame'] is not None
            ticket[field] = frame
        assert not before & ~after, 'Control action truncated a physical source'
        for owner in range(16):
            bit = 1 << owner
            if after & bit and not before & bit:
                previous = current.get(owner)
                if previous:
                    assert previous['retired_frame'] is not None and previous['retired_frame'] <= frame
                families = [i for i, k in enumerate(FAMILY_COLUMNS) if int(state[k]) & bit]
                assert len(families) == 1
                ticket = dict(owner=owner, family=families[0], reserved_frame=frame,
                              on_frame=None, off_frame=None, retired_frame=None)
                current[owner] = ticket
                tickets.append(ticket)
    assert not by_frame and tickets
    for ticket in tickets:
        assert ticket['retired_frame'] is not None
        if ticket['on_frame'] is None:
            assert ticket['off_frame'] is None
        else:
            assert ticket['off_frame'] is not None
            assert ticket['on_frame'] <= ticket['off_frame'] <= ticket['retired_frame']
    assert sum(t['on_frame'] is not None for t in tickets) == sum(e['on'] == '1' for e in events)
    return tickets


def canonical_state(states):
    # ambient_room_peak() measures the LAST render block, so64/512 observe
    # different wet-peak windows despite identical complete audio.
    return [{k: v for k, v in s.items() if k not in ('room_peak_last_block', 'pitch_history_count')} for s in states]


def canonical_actions(actions):
    return [{k: v for k, v in s.items() if k != 'room_peak_last_block'} for s in actions]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    assert subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=repo, text=True).strip() == TREE
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')], cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in ('.c', '.h', '.s', '.ld', '.cmake') or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 251
    default_guard = compare_inputs(repo, ROOT, BASE, compiled)
    sources = [ROOT/'src'/f'{name}.c' for name in SOURCES]
    inputs = [*sources, *sorted((ROOT/'include').glob('*.h')), Path(__file__),
        ROOT/'tools/render_world_transitions.c', ROOT/'tools/check_world_transitions.c',
        ROOT/'tools/review_activity_comparison.py', ROOT/'tools/review_release_comparison.py',
        ROOT/'tools/review_room_comparison.py', ROOT/'tools/package_musical_review.py']
    manifest = dict(scope='SD37 only: all six directed actual Product World changes; full generated '
        'and manual pools, pending cancellation, released sources, latest targets and shared Room. '
        'Directed scenarios, not every key/tuning/timing combination, continuous long-run or device ISR proof.',
        baseline_commit=BASE, baseline_tree=TREE, default_runtime_changed=False, prepared_scene_hooks_changed=True,
        compilation_inputs_unchanged_from_pr162=False, compilation_input_count=251,
        byte_equal_compilation_input_count=default_guard['existing_inputs_byte_equal'],
        default_scene_guard=default_guard,
        source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        hearing_accepted=False, hardware_accepted=False, full_pcm_retained=False,
        block_sizes=[64, 512], control_tick_ms=10, silent_settling_seconds=1,
        parameters=dict(activity=.5, attack=1, release=1, color=1, room=1, volume=.6,
                        default_nature=0, optional_nature_comparison=1),
        musical_contexts=[dict(seed=1234, key=2, collection='major', tuning='Equal'),
                          dict(seed=42, key=9, collection='minor', tuning='Just')],
        generated_raw_seconds=128, generated_stop_seconds=96,
        manual_raw_seconds=80, manual_key_up_and_nature_stop_seconds=4,
        source_clear_used=False, room_peak_observation='Last-block wet peaks have different observation '
            'windows at64/512; exact PCM/events/ownership/targets are checked separately.', cases=[])
    with tempfile.TemporaryDirectory(prefix='world-transition-review-') as directory:
        tmp = Path(directory)
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT', '-I'+str(ROOT/'include')]
        binary, contract = tmp/'render', tmp/'contract'
        for output, name in ((binary, 'render_world_transitions'), (contract, 'check_world_transitions')):
            subprocess.run([*flags, str(ROOT/'tools'/f'{name}.c'), *map(str, sources), '-lm', '-o', str(output)], check=True)
        manifest['paired_pcm_contract'] = subprocess.run([str(contract)], check=True, capture_output=True, text=True).stdout.strip()
        print(manifest['paired_pcm_contract'], flush=True)
        files, data, sealed = {}, {}, []

        def render(fr, to, mode=0, context=0, nature=0, wrap=0):
            label = f'{WORLDS[fr]}_to_{WORLDS[to]}_{MODES[mode]}_context{context}_nature{nature}_wrap{wrap}'
            seconds, stop = (80, 4) if mode == 5 else (128, 96)
            raw, control = tmp/(label+'.wav'), tmp/'block-control.wav'
            command = [str(binary), str(fr), str(to), str(mode), str(context), str(nature)]
            report = subprocess.run([*command, '512', str(wrap), str(raw)], check=True, capture_output=True, text=True).stderr.strip()
            subprocess.run([*command, '64', str(wrap), str(control)], check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Complete PCM differs at64/512'
            events, states, actions = [trace(Path(str(raw)+e)) for e in ('.events.csv', '.state.csv', '.actions.csv')]
            events64, states64, actions64 = [trace(Path(str(control)+e)) for e in ('.events.csv', '.state.csv', '.actions.csv')]
            assert events == events64 and canonical_state(states) == canonical_state(states64)
            assert canonical_actions(actions) == canonical_actions(actions64)
            history_differences, run, longest = [], 0, 0
            for a, b in zip(states, states64):
                if a['pitch_history_count'] != b['pitch_history_count']:
                    assert a['active_sources'] == b['active_sources'] == '0'
                    history_differences.append(dict(seconds=int(a['frame'])/SR,
                        block512_count=int(a['pitch_history_count']), block64_count=int(b['pitch_history_count'])))
                    run += 1;longest = max(longest, run)
                else:
                    run = 0
            assert longest <= 2, 'Quiet history divergence exceeds20ms in a source-free interval'
            assert len(states) == seconds*100
            for s in states:
                masks = [int(s[k]) for k in FAMILY_COLUMNS]
                assert not masks[0] & masks[1] and not masks[0] & masks[2] and not masks[1] & masks[2]
                assert int(s['active_sources']) <= 3 and masks[1].bit_count() <= 2
                assert int(s['after_mask']) == masks[0] | masks[1] | masks[2]
            assert all(s['active_sources'] == '0' and s['after_mask'] == '0' for s in states[-100:])
            tickets = physical_tickets(events, states)
            first, last = int(actions[0]['frame']), int(actions[-1]['frame'])
            assert all(s['world'] == str(to) for s in states if int(s['frame']) >= last)
            on = [e for e in events if e['on'] == '1']
            later = [e for e in on if int(e['ack_frame']) >= last]
            assert all(int(e['family']) == to for e in later), 'Stale/intermediate family restarted'
            old = [t for t in tickets if t['reserved_frame'] <= first and t['family'] == fr]
            assert old
            if mode != 5:
                assert later and int(states[-1]['completed_phrases']) >= 1
                assert all(t['retired_frame'] <= first+round(.101*SR) for t in old)
                if mode == 1:
                    assert all(t['on_frame'] is None for t in old)
                    assert sum(t['on_frame'] is None for t in tickets) == (2 if fr == 0 else 1)
                    assert states[1]['pitch_history_count'] == '0'
                else:
                    assert len(old) == (2 if fr == 1 else 3)
                    assert all(t['on_frame'] is not None for t in old)
                    assert float(actions[0]['room_peak_last_block']) > 0
                    retire = max(t['retired_frame'] for t in old)
                    state = next(s for s in states if int(s['frame']) == retire)
                    assert int(state['pitch_history_count']) >= len(old), 'Outgoing Hz/tail history lost'
            else:
                assert len(old) == (2 if fr == 1 else 3)
                assert all(t['off_frame'] == 4*SR and t['retired_frame'] > 4*SR for t in old)
                assert len(later) == (1 if fr == 1 else 0)
                assert int(states[-1]['rejections']) >= 1
            pcm = complete_pcm(raw, seconds)
            assert not np.any(pcm[-SR:])
            if mode == 1:
                beginning = min(t['reserved_frame'] for t in tickets if t['on_frame'] is not None)
                assert not np.any(pcm[:beginning]), 'Canceled preparation made phantom audio'
            metrics, levels = pcm_metrics(pcm), measure(raw)
            assert levels['true_peak_dbfs'] <= -6 and metrics['mono_energy_ratio'] > .65
            assert max(map(abs, metrics['dc'])) < .0002
            hashes = {}
            for extension in ('.events.csv', '.state.csv', '.actions.csv'):
                path = Path(str(raw)+extension);hashes[extension] = sha(path);shutil.copy2(path, out/path.name)
            ledger = out/(label+'.physical.csv')
            with ledger.open('w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=list(tickets[0]));writer.writeheader();writer.writerows(tickets)
            row = dict(label=label, from_world=WORLDS[fr], to_world=WORLDS[to], mode=MODES[mode],
                context=context, nature=nature, timer_wrap=bool(wrap), seconds=seconds,
                switch_first_seconds=first/SR, switch_latest_seconds=last/SR,
                old_physical_sources=len(old), old_acknowledged_sources=sum(t['on_frame'] is not None for t in old),
                last_old_physical_end_after_first_request_seconds=(max(t['retired_frame'] for t in old)-first)/SR,
                first_new_family_ack_after_latest_request_seconds=(int(later[0]['ack_frame'])-last)/SR if later else None,
                acknowledged_starts=len(on), canceled_before_dsp_starts=sum(t['on_frame'] is None for t in tickets),
                completed_target_phrases=int(states[-1]['completed_phrases']),
                maximum_sources=max(int(s['active_sources']) for s in states),
                maximum_string_sources=max(int(s['pluck_mask']).bit_count() for s in states),
                nonfinite_samples=0, limited_samples=0, final_second_exact_zero=True,
                full_pcm_events_physical_state_targets_64_512_identical=True,
                fresh_physical_reservations=len(tickets), physical_ledger_sha256=sha(ledger),
                source_free_quiet_history_differences=history_differences,
                longest_quiet_history_difference_seconds=longest/100,
                trace_sha256=hashes, raw_sha256=sha(raw), renderer_report=report, **metrics, **levels)
            files[fr, to, mode, context, nature, wrap] = raw
            data[fr, to, mode, context, nature, wrap] = events, canonical_state(states)
            manifest['cases'].append(row);sealed.append((raw, row['raw_sha256'], seconds))
            print(label, f"{row['acknowledged_starts']} starts; old ends+{row['last_old_physical_end_after_first_request_seconds']:.3f}s;64/512 exact", flush=True)
            return row

        for fr in range(3):
            for to in range(3):
                if fr == to:
                    continue
                normal = render(fr, to)
                render(fr, to, context=1)
                for mode in (1, 2, 3, 4, 5):
                    row = render(fr, to, mode)
                    if mode == 3:
                        assert row['raw_sha256'] == normal['raw_sha256']
                        assert data[fr, to, mode, 0, 0, 0] == data[fr, to, 0, 0, 0, 0]
                        row['batch_equals_direct_in_complete_pcm_events_physical_state'] = True
                optional = render(fr, to, nature=1)
                assert data[fr, to, 0, 0, 1, 0] == data[fr, to, 0, 0, 0, 0]
                optional['optional_nature_keeps_complete_score_and_physical_state'] = True
                wrapped = render(fr, to, wrap=1)
                assert wrapped['raw_sha256'] == normal['raw_sha256']
                assert data[fr, to, 0, 0, 0, 1] == data[fr, to, 0, 0, 0, 0]
                wrapped['timer_wrap_equals_zero_clock_complete_pcm_events_state'] = True

        parts, sections, position = [], [], 0.
        for fr in range(3):
            for to in range(3):
                if fr == to:
                    continue
                raw = files[fr, to, 0, 0, 0, 0]
                segment = np.asarray(complete_pcm(raw, 128)[round(.7*SR):round(8.7*SR)], dtype=np.float64)/32768
                edge = round(.04*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None];segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                sections.append(dict(from_world=WORLDS[fr], to_world=WORLDS[to], begin_seconds=position,
                    end_seconds=position+8, actual_world_request_in_file_seconds=position+2,
                    raw_excerpt_seconds=[.7, 8.7], raw_sha256=sha(raw)))
                parts.append(segment);position += 8
                if len(sections) < 6:
                    parts.append(np.zeros((SR//2, 2)));position += .5
        joined = np.concatenate(parts);write(tmp/'joined.wav', joined)
        pre = measure(tmp/'joined.wav');gain = min(12., -24-pre['integrated_lufs'], -6-pre['true_peak_dbfs'])
        preview = out/'AMBIENT_Worldwechsel_50p5s.wav';write(preview, joined*10**(gain/20));level = measure(preview)
        assert level['true_peak_dbfs'] <= -5.9 and len(joined) == round(50.5*SR)
        manifest['listening'] = dict(file=preview.name, seconds=50.5, sections=sections,
            constant_gain_db=gain, no_agc=True, source='Actual generated maxRoom/Attack/Release/Color transition; '
                'Nature0, one gain for six genuine changes.40ms crop edges, five0.5s gaps; no crossfade at World request.',
            sha256=sha(preview), **level)
        for path, expected, seconds in sealed:
            complete_pcm(path, seconds);assert sha(path) == expected
        manifest['full_wav_render_count'] = 2*len(manifest['cases'])
        manifest['total_rendered_pcm_minutes'] = 2*sum(r['seconds'] for r in manifest['cases'])/60
        fields = ['label', 'from_world', 'to_world', 'mode', 'context', 'nature', 'timer_wrap',
            'acknowledged_starts', 'canceled_before_dsp_starts', 'maximum_sources', 'maximum_string_sources',
            'last_old_physical_end_after_first_request_seconds', 'first_new_family_ack_after_latest_request_seconds',
            'integrated_lufs', 'true_peak_dbfs', 'mono_energy_ratio', 'raw_sha256']
        with (out/'SD37_WORLD_TRANSITION_CASES.csv').open('w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=fields);writer.writeheader()
            writer.writerows({k: row[k] for k in fields} for row in manifest['cases'])
    (out/'SD37_WORLD_TRANSITION_METRICS.json').write_text(json.dumps(manifest, indent=2, allow_nan=False)+'\n')
    print('SD37 PASS', manifest['full_wav_render_count'], 'whole WAV renders;',
          manifest['total_rendered_pcm_minutes'], 'minutes of separate PCM; hearing/device open', flush=True)


if __name__ == '__main__':
    main()
