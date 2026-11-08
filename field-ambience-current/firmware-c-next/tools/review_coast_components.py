#!/usr/bin/env python3
"""SD02: isolate the actual COAST components without changing product DSP.

Compile explicitly labelled, temporary diagnostic copies of bowed.c. The full
control uses the unmodified source and must match the existing product preview.
Every exported WAV is 27 s; full-register raw/listen pairs retain event traces.
"""
import argparse
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

ROOT = Path(__file__).resolve().parents[1]
SR = 44100
SOURCES = ['engine_product', 'world_grammar', 'bowed', 'horn', 'pluck',
           'ambient_room', 'nature', 'dsp', 'shape', 'tuning', 'brain', 'cells']
GRAIN = 'float bn = dsp_svf_bp(&v->bowbp, wnoise(&v->rng)) * (0.015f + 0.05f * v->bow);'
SYMP = 'float sy = dsp_svf_bp(&v->symp1, body) + dsp_svf_bp(&v->symp2, body);'
INIT = 's_colour = 0;\n}'
VARIANTS = {'full': (1, 1, 1), 'without_fifth': (1, 0, 1),
            'without_octave': (1, 1, 0), 'without_grain': (0, 1, 1),
            'body_only': (0, 0, 0)}


def diagnostic_source(original, colour, variant):
    """Keep clocks, noise/filter states and envelopes running in every case."""
    assert original.count(GRAIN) == original.count(SYMP) == original.count(INIT) == 1
    grain, fifth, octave = VARIANTS[variant]
    source = original.replace(INIT, f's_colour = {colour};\n}}') if colour else original
    if not grain:
        source = source.replace(GRAIN, GRAIN + '\n            bn = 0.0f;')
    if not fifth or not octave:
        replacement = ('float sy1 = dsp_svf_bp(&v->symp1, body);\n'
                       '            float sy2 = dsp_svf_bp(&v->symp2, body);\n'
                       f'            float sy = {fifth}.0f * sy1 + {octave}.0f * sy2;')
        source = source.replace(SYMP, replacement)
    return source


