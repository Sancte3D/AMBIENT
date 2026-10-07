#!/usr/bin/env python3
"""SD03: actual COAST voice independence, plus matched dry Grain comparisons.

No new product control or DSP change. A temporary no-Grain source provides
the stationary reference; the full source is unmodified.
"""
import argparse
import csv
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import tempfile
import wave

import numpy as np

from package_musical_review import measure, read, write
from review_coast_components import ROOT, SR, SOURCES, diagnostic_source

SECONDS = 27
MIDI = (50, 57, 66)
START = (0, 3, 6)
STOP = (10, 14, 17)
BLOCK = 512


def load(compiler, output, bowed):
    sources = [bowed if s == 'bowed' else ROOT / 'src' / f'{s}.c' for s in SOURCES]
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fPIC', '-shared', '-Wl,-Bsymbolic', '-DFAM_SOUND_PRODUCT',
                    '-I' + str(ROOT / 'include'), *map(str, sources), '-lm', '-o', str(output)], check=True)
    lib = C.CDLL(str(output))
    lib.bowed_try_note_on.argtypes = [C.c_int, C.c_float, C.c_float]
    lib.bowed_try_note_on.restype = C.c_bool
    lib.bowed_note_off.argtypes = [C.c_int]
    lib.bowed_render_mix.argtypes = [C.POINTER(C.c_float)] * 4 + [C.c_int, C.c_float]
    lib.engine_try_note_on.argtypes = [C.c_uint8, C.c_float, C.c_float]
    lib.engine_try_note_on.restype = C.c_bool
    lib.engine_note_off.argtypes = [C.c_uint8]
    lib.engine_render.argtypes = [C.POINTER(C.c_int16), C.c_int]
    lib.engine_set_gen_seed.argtypes = [C.c_uint32]
    lib.engine_generative_tick.argtypes = [C.c_uint32]
    return lib


def events():
    return sorted([(int(t * SR), True, i) for i, t in enumerate(START)] +
                  [(int(t * SR), False, i) for i, t in enumerate(STOP)])


def render(lib, owners, product=False, block=BLOCK):
    """Every run advances the same clock, including all idle/event boundaries."""
    script = events()
    pointer = 0
    at = 0
    hook_rows = []
    if product:
        lib.engine_init()
        lib.engine_set_fx_mode(0)
        silence = np.zeros((512, 2), np.int16)
        for _ in range(9):
            lib.engine_render(silence.ctypes.data_as(C.POINTER(C.c_int16)), 512)
        lib.engine_set_gen_seed(1234)
        callback_type = C.CFUNCTYPE(None, C.c_int, C.c_uint8, C.c_float, C.c_float)
        def callback(on, owner, hz, velocity):
            hook_rows.append((at, on, owner, hz, velocity))
        hook = callback_type(callback)
        lib.engine_set_note_hook.argtypes = [callback_type]
        lib.engine_set_note_hook(hook)
        output = np.zeros((SECONDS * SR, 2), np.int16)
    else:
        lib.dsp_init(); lib.shape_init(); lib.bowed_init()
        output = np.zeros((SECONDS * SR, 2), np.float32)
    while at < len(output):
        while pointer < len(script) and script[pointer][0] == at:
            _, on, owner = script[pointer]
            if owner in owners:
                hz = C.c_float(440 * 2 ** ((MIDI[owner] - 69) / 12)).value
                if on:
                    accepted = (lib.engine_try_note_on(owner, hz, .75) if product else
                                lib.bowed_try_note_on(owner, hz, .375))
                    assert accepted, ('rejected diagnostic event', owner, hz)
                elif product:
                    lib.engine_note_off(owner)
                else:
                    lib.bowed_note_off(owner)
            pointer += 1
        boundary = script[pointer][0] if pointer < len(script) else len(output)
        n = min(block, boundary - at, len(output) - at)
        assert n > 0
        if product:
            lib.engine_generative_tick(int(at * 1000 / SR))
            assert lib.engine_active_voices() <= 3
            lib.engine_render(output[at:at+n].ctypes.data_as(C.POINTER(C.c_int16)), n)
        else:
            buffers = [np.zeros(n, np.float32) for _ in range(4)]
            lib.bowed_render_mix(*[b.ctypes.data_as(C.POINTER(C.c_float)) for b in buffers], n, 0.)
            output[at:at+n] = np.column_stack(buffers[:2])
        at += n
    if product:
        lib.engine_generative_tick(SECONDS * 1000)
        assert lib.engine_active_voices() == 0
        assert lib.engine_nonfinite_samples() == lib.engine_output_limited_samples() == 0
        assert [r[2] for r in hook_rows if r[1] == 1] == list(owners)
        assert sorted(r[2] for r in hook_rows if r[1] == 0) == sorted(owners)
        # Detach before Python releases the callback. This never runs in an ISR.
        lib.engine_set_note_hook(C.cast(None, callback_type))
    else:
        assert lib.bowed_active_count() == 0
    assert np.isfinite(output).all()
    return output, hook_rows


