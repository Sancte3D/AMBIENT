#!/usr/bin/env python3
"""Actual matched World identity audit; descriptors are not a listening verdict.

Requires numpy and ffmpeg. Source/render guard, actual admitted pitches, full
partition comparisons and retained event/state traces. No Nature in any case.
"""
import argparse
import csv
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import wave
import numpy as np
from review_cell_unfold import ROOT,SOURCES

SR=44100
NAMES=['COAST','WOODLAND','HIGHLANDS']
def run(args,**kw):return subprocess.check_output([str(x) for x in args],text=True,**kw)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path):
    with wave.open(str(path)) as w:
        assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(SR,2,2)
        n=w.getnframes();raw=w.readframes(n);assert len(raw)==n*4 and path.stat().st_size==44+n*4
    return np.frombuffer(raw,'<i2').reshape(-1,2).astype(np.float64)/32768
def rms(x):return float(np.sqrt(np.mean(x*x)))
def db(x):return None if x==0 else 20*math.log10(x)
def similarity(a,b):return float(np.dot(a,b)/(np.linalg.norm(a)*np.linalg.norm(b)))
def describe(x,midi):
    mono=np.mean(x,axis=1);hz=440*2**((midi-69)/12)
    hold=mono[2*SR:10*SR];length=SR//10
    envelope=np.sqrt(np.mean(hold[:len(hold)//length*length].reshape(-1,length)**2,axis=1))
    window=mono[3*SR:5*SR];weight=np.hanning(len(window));t=np.arange(len(window))/SR
    harmonics=np.array([2*abs(np.sum(window*weight*np.exp(-2j*np.pi*hz*k*t)))/sum(weight) for k in range(1,17)])
    power=harmonics**2;assert sum(power)>0
    # These shape descriptors deliberately include the common fundamental.
    # An overtone-only comparison is separate, and neither is a classifier.
    return dict(steady_rms_dbfs=db(rms(x[3*SR:8*SR])),peak=float(np.max(np.abs(x))),
                late_hold_change_db=db(rms(x[9*SR:10*SR])/rms(x[2*SR:3*SR])),
                late_release_change_db=db(rms(x[16*SR:17*SR])/rms(x[9*SR:10*SR])),
                harmonic_energy_fraction=(power/sum(power)).tolist(),
                harmonic_centroid_hz=float(np.dot(power,np.arange(1,17)*hz)/sum(power)),
                overtone_energy_fraction=float(sum(power[1:])/sum(power)),
                hold_rms_envelope=envelope.tolist(),mono_energy_ratio=float(np.mean(mono*mono)/np.mean(x*x)))

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);a=ap.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    files=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']]
    files+=[ROOT/'tools/render_world_audit.c',Path(__file__)]
    before={str(p.relative_to(ROOT)):sha(p) for p in sorted(files)}
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-DFAM_SOUND_FINISH_CANDIDATE']
    cases=[];pcm={};partitions=0;music=[]
    with tempfile.TemporaryDirectory(prefix='world-identity-') as directory:
        tmp=Path(directory);binary=tmp/'render'
        run([os.environ.get('CC','cc'),*flags,'-I'+str(ROOT/'include'),ROOT/'tools/render_world_audit.c',
             *[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']],'-lm','-o',binary])
        def capture(world,midi=50,fx=0,color=.5,cap=512,seed=1234,generate=False):
            nonlocal partitions
            name=f'w{world}_n{midi}_fx{fx}_c{color}_s{seed}_g{int(generate)}_b{cap}'
            path=tmp/(name+'.wav');log=run([binary,path,world,midi,fx,color,cap,seed,int(generate)])
            result=read(path)
            assert np.all(np.isfinite(result)) and np.max(np.abs(result))<.75
            events=Path(str(path)+'.events.csv').read_bytes();states=Path(str(path)+'.states.csv').read_bytes()
            if cap==512:
                (out/(name+'.events.csv')).write_bytes(events);(out/(name+'.states.csv')).write_bytes(states)
            return result,events,states,log
        for world in range(3):
            for midi,color in [(m,.5) for m in (50,57,62)]+[(50,0),(50,1)]:
                for fx in (0,1):
                    x,e,s,log=capture(world,midi,fx,color)
                    y,ey,sy,_=capture(world,midi,fx,color,cap=37)
                    assert x.tobytes()==y.tobytes() and e==ey and s==sy
                    partitions+=len(x);key=(world,midi,fx,color);pcm[key]=x
                    rows=list(csv.DictReader(e.decode().splitlines()));on=[r for r in rows if int(r['on'])>0]
                    assert len(on)==1 and abs(float(on[0]['hz'])-440*2**((midi-69)/12))<.01
                    d=describe(x,midi);cases.append(dict(world=world,name=NAMES[world],midi=midi,fx=fx,color=color,**d))
            for seed in (1234,42):
                x,e,s,log=capture(world,fx=1,seed=seed,generate=True)
                y,ey,sy,_=capture(world,fx=1,seed=seed,cap=37,generate=True)
                assert x.tobytes()==y.tobytes() and e==ey and s==sy
                partitions+=len(x)
                rows=list(csv.DictReader(e.decode().splitlines()));on=[r for r in rows if int(r['on'])>0]
                states=list(csv.DictReader(s.decode().splitlines()));active=np.array([int(r['sources']) for r in states])
                assert len(on)>=3 and not np.any(x[-SR:])
                intervals=np.diff([int(r['frame'])/SR for r in on])
                heard=np.array([float(r['hz']) for r in on])
                music.append(dict(world=world,name=NAMES[world],seed=seed,starts=len(on),
                    source_limit=int(max(active)),pitch_min=float(min(heard)),pitch_max=float(max(heard)),
                    first_to_last_start_seconds=(int(on[-1]['frame'])-int(on[0]['frame']))/SR,
                    onset_intervals_seconds=intervals.tolist(),
                    source_free_fraction=float(np.mean(active[100:8100]==0)),
                    exact_final_second_silence=True,whole_rms_dbfs=db(rms(x))))
            print(f'{NAMES[world]}: matched notes, Color ends, dry/FX, Generate seeds and complete partitions pass',flush=True)
        pairs=[]
        for midi in (50,57,62):
            for fx in (0,1):
                selected=[next(c for c in cases if c['world']==w and c['midi']==midi and c['fx']==fx and c['color']==.5) for w in range(3)]
                for i,j in itertools.combinations(range(3),2):
                    first,second=selected[i],selected[j]
                    p=np.sqrt(first['harmonic_energy_fraction']);q=np.sqrt(second['harmonic_energy_fraction'])
                    pairs.append(dict(a=NAMES[i],b=NAMES[j],midi=midi,fx=fx,
                        harmonic_amplitude_cosine=similarity(p,q),overtone_amplitude_cosine=similarity(p[1:],q[1:]),
                        centroid_difference_hz=abs(first['harmonic_centroid_hz']-second['harmonic_centroid_hz']),
                        hold_decay_difference_db=abs(first['late_hold_change_db']-second['late_hold_change_db'])))
        # One fixed gain per World from its dry steady root, reused for FX.
        # This removes simple loudness identification without flattening the
        # source's attack/decay or independently making Room louder/quieter.
        gains=[]
        for world in range(3):
            dry=pcm[(world,50,0,.5)];wet=pcm[(world,50,1,.5)]
            gain=min(10**(-26/20)/rms(dry[3*SR:8*SR]),.49/max(np.max(np.abs(dry)),np.max(np.abs(wet))))
            gains.append(float(gain))
        output={}
        for fx,name in [(0,'AMBIENT_Worlds_Trocken'),(1,'AMBIENT_Worlds_Mit_FX')]:
            chunks=[]
            for world in range(3):
                x=pcm[(world,50,fx,.5)].copy()*gains[world]
                # Editorial cut edges only: the original full captures and
                # their complete event/state traces supply the measurements.
                edge=int(.04*SR);x[:edge]*=np.linspace(0,1,edge)[:,None];x[-edge:]*=np.linspace(1,0,edge)[:,None]
                if chunks:chunks.append(np.zeros((SR//2,2)))
                chunks.append(x)
            joined=np.concatenate(chunks);assert np.max(np.abs(joined))<.5
            raw=np.rint(joined*32767).astype('<i2').tobytes();wav=out/(name+'.wav')
            with wav.open('wb') as f:
                with wave.open(f,'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(SR);w.writeframes(raw)
                f.flush();os.fsync(f.fileno())
            assert len(read(wav))==55*SR
            mp3=out/(name+'.mp3');run(['ffmpeg','-nostdin','-v','error','-y','-i',wav,'-codec:a','libmp3lame','-b:a','160k',mp3])
            with mp3.open('rb') as f:os.fsync(f.fileno())
            decoded=subprocess.check_output(['ffmpeg','-nostdin','-v','error','-i',str(mp3),'-f','s16le','-'])
            assert len(decoded)==55*SR*4
            for p in (wav,mp3):output[p.name]=dict(bytes=p.stat().st_size,sha256=sha(p))
        after={str(p.relative_to(ROOT)):sha(p) for p in sorted(files)};assert before==after
        proof=dict(code_baseline='500bd362056540895f0f67cecccf26d995e509e0',
            nature=0,manual_velocity=.7,attack_release=.5,master=.75,seed=1234,
            manual_capture_seconds=18,manual_on_seconds=1,manual_off_seconds=11,
            matched_manual_cases=len(cases),full_partition_stereo_frames_compared=partitions,
            previews_seconds_each=55,preview_cues=['COAST 0:00','WOODLAND 0:18.5','HIGHLANDS 0:37'],
            per_world_fixed_gain_db=[db(g) for g in gains],
            cases=cases,pairs=pairs,generative=music,files=output,inputs_sha256=after,
            limitations='Descriptors compare a controlled common pitch/controls and actual grammar traces. Similarity is not perceptual identity, genre quality, calmness or hearing approval. Physical H743 output/IRQ/stack remains unmeasured.')
        (out/'WORLD_IDENTITY_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n')
        print(json.dumps({k:v for k,v in proof.items() if k not in ('cases','pairs','generative','inputs_sha256','files')}),flush=True)

if __name__=='__main__':main()
