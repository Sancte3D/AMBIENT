#!/usr/bin/env python3
"""Current V2 blind material audition; actual C sources, no listening verdict."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import random
import secrets
import subprocess
import tempfile
import numpy as np
from review_world_identity import ROOT,SOURCES,SR,NAMES,read,rms,db
from review_world_materials import save_wav,encode

def run(args):return subprocess.check_output([str(x) for x in args],text=True)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--order-seed',type=int);args=ap.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    inputs=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{s}.c' for s in SOURCES+['gesture']]
    inputs += [ROOT/'tools/render_world_audit.c',ROOT/'tools/review_world_identity.py',
               ROOT/'tools/review_world_materials.py',Path(__file__)]
    before={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
    seed=args.order_seed if args.order_seed is not None else secrets.randbits(32)
    order=list(range(3));random.Random(seed).shuffle(order)
    pcm={};full_frames=0;trace_hashes={};gains={};public_cues=[];files={}
    with tempfile.TemporaryDirectory(prefix='blind-materials-') as folder:
        tmp=Path(folder);binary=tmp/'render'
        run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
             '-DFAM_SOUND_FINISH_CANDIDATE','-I'+str(ROOT/'include'),ROOT/'tools/render_world_audit.c',
             *[ROOT/'src'/f'{s}.c' for s in SOURCES+['gesture']],'-lm','-o',binary])
        for midi in (50,57,62):
            for world in range(3):
                for fx in (0,1):
                    captures=[];traces=[]
                    for cap in (512,37):
                        p=tmp/f'{world}-{midi}-{fx}-{cap}.wav'
                        run([binary,p,world,midi,fx,.5,cap,1234,0]);captures.append(read(p))
                        traces.append(tuple(Path(str(p)+suffix).read_bytes() for suffix in ('.events.csv','.states.csv')))
                    assert captures[0].tobytes()==captures[1].tobytes() and traces[0]==traces[1]
                    rows=list(csv.DictReader(traces[0][0].decode().splitlines()))
                    ons=[e for e in rows if int(e['on'])>0]
                    assert len(ons)==1 and abs(float(ons[0]['hz'])-440*2**((midi-69)/12))<.01
                    pcm[(world,midi,fx)]=captures[0];full_frames+=len(captures[0])
                    trace_hashes[f'{world}-{midi}-{fx}']=hashlib.sha256(b''.join(traces[0])).hexdigest()
            # Fixed gains per note/source from dry body, reused for wet. A
            # common lowered target, if necessary, preserves the whole attack.
            target=min(10**(-26/20),min(.48*rms(pcm[(w,midi,0)][3*SR:8*SR])/
                max(np.max(np.abs(pcm[(w,midi,f)])) for f in (0,1)) for w in range(3)))
            for w in range(3):gains[(w,midi)]=target/rms(pcm[(w,midi,0)][3*SR:8*SR])
        for fx,label in ((0,'Trocken'),(1,'Mit_FX')):
            chunks=[];at=0;edge=int(.04*SR)
            for group,midi in enumerate((50,57,62),1):
                for letter,w in zip('ABC',order):
                    if chunks:chunks.append(np.zeros((SR//2,2)));at+=SR//2
                    if not fx:public_cues.append(dict(group=group,label=letter,seconds=at/SR))
                    # Same continuous 12s excerpt: onset at .5s, ten-second
                    # hold, then release. These are not complete natural tails.
                    x=pcm[(w,midi,fx)][SR//2:25*SR//2].copy()*gains[(w,midi)]
                    x[:edge]*=np.linspace(0,1,edge)[:,None];x[-edge:]*=np.linspace(1,0,edge)[:,None]
                    chunks.append(x);at+=len(x)
            p=out/f'AMBIENT_Blind_V2_{label}.wav';save_wav(p,np.concatenate(chunks));files.update(encode(p))
    assert before=={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
    proof=dict(material_model=2,full_cases=18,full_stereo_frames_compared=full_frames,
        partition_37_512_exact=True,actual_admitted_hz_verified=True,nature=0,seed=1234,
        color=.5,velocity=.7,attack_release=.5,master=.75,fx_room=.5,editorial_edge_ms=40,
        excerpt_seconds=12,audition_seconds=112,cues=public_cues,files=files,
        source_sha256=before,trace_sha256=trace_hashes,hearing_accepted=False,device_accepted=False,
        limitations='Fixed dry-body RMS matching is not subjective equal loudness. Editorial 12s excerpts retain onset/hold and 1.5s release, not complete natural tails. No blind listener test performed.')
    (out/'BLIND_MATERIALS_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n')
    cues='\n'.join(f"| {c['group']}{c['label']} | {int(c['seconds']//60)}:{c['seconds']%60:04.1f} |" for c in public_cues)
    (out/'AMBIENT_Blind_V2_Anleitung.md').write_text('''# Aktueller World-Vergleich, unbekannte Reihenfolge

Zuerst Trocken hören, danach Mit FX. A/B/C bezeichnen in beiden Dateien
dieselben drei Quellen. Gruppe 1/2/3 spielt jeweils denselben Ton pro Quelle,
in drei Registern. Nature ist aus, Velocity und Klangregler sind gleich.
Ein fester Abgleich des trockenen Körpers gilt auch für die jeweilige FX-
Fassung. Kein dynamischer Lautheitsausgleich und kein Ersatzsynth.

Beschreibe pro A/B/C Einsatz, gehaltenen Körper, Nachklang und eventuelle
störende Stellen mit Zeit. Sind die drei bei denselben Tönen gut erkennbar,
bleiben sie angenehm? Die 12-s-Ausschnitte enthalten 1,5 s Release und sind
keine vollständigen natürlichen Ausklänge. Keine Hörabnahme vorausgesetzt.
Die getrennte Auflösung erst nach eigenen Notizen öffnen.

| Abschnitt | Start in beiden Dateien |
|---|---|
'''+cues+'\n')
    key=dict(order_seed=seed,labels={letter:NAMES[w] for letter,w in zip('ABC',order)},
             fixed_gain_db={f'{NAMES[w]} MIDI{m}':db(g) for (w,m),g in gains.items()},proof=proof)
    (out/'AMBIENT_Blind_V2_Aufloesung.md').write_text('# Auflösung, erst nach dem Hören öffnen\n\n'+
        '\n'.join(f'{letter}: {NAMES[w]}' for letter,w in zip('ABC',order))+'\n\n```json\n'+json.dumps(key,indent=2)+'\n```\n')
    print(json.dumps(dict(status='prepared',cases=18,seconds_each=112,source_and_partition_verified=True,hearing_accepted=False)))
if __name__=='__main__':main()
