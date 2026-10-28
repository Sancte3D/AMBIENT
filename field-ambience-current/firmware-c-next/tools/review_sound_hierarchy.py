#!/usr/bin/env python3
"""Measure Cell / music-return / Nature hierarchy in the unchanged Product DSP.

The observer wraps two calls in a host-only engine object. Ordinary and observed
engines are compared byte for byte, as are selected 37/512-frame performances.
RMS ratios are engineering diagnostics, not loudness or masking judgements.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile

from review_cell_unfold import ROOT, SOURCES

SR = 44100
SECONDS = 40
NAMES = ['COAST', 'WOODLAND', 'HIGHLANDS']
PROFILES = ['default', 'room_only', 'delay_max', 'grain_max', 'drive_max',
            'crush_max', 'phaser_max', 'all_max', 'grain_freeze', 'color_only']


def run(command, **kwargs):
    return subprocess.check_output([str(x) for x in command], **kwargs)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def db(value):
    return 20 * math.log10(value) if value > 0 else None


def relative(numerator, denominator):
    return db(numerator / denominator) if denominator > 1e-12 else None


def band(rows, start, stop):
    chosen = rows[start:stop]
    # Each row contains equal-duration stereo RMS, then mono RMS and PCM peak.
    rms = [math.sqrt(sum(row[k]**2 for row in chosen) / len(chosen)) for k in range(5)]
    return dict(dry_rms=rms[0], music_return_rms=rms[1], nature_rms=rms[2],
                output_rms_dbfs=db(rms[3]), mono_to_stereo_db=relative(rms[4], rms[3]),
                return_to_dry_db=relative(rms[1], rms[0]),
                nature_to_dry_db=relative(rms[2], rms[0]),
                worst_1s_return_to_dry_db=max((relative(r[1], r[0]) for r in chosen if r[0]>1e-12 and r[1]>0), default=None),
                worst_1s_nature_to_dry_db=max((relative(r[2], r[0]) for r in chosen if r[0]>1e-12 and r[2]>0), default=None))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    inputs = sorted(set(list((ROOT/'include').glob('*.h')) +
                        [ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']] +
                        [Path(__file__), ROOT/'tools/render_sound_hierarchy.c', ROOT/'tools/review_cell_unfold.py']))
    before = {str(p.relative_to(ROOT)): sha(p) for p in inputs}
    cases = {}; selected_pcm = {}; renders = 0; exact_frames = 0
    cc = os.environ.get('CC', 'cc')
    flags = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
             '-DFAM_SOUND_PRODUCT', '-DFAM_SOUND_FINISH_CANDIDATE', '-I'+str(ROOT/'include')]
    # Exact current physical-input floor, read from the compiled cells.h below.
    with tempfile.TemporaryDirectory(prefix='sound-hierarchy-') as name:
        tmp = Path(name)
        floor_c = tmp/'floor.c'
        floor_c.write_text('#include "cells.h"\n#include <stdio.h>\nint main(void){printf("%.9g",(double)(CELL_AMP_MIN/CELL_AMP_MAX));}\n')
        run([cc, *flags, floor_c, '-o', tmp/'floor'])
        physical_floor = float(run([tmp/'floor'], text=True))
        objects = []
        for source in SOURCES+['gesture']:
            obj = tmp/(source+'.o')
            extra = (['-Dambient_room_process=hierarchy_room_process', '-Dnature_render=hierarchy_nature_render']
                     if source == 'engine_product' else [])
            run([cc, *flags, *extra, '-c', ROOT/'src'/f'{source}.c', '-o', obj])
            objects.append(obj)
        driver = ROOT/'tools/render_sound_hierarchy.c'
        run([cc, *flags, '-DHIERARCHY_OBSERVER', driver, *objects, '-lm', '-o', tmp/'observed'])
        run([cc, *flags, driver, ROOT/'src/engine_product.c', *objects[1:], '-lm', '-o', tmp/'plain'])
        sanitized_flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        run([cc, *flags, *sanitized_flags, driver,
             *[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']], '-lm', '-o', tmp/'sanitized'])
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
        policy = json.loads(run([tmp/'plain', '--policy'], text=True))
        assert json.loads(run([tmp/'sanitized', '--policy'], text=True, env=env)) == policy

        def capture(world, midi=50, velocity=.7, plate=1, room=.5, nature=1,
                    profile=0, density=1, seed=1234, cap=512, binary='observed'):
            nonlocal renders
            pcm = tmp/'capture.pcm'
            command = [tmp/binary, pcm, world, midi, velocity, plate, room, nature,
                       profile, cap, density, seed]
            value = json.loads(run(command, text=True, env=env))
            raw = pcm.read_bytes(); assert len(raw) == SECONDS*SR*4
            renders += 1
            value['pcm_sha256'] = hashlib.sha256(raw).hexdigest()
            assert not value['nonfinite_samples'] and not value['limited_samples'], command
            value.update(world=NAMES[world], world_index=world, midi=midi,
                         velocity=velocity, plate=bool(plate), room=room, nature=nature,
                         profile=PROFILES[profile], density=density, seed=seed,
                         cap=cap, observer=binary == 'observed')
            if binary == 'observed':
                value['mature_6_24s'] = band(value['seconds'], 6, 24)
                value['late_20_24s'] = band(value['seconds'], 20, 24)
                value['release_30_40s'] = band(value['seconds'], 30, 40)
            return value, raw

        def key(w, midi, velocity, plate, room, nature, profile, density, seed):
            return (w, midi, velocity, plate, room, nature, profile, density, seed)

        def add(w, midi=50, velocity=.7, plate=1, room=.5, nature=1,
                profile=0, density=1, seed=1234):
            k = key(w, midi, velocity, plate, room, nature, profile, density, seed)
            if k not in cases:
                value, raw = capture(*k)
                cases[k] = value
                if (midi==50 and velocity==.7 and room in (.5, 1) and nature==1 and density==1 and seed==1234
                        and profile in (0, 1, 7, 8, 9)):
                    selected_pcm[k] = raw
            return cases[k]

        for w in range(3):
            for plate in (0, 1):
                for midi in (50, 57, 62):
                    for velocity in (physical_floor, .7):add(w, midi, velocity, plate)
                for room in (0, .25, .5, .75, 1):
                    for profile in (0, 1):add(w, plate=plate, room=room, profile=profile)
                for profile in range(2, 10):add(w, plate=plate, room=1, profile=profile)
                for velocity in (physical_floor, .7):
                    for nature in (0, .35):add(w, velocity=velocity, plate=plate, nature=nature)
                add(w, velocity=1, plate=plate, room=1, profile=7, density=2 if w==1 else 3)
                for velocity in (physical_floor, .7):add(w, velocity=velocity, plate=plate, seed=42)
            print(f'{NAMES[w]}: default/Room/FX endpoints, physical Cell floor, Nature, seed and full-pool captures measured', flush=True)

        # Observe without changing output; compare complete ordinary PCM, not a
        # waveform excerpt or a digest alone. Check selected render partitions.
        observer_pairs = []; partition_pairs = []; sanitizer_pairs = []
        for w in range(3):
            for profile, room in ((0, .5), (7, 1), (8, 1)):
                k = key(w, 50, .7, 1, room, 1, profile, 1, 1234)
                reference = cases[k]; raw = selected_pcm[k]
                for binary, cap, destination in [('plain', 512, observer_pairs), ('observed', 37, partition_pairs)]:
                    other, compared = capture(*k, cap=cap, binary=binary)
                    assert raw == compared and other['event_fingerprint'] == reference['event_fingerprint']
                    if binary == 'observed':
                        assert other['seconds'] == reference['seconds'] and other['stem_fingerprints'] == reference['stem_fingerprints']
                    exact_frames += SECONDS*SR
                    destination.append(dict(world=NAMES[w], profile=PROFILES[profile], stereo_frames=SECONDS*SR))
            k = key(w, 50, .7, 1, 1, 1, 7, 1, 1234)
            other, compared = capture(*k, binary='sanitized')
            assert compared == selected_pcm[k] and other['event_fingerprint'] == cases[k]['event_fingerprint']
            sanitizer_pairs.append(dict(world=NAMES[w], profile='all_max', stereo_frames=SECONDS*SR))
            exact_frames += SECONDS*SR

        # All settings leave the direct source and note starts unchanged. The
        # hashes here are explicitly fingerprints, unlike full PCM pairs above.
        sources = {}; event_traces = {}
        for k, c in cases.items():
            group = (k[0], k[1], k[2], k[7], k[8])
            fingerprint = c['stem_fingerprints'][0]
            assert sources.setdefault(group, fingerprint) == fingerprint
            assert event_traces.setdefault(group, c['event_fingerprint']) == c['event_fingerprint']
            if c['room'] == 0:assert all(row[1] == 0 for row in c['seconds'][2:])
            if c['nature'] == 0:assert all(row[2] == 0 for row in c['seconds'])

        fdn_bypass = []
        for w in range(3):
            base = key(w, 50, .7, 0, 1, 1, 1, 1, 1234)
            color = key(w, 50, .7, 0, 1, 1, 9, 1, 1234)
            assert selected_pcm[base] == selected_pcm[color]
            exact_frames += SECONDS*SR
            fdn_bypass.append(dict(world=NAMES[w], stereo_frames=SECONDS*SR,
                                   room_only_equals_all_color_at_max=True))
        after = {str(p.relative_to(ROOT)): sha(p) for p in inputs}; assert before == after
        repo = run(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip()
        proof = dict(schema=1, firmware_base=run(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(),
                     sample_rate=SR, seconds_each=SECONDS, captures=renders, measured_cases=len(cases),
                     physical_engine_velocity_floor=physical_floor, master=1,
                     source_start_s=2, physical_hold_release_s=26, freeze_on_s=10,
                     measurement='RMS of exact bus differences with matching per-stem DC tracking, before common Volume/gate/PCM; no normalization',
                     diagnostic_nature_goal=dict(mean_below_dry_db=-24, worst_1s_below_dry_db=-20,
                                                origin='Existing two-Cell Nature regression thresholds; applied diagnostically to single/quiet Cells, not newly approved product ranges'),
                     policy=policy, observer_pairs=observer_pairs, partition_pairs=partition_pairs,
                     sanitizer_pairs=sanitizer_pairs, fdn_color_bypass_pairs=fdn_bypass,
                     exact_stereo_frames_compared=exact_frames,
                     dry_source_fingerprints_and_note_event_fingerprints_invariant=True,
                     all_measured_output_protection_counters_zero=True,
                     hearing_accepted=False, device_accepted=False,
                     limitations='One key/collection and neutral Color/Attack/Release, three notes, two seeds (42 only selected D3 cases), fixed 40s manual passages. Not exhaustive parameter combinations, perceptual masking, World recognition, real PCB/DAC/stack/timing or long-term listening. Silent-Freeze restriction is retained-harmony policy with a UX conflict, not permission to discard the pitch ledger.',
                     inputs_sha256=after, cases=list(cases.values()))
        metrics = out/'SOUND_HIERARCHY_METRICS.json'
        with metrics.open('w') as f:
            # Full one-second values remain available; compact case rows avoid
            # a huge mostly-whitespace report while keeping exact raw numbers.
            f.write('{\n')
            for index, (name, value) in enumerate(proof.items()):
                if index:f.write(',\n')
                f.write(json.dumps(name)+': ')
                if name == 'cases':f.write('[\n'+',\n'.join(json.dumps(c, separators=(',', ':')) for c in value)+'\n]')
                else:f.write(json.dumps(value, indent=2))
            f.write('\n}\n');f.flush();os.fsync(f.fileno())
        print(json.dumps({k:v for k,v in proof.items() if k not in ('cases', 'inputs_sha256', 'policy')}, indent=2), flush=True)


if __name__ == '__main__':
    main()
