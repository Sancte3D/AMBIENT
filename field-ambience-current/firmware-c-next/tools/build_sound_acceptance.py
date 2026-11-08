#!/usr/bin/env python3
"""Real C/PCM evidence for the open sound gates; no automatic hearing approval.

Exports <=27-s WAVs, later excerpts after rendering every intervening sample,
multiple-seed blind dry/room pairs, isolated macro endpoints and all six World
transitions. Temporary component-removal builds are explicitly counterfactual;
they do not modify or configure the shipping firmware.
"""
import argparse
import array
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import random
import re
import shutil
import subprocess
import sys
import tempfile
import wave
import zipfile

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
NAMES = ('COAST', 'WOODLAND', 'HIGHLANDS')
SOURCES = ('engine_product', 'world_grammar', 'bowed', 'horn', 'pluck',
           'ambient_room', 'nature', 'dsp', 'shape', 'tuning', 'brain', 'cells')
SR = 44100
GROUPS = {
    'sources': ('1. Trockene Tonkörper', [2, 3, 4, 5, 6, 11, 34]),
    'components': ('2. Einzelne Klangbestandteile', [2, 3, 4, 5, 31]),
    'controls': ('3. Color, Attack, Release und Activity', [5, 6, 33, 35]),
    'tails': ('4. Ausklang und gemeinsamer Raum', [12, 19]),
    'nature': ('5. Natur allein und im musikalischen Zusammenhang', [28, 29, 30, 31]),
    'transitions': ('6. Alle sechs Worldwechsel', [37]),
    'blind': ('7. Unbekannte Worlds, drei Durchgänge', [14, 15, 16, 17, 44]),
    'late': ('8. Nach 18 Minuten tatsächlichem Verlauf', [14, 15, 16, 17, 45]),
}
QUESTIONS = {
    2: 'Trägt COAST tief/mittel/hoch? Ist Grain oder Sympathie eine konkrete Störstelle?',
    3: 'Wirkt die Entwicklung eigenständig oder wie eine mechanische Welle?',
    4: 'Hat HIGHLANDS trocken eine eigene ruhige Rolle gegenüber COAST?',
    5: 'Bleiben kurzer Einsatz, Onset-Air und Attack-/Release-Grenzen rund?',
    6: 'Sind WOODLAND-Anschlag, Dämpfung und Ausklang brauchbar?',
    11: 'Wo stören tatsächlich gemeinsam klingende Grund- oder Teiltöne?',
    12: 'Verbindet die Raumfahne, ohne einen Farbwechsel hörbar zu verschmieren?',
    14: 'Hört man in COAST Zusammenhang und Stimmenübergaben?',
    15: 'Hört man in WOODLAND Figur, Antwort, Variation und Rückkehr?',
    16: 'Tragen HIGHLANDS-Fragmente und echte Pausen?',
    17: 'Schafft Wiederkehr Zusammenhang, ohne einen kurzen Dauerschleifencharakter?',
    19: 'Schafft der eine Raum Abstand ohne dominante Tonresonanz oder Verlust des Körpers?',
    28: 'Verbessert die optionale Natur den Ort, ohne die Musik/Pausen zu maskieren?',
    29: 'Ist Wind unregelmäßig und frei von auffälligem Pfeifen/Rumpeln?',
    30: 'Sind Tropfen/Küstenbewegung frei von nervösem Ticken oder gleichem Schwall?',
    31: 'Hat jedes verbliebene Geräusch eine ruhige Orts- oder Artikulationsrolle?',
    33: 'Sind Color/Attack/Release-Grenzen und Activity 0/1 musikalisch brauchbar? Ruhe muss auch bei hoher Activity tragen.',
    34: 'Überraschen die ORIGINALPEGEL beim Quellenwechsel? Vergleichskopien beantworten diese Frage nicht.',
    35: 'Gibt es konkrete Störstellen an den Endwerten oder während schneller Änderungen?',
    37: 'Ist ein gerichteter Wechsel mit maximalen Releases/Room musikalisch schlüssig?',
    41: 'Offen: ECC-sicherer Flash-Read, reale Power-cuts und Live-Save-/Boot-Abnahme.',
    44: 'Beschreibe die unbekannten Worlds durch Artikulation, Tonbeziehungen und Zeit; ohne Namensraten.',
    45: 'Bleibt die spätere Entwicklung schlüssig? Ein 27-s-Ausschnitt ersetzt keine echte lange Hörsitzung.',
    47: 'Offen: gemessene Save-/Onset-Spitzen und Storage-Handoff am H743.',
    48: 'Am Gerät: peak_load <0,60, null Deadline-Misses, Stackreserve unter UI/MIDI/Storage-Verkehr.',
    49: 'Am gebauten Ausgang: Pegel, DC, Noise, Pops, Lasten und unterstützte Stromzustände.',
    50: 'Echte längere Hörsitzungen bei leiser/typischer Lautstärke mit konkreten Zeitstellen.',
    51: 'Endwerte erst nach Quellen-/Hör-/Gerätebefunden einfrieren.',
    52: 'Später: UI/Wake/15-min-Ruhe auf denselben Audiobefehlen; keine neuen Klangpfade.',
    53: 'Abschluss erst nach allen zutreffenden Software-, Hör- und Gerätegates.',
}


