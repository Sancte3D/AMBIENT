#!/usr/bin/env python3
"""SD04: matched dry HIGHLANDS/COAST source comparison, no product edits."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import tempfile

import numpy as np

from package_musical_review import measure, read, write
from review_coast_components import ROOT, SR, SOURCES, compile_preview


def fingerprint(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(path, with_registers=True):
    x = read(path)
    assert x.shape == (27 * SR, 2) and np.isfinite(x).all()
    level = measure(path)
    assert math.isfinite(level['integrated_lufs']) and level['true_peak_dbfs'] <= -6
    energy = float(np.sum(x * x) / 2)
    mono = float(np.sum(x.mean(axis=1) ** 2) / energy)
    dc = np.mean(x, axis=0).tolist()
    assert mono > .65 and max(map(abs, dc)) < .0002
    registers = []
    for segment, midi in enumerate((50, 62, 69) if with_registers else ()):
        hz = 440 * 2 ** ((midi - 69) / 12)
        start = (segment * 9 + 1) * SR
        y = x[start:start + SR].mean(axis=1)
        p = abs(np.fft.rfft(y * np.hanning(SR))) ** 2
        bins = np.fft.rfftfreq(SR, 1 / SR)
        def band(ratio):
            return float(p[abs(bins - ratio * hz) <= 8].sum())
        root = band(1)
        assert root > 0
        registers.append({'midi': midi, 'hz': hz,
                          'window_seconds': [segment * 9 + 1, segment * 9 + 2],
                          'harmonic_to_root_db': {
                              str(h): 10 * math.log10(max(band(h), 1e-30) / root)
                              for h in (2, 3, 4)},
                          'suboctave_to_root_db': 10 * math.log10(max(band(.5), 1e-30) / root),
                          'spectral_centroid_hz': float(np.sum(p * bins) / np.sum(p))})
    return {**level, 'mono_energy_ratio': mono, 'mean_dc_lr': dc,
            'registers': registers, 'sha256': fingerprint(path)}


def gain_for(level):
    return min(12., -26 - level['integrated_lufs'], -6 - level['true_peak_dbfs'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    cc, ffmpeg = shutil.which('cc'), shutil.which('ffmpeg')
    assert cc and ffmpeg, 'cc and ffmpeg required'
    inputs = [*(ROOT / 'src' / f'{s}.c' for s in SOURCES),
              ROOT / 'tools/render_product_preview.c',
              ROOT / 'tools/review_coast_components.py',
              ROOT / 'tools/package_musical_review.py', Path(__file__),
              *sorted((ROOT / 'include').rglob('*.h'))]
    manifest = {'task': 'SD04', 'source_commit': subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
        'profile': 'product', 'sample_rate': SR, 'frames': 27 * SR,
        'source_sha256': {str(p.relative_to(ROOT)): fingerprint(p) for p in inputs},
        'reference': {'seed': 1234, 'volume': .6, 'velocity': .75,
                      'color': .5, 'attack': .5, 'release': .5,
                      'room': 'Dry', 'nature': 0, 'midi': [50, 62, 69],
                      'onset_seconds': [0, 9, 18], 'key_up_seconds': [3, 12, 21],
                      'clear_seconds': [8.75, 17.75, 26.75]},
        'scope': 'Unmodified actual Product DSP. Source comparison only, not World grammar or hearing acceptance.',
        'trace_timing': 'The existing renderer observes start acknowledgements after the first 512-frame block; these are observation frames, not delayed audio onsets.',
        'analysis': 'One-second Hann spectra, +/-8 Hz bands; energy ratios and centroid do not establish warmth, calmness or source acceptance.',
        'files': []}
    with tempfile.TemporaryDirectory(prefix='ambient-sd04-') as tmp_value:
        tmp = Path(tmp_value)
        binary = tmp / 'preview'
        compile_preview(cc, binary, ROOT / 'src/bowed.c')
        for world, name in ((0, 'COAST'), (2, 'HIGHLANDS')):
            raw = out / f'SD04_{name}_raw_27s.wav'
            render = subprocess.run([str(binary), str(world), '1234', 'dry', str(raw)],
                                    check=True, capture_output=True, text=True)
            assert 'limited=0 nonfinite=0' in render.stderr
            trace = Path(str(raw) + '.events.csv')
            events = list(csv.DictReader(trace.open()))
            starts = [e for e in events if int(e['on']) == 1]
            assert len(starts) == 3
            assert [int(e['frame']) for e in starts] == [512, 9 * SR + 512, 18 * SR + 512]
            releases = [e for e in events if int(e['on']) == 0]
            assert [int(e['frame']) for e in releases] == [3 * SR, 12 * SR, 21 * SR]
            for event, midi in zip(starts, (50, 62, 69)):
                assert int(event['owner']) == 0 and float(event['velocity']) == .75
                assert abs(float(event['hz']) - 440 * 2 ** ((midi - 69) / 12)) < .001
            raw_metrics = inspect(raw)
            gain = gain_for(raw_metrics)
            listen = out / raw.name.replace('_raw_', '_listen_')
            subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(raw), '-af',
                            f'volume={gain:.8f}dB', '-c:a', 'pcm_s16le', str(listen)], check=True)
            listen_metrics = inspect(listen)
            manifest['files'].append({'world': world, 'source': name, 'raw_file': raw.name,
                'listen_file': listen.name, 'constant_gain_db': gain,
                'event_trace': trace.name, 'trace_sha256': fingerprint(trace),
                'confirmed_starts': starts, 'confirmed_key_ups': releases,
                'renderer_report': render.stderr.strip(),
                'raw': raw_metrics, 'listen': listen_metrics})
            print(f"SD04 {name}: {raw_metrics['integrated_lufs']:.1f} LUFS, "
                  f"{raw_metrics['true_peak_dbfs']:.1f} dBFS TP; fixed gain {gain:.1f} dB", flush=True)
        coast, highlands = manifest['files']
        assert coast['confirmed_starts'] == highlands['confirmed_starts']
        assert coast['raw']['sha256'] != highlands['raw']['sha256']
        assert abs(coast['listen']['integrated_lufs'] - highlands['listen']['integrated_lufs']) <= .2
        manifest['event_traces_identical'] = coast['trace_sha256'] == highlands['trace_sha256']
        parts, segments = [], []
        for register in range(3):
            for row in manifest['files']:
                index = len(segments)
                x = read(out / row['raw_file'])[register * 9 * SR:(register * 9 + 4) * SR].copy()
                # Explicit excerpt fade; complete RAW files retain the real renderer.
                x[-SR // 10:] *= np.linspace(1, 0, SR // 10)[:, None]
                scratch = tmp / 'segment.wav'
                write(scratch, x)
                gain = gain_for(measure(scratch))
                if index:
                    parts.append(np.zeros((round(.6 * SR), 2)))
                parts.append(x * 10 ** (gain / 20))
                segments.append({'source': row['source'], 'midi': (50, 62, 69)[register],
                    'start_seconds': round(index * 4.6, 1), 'end_seconds': round(index * 4.6 + 4, 1),
                    'constant_gain_db': gain, 'end_fade_seconds': .1})
        quick = out / 'SD04_COAST_HIGHLANDS_AB_27s.wav'
        write(quick, np.concatenate(parts))
        quick_metrics = inspect(quick, with_registers=False)
        # Spectrum windows on the montage are not register measurements.
        quick_metrics.pop('registers')
        manifest['quick_comparison'] = {'file': quick.name, 'segments': segments, **quick_metrics}
    (out / 'SD04_SOURCE_METRICS.json').write_text(json.dumps(manifest, indent=2, allow_nan=False) + '\n')
    (out / 'README.md').write_text('''# SD04 — HIGHLANDS behalten oder verwerfen

Zuerst SD04_COAST_HIGHLANDS_AB_27s.wav bei angenehmer Lautstärke anhören:
0–4 s COAST D3, 4,6–8,6 s HIGHLANDS D3;
9,2–13,2 s COAST D4, 13,8–17,8 s HIGHLANDS D4;
18,4–22,4 s COAST A4, 23–27 s HIGHLANDS A4.
Je 0,6 s Pause. Gleiche Noten, Velocity und Targets, kein Raum oder Natur.
Je Ausschnitt ein dokumentierter konstanter Pegelabgleich auf -26 LUFS,
100 ms Schlussfade als Ausschnittbearbeitung. Keine AGC oder Kompression.

Die vollständigen 27-s-Dateien spielen D3/D4/A4 bei 0/9/18 s,
Key-up nach je 3 s, Clear bei je 8,75 s. Damit wird kein vollständiger
natürlicher Ausklang ohne Clear behauptet. RAW erhält den echten Firmwarepegel;
LISTEN hat einen festen Gain über die ganze Datei. CSV und Messwerte liegen bei.
Der Renderer kompiliert die unveränderten echten Product-Quellen. Color,
Attack und Release sind 0,5; Volume 0,6; Velocity 0,75; Seed 1234.

Hörentscheidung: Ist HIGHLANDS ein eigener warmer Tonkörper mit anderer
Artikulation, ohne Signal-, Sirenen- oder hohlen Röhrencharakter? Oder klingt
es nur wie dünneres COAST? D3/D4/A4 und Mono prüfen, Dateiname/Zeitstelle nennen.
KEEP nur bei überzeugender eigener Rolle; andernfalls Ursache trocken
benennen, höchstens eine begründete neue Fassung, bei weiterem Scheitern REMOVE.
Zwei starke Welten sind zulässig. SD04 bleibt bis zur Hörentscheidung offen.
Die COAST-Auswahl aus SD02 ist ebenfalls noch offen; kein Ersatzkandidat
oder zusätzliches Layer wurde eingeführt. Grammatik/SD05 folgen separat.
''')
    print('SD04 PASS: actual matched source/register renders, safe levels and montage; KEEP/REMOVE hearing gate open.', flush=True)


if __name__ == '__main__':
    main()
