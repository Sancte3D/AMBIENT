#!/usr/bin/env python3
"""Render one composed ensemble proposal with actual unchanged C DSP."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

import numpy as np

from package_musical_review import measure, read
from review_coast_components import ROOT, SR


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    out=parser.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    cc,ffmpeg=shutil.which('cc'),shutil.which('ffmpeg');assert cc and ffmpeg
    sources=[ROOT/'src'/f'{s}.c' for s in ('bowed','ambient_room','shape','dsp')]
    driver=ROOT/'tools/render_ensemble_sketch.c'
    inputs=[*sources,driver,Path(__file__),ROOT/'tools/package_musical_review.py',
            *sorted((ROOT/'include').glob('*.h'))]
    manifest={'task':'Musical ensemble direction, user feedback 2026-10-08',
        'source_tree':subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=ROOT,text=True).strip(),
        'scope':'Composed host proposal using real Product Bowed/Room DSP directly, not the autonomous generator or normal admission policy.',
        'diagnostic_changes':'Direct per-note SHAPE attack, direct release=0 (scale .25); shorter than current product macro minimum. G3/C#4 expand beyond D pentatonic. No production sources changed.',
        'score':'D major -> B minor/F# -> G major -> A sus4 -> A major; common tones, F#4/F#3 register handover, staggered starts and unequal holds.',
        'reference':{'sample_rate':SR,'seconds':27,'voices':3,'volume':.6,'source_calibration':.5,'room_amount':.24,'nature':0},
        'source_sha256':{str(p.relative_to(ROOT)):sha(p) for p in inputs},'files':[]}
    with tempfile.TemporaryDirectory(prefix='ambient-ensemble-') as temp:
        tmp=Path(temp);binary=tmp/'render'
        subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'),str(driver),*map(str,sources),'-lm','-o',str(binary)],check=True)
        for mode in ('dry','room'):
            raw=out/f'AMBIENT_Ensemble_{mode}_raw_27s.wav'
            report=subprocess.run([str(binary),mode,'512',str(raw)],check=True,capture_output=True,text=True).stderr
            control=tmp/f'{mode}_64.wav'
            subprocess.run([str(binary),mode,'64',str(control)],check=True,capture_output=True)
            assert raw.read_bytes()==control.read_bytes(), 'Block-size-dependent musical rendering'
            assert Path(str(raw)+'.events.csv').read_bytes()==Path(str(control)+'.events.csv').read_bytes()
            x=read(raw);assert x.shape==(27*SR,2) and np.isfinite(x).all()
            mono=float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2))
            dc=x.mean(axis=0).tolist();assert mono>.65 and max(map(abs,dc))<.0002
            level=measure(raw);assert level['true_peak_dbfs']<=-6
            gain=min(12.,-23-level['integrated_lufs'],-6-level['true_peak_dbfs'])
            listen=out/raw.name.replace('_raw_','_listen_')
            subprocess.run([ffmpeg,'-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                '-c:a','pcm_s16le',str(listen)],check=True)
            checked=measure(listen);assert checked['true_peak_dbfs']<=-5.9
            manifest['files'].append({'mode':mode,'raw_file':raw.name,'listen_file':listen.name,
                'constant_gain_db':gain,'raw':level,'listen':checked,'mono_energy_ratio':mono,
                'mean_dc_lr':dc,'raw_sha256':sha(raw),'listen_sha256':sha(listen),
                'trace_sha256':sha(Path(str(raw)+'.events.csv')),'renderer_report':report.strip(),
                'pcm_64_512_identical':True})
            print(f"ENSEMBLE {mode}: {level}, fixed gain {gain:.1f} dB, 64/512 exact",flush=True)
    (out/'ENSEMBLE_METRICS.json').write_text(json.dumps(manifest,indent=2,allow_nan=False)+'\n')
    (out/'README.md').write_text('''# AMBIENT — musikalischer Ensemble-Entwurf

Zuerst AMBIENT_Ensemble_room_listen_27s.wav hören.
D-Dur öffnet sich zu h-Moll/F#, G-Dur, A sus4 und A-Dur.
Gemeinsame Töne tragen weiter, F# wandert zwischen Oktaven, Stimmen haben
verschiedene Dauern und versetzte Einsätze. Keine Creep-Kopie und keine Samples.

Dies ist eine komponierte Richtungsprobe aus unseren echten unveränderten
C-Tonkörpern mit einem gemeinsamen vorhandenen Raum, kein fertiger Generator.
Maximal drei echte Stimmen inklusive Releases; zehn Starts; natürlicher
Quellenausklang, kein Clear oder Schlussfade. PCM bei 64/512 Frames bytegleich.
Alle Dateien sind 27 s, PCM16 Stereo 44,1 kHz. RAW ist der feste Renderpegel;
LISTEN verwendet nur einen festen dokumentierten Gain auf -23 LUFS.

Die Probe verwendet direkte SHAPE-Release=0 und pro Note andere Attackwerte.
Das ist kürzer/freier als die aktuellen Produkt-Makrogrenzen. G3 und C#4
erweitern den D-Pentatonik-Vorrat. Normale Produkt-Zulassung und Generator
werden bewusst nicht als implementiert behauptet. Diese Übertragung ist
der nächste Softwarepunkt, falls die musikalische Richtung überzeugt.
Der Raum ist derselbe vorhandene C-FDN bei 0,24; Nature bleibt aus.
Dry spielt denselben Verlauf und dient als Gegenprobe für die Komposition.
Hörfrage: Sind Akkordbogen, gemeinsame Töne und Stimmantworten erkennbar und
lohnt diese Richtung, bevor Klangfarben und Weltvarianten weitergebaut werden?
''')
    print('ENSEMBLE PASS: composed proposal, actual sources, three voices, natural source retirement, safe PCM; hearing and generator integration open.')


if __name__=='__main__':
    main()