def run(command, **kwargs):
    try:
        return subprocess.run(list(map(str, command)), check=True, **kwargs)
    except subprocess.CalledProcessError as failure:
        if failure.stderr:
            print(failure.stderr, file=sys.stderr, flush=True)
        raise


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def measure(path, ffmpeg, tonal_mono=True):
    with wave.open(str(path), 'rb') as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (2, 2, SR)
        frames = wav.getnframes()
        assert frames == SR * 27, (path, frames)
        samples = array.array('h', wav.readframes(frames))
        if sys.byteorder != 'little':
            samples.byteswap()
    energy = sum(value * value for value in samples)
    peak = max(map(abs, samples))
    mono = sum(((samples[i] + samples[i + 1]) / 2) ** 2
               for i in range(0, len(samples), 2))
    result = run([ffmpeg, '-hide_banner', '-nostats', '-i', path, '-af',
                  'loudnorm=I=-26:TP=-6:LRA=11:print_format=json', '-f', 'null', '-'],
                 capture_output=True, text=True)
    report = re.findall(r'\{\s*"input_i"[\s\S]*?\}', result.stderr)
    assert report, result.stderr
    loudness = json.loads(report[-1])
    result = {
        'frames': frames, 'seconds': 27, 'sha256': sha(path),
        'LUFS': float(loudness['input_i']), 'true_peak_dbfs': float(loudness['input_tp']),
        'sample_peak_dbfs': 20 * math.log10(peak / 32768) if peak else -999,
        'rms_dbfs': 10 * math.log10(energy / len(samples) / 32768**2) if energy else -999,
        'mean_L': sum(samples[::2]) / frames / 32768,
        'mean_R': sum(samples[1::2]) / frames / 32768,
        'mono_energy_ratio': mono / (energy / 2) if energy else 1,
    }
    assert all(math.isfinite(value) for value in result.values() if isinstance(value, float)), result
    assert energy and result['true_peak_dbfs'] <= -6, result
    # Independent stereo noise retains about half its summed channel energy
    # in mono. Report isolated Nature honestly; the tonal-chain .65 gate does
    # not describe that fixture. Nature+music still passes the tonal gate.
    if tonal_mono:
        assert result['mono_energy_ratio'] > .65, result
    assert max(abs(result['mean_L']), abs(result['mean_R'])) < .0002, result
    return result


def source_partials(path):
    """Observed narrow-band inventory, not a roughness/calming model."""
    with wave.open(str(path), 'rb') as wav:
        samples = array.array('h', wav.readframes(wav.getnframes()))
    if sys.byteorder != 'little':
        samples.byteswap()
    rows = []
    for segment, midi in enumerate((50, 62, 69)):
        frequency = 440 * 2 ** ((midi - 69) / 12)
        start = int((segment * 9 + 1.25) * SR)
        weighted = [((samples[2 * (start + i)] + samples[2 * (start + i) + 1]) / 65536)
                    * (.5 - .5 * math.cos(2 * math.pi * i / (SR - 1))) for i in range(SR)]

        def band(factor):
            power = 0
            for offset in (-2, -1, 0, 1, 2):
                coefficient = 2 * math.cos(2 * math.pi * (frequency * factor + offset) / SR)
                previous = older = 0
                for sample in weighted:
                    current = sample + coefficient * previous - older
                    older, previous = previous, current
                power += max(0, previous**2 + older**2 - coefficient * previous * older)
            return power

        fundamental = band(1)
        assert fundamental > 0
        rows.append({'midi': midi, 'nominal_hz': frequency,
                     'window_seconds': [segment * 9 + 1.25, segment * 9 + 2.25],
                     'band_estimate_db_relative_fundamental': {
                         str(factor): 10 * math.log10(max(band(factor) / fundamental, 1e-30))
                         for factor in (1.5, 2, 3, 4, 5, 6, 7, 8)}})
    return rows


