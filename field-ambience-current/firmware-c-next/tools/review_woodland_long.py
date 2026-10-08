#!/usr/bin/env python3
"""Long, soft WOODLAND string candidate after rejection of short pluck figures.

Temporary source edits are diagnostic only, not Product defaults. The existing
Karplus-Strong loop rings longer; no waveform stretching, layers or extra FX.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

import numpy as np
from package_musical_review import measure, read
from review_coast_components import ROOT, SR

SCORE = ((0,1,0,50,.62,.50),(2,1,1,54,.50,.65),
         (11,0,0,50,0,0),(14,1,0,59,.58,.55),
         (20,0,1,54,0,0),(23,1,1,62,.50,.65),
         (24,0,0,59,0,0),(27,1,0,54,.56,.55),
         (31,0,1,62,0,0),(34,1,1,57,.50,.60),
         (36,0,0,54,0,0),(40,0,1,57,0,0))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def driver(score, seconds, voices):
    text = (ROOT/'tools/render_ensemble_sketch.c').read_text()
    value = 'static const event_t score[]={\n'+''.join(
        '    {%.2ff,%d,%d,%d,%.3ff,%.3ff},\n' % e for e in score)+'};'
    text, count = re.subn(r'static const event_t score\[\]=\{.*?\n\};',
                         lambda _:value, text, count=1, flags=re.S)
    assert count == 1
    for old,new in {'SECONDS=27':f'SECONDS={seconds}',
                    'starts==10':f'starts=={sum(e[1] for e in score)}',
                    'sources<=3':f'sources<={voices}',
                    'max_sources==3':f'max_sources=={voices}',
                    'shape_set_release(0)':'shape_set_release(.5f)',
                    '%.3f,%.3f,0\\n':'%.3f,%.3f,0.50\\n'}.items():
        assert text.count(old)==1
        text=text.replace(old,new)
    return text.replace('bowed','pluck').replace('pluck_try_note_on','pluck_note_on').replace(
        'pluck_render_mix(l,r,sl,sr,n,.35f)','pluck_render_mix(l,r,sl,sr,n)').replace(
        'pluck_init();','pluck_init();pluck_set_damp(.015f);')


def rms(x, a, b):
    return float(np.sqrt(np.mean(x[int(a*SR):int(b*SR)]**2)))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    out=parser.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    original=ROOT/'src/pluck.c';source=original.read_text()
    replacements={'#define T60_S     3.2f':'#define T60_S     36.0f',
                  '#define STOP_FRAMES ((uint32_t)(0.020f * SR))':'#define STOP_FRAMES ((uint32_t)(2.0f * SR))',
                  'dsp_clampf(0.008f * shape_attack_scale(), 0.004f, 0.032f)':
                  'dsp_clampf(0.8f * shape_attack_scale(), 0.4f, 2.4f)'}
    for old,new in replacements.items():
        assert source.count(old)==1
        source=source.replace(old,new)
    inputs=[original,ROOT/'tools/render_ensemble_sketch.c',Path(__file__),
            ROOT/'tools/package_musical_review.py',
            *[ROOT/'src'/f'{s}.c' for s in ('ambient_room','shape','dsp')],
            *sorted((ROOT/'include').glob('*.h'))]
    manifest=dict(scope='Diagnostic long-ring string candidate, not production generator/defaults.',
        feedback='Previous WOODLAND sample rejected: too short and plucked; seek long stretched tones.',
        changes=dict(nominal_t60_seconds=36,attack_base_seconds=.8,stop_seconds=2,
                     damping=.015,release=.5,room=.24,nature=0,volume=.6,calibration=.5),
        score=SCORE,source_sha256={str(p.relative_to(ROOT)):sha(p) for p in inputs},
        candidate_source_sha256=hashlib.sha256(source.encode()).hexdigest(),files=[])
    with tempfile.TemporaryDirectory(prefix='woodland-long-') as value:
        tmp=Path(value);candidate=tmp/'pluck.c';candidate.write_text(source)
        def build(score, seconds, voices, name):
            c=tmp/f'{name}.c';c.write_text(driver(score,seconds,voices));binary=tmp/name
            subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
                '-I'+str(ROOT/'include'),str(c),str(candidate),
                *[str(ROOT/'src'/f'{s}.c') for s in ('ambient_room','shape','dsp')],
                '-lm','-o',str(binary)],check=True)
            return binary
        isolated=build(((0,1,0,50,.62,.5),(11,0,0,50,0,0)),15,1,'isolated')
        probe=tmp/'isolated.wav'
        subprocess.run([str(isolated),'dry','512',str(probe)],check=True,capture_output=True)
        x=read(probe)
        onset=rms(x,0,.1);body=rms(x,2,3);late=rms(x,8,9)
        assert onset/body<.1,'Fast pluck remains at onset'
        assert late/body>.2,'String does not carry a meaningful eight-second body'
        manifest['isolated_D3']=dict(onset_body_ratio=onset/body,late_body_ratio=late/body,
                                    body_rms=body,late_rms=late)
        binary=build(SCORE,45,2,'render')
        for mode in ('dry','room'):
            raw=out/f'AMBIENT_WOODLAND_Long_{mode}_raw_45s.wav'
            report=subprocess.run([str(binary),mode,'512',str(raw)],check=True,capture_output=True,text=True).stderr
            control=tmp/'control.wav'
            subprocess.run([str(binary),mode,'64',str(control)],check=True,capture_output=True)
            assert raw.read_bytes()==control.read_bytes()
            assert Path(str(raw)+'.events.csv').read_bytes()==Path(str(control)+'.events.csv').read_bytes()
            x=read(raw);assert x.shape==(45*SR,2) and np.isfinite(x).all()
            mono=float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2));dc=x.mean(axis=0).tolist()
            assert mono>.65 and max(map(abs,dc))<.0002
            level=measure(raw);assert level['true_peak_dbfs']<=-6
            gain=min(18,-23-level['integrated_lufs'],-6-level['true_peak_dbfs'])
            listen=out/raw.name.replace('_raw_','_listen_')
            subprocess.run(['ffmpeg','-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                            '-c:a','pcm_s16le',str(listen)],check=True)
            checked=measure(listen);assert checked['true_peak_dbfs']<=-5.9
            manifest['files'].append(dict(mode=mode,raw=level,listen=checked,gain_db=gain,
                file=listen.name,sha256=sha(listen),mono=mono,dc=dc,pcm_64_512_identical=True,
                renderer_report=report.strip()))
            print(mode,checked,report.strip(),flush=True)
    (out/'WOODLAND_LONG_METRICS.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('WOODLAND LONG PASS: soft onset, measured 8s body, two actual strings, 2s release and block-exact PCM. Hearing/production integration open.')


if __name__=='__main__':
    main()
