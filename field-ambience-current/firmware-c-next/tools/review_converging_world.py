#!/usr/bin/env python3
"""COAST direction: independent outer registers converge on one common tone."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import shutil
import subprocess
import tempfile

import numpy as np

from package_musical_review import measure, read
from review_coast_components import ROOT, SR


def plan(seed, center=62):
    """A bounded musical rule, with seed-dependent phrase timing/accents."""
    rng=random.Random(seed)
    spacing=5.8+rng.randrange(41)/100
    lag=.15+rng.randrange(31)/100
    lower=[center-12,center-5,center-3,center]
    upper=[center+12,center+9,center+4,center]
    events=[]
    for role,path in enumerate((lower,upper)):
        for step,midi in enumerate(path[:-1]):
            onset=round(step*spacing+(lag if role and step else 0),2)
            hold=2.8+rng.randrange(31)/100
            velocity=(.55 if role==0 else .44)+rng.randrange(6)/100
            events.extend([(onset,1,role,midi,velocity,.50 if role==0 else .45),
                           (round(onset+hold,2),0,role,midi,0,0)])
    # A quiet chord tone holds the phrase together, instead of another bed.
    events.extend([(.7,1,2,center-8,.32,.60),(22.6,0,2,center-8,0,0)])
    meet=round(3*spacing+.25,2)
    # Both paths intend the same destination. Render one shared note once.
    events.extend([(meet,1,0,center,.59,.50),(23.5,0,0,center,0,0)])
    events.sort(key=lambda e:(e[0],e[1],e[2]))
    assert lower==sorted(lower) and upper==sorted(upper,reverse=True)
    assert lower[0]==center-12 and upper[0]==center+12 and lower[-1]==upper[-1]
    assert len([e for e in events if e[1]])==8
    assert len([e for e in events if e[1] and e[3]==center])==1
    for owner in range(3):
        owned=[e for e in events if e[2]==owner]
        assert [e[1] for e in owned]==[1,0]*(len(owned)//2)
        for off,on in zip(owned[1::2],owned[2::2]):
            assert on[0]-off[0]>=2.5, 'Retirement margin before source reuse'
    return {'seed':seed,'center_midi':center,'spacing_seconds':spacing,
            'upper_lag_seconds':lag,'lower_path':lower,'upper_path':upper,
            'meeting_seconds':meet,'meeting_rendered_sources':1,'events':events}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    out=parser.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    cc,ffmpeg=shutil.which('cc'),shutil.which('ffmpeg');assert cc and ffmpeg
    chosen=plan(1234)
    alternatives=[plan(seed) for seed in (1,42,91267)]
    assert any(p['events']!=chosen['events'] for p in alternatives)
    assert plan(1234)==chosen
    for center in (57,62,65):
        for seed in range(100): plan(seed,center)
    template=ROOT/'tools/render_ensemble_sketch.c'
    score='static const event_t score[]={\n'+''.join(
        '    {%s,%d,%d,%d,%sf,%sf},\n' % (f'{s:.2f}f',on,owner,midi,f'{v:.3f}',f'{a:.3f}')
        for s,on,owner,midi,v,a in chosen['events'])+'};'
    original=template.read_text()
    pattern=r'static const event_t score\[\]=\{.*?\n\};'
    assert len(re.findall(pattern,original,re.S))==1 and original.count('starts==10')==1
    driver=re.sub(pattern,lambda _:score,original,count=1,flags=re.S).replace('starts==10','starts==8')
    sources=[ROOT/'src'/f'{s}.c' for s in ('bowed','ambient_room','shape','dsp')]
    inputs=[*sources,template,ROOT/'tools/package_musical_review.py',Path(__file__),
            *sorted((ROOT/'include').glob('*.h'))]
    manifest={'task':'COAST converging-register algorithm prototype',
        'plan':chosen,'alternate_plan_checks':alternatives,
        'scope':'Seeded host World algorithm using actual unchanged Product Bowed/Room DSP. Production generator/admission is not yet integrated.',
        'diagnostic_changes':'Direct SHAPE release=0, per-role attacks, D5/B4 above current Product register. Common destination merged to one physical oscillator.',
        'reference':{'seconds':27,'sample_rate':SR,'volume':.6,'room':.24,'nature':0},
        'source_sha256':{str(p.relative_to(ROOT)):sha(p) for p in inputs},
        'temporary_driver_sha256':hashlib.sha256(driver.encode()).hexdigest(),'files':[]}
    with tempfile.TemporaryDirectory(prefix='ambient-converge-') as value:
        tmp=Path(value);c=tmp/'driver.c';c.write_text(driver);binary=tmp/'render'
        subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'),str(c),*map(str,sources),'-lm','-o',str(binary)],check=True)
        for mode in ('dry','room'):
            raw=out/f'AMBIENT_COAST_Converge_{mode}_raw_27s.wav'
            report=subprocess.run([str(binary),mode,'512',str(raw)],check=True,capture_output=True,text=True).stderr
            control=tmp/f'{mode}.wav'
            subprocess.run([str(binary),mode,'64',str(control)],check=True,capture_output=True)
            assert raw.read_bytes()==control.read_bytes()
            assert Path(str(raw)+'.events.csv').read_bytes()==Path(str(control)+'.events.csv').read_bytes()
            x=read(raw);assert x.shape==(27*SR,2) and np.isfinite(x).all()
            mono=float(np.sum(x.mean(axis=1)**2)/(np.sum(x*x)/2));dc=x.mean(axis=0).tolist()
            assert mono>.65 and max(map(abs,dc))<.0002
            level=measure(raw);assert level['true_peak_dbfs']<=-6
            gain=min(12.,-23-level['integrated_lufs'],-6-level['true_peak_dbfs'])
            listen=out/raw.name.replace('_raw_','_listen_')
            subprocess.run([ffmpeg,'-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                '-c:a','pcm_s16le',str(listen)],check=True)
            checked=measure(listen);assert checked['true_peak_dbfs']<=-5.9
            manifest['files'].append({'mode':mode,'raw_file':raw.name,'listen_file':listen.name,
                'raw':level,'listen':checked,'constant_gain_db':gain,'mono_energy_ratio':mono,
                'mean_dc_lr':dc,'raw_sha256':sha(raw),'listen_sha256':sha(listen),
                'trace_sha256':sha(Path(str(raw)+'.events.csv')),'pcm_64_512_identical':True,
                'renderer_report':report.strip()})
            print(f'CONVERGE {mode}: {checked}, meeting {chosen["meeting_seconds"]:.2f}s, 64/512 exact',flush=True)
    (out/'CONVERGE_METRICS.json').write_text(json.dumps(manifest,indent=2,allow_nan=False)+'\n')
    (out/'README.md').write_text(f'''# COAST — entgegenlaufende Register

Zuerst AMBIENT_COAST_Converge_room_listen_27s.wav hören.
Beide äußeren Stimmen starten gleichzeitig: D3 unter D4, D5 über D4.
Unten: D3 → A3 → B3 → D4. Oben: D5 → B4 → F#4 → D4.
Ein leises gehaltenes F#3 verbindet die wechselnden Harmonien.
Bei {chosen['meeting_seconds']:.2f} s treffen die Wege auf D4; das gemeinsame
Ziel wird einmal gespielt, mit gemeinsamem Halten statt doppelter Oszillatoren.

Das ist ein seed-gesteuerter Host-Algorithmus, noch kein integrierter Product-
Generator. Parameter: Seed1234, Room0,24, Nature0, Volume0,6; direkter kurzer
Release wie beim positiv bewerteten Ensemble-Entwurf. D5/B4 erweitern den
Produktregister-Kandidaten und brauchen noch eine Klang-/Geräteabnahme.
Echte unveränderte C-Tonkörper, maximal drei Quellen inklusive Releases,
acht angenommene Starts, natürliches Quellenende, kein Clear/Schlussfade.
27 s PCM16 Stereo/44,1 kHz; Dry ist dieselbe Folge; LISTEN hat festen Gain
auf -23 LUFS, RAW behält den Renderpegel. Planung, Traces und Hashes liegen bei.
''')
    print('CONVERGE PASS: opposing monotonic paths, shared destination, bounded actual voices and safe PCM; integration/hearing open.')


if __name__=='__main__':
    main()
