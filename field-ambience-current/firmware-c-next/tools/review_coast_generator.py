#!/usr/bin/env python3
"""Render the integrated autonomous COAST through actual Product APIs/DSP."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure,read
from review_coast_components import ROOT,SR


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    out=parser.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    sources=[ROOT/'src'/f'{s}.c' for s in ('engine_product','world_grammar','bowed','horn','pluck',
        'ambient_room','nature','dsp','shape','tuning','brain','cells')]
    driver=ROOT/'tools/render_coast_generator.c'
    inputs=[*sources,driver,Path(__file__),ROOT/'tools/package_musical_review.py',
            *sorted((ROOT/'include').glob('*.h'))]
    manifest=dict(scope='Integrated Product COAST generator, normal owner/tail/admission, real DSP start acks; no direct source calls or SHAPE bypass.',
        parameters=dict(seed=1234,key='D',collection='major',tuning='equal',activity=.5,
                        color=.5,attack=.5,release=.5,volume=.6,room=.24,nature=0),
        seconds=70,control_tick_ms=10,
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in inputs},files=[])
    with tempfile.TemporaryDirectory(prefix='coast-engine-') as value:
        tmp=Path(value);binary=tmp/'render'
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'),str(driver),*map(str,sources),'-lm','-o',str(binary)],check=True)
        for mode in ('dry','room'):
            raw=out/f'AMBIENT_COAST_Generator_{mode}_raw_70s.wav'
            report=subprocess.run([str(binary),mode,'512',str(raw)],check=True,capture_output=True,text=True).stderr
            control=tmp/f'{mode}.wav'
            subprocess.run([str(binary),mode,'64',str(control)],check=True,capture_output=True)
            assert raw.read_bytes()==control.read_bytes(),'Control-grid PCM differs across block sizes'
            assert Path(str(raw)+'.events.csv').read_bytes()==Path(str(control)+'.events.csv').read_bytes()
            x=read(raw);assert x.shape==(70*SR,2) and np.isfinite(x).all()
            mono=float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2));dc=x.mean(axis=0).tolist()
            assert mono>.65 and max(map(abs,dc))<.0002
            level=measure(raw);assert level['true_peak_dbfs']<=-6
            gain=min(18.,-23-level['integrated_lufs'],-6-level['true_peak_dbfs'])
            listen=out/raw.name.replace('_raw_','_listen_')
            subprocess.run(['ffmpeg','-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                            '-c:a','pcm_s16le',str(listen)],check=True)
            checked=measure(listen);assert checked['true_peak_dbfs']<=-5.9
            manifest['files'].append(dict(mode=mode,raw=level,listen=checked,constant_gain_db=gain,
                listen_file=listen.name,mono=mono,dc=dc,raw_sha256=sha(raw),listen_sha256=sha(listen),
                trace_sha256=sha(Path(str(raw)+'.events.csv')),pcm_64_512_identical=True,renderer_report=report.strip()))
            print(mode,checked,report.strip(),flush=True)
    (out/'COAST_GENERATOR_METRICS.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('COAST ENGINE AUDIO PASS: autonomous real engine phrase, normal Product macros, shared goal, exact PCM, actual source retirement; device/hearing open.')


if __name__=='__main__':
    main()
