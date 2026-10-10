#!/usr/bin/env python3
"""SD19: matched actual-ensemble Room amounts, float impulse/decay/mono proof."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

import numpy as np
from package_musical_review import measure, read

ROOT = Path(__file__).resolve().parents[1]
SR = 44100
AMOUNTS = (0., .24, .5, 1.)
SOURCES = ('engine_product', 'world_grammar', 'bowed', 'horn', 'pluck',
           'ambient_room', 'nature', 'dsp', 'shape', 'tuning', 'brain', 'cells')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, x):
    assert np.isfinite(x).all() and np.max(np.abs(x)) < 1
    with wave.open(str(path), 'wb') as wav:
        wav.setparams((2, 2, SR, 0, 'NONE', 'not compressed'))
        wav.writeframes(np.rint(x*32768).astype('<i2').tobytes())


def mono_ratio(x):
    energy = np.sum(x*x)
    return float(np.sum(x.mean(axis=1)**2)/(energy/2)) if energy else 1.


def impulse_metrics(path, amount):
    data = path.read_bytes()
    assert data[:4] == b'RIFF' and data[8:16] == b'WAVEfmt '
    assert struct.unpack_from('<HHIIHH', data, 20) == (3, 2, SR, SR*8, 8, 32)
    assert data[36:40] == b'data' and struct.unpack_from('<I', data, 40)[0] == 32*SR*8
    x = np.frombuffer(data, '<f4', offset=44).reshape(-1, 2).astype(np.float64)
    assert x.shape == (32*SR, 2) and np.isfinite(x).all()
    energy = np.sum(x*x, axis=1)
    total = float(energy.sum())
    result = dict(amount=amount, nominal_feedback_T60_seconds=1.2+3.6*amount**2,
                  wet_energy=total, peak=float(np.max(np.abs(x))),
                  mono_energy_ratio=mono_ratio(x), late_9s_energy_ratio=0.,
                  float32_sha256=sha(path), pcm_64_512_identical=True)
    assert result['peak'] < .3
    if amount == 0:
        assert total == 0
        return result
    assert total > 0 and result['mono_energy_ratio'] > .65
    result['late_9s_energy_ratio'] = float(energy[9*SR:].sum()/total)
    assert result['late_9s_energy_ratio'] < 1e-6
    result['first_wet_frame'] = int(np.flatnonzero(energy)[0])
    # Broadband Schroeder energy decay; T20/T30 are extrapolations, not the
    # feedback parameter nor a guarantee of uniform decay at every frequency.
    decay = np.cumsum(energy[::-1])[::-1]/total
    times = np.arange(len(decay))/SR
    db = 10*np.log10(np.maximum(decay, 1e-300))
    for lower, label in ((-25, 'broadband_T20_seconds'), (-35, 'broadband_T30_seconds')):
        mask = (db <= -5) & (db >= lower)
        assert np.count_nonzero(mask) > 100
        slope, intercept = np.polyfit(times[mask], db[mask], 1)
        assert slope < 0
        predicted = slope*times[mask]+intercept
        error = np.sum((db[mask]-predicted)**2)
        variance = np.sum((db[mask]-db[mask].mean())**2)
        result[label] = float(-60/slope)
        result[label+'_fit_r2'] = float(1-error/variance)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT/'src'/f'{s}.c' for s in SOURCES]
    driver = ROOT/'tools/render_room_comparison.c'
    inputs = [*sources, driver, Path(__file__), ROOT/'tools/package_musical_review.py',
              ROOT/'test/test_room_nature.c',
              *sorted((ROOT/'include').glob('*.h'))]
    manifest = dict(scope='Actual Product engine. Four fixed Room amounts; normal core, no source or generator substitutes. '
        '27.5s montage per World: identical 3.0–9.5s excerpt repeated with 0/.24/.50/1 Room, '
        'three 0.5s editorial gaps and 40ms excerpt-edge fades. No continuous transition.',
        parameters=dict(seed=1234, key='D', collection='major', tuning='equal', activity=.5,
            color=.5, attack=.5, release=.5, volume=.6, nature=0),
        raw_performance_seconds=32, generate_stop_seconds=16, silent_warmup_seconds=1,
        control_tick_ms=10, source_sha256={str(p.relative_to(ROOT)): sha(p) for p in inputs},
        worlds=[], impulse=[], hearing_accepted=False)
    with tempfile.TemporaryDirectory(prefix='room-engine-') as value:
        tmp = Path(value)
        binary = tmp/'render'
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
                 '-I'+str(ROOT/'include')]
        subprocess.run([*flags, str(driver), *map(str, sources), '-lm', '-o', str(binary)], check=True)
        # Existing actual DSP regression, including low sustain, Dry revival,
        # Clear, independent Nature/score and block/seed/input invariance.
        regression = tmp/'room-nature-test'
        subprocess.run([*flags, str(ROOT/'test/test_room_nature.c'), *map(str, sources),
                        '-lm', '-o', str(regression)], check=True)
        manifest['room_nature_regression'] = subprocess.run([str(regression)], check=True,
            capture_output=True, text=True).stdout.strip()
        print(manifest['room_nature_regression'], flush=True)
        for amount in AMOUNTS:
            raw = out/f'Room_impulse_{round(amount*100):03d}_float32.wav'
            report = subprocess.run([str(binary), 'impulse', '0', str(amount), '512', str(raw)],
                check=True, capture_output=True, text=True).stderr.strip()
            control = tmp/'control-impulse.wav'
            subprocess.run([str(binary), 'impulse', '0', str(amount), '64', str(control)],
                           check=True, capture_output=True)
            assert sha(raw) == sha(control), 'Float impulse differs across block sizes'
            row = impulse_metrics(raw, amount)
            row['renderer_report'] = report
            manifest['impulse'].append(row)
            print('IMPULSE', amount, row, flush=True)
        for world, name in enumerate(('COAST', 'WOODLAND', 'HIGHLANDS')):
            rows, clips, traces = [], [], []
            dry = None
            for amount in AMOUNTS:
                raw = out/f'{name}_room_{round(amount*100):03d}_raw_32s.wav'
                report = subprocess.run([str(binary), 'engine', str(world), str(amount), '512', str(raw)],
                    check=True, capture_output=True, text=True).stderr.strip()
                control = tmp/'control-engine.wav'
                subprocess.run([str(binary), 'engine', str(world), str(amount), '64', str(control)],
                               check=True, capture_output=True)
                trace = Path(str(raw)+'.events.csv')
                assert sha(raw) == sha(control), 'Engine PCM differs across block sizes'
                assert sha(trace) == sha(Path(str(control)+'.events.csv'))
                events = list(csv.DictReader(trace.open()))
                traces.append(trace.read_bytes())
                assert traces[-1] == traces[0], 'Room changed actual note-on/off history'
                x = read(raw)
                assert x.shape == (32*SR, 2) and np.isfinite(x).all()
                mono, dc = mono_ratio(x), x.mean(axis=0).tolist()
                assert mono > .65 and max(map(abs, dc)) < .0002
                raw_level = measure(raw)
                assert raw_level['true_peak_dbfs'] <= -6
                segment = x[3*SR:int(9.5*SR)].copy()
                if dry is None:
                    dry = segment.copy()
                residual = segment-dry
                wet_relative = float(np.sqrt(np.sum(residual**2)/np.sum(dry**2)))
                edge = int(.04*SR)
                segment[:edge] *= np.linspace(0, 1, edge)[:, None]
                segment[-edge:] *= np.linspace(1, 0, edge)[:, None]
                excerpt = tmp/'excerpt.wav'
                write(excerpt, segment)
                level = measure(excerpt)
                # Shared feasible target; ONE fixed scalar per entire variant.
                target_ceiling = min(-23., level['integrated_lufs']+18.,
                                     level['integrated_lufs']-6-level['true_peak_dbfs'])
                clips.append(segment)
                rows.append(dict(amount=amount, renderer_report=report, raw_level=raw_level,
                    mono_energy_ratio=mono, dc=dc, source_start_count=sum(int(e['on'])>0 for e in events),
                    raw_sha256=sha(raw), trace_sha256=sha(trace), pcm_64_512_identical=True,
                    excerpt_before_gain=level, target_ceiling_lufs=target_ceiling,
                    wet_difference_to_dry_rms_ratio=wet_relative))
                print(name, report, flush=True)
            target = min(row['target_ceiling_lufs'] for row in rows)
            montage = []
            for i, (row, segment) in enumerate(zip(rows, clips)):
                gain = target-row['excerpt_before_gain']['integrated_lufs']
                listen = tmp/f'variant-{i}.wav'
                delivered = segment*10**(gain/20)
                write(listen, delivered)
                row.update(constant_gain_db=gain, excerpt_after_gain=measure(listen),
                           montage_start_seconds=i*7., excerpt_seconds=[3., 9.5], cut_edge_fade_ms=40)
                assert abs(row['excerpt_after_gain']['integrated_lufs']-target) <= .2
                assert row['excerpt_after_gain']['true_peak_dbfs'] <= -5.9
                if montage:
                    montage.append(np.zeros((SR//2, 2)))
                montage.append(delivered)
            listen = out/f'AMBIENT_{name}_Room_AB_27p5s.wav'
            joined = np.concatenate(montage)
            assert joined.shape == (int(27.5*SR), 2)
            write(listen, joined)
            checked = measure(listen)
            assert checked['true_peak_dbfs'] <= -5.9
            manifest['worlds'].append(dict(world=name, variants=rows, note_on_off_all_amounts_identical=True,
                shared_excerpt_target_lufs=target, comparison_gap_seconds=.5,
                listen_file=listen.name, listen=checked, listen_sha256=sha(listen)))
    (out/'SD19_ROOM_METRICS.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('SD19 ROOM PASS: matched real note histories and 64/512 PCM, float decay/mono, '
          'existing Room/Nature regression; three compact comparisons. Room choice/hearing/device open.')


if __name__ == '__main__':
    main()