def mono(x):
    return x.astype(np.float64).mean(axis=1)


def root_motion(x, owner):
    hz = C.c_float(440 * 2 ** ((MIDI[owner] - 69) / 12)).value
    values = []
    size = SR // 5
    window = np.hanning(size)
    kernel = np.exp(-2j * np.pi * hz * np.arange(size) / SR) * window
    signal = mono(x)
    for frame in range((START[owner] + 2) * SR, int((STOP[owner] - .5) * SR) - size, size):
        amplitude = abs(np.dot(signal[frame:frame+size], kernel)) * 2 / window.sum()
        values.append((frame / SR - START[owner] + .1, 20 * math.log10(amplitude)))
    t, db = np.array(values).T
    fit = np.linalg.lstsq(np.column_stack((np.ones_like(t), t-t.mean(),
                       np.sin(2*np.pi*.13*t), np.cos(2*np.pi*.13*t))), db, rcond=None)[0]
    return {'midi': MIDI[owner], 'hz': hz, 'steady_root_swing_db': float(db.max()-db.min()),
            'fitted_0p13hz_root_peak_to_peak_db': float(2*np.hypot(fit[2], fit[3])),
            'window_seconds_after_onset': [float(t[0]-.1), float(t[-1]+.1)],
            'analysis': '200-ms Hann root demodulation; finite excerpt, not a general perceptual-periodicity test'}


