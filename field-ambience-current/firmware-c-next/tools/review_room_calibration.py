#!/usr/bin/env python3
"""SD19: exact PR152 Room vs calibrated return on matched real performances."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure, read
from review_room_comparison import ROOT, SR, SOURCES, impulse_metrics, mono_ratio, sha, write

BASE = '09319610912b81a8c71860479a6eeba754f53427'
ROOM_PATH = 'field-ambience-current/firmware-c-next/src/ambient_room.c'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--candidate-audit', type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    audit = args.candidate_audit.resolve() if args.candidate_audit else out/'candidate-audit'
    if not args.candidate_audit:
        subprocess.run(['python3', str(ROOT/'tools/review_room_comparison.py'), str(audit)], check=True)
    current = json.loads((audit/'SD19_ROOM_METRICS.json').read_text())
    for path, expected in current['source_sha256'].items():
        assert sha(ROOT/path) == expected, f'Stale candidate audit: {path}'
    historical = json.loads((ROOT.parent/'docs/audio/SD19_ROOM_METRICS.json').read_text())
    sources = [ROOT/'src'/f'{s}.c' for s in SOURCES]
    driver = ROOT/'tools/render_room_comparison.c'
    for source in [p for p in sources if p.name != 'ambient_room.c']:
        assert sha(source) == historical['source_sha256'][str(source.relative_to(ROOT))]
    for path, expected in historical['source_sha256'].items():
        if path.startswith('include/'):
            assert sha(ROOT/path) == expected, 'Baseline header changed'
    baseline_source = subprocess.check_output(['git', 'show', f'{BASE}:{ROOM_PATH}'], cwd=ROOT)
    assert hashlib.sha256(baseline_source).hexdigest() == historical['source_sha256']['src/ambient_room.c']
    manifest = dict(baseline_commit=BASE, scope='Only actual shared Room return calibration differs. '
        'Real note-on/off histories matched, Room .5 in both. Four 6.5s sections: '
        'old held / new held / old release+tail / new release+tail, three .5s gaps.',
        parameters=current['parameters'] | {'room': .5}, candidate_audit_sha256=sha(audit/'SD19_ROOM_METRICS.json'),
        candidate_core_sha256=current['source_sha256'], baseline_room_sha256=hashlib.sha256(baseline_source).hexdigest(),
        tool_sha256=sha(Path(__file__)), comparison_gap_seconds=.5, cut_edge_fade_ms=40,
        hearing_accepted=False, worlds=[], impulse=[])
    with tempfile.TemporaryDirectory(prefix='room-baseline-') as value:
        tmp = Path(value)
        baseline = tmp/'ambient_room_pr152.c'
        baseline.write_bytes(baseline_source)
        binary = tmp/'render-baseline'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'), str(driver),
            *[str(baseline if p.name == 'ambient_room.c' else p) for p in sources],
            '-lm', '-o', str(binary)], check=True)
        for amount in (.24, .5, 1.):
            impulse = tmp/'impulse.wav'
            subprocess.run([str(binary), 'impulse', '0', str(amount), '512', str(impulse)],
                           check=True, capture_output=True)
            old = impulse_metrics(impulse, amount)
            new = next(r for r in current['impulse'] if r['amount'] == amount)
            for field in ('broadband_T30_seconds', 'mono_energy_ratio'):
                assert abs(old[field]-new[field]) < 1e-6, 'Calibration changed decay or mono shape'
            assert old['first_wet_frame'] == new['first_wet_frame']
            manifest['impulse'].append(dict(amount=amount, baseline=old, candidate=new,
                unchanged_normalized_decay_mono_onset=True))
        for world, name in enumerate(('COAST', 'WOODLAND', 'HIGHLANDS')):
            raw = out/f'{name}_pr152_room050_raw_32s.wav'
            report = subprocess.run([str(binary), 'engine', str(world), '.5', '512', str(raw)],
                check=True, capture_output=True, text=True).stderr.strip()
            trace = Path(str(raw)+'.events.csv')
            old_world = next(w for w in historical['worlds'] if w['world'] == name)
            old_reference = next(v for v in old_world['variants'] if v['amount'] == .5)
            # The shared driver gained explicit I/O error checks; its normal
            # output must still reproduce the exact published PR152 baseline.
            published_pcm_match = sha(raw) == old_reference['raw_sha256']
            published_trace_match = sha(trace) == old_reference['trace_sha256']
            new_raw = audit/f'{name}_room_050_raw_32s.wav'
            assert trace.read_bytes() == Path(str(new_raw)+'.events.csv').read_bytes(), 'Changed musical score'
            row = next(w for w in current['worlds'] if w['world'] == name)
            new = next(v for v in row['variants'] if v['amount'] == .5)
            old_end = int(re.search(r'final_ms=(\d+)', report)[1])
            new_end = int(re.search(r'final_ms=(\d+)', new['renderer_report'])[1])
            assert old_end == new_end
            old_pcm, new_pcm = read(raw), read(new_raw)
            assert mono_ratio(old_pcm) > .65 and mono_ratio(new_pcm) > .65
            # Dry is a real render from the candidate audit. Room changes only
            # the return, so this also measures the identical baseline body.
            dry = read(audit/f'{name}_room_000_raw_32s.wav')[3*SR:int(9.5*SR)]
            ratios = [float(np.sqrt(np.sum((x[3*SR:int(9.5*SR)]-dry)**2)/np.sum(dry**2)))
                      for x in (old_pcm, new_pcm)]
            assert ratios[1] > ratios[0]*10 and .07 < ratios[1] < .2
            segments = [old_pcm[3*SR:int(9.5*SR)].copy(), new_pcm[3*SR:int(9.5*SR)].copy()]
            edge = int(.04*SR)
            for segment in segments:
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
            levels = []
            for segment in segments:
                path = tmp/'excerpt.wav'
                write(path, segment)
                levels.append(measure(path))
            target = min([-23.]+[min(level['integrated_lufs']+18,
                level['integrated_lufs']-6-level['true_peak_dbfs']) for level in levels])
            gains = [target-level['integrated_lufs'] for level in levels]
            # Generate Stop starts the real releases at 16s. Owner retirement
            # is later and can follow PCM silence; do not audition that silence.
            # Apply the SAME version scalar to held and tail; keep their dynamics.
            release = 16*SR
            for x in (old_pcm, new_pcm):
                segment = x[release:release+int(6.5*SR)].copy()
                assert len(segment) == int(6.5*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                segments.append(segment)
            joined = []
            delivered = []
            for i, segment in enumerate(segments):
                segment *= 10**(gains[i%2]/20)
                check = tmp/f'part-{i}.wav'
                write(check, segment)
                level = measure(check)
                assert level['true_peak_dbfs'] <= -5.9
                delivered.append(level)
                if joined:
                    joined.append(np.zeros((SR//2, 2)))
                joined.append(segment)
            listen = out/f'AMBIENT_{name}_Room_Calibrated_AB_27p5s.wav'
            write(listen, np.concatenate(joined))
            assert read(listen).shape == (int(27.5*SR), 2)
            manifest['worlds'].append(dict(world=name, baseline_report=report,
                candidate_report=new['renderer_report'], baseline_raw_sha256=sha(raw),
                candidate_raw_sha256=sha(new_raw), actual_note_on_off_identical=True,
                baseline_matches_published_pcm=published_pcm_match,
                baseline_matches_published_trace=published_trace_match,
                trace_sha256=sha(trace), held_source_seconds=[3., 9.5],
                release_source_seconds=[release/SR, release/SR+6.5], final_source_end_seconds=old_end/1000,
                baseline_wet_to_dry_rms_ratio=ratios[0], candidate_wet_to_dry_rms_ratio=ratios[1],
                constant_gain_db_old_new=gains, held_target_lufs=target, delivered_sections=delivered,
                listen_file=listen.name, listen_sha256=sha(listen), listen=measure(listen)))
            print(name, 'wet/Dry RMS old/new', ratios, 'final source', old_end,
                  'matched actual score, held and real release packaged', flush=True)
    (out/'SD19_ROOM_CALIBRATION_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('SD19 CALIBRATION PASS: exact old Room vs candidate, matched actual score, '
          'unchanged normalized impulse decay/mono, held/release A/B. Hearing remains open.')


if __name__ == '__main__':
    main()