def compile_preview(destination, compiler, overrides=None):
    overrides = overrides or {}
    paths = [overrides.get(name, ROOT / 'src' / f'{name}.c') for name in SOURCES]
    run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
         '-I' + str(ROOT / 'include'), ROOT / 'tools/render_product_preview.c',
         *paths, '-lm', '-o', destination], capture_output=True, text=True)


def montage(first, second, destination):
    chunks = []
    for path in (first, second, first, second):
        with wave.open(str(path), 'rb') as wav:
            wav.setpos(SR * 9)  # D4 segment, 3-s hold plus 3-s release
            chunk = array.array('h', wav.readframes(SR * 6))
            if sys.byteorder != 'little':
                chunk.byteswap()
        # Editing boundary only, not a firmware envelope or mastering process.
        fade = SR // 50
        for i in range(fade):
            for channel in (0, 1):
                chunk[2 * i + channel] = int(chunk[2 * i + channel] * i / fade)
                index = len(chunk) - 2 * i - 2 + channel
                chunk[index] = int(chunk[index] * i / fade)
        if sys.byteorder != 'little':
            chunk.byteswap()
        chunks.append(chunk.tobytes())
    silence = bytes(SR * 4)
    with wave.open(str(destination), 'wb') as wav:
        wav.setparams((2, 2, SR, 0, 'NONE', 'not compressed'))
        wav.writeframes(silence.join(chunks))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--late-seconds', type=int, default=1080)
    parser.add_argument('--blind-seed', type=int)
    args = parser.parse_args()
    if not 0 <= args.late_seconds <= 1800:
        parser.error('--late-seconds must be 0..1800')
    destination = args.output.resolve()
    assert not destination.exists(), 'Use a fresh output directory'
    # Build closed, verified files away from the published workspace. Readers
    # must never observe a renderer's initial zero-length output as evidence.
    staging = tempfile.TemporaryDirectory(prefix='ambient-acceptance-')
    out = Path(staging.name) / 'review'
    out.mkdir(parents=True, exist_ok=True)
    compiler, ffmpeg = shutil.which('cc'), shutil.which('ffmpeg')
    assert compiler and ffmpeg, 'cc and ffmpeg required'
    paths = [ROOT / 'src' / f'{name}.c' for name in SOURCES]
    renderer = out / 'render_product_preview'
    compile_preview(renderer, compiler)
    manifest = {
        'commit': run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, capture_output=True, text=True).stdout.strip(),
        'tree': run(['git', 'rev-parse', 'HEAD^{tree}'], cwd=ROOT, capture_output=True, text=True).stdout.strip(),
        'workspace_dirty': bool(run(['git', 'status', '--porcelain'], cwd=ROOT, capture_output=True, text=True).stdout),
        'source_sha256': {str(path.relative_to(ROOT)): sha(path) for path in paths},
        'renderer_sha256': sha(ROOT / 'tools/render_product_preview.c'),
        'builder_sha256': sha(Path(__file__)),
        'compiler': run([compiler, '--version'], capture_output=True, text=True).stdout.splitlines()[0],
        'ffmpeg': run([ffmpeg, '-version'], capture_output=True, text=True).stdout.splitlines()[0],
        'profile': 'product', 'sample_rate': SR, 'block': 512,
        'reference': {'key': 'D', 'collection': 'Major', 'tuning': 'Equal', 'volume': .6,
                      'color': .5, 'activity': .5, 'attack': .5, 'release': .5, 'room': .5, 'nature': 0},
        'listening_gain': 'One constant gain per fixture/pair, target -26 LUFS, true-peak ceiling -6 dBFS. No AGC. Pair members use the SAME gain.',
        'onset_trace': 'Main DSP acknowledgement; onset lies in preceding render block. Frame is excerpt-relative; absolute_frame includes actual preroll.',
        'hearing_approved': False, 'files': [], 'pairs': [],
    }
    fixtures = {}

    def render(identifier, group, title, world, mode, seed=1234, start=0, target=None,
               binary=renderer, blind=False):
        directory = out / group
        directory.mkdir(exist_ok=True)
        path = directory / f'{identifier}_raw_27s.wav'
        command = [binary, world, seed, mode, path]
        if start or target is not None:
            command.append(start)
        if target is not None:
            command.append(target)
        print(f'RENDER {identifier}: {mode}, {start + 27}s actual PCM', flush=True)
        result = run(command, capture_output=True, text=True)
        stats = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', result.stderr)}
        assert stats['limited'] == stats['nonfinite'] == 0
        assert path.stat().st_size == 44 + SR * 27 * 4, f'Incomplete output: {identifier}'
        row = {'id': identifier, 'group': group, 'title': title,
               'world': None if blind else NAMES[world], 'mode': mode, 'seed': seed,
               'start_seconds': start, 'raw_file': str(path.relative_to(out)),
               'listen_file': str(path.with_name(f'{identifier}_listen_27s.wav').relative_to(out)),
               'raw': measure(path, ffmpeg, tonal_mono=mode != 'nature'),
               'stats': {key: value for key, value in stats.items() if key != 'world'},
               'events_file': str(Path(str(path) + '.events.csv').relative_to(out))}
        if blind:
            # Remove implementation owner IDs from the public blind trace.
            trace = Path(str(path) + '.events.csv')
            rows = list(csv.DictReader(trace.read_text().splitlines()))
            with trace.open('w', newline='') as stream:
                columns = ['frame', 'on', 'hz', 'velocity', 'absolute_frame']
                writer = csv.DictWriter(stream, fieldnames=columns)
                writer.writeheader()
                writer.writerows({key: event[key] for key in columns} for event in rows)
        fixtures[identifier] = row
        manifest['files'].append(row)
        return row

    def match(rows, paired=False):
        # Paired source/room/nature comparisons must preserve their level relation.
        gain = min(-26 - max(row['raw']['LUFS'] for row in rows),
                   -6.1 - max(row['raw']['true_peak_dbfs'] for row in rows))
        for row in rows:
            assert sha(out / row['raw_file']) == row['raw']['sha256'], f'Raw fixture changed: {row["id"]}'
            run([ffmpeg, '-v', 'error', '-y', '-i', out / row['raw_file'], '-af',
                 f'volume={gain:.8f}dB', '-ar', SR, '-ac', 2, '-c:a', 'pcm_s16le',
                 out / row['listen_file']], capture_output=True, text=True)
            row['constant_gain_db'] = gain
            row['listen'] = measure(out / row['listen_file'], ffmpeg, tonal_mono=row['mode'] != 'nature')
            assert row['listen']['true_peak_dbfs'] <= -5.9
        if paired:
            manifest['pairs'].append([row['id'] for row in rows])

    dry = {}
    for world, name in enumerate(NAMES):
        dry[world] = render(f'{name}_dry', 'sources', f'{name}: D3 / D4 / A4, trocken', world, 'dry')
        dry[world]['source_partials'] = source_partials(out / dry[world]['raw_file'])
        match([dry[world]])
        for mode in ('color', 'attack', 'release'):
            row = render(f'{name}_{mode}', 'controls', f'{name}: {mode} 0 / 0,5 / 1', world, mode)
            match([row])
        first = render(f'{name}_activity_low', 'controls', f'{name}: Activity 0, ab 90 s',
                       world, 'activity_low', seed=23891, start=90)
        second = render(f'{name}_activity_high', 'controls', f'{name}: Activity 1, ab 90 s',
                        world, 'activity_high', seed=23891, start=90)
        match([first, second], paired=True)
        first = render(f'{name}_tail_dry', 'tails', f'{name}: maximaler Quellrelease, trocken', world, 'tail_dry')
        second = render(f'{name}_tail_room', 'tails', f'{name}: gleicher Quellrelease + maximaler Room', world, 'tail')
        match([first, second], paired=True)
        row = render(f'{name}_nature_solo', 'nature', f'{name}: Natur allein, Amount 0,7', world, 'nature')
        match([row])
        row = render(f'{name}_nature_seed2', 'nature', f'{name}: Natur allein, zweiter Seed ab 45 s',
                     world, 'nature', seed=23891, start=45)
        match([row])
        first = render(f'{name}_nature_off', 'nature', f'{name}: World, Nature aus', world, 'world', start=90)
        second = render(f'{name}_nature_on', 'nature', f'{name}: gleiche World + Nature 0,7', world, 'world_nature', start=90)
        # Nature may change output/DC, but must not change actual source events.
        events = [Path(out / row['events_file']).read_bytes() for row in (first, second)]
        assert events[0] == events[1], f'Nature changed score: {name}'
        match([first, second], paired=True)
    for world, name in enumerate(NAMES):
        for target, destination_name in enumerate(NAMES):
            if target == world:
                continue
            row = render(f'{name}_to_{destination_name}', 'transitions',
                         f'{name} → {destination_name}: Wechsel bei 12 s', world, 'transition', target=target)
            match([row])
    blind_seed = args.blind_seed if args.blind_seed is not None else random.SystemRandom().getrandbits(64)
    rng = random.Random(blind_seed)
    private = {'blinding_seed': blind_seed, 'rounds': []}
    for round_number, (seed, start) in enumerate(((91267, 45), (23891, 90), (67431, 150)), 1):
        order = list(range(3))
        rng.shuffle(order)
        private['rounds'].append({'round': round_number,
                                 'key': dict(zip('ABC', (NAMES[world] for world in order)))})
        for letter, world in zip('ABC', order):
            rows = []
            for mode, suffix in (('score_dry', 'dry'), ('world', 'room')):
                row = render(f'R{round_number}_{letter}_{suffix}', 'blind',
                             f'Durchgang {round_number}, {letter}, {suffix}', world, mode,
                             seed=seed, start=start, blind=True)
                rows.append(row)
            # Equal reference loudness between unknown Worlds; paired gain holds
            # dry/room level relation. Remaining loudness spread is documented.
            match(rows, paired=True)
    for world, name in enumerate(NAMES):
        row = render(f'{name}_late', 'late', f'{name}: ab {args.late_seconds // 60} min',
                     world, 'world', seed=38291, start=args.late_seconds)
        match([row])
    recipes = [
        ('COAST_grain', 0, 'bowed', '(0.015f + 0.05f * v->bow)', '0.0f',
         'COAST: A aktuell / B ohne Grain'),
        ('COAST_sympathy', 0, 'bowed', 'body + sy * v->symp_gain', 'body + sy * 0.0f',
         'COAST: A aktuell / B ohne beide Sympathieresonatoren'),
        ('HIGHLANDS_air', 2, 'horn', '* a * a * 0.12f', '* a * a * 0.0f',
         'HIGHLANDS: A aktuell / B ohne Onset-Air'),
    ]
    manifest['component_recipes'] = []
    with tempfile.TemporaryDirectory(prefix='ambient-components-') as temporary:
        temp = Path(temporary)
        for identifier, world, source, old, new, title in recipes:
            original = ROOT / 'src' / f'{source}.c'
            text = original.read_text()
            assert text.count(old) == 1, f'Component recipe needs explicit review: {source}'
            altered = temp / f'{identifier}.c'
            altered.write_text(text.replace(old, new))
            variant = temp / identifier
            compile_preview(variant, compiler, {source: altered})
            recording = temp / f'{identifier}.wav'
            run([variant, world, 1234, 'dry', recording], capture_output=True, text=True)
            path = out / 'components' / f'{identifier}_raw_27s.wav'
            path.parent.mkdir(exist_ok=True)
            montage(out / dry[world]['raw_file'], recording, path)
            row = {'id': identifier, 'group': 'components', 'title': title,
                   'world': NAMES[world], 'mode': 'component_ABAB', 'seed': 1234,
                   'start_seconds': 0, 'raw_file': str(path.relative_to(out)),
                   'listen_file': str(path.with_name(f'{identifier}_listen_27s.wav').relative_to(out)),
                   'raw': measure(path, ffmpeg), 'editing': 'A 0–6 / B 7–13 / A 14–20 / B 21–27 s; 20-ms boundary fades only'}
            manifest['files'].append(row)
            match([row])
            manifest['component_recipes'].append({'id': identifier, 'source': str(original.relative_to(ROOT)),
                'original_sha256': sha(original), 'variant_sha256': sha(altered), 'replace': [old, new],
                'shipping_firmware_changed': False})
    for source, fingerprint in manifest['source_sha256'].items():
        assert sha(ROOT / source) == fingerprint, 'Source changed during rendering'
    for row in manifest['files']:
        assert sha(out / row['raw_file']) == row['raw']['sha256'], f'Raw fixture changed: {row["id"]}'
        assert sha(out / row['listen_file']) == row['listen']['sha256'], f'Listening fixture changed: {row["id"]}'
    todo = (REPO / 'field-ambience-current/docs/audio/AMBIENT_SOUND_DESIGN_TODO.md').read_text()
    tasks = [{'id': int(number), 'title': title, 'question': QUESTIONS[int(number)],
              'groups': [group for group, (_, ids) in GROUPS.items() if int(number) in ids],
              'status': 'offen', 'evidence_kind': 'hearing' if int(number) in QUESTIONS and int(number) < 41 or int(number) in (44, 45) else 'device_or_followup'}
             for number, title in re.findall(r'^- \[ \] \*\*SD(\d+) — (.*?)\*\*', todo, re.M)]
    assert len(tasks) == 30 and len(QUESTIONS) == 30
    manifest['tasks'] = tasks
    manifest['partial_inventory_method'] = ('Actual dry PCM, 1-s Hann hold windows; Goertzel sum at nominal partial +/- 2/1/0 Hz. Relative estimates only. '
                                           'A narrow-band estimate is neither exact harmonic amplitude nor a validated perceptual roughness/comfort score. '
                                           'Admission rules and firmware coefficients are unchanged.')
    manifest['group_titles'] = {group: title for group, (title, _) in GROUPS.items()}
    manifest['rendered_seconds'] = sum(row.get('stats', {}).get('rendered_frames', 0) / SR
                                       for row in manifest['files'])
    manifest['exported_wavs'] = 2 * len(manifest['files'])
    manifest['review_id'] = hashlib.sha256(json.dumps(
        [(row['id'], row['raw']['sha256']) for row in manifest['files']],
        separators=(',', ':')).encode()).hexdigest()
    (out / 'PRODUCT_ACCEPTANCE_METRICS.json').write_text(json.dumps(manifest, indent=2, allow_nan=False) + '\n')
    # The answer key is deliberately outside the delivered audition archive.
    (out.parent / 'AMBIENT_PRIVATE_BLIND_KEY.json').write_text(json.dumps(private, indent=2) + '\n')
    write_html(out, manifest)
    (out / 'README.md').write_text(
        '# AMBIENT — offene Soundabnahmen\n\n'
        'ZIP entpacken und START_HERE.html im Browser öffnen. Mit trockenen Quellen anfangen. '
        'Jede Datei dauert 27 s. Originalpegel und feste Vergleichskopie getrennt; kein AGC. '
        'Die Reglerclips zeigen 0/0,5/1 bei 0/9/18 s; jeweils 3 s halten, Clear bei 8,75 s. '
        'Attack und Release verändern jeweils nur den benannten Parameter. '
        'Activity 0/1: gleiche World und Seed, jeweils ab 90 s nach durchgehendem Rendern; keine Zeitmontage. '
        'Natur allein: zwei Seeds, einschließlich späterer Entwicklung. '
        'Tail: D4 eine Sekunde halten, maximaler Release, dann bis 27 s ohne Clear ausklingen. '
        'Worldwechsel bei 12 s, maximaler Room/Release. Naturpaare haben identische Ereignis-CSV und gemeinsamen Gain.\n\n'
        'Blind: drei Seeds und spätere Ausschnitte; A/B/C wird pro Durchgang neu zugeordnet, '
        'innerhalb eines Durchgangs bleiben Dry/Room identisch zugeordnet. Der Schlüssel liegt separat. '
        'Ein gemeinsamer Pair-Gain bewahrt die Dry/Room-Relation; gemessene Lautheiten stehen im JSON. '
        'Nicht alle individuellen dry-Dateien sind dadurch exakt gleich laut.\n\n'
        'Komponenten-AB: A aktuell 0–6/14–20 s, B mit ausschließlich dem benannten Bestandteil entfernt '
        '7–13/21–27 s; dazwischen eine Sekunde Ruhe. Nur diese Montagen besitzen 20-ms-Schnittblenden. '
        'Die Gegenmodelle wurden in temporären Builds erzeugt, nicht im Firmware-DSP übernommen.\n\n'
        'Späte Ausschnitte wurden nach vollständigem Rendern aller vorherigen Samples gewonnen. '
        'Die Zustände wurden nicht auf einen späteren Timerwert vorgesetzt. '
        'Hörnotizen als JSON exportieren. Eine Hörantwort schließt keinen Geräte-/ECC-/UX-Gate.\n')
    archive = out.parent / 'AMBIENT_Sound_Review_2026-10-07.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zipped:
        for path in sorted(out.rglob('*')):
            if path.is_file() and path != renderer:
                zipped.write(path, arcname='AMBIENT_Sound_Review/' + str(path.relative_to(out)))
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(out, destination, ignore=shutil.ignore_patterns(renderer.name))
    for row in manifest['files']:
        assert sha(destination / row['raw_file']) == row['raw']['sha256']
        assert sha(destination / row['listen_file']) == row['listen']['sha256']
    for name in (archive.name, 'AMBIENT_PRIVATE_BLIND_KEY.json'):
        source = out.parent / name
        with tempfile.NamedTemporaryFile(dir=destination.parent, prefix='.ambient-publish-', delete=False) as stream:
            temporary = Path(stream.name)
            with source.open('rb') as incoming:
                shutil.copyfileobj(incoming, stream)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, destination.parent / name)
        assert sha(source) == sha(destination.parent / name)
    print(f'PASS: {len(manifest["files"])} comparisons; {manifest["exported_wavs"]} WAVs <=27s; '
          f'{manifest["rendered_seconds"] / 60:.2f} minutes actual PCM; archive {archive.stat().st_size} B', flush=True)
    staging.cleanup()