def raw_write(path, pcm):
    assert pcm.dtype == np.int16 and pcm.shape == (27*SR, 2)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(pcm.astype('<i2').tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    compiler, ffmpeg = shutil.which('cc'), shutil.which('ffmpeg')
    assert compiler and ffmpeg
    original = (ROOT / 'src/bowed.c').read_text()
    report = {'task': 'SD03', 'candidate_status': 'SD02 source selection still open',
              'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
              'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'sample_rate': SR, 'frames': SECONDS*SR, 'block': BLOCK,
              'reference': {'world': 'COAST', 'source_colour': 0, 'color': .5, 'volume': .6,
                            'velocity': .75, 'attack': .5, 'release': .5, 'room': 'Dry', 'nature': 0, 'seed': 1234},
              'events': [{'owner': i, 'midi': MIDI[i], 'on_seconds': START[i], 'off_seconds': STOP[i]} for i in range(3)],
              'source_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in [*(ROOT/'src'/f'{s}.c' for s in SOURCES), Path(__file__),
                            ROOT/'tools/review_coast_components.py', ROOT/'tools/package_musical_review.py',
                            *sorted((ROOT/'include').rglob('*.h'))]}, 'probes': {}, 'files': []}
    with tempfile.TemporaryDirectory(prefix='ambient-sd03-') as temp:
        tmp = Path(temp)
        libraries = {}
        for variant in ('full', 'without_grain'):
            source = original if variant == 'full' else diagnostic_source(original, 0, variant)
            path = tmp / f'bowed_{variant}.c'; path.write_text(source)
            libraries[variant] = load(compiler, tmp/f'{variant}.so', path)
            report.setdefault('diagnostic_source_sha256', {})[variant] = hashlib.sha256(source.encode()).hexdigest()
        solo = {}
        for variant, lib in libraries.items():
            solo[variant] = [render(lib, (owner,))[0] for owner in range(3)]
            combined, _ = render(lib, (0, 1, 2))
            expected = sum(mono(x) for x in solo[variant])
            residual = mono(combined) - expected
            maximum = float(abs(residual).max())
            assert maximum < 1e-6, ('voices coupled', variant, maximum)
            report['probes'][variant] = {'mono_sum_max_error': maximum,
                'mono_sum_rms_error': float(np.sqrt(np.mean(residual**2))),
                'root_motion': [root_motion(x, i) for i, x in enumerate(solo[variant])]}
            assert max(r['steady_root_swing_db'] for r in report['probes'][variant]['root_motion']) < .2
        # Pure source-Grain response: full minus stationary no-Grain response.
        # The window is after all attacks settle and before any note is released.
        grain = np.array([mono(solo['full'][i])[int(7.5*SR):int(9.5*SR)] -
                          mono(solo['without_grain'][i])[int(7.5*SR):int(9.5*SR)] for i in range(3)])
        assert min(np.sqrt(np.mean(grain**2, axis=1))) > 1e-8
        correlation = np.corrcoef(grain)
        maximum = float(abs(correlation-np.eye(3)).max())
        assert maximum < .12, ('shared Grain sequence', maximum)
        report['probes']['grain'] = {'window_seconds': [7.5, 9.5], 'correlation': correlation.tolist(),
                                    'max_abs_cross_correlation': maximum,
                                    'rms': np.sqrt(np.mean(grain**2, axis=1)).tolist(),
                                    'scope': 'Different admitted pitches, this deterministic sequence; not all seeds or repeated same-pitch notes.'}
        for variant, lib in libraries.items():
            pcm, trace = render(lib, (0, 1, 2), product=True)
            small, small_trace = render(lib, (0, 1, 2), product=True, block=64)
            assert np.array_equal(pcm, small), ('block partition changed PCM', variant)
            # The DSP acknowledgement is observed at a caller-block boundary.
            assert [r[1:] for r in trace] == [r[1:] for r in small_trace]
            raw = out / f'SD03_COAST_{variant}_raw_27s.wav'; raw_write(raw, pcm)
            with Path(str(raw)+'.events.csv').open('w', newline='') as f:
                writer = csv.writer(f); writer.writerow(('frame', 'on', 'owner', 'hz', 'velocity')); writer.writerows(trace)
            level = measure(raw)
            assert math.isfinite(level['integrated_lufs']) and level['true_peak_dbfs'] <= -6
            x = pcm.astype(np.float64) / 32768
            mono_ratio = float((x.mean(axis=1)**2).sum() / ((x*x).sum()/2))
            assert mono_ratio > .65 and max(abs(x.mean(axis=0))) < .0002
            gain = min(12., -26-level['integrated_lufs'], -6-level['true_peak_dbfs'])
            listen = out / raw.name.replace('_raw_', '_listen_')
            subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(raw), '-af', f'volume={gain:.8f}dB',
                            '-ar', str(SR), '-ac', '2', '-c:a', 'pcm_s16le', str(listen)], check=True)
            matched = measure(listen); assert matched['true_peak_dbfs'] <= -5.9
            report['files'].append({'variant': variant, 'raw_file': raw.name, 'listen_file': listen.name,
                'constant_gain_db': gain, 'raw': {**level, 'mono_energy_ratio': mono_ratio,
                    'mean_L': float(x[:,0].mean()), 'mean_R': float(x[:,1].mean()),
                    'sha256': hashlib.sha256(raw.read_bytes()).hexdigest()},
                'listen': {**matched, 'sha256': hashlib.sha256(listen.read_bytes()).hexdigest()},
                'block_64_vs_512_pcm_max_delta': 0, 'confirmed_starts': 3, 'confirmed_releases': 3,
                'natural_source_retirement_by_seconds': 27})
        quick_parts, quick_segments = [], []
        for index, row in enumerate(report['files']):
            x = read(out/row['raw_file'])[:int(12.75*SR)].copy()
            path = tmp/'excerpt.wav'; write(path, x); level = measure(path)
            gain = min(12., -26-level['integrated_lufs'], -6-level['true_peak_dbfs'])
            x[-SR//10:] *= np.linspace(1, 0, SR//10)[:,None]
            if index: quick_parts.append(np.zeros((int(1.5*SR),2)))
            quick_parts.append(x*10**(gain/20))
            quick_segments.append({'start_seconds': index*14.25, 'end_seconds': index*14.25+12.75,
                'variant': row['variant'], 'constant_gain_db': gain, 'excerpt_end_fade_seconds': .1})
        quick = out/'SD03_COAST_Grain_AB_27s.wav'; write(quick, np.concatenate(quick_parts))
        with wave.open(str(quick)) as w: assert w.getnframes() == 27*SR
        level = measure(quick); assert level['true_peak_dbfs'] <= -5.9
        report['quick_comparison'] = {'file': quick.name, 'segments': quick_segments, **level,
            'sha256': hashlib.sha256(quick.read_bytes()).hexdigest()}
    (out/'SD03_INDEPENDENCE_METRICS.json').write_text(json.dumps(report, indent=2, allow_nan=False)+'\n')
    (out/'README.md').write_text('''# SD03 — unabhängige COAST-Stimmen

Zuerst `SD03_COAST_Grain_AB_27s.wav`: 0–12,75 s aktueller Tonkörper,
14,25–27 s derselbe Verlauf ohne Grain; dazwischen 1,5 s Pause.
Beide Ausschnitte verwenden dokumentierte konstante Gainanpassung und einen
100-ms-Schlussfade. Die Montage zeigt alle Einsätze und den ersten Release.
Die Einzeldateien enthalten den vollen 27-s-Verlauf und natürliche Releases.

D3 beginnt bei 0 s und wird bei 10 s losgelassen, A3 bei 3/14 s,
F#4 bei 6/17 s. Alle Quellen enden natürlich vor 27 s, kein Clear-Schluss.
World COAST, Color/Attack/Release 0,5, Velocity 0,75, Volume 0,6,
Room und Nature aus. `raw` erhält den Firmwarepegel; `listen` ergänzt einen
konstanten Vergleichsgain, maximal +12 dB, Ziel −26 LUFS/TP-Decke −6 dBFS.

Der Float-Quellentest vergleicht gemeinsame Stimmen mit der Summe separat
gespielter Stimmen bei identischer Zeitachse. Er prüft auch alle Releases.
Das tatsächliche Product-PCM muss bei 64 und 512 Frames bytegleich sein;
DSP-Acknowledgements werden am jeweiligen Caller-Blockrand beobachtet.
Quellzustände und Grain sind pro Stimme, eine gemeinsame Farbsteuerung bleibt
bewusst gemeinsam. Das Produkt enthält weiterhin keinen 0,13-Hz-Body-LFO.

Die zweite Variante ist nur eine temporäre Quellgegenprobe. Es wurde kein
neuer Klangregler, keine Bewegung und keine neue Zufälligkeit eingebaut.
Hörfrage: ruhige eigenständige Einsätze und Ausklänge oder gemeinsame Welle,
störende Reibung/Rauschteppich? SD02-Quellenwahl und SD03-Hörabnahme bleiben offen.
''')
    print(json.dumps({'mono_sum_max_error': {v: report['probes'][v]['mono_sum_max_error'] for v in libraries},
                      'grain_max_cross_correlation': report['probes']['grain']['max_abs_cross_correlation'],
                      'root_motion': {v: report['probes'][v]['root_motion'] for v in libraries}}, indent=2))
    print('SD03 PASS: independent sources/releases, natural retirement, stable roots, Grain, actual 64/512 PCM; hearing open.')


if __name__ == '__main__':
    main()