def spectrum(path):
    x = read(path)
    assert x.shape == (27 * SR, 2)
    rows = []
    for segment, midi in enumerate((50, 62, 69)):
        hz = 440 * 2 ** ((midi - 69) / 12)
        start = (segment * 9 + 1) * SR
        mono = x[start:start + SR].mean(axis=1)
        power = abs(np.fft.rfft(mono * np.hanning(len(mono)))) ** 2
        bins = np.fft.rfftfreq(len(mono), 1 / SR)
        def energy(ratio):
            return float(power[abs(bins - ratio * hz) <= 8].sum())
        root = energy(1)
        assert root > 0
        rows.append({'midi': midi, 'hz': hz, 'window_seconds': [segment * 9 + 1, segment * 9 + 2],
                     'fifth_band_to_root_db': 10 * math.log10(max(energy(1.5), 1e-30) / root),
                     'octave_band_to_root_db': 10 * math.log10(max(energy(2), 1e-30) / root)})
    energy = float((x * x).sum())
    return {'register': rows, 'mono_energy_ratio': float((x.mean(axis=1) ** 2).sum() / (energy / 2)),
            'mean_L': float(x[:, 0].mean()), 'mean_R': float(x[:, 1].mean()),
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def compile_preview(compiler, binary, bowed_path):
    paths = [bowed_path if name == 'bowed' else ROOT / 'src' / f'{name}.c' for name in SOURCES]
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-DFAM_SOUND_PRODUCT', '-I' + str(ROOT / 'include'),
                    str(ROOT / 'tools/render_product_preview.c'), *map(str, paths),
                    '-lm', '-o', str(binary)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    compiler = shutil.which('cc')
    ffmpeg = shutil.which('ffmpeg')
    assert compiler and ffmpeg, 'cc and ffmpeg required'
    original = (ROOT / 'src/bowed.c').read_text()
    manifest = {'task': 'SD02', 'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                'profile': 'product', 'sample_rate': SR, 'frames': 27 * SR,
                'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                'source_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in [*(ROOT / 'src' / f'{s}.c' for s in SOURCES),
                                            ROOT / 'tools/render_product_preview.c',
                                            ROOT / 'tools/package_musical_review.py', Path(__file__),
                                            *sorted((ROOT / 'include').rglob('*.h'))]},
                'reference': {'volume': .6, 'velocity': .75, 'color': .5, 'attack': .5, 'release': .5,
                              'room': 'Dry', 'nature': 0, 'seed': 1234, 'midi': [50, 62, 69]},
                'diagnostics': 'Temporary source ablations; no new product parameters or accepted timbres.',
                'band_measurement': 'One-second Hann spectrum, +/-8 Hz bands; ratios describe energy, not perceived roughness.',
                'files': []}
    with tempfile.TemporaryDirectory(prefix='ambient-sd02-') as temp:
        tmp = Path(temp)
        binary = tmp / 'preview'
        compile_preview(compiler, binary, ROOT / 'src/bowed.c')
        control = tmp / 'control.wav'
        subprocess.run([str(binary), '0', '1234', 'dry', str(control)], check=True, capture_output=True)
        manifest['unmodified_control_sha256'] = hashlib.sha256(control.read_bytes()).hexdigest()
        for colour in (0, 1):
            for variant in VARIANTS:
                source = diagnostic_source(original, colour, variant)
                candidate = tmp / 'bowed.c'
                candidate.write_text(source)
                compile_preview(compiler, binary, candidate)
                raw = out / f'COAST_colour{colour}_{variant}_raw_27s.wav'
                subprocess.run([str(binary), '0', '1234', 'dry', str(raw)], check=True, capture_output=True)
                if colour == 0 and variant == 'full':
                    assert raw.read_bytes() == control.read_bytes(), 'Diagnostic full control changed real product output'
                    assert Path(str(raw) + '.events.csv').read_bytes() == Path(str(control) + '.events.csv').read_bytes()
                level = measure(raw)
                assert level['true_peak_dbfs'] <= -6 and math.isfinite(level['integrated_lufs'])
                metrics = spectrum(raw)
                assert metrics['mono_energy_ratio'] > .65
                assert max(abs(metrics['mean_L']), abs(metrics['mean_R'])) < .0002
                gain = min(12., -26 - level['integrated_lufs'], -6 - level['true_peak_dbfs'])
                listen = out / raw.name.replace('_raw_', '_listen_')
                subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(raw), '-af', f'volume={gain:.8f}dB',
                                '-ar', str(SR), '-ac', '2', '-c:a', 'pcm_s16le', str(listen)], check=True)
                matched = measure(listen)
                assert matched['true_peak_dbfs'] <= -5.9
                row = {'colour': colour, 'variant': variant, 'components': dict(zip(('grain', 'fifth', 'octave'), VARIANTS[variant])),
                       'raw_file': raw.name, 'listen_file': listen.name, 'constant_gain_db': gain,
                       'diagnostic_source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                       'raw': {**level, **metrics}, 'listen': {**matched, 'sha256': hashlib.sha256(listen.read_bytes()).hexdigest()}}
                manifest['files'].append(row)
                print(f"SD02 colour={colour} {variant}: LUFS={level['integrated_lufs']:.1f}, TP={level['true_peak_dbfs']:.1f}, gain={gain:.1f} dB", flush=True)
        # Short first decision: the same six seconds of D4, a one-second gap.
        # Each segment uses its own single documented comparison gain, never AGC.
        for colour in (0, 1):
            parts, segments = [], []
            variants = ('full', 'without_fifth', 'without_octave', 'without_grain')
            for index, variant in enumerate(variants):
                row = next(r for r in manifest['files'] if r['colour'] == colour and r['variant'] == variant)
                x = read(out / row['raw_file'])[9 * SR:15 * SR].copy()
                scratch = tmp / 'segment.wav'
                write(scratch, x)
                level = measure(scratch)
                gain = min(12., -26 - level['integrated_lufs'], -6 - level['true_peak_dbfs'])
                # The source has a natural release at 3 s. A 100 ms end fade is
                # declared excerpt editing, not the firmware's retirement time.
                x[-SR // 10:] *= np.linspace(1, 0, SR // 10)[:, None]
                if index:
                    parts.append(np.zeros((SR, 2)))
                parts.append(x * 10 ** (gain / 20))
                segments.append({'start_seconds': index * 7, 'end_seconds': index * 7 + 6,
                                 'variant': variant, 'constant_gain_db': gain, 'end_fade_seconds': .1})
            quick = out / f'SD02_COAST_colour{colour}_AB_27s.wav'
            write(quick, np.concatenate(parts))
            with wave.open(str(quick)) as w:
                assert w.getnframes() == 27 * SR
            level = measure(quick)
            assert level['true_peak_dbfs'] <= -5.9
            manifest.setdefault('quick_comparisons', []).append({'file': quick.name, 'segments': segments,
                **level, 'sha256': hashlib.sha256(quick.read_bytes()).hexdigest()})
    (out / 'SD02_COMPONENT_METRICS.json').write_text(json.dumps(manifest, indent=2, allow_nan=False) + '\n')
    (out / 'README.md').write_text('''# SD02 — COAST einzeln beurteilen

Zuerst `SD02_COAST_colour0_AB_27s.wav`:
0–6 s vollständiger aktueller Ton, 7–13 s ohne 1,5f-Resonator,
14–20 s ohne 2f-Resonator, 21–27 s ohne Grain. Jeweils D4,
gleicher Einsatz, Loslassen nach 3 s; eine Sekunde Pause zwischen Varianten.
Nur die Hörmontage hat einen deklarierten 100-ms-Schlussfade pro Ausschnitt.

Die 27-s-Einzeldateien enthalten D3 (0–9), D4 (9–18), A4 (18–27),
jeweils Loslassen nach 3 s, Clear bei 8,75 s. `raw` erhält Firmwarepegel,
`listen` verwendet einen dokumentierten konstanten Gain, maximal +12 dB.
Room und Nature sind aus. Keine laufende Normalisierung.

colour0 ist der tatsächliche Produktkandidat. colour1 ist die noch vorhandene,
dunklere Quellenkonstruktion, hier ein ausdrücklich separater Diagnosekandidat.
Das ist kein weiterer Produktregler und kein World-Preset. Der Color-Makro
steht in allen Fällen auf 0,5. `body_only` entfernt alle drei Zusatzkomponenten.

Die Gegenproben werden aus temporären Kopien der echten `bowed.c` kompiliert.
RNG, Filterzustände, Zeitverlauf und Hüllkurven laufen unverändert. Der volle
colour0-Kontrollrender ist bytegleich zum unveränderten Product-Renderer.
Frequenzbandverhältnisse sind Messbefunde, keine Klang- oder Ruhefreigabe.

Hörentscheidung: Welche Variante trägt ohne Buzz, hohlen Röhrenklang oder
dominantes Pfeifen? Dateiname und Zeitstelle festhalten. Danach das gewählte
Prinzip an D3/A4 und in Mono prüfen. SD02 und SD03 bleiben bis dahin offen.
''')
    print('SD02: 10 register comparisons, 2 short montages; hearing acceptance remains open.', flush=True)


if __name__ == '__main__':
    main()