def write_html(out, manifest):
    data = json.dumps(manifest, ensure_ascii=False, allow_nan=False).replace('</', '<\\/')
    page = '''<!doctype html>
<html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>AMBIENT · Sound Review</title><style>
:root{font-family:system-ui,sans-serif;color:#e6e8e5;background:#131817}body{max-width:980px;margin:44px auto;padding:0 24px}
h1{font-size:36px;font-weight:500;letter-spacing:-1px}h2{font-weight:500;font-size:23px;margin-top:44px}p{line-height:1.6;color:#b8c1bb}
.card{padding:20px 0;border-top:1px solid #35403b}.tag{font-size:12px;letter-spacing:1px;color:#b3ccba}audio{width:100%;height:38px;margin:12px 0}
.muted{font-size:13px;color:#9faea4}button,select,textarea,input{font:inherit;background:#202924;color:#e4eee7;border:1px solid #53665a;border-radius:4px;padding:9px}
button{cursor:pointer}button:focus,select:focus,textarea:focus{outline:2px solid #d1e8d7}textarea{width:100%;box-sizing:border-box;min-height:80px;margin:8px 0}
a{color:#c0dac9}label{display:block;margin:10px 0}summary{cursor:pointer;line-height:1.8}.toolbar{display:flex;gap:12px;flex-wrap:wrap;position:sticky;top:0;background:#131817;padding:16px 0;z-index:2}
details{margin:10px 0}blockquote{margin:20px 0;padding-left:18px;border-left:2px solid #53665a;color:#bdc9c1}.small{font-size:13px}
</style><header><div class="tag">AMBIENT / SOUND CANDIDATE 0.3</div><h1>Die offenen Soundentscheidungen.</h1>
<p>Beginne mit den drei trockenen Tonkörpern. Notiere konkrete Störstellen mit Datei und Zeitpunkt.
Danach Bestandteile, Reglergrenzen, Raum, Natur und unbekannte Worlds prüfen. Jede Datei dauert 27 Sekunden.</p>
<blockquote>„Vergleich“ besitzt einen festen Dateigain. „Original“ erhält die Firmwarepegel. Für Lautheitsentscheidungen Original verwenden.
Eine positive Hörnotiz ersetzt die Prüfung am gebauten Gerät nicht.</blockquote></header>
<div class="toolbar"><button id="export">Hörnotizen exportieren</button><button id="notes">Zu den 30 Aufgaben</button><span class="small" id="saved" aria-live="polite"></span></div>
<main id="clips"></main><section id="tasks"><h2>30 Aufgaben · dein Befund</h2><p>Die letzten acht Aufgaben benötigen zusätzlich Speicher-, Geräte-, Freeze- oder UX-Arbeit.
Hier bleibt sichtbar, was als nächster Nachweis fehlt.</p><div id="task-list"></div></section>
<footer><p class="small">Belege: <a href="PRODUCT_ACCEPTANCE_METRICS.json">Messwerte und Herkunft</a> · <a href="README.md">Ablauf</a>.
Komponentenvergleiche sind Gegenmodelle; der aktuelle Firmwareklang wurde dadurch nicht geändert.</p></footer>
<script>const data=__DATA__;
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let answers={};const storageKey='ambient-review-'+data.review_id;
try{answers=JSON.parse(localStorage.getItem(storageKey)||'{}')}catch(e){}
const clips=document.getElementById('clips');
for(const [group,title]of Object.entries(data.group_titles)){
 const section=document.createElement('section');section.id='group-'+group;section.innerHTML='<h2>'+esc(title)+'</h2>';
 if(group==='blind')section.innerHTML+='<p>A/B/C sind pro Durchgang neu zugeordnet. Erst trocken beschreiben, danach dieselben Buchstaben mit Room hören.</p>';
 if(group==='components')section.innerHTML+='<p>A aktuell bei 0–6 und 14–20 s. B mit nur dem benannten Bestandteil entfernt bei 7–13 und 21–27 s.</p>';
 if(group==='controls')section.innerHTML+='<p>Color/Attack/Release: D4; Werte 0 / 0,5 / 1 bei 0 / 9 / 18 s. Nur der benannte Regler ändert sich. Activity: getrennte 0/1-Ausschnitte nach 90 s, gleicher Seed.</p>';
 for(const file of data.files.filter(f=>f.group===group)){
  const card=document.createElement('div');card.className='card';
  card.innerHTML='<strong>'+esc(file.title)+'</strong><audio controls preload="none" src="'+esc(file.listen_file)+'"></audio>'+
   '<div><button class="toggle">Originalpegel</button> <button class="stamp">Zeitstelle notieren</button> <span class="muted">Vergleich</span></div>'+
   '<details><summary class="muted">Pegel / Quelle</summary><p class="small">Original '+file.raw.LUFS.toFixed(1)+' LUFS; Vergleich '+file.listen.LUFS.toFixed(1)+' LUFS; fester Gain '+file.constant_gain_db.toFixed(2)+' dB. Kein AGC.</p></details>';
  const player=card.querySelector('audio');let raw=false;
  card.querySelector('.toggle').onclick=e=>{const time=player.currentTime,play=!player.paused;raw=!raw;player.src=raw?file.raw_file:file.listen_file;
   player.onloadedmetadata=()=>{player.currentTime=time;if(play)player.play().catch(()=>{})};e.target.textContent=raw?'Vergleichspegel':'Originalpegel';card.querySelector('span').textContent=raw?'Original':'Vergleich'};
  card.querySelector('.stamp').onclick=()=>{const text=file.id+' @ '+player.currentTime.toFixed(1)+' s ('+(raw?'Original':'Vergleich')+')';
   const input=document.getElementById('session-notes');input.value+=(input.value?'\\n':'')+text;answers.session=input.value;save()};
  player.onplay=()=>{for(const other of document.querySelectorAll('audio'))if(other!==player)other.pause()};
  section.appendChild(card);
 }clips.appendChild(section);
}
const tasks=document.getElementById('task-list');
const session=document.createElement('label');session.innerHTML='Dateien und Zeitstellen<textarea id="session-notes"></textarea>';tasks.appendChild(session);
session.querySelector('textarea').value=answers.session||'';session.querySelector('textarea').oninput=e=>{answers.session=e.target.value;save()};
for(const task of data.tasks){const card=document.createElement('div');card.className='card';
 card.innerHTML='<div class="tag">SD'+String(task.id).padStart(2,'0')+'</div><strong>'+esc(task.title)+'</strong><p>'+esc(task.question)+'</p>'+
  '<label>Befund <select><option value="offen">Noch offen</option><option value="passt">Für mich passend</option><option value="stoert">Konkrete Störstelle</option><option value="entfaellt">Weglassen vorschlagen</option></select></label><textarea aria-label="Notiz SD'+task.id+'" placeholder="Datei, Zeitpunkt und konkrete Beobachtung"></textarea>';
 const select=card.querySelector('select'),input=card.querySelector('textarea');const value=answers[task.id]||{};select.value=value.status||'offen';input.value=value.note||'';
 select.onchange=input.oninput=()=>{answers[task.id]={status:select.value,note:input.value};save()};
 if(task.groups.length){const links=document.createElement('p');links.className='small';links.innerHTML=task.groups.map(g=>'<a href="#group-'+g+'">'+esc(data.group_titles[g])+'</a>').join(' · ');card.appendChild(links)}
 tasks.appendChild(card);
}
function save(){try{localStorage.setItem(storageKey,JSON.stringify(answers));document.getElementById('saved').textContent='Notizen lokal gespeichert'}catch(e){document.getElementById('saved').textContent='Bitte Notizen als JSON exportieren'}}
document.getElementById('notes').onclick=()=>document.getElementById('tasks').scrollIntoView({behavior:'smooth'});
document.getElementById('export').onclick=()=>{const report={review_version:1,review_id:data.review_id,renderer_sha256:data.renderer_sha256,source_sha256:data.source_sha256,
 created_at:new Date().toISOString(),hearing_answers:answers,device_approval:false};const url=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)+'\\n'],{type:'application/json'}));
 const link=document.createElement('a');link.href=url;link.download='AMBIENT_Hoerbefund.json';link.click();setTimeout(()=>URL.revokeObjectURL(url),2000)};
</script></html>'''
    (out / 'START_HERE.html').write_text(page.replace('__DATA__', data))


if __name__ == '__main__':
    main()
