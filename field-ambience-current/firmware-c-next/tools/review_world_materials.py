#!/usr/bin/env python3
"""Material model 2: actual matched Cells, old-audio provenance and auditions.

Descriptors document differences; no arbitrary score is a hearing approval.
Requires numpy/ffmpeg, like review_world_identity. All full captures, actual
event/state traces and sample partitions are compared before editorial cuts.
"""
import argparse
import csv
import hashlib
import io
import itertools
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import wave
import numpy as np
from review_world_identity import ROOT,SOURCES,SR,NAMES,read,rms,db,describe,similarity

BASE='df9a8a903d444d48133b980760a9686202455637'
def run(args,**kw):return subprocess.check_output([str(x) for x in args],text=True,**kw)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save_wav(path,x):
    assert np.max(np.abs(x))<.5
    with path.open('wb') as raw:
        with wave.open(raw,'wb') as w:
            w.setnchannels(2);w.setsampwidth(2);w.setframerate(SR)
            w.writeframes(np.rint(x*32767).astype('<i2').tobytes())
        raw.flush();os.fsync(raw.fileno())
    assert len(read(path))==len(x)
def encode(path):
    dest=path.with_suffix('.mp3')
    run(['ffmpeg','-nostdin','-v','error','-y','-i',path,'-codec:a','libmp3lame','-b:a','160k',dest])
    with dest.open('rb') as f:os.fsync(f.fileno())
    decoded=subprocess.check_output(['ffmpeg','-nostdin','-v','error','-i',str(dest),'-f','s16le','-'])
    frames=len(read(path));assert len(decoded)==frames*4
    return {p.name:dict(bytes=p.stat().st_size,sha256=sha(p),frames=frames) for p in (path,dest)}
def joined(pcm,gains,fx):
    chunks=[]
    for world in range(3):
        x=pcm[(world,50,fx,.5)].copy()*gains[world]
        edge=int(.04*SR)
        x[:edge]*=np.linspace(0,1,edge)[:,None];x[-edge:]*=np.linspace(1,0,edge)[:,None]
        if chunks:chunks.append(np.zeros((SR//2,2)))
        chunks.append(x)
    return np.concatenate(chunks)
def gains_for(pcm):
    return [min(10**(-26/20)/rms(pcm[(w,50,0,.5)][3*SR:8*SR]),
                .49/max(np.max(np.abs(pcm[(w,50,fx,.5)])) for fx in (0,1))) for w in range(3)]
def summary(c):
    # Keep the measured harmonic shape and coarse held-time trajectory;
    # full event/state traces are retained separately, not hidden in a plot.
    return {k:v for k,v in c.items() if k!='hold_rms_envelope'}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);a=ap.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    inputs=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']]
    inputs+=[ROOT/'tools'/n for n in ['render_world_audit.c','render_sound_finish.c','review_world_identity.py','review_cell_unfold.py']]
    inputs+=[Path(__file__)]
    before={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-DFAM_SOUND_FINISH_CANDIDATE']
    cases=[];pcm={};frames=0;old_pcm={};files={}
    with tempfile.TemporaryDirectory(prefix='world-materials-') as directory:
        tmp=Path(directory);repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'base',filter='data')
        base=tmp/'base'/prefix
        def compile_at(root,name,binary):
            run([os.environ.get('CC','cc'),*flags,'-I'+str(root/'include'),root/'tools'/name,
                 *[root/'src'/f'{n}.c' for n in SOURCES+['gesture']],'-lm','-o',binary])
        compile_at(ROOT,'render_world_audit.c',tmp/'current')
        compile_at(base,'render_world_audit.c',tmp/'previous')
        old=json.loads(run(['git','show',BASE+':'+str(prefix.parent/'docs/audio/WORLD_IDENTITY_METRICS.json')],cwd=repo))
        assert all(sha(base/p)==s for p,s in old['inputs_sha256'].items())
        def capture(binary,world,midi,fx,color,cap):
            p=tmp/f'{binary.name}-w{world}-n{midi}-fx{fx}-c{color}-b{cap}.wav'
            run([binary,p,world,midi,fx,color,cap,1234,0]);x=read(p)
            assert len(x)==18*SR and np.max(np.abs(x))<.75
            e=Path(str(p)+'.events.csv').read_bytes();s=Path(str(p)+'.states.csv').read_bytes()
            rows=list(csv.DictReader(e.decode().splitlines()));on=[v for v in rows if int(v['on'])>0]
            off=[v for v in rows if int(v['on'])==0]
            assert len(on)==len(off)==1 and abs(float(on[0]['hz'])-440*2**((midi-69)/12))<.01
            if cap==512 and binary.name=='current':
                (out/(p.name+'.events.csv')).write_bytes(e);(out/(p.name+'.states.csv')).write_bytes(s)
            return x,e,s
        for world in range(3):
            for midi,color in [(m,c) for m in (50,57,62) for c in (0,.5,1)]+[(69,.5)]:
                for fx in (0,1):
                    x,e,s=capture(tmp/'current',world,midi,fx,color,512)
                    y,ey,sy=capture(tmp/'current',world,midi,fx,color,37)
                    assert x.tobytes()==y.tobytes() and e==ey and s==sy
                    frames+=len(x)
                    if midi==50 and color==.5:pcm[(world,midi,fx,color)]=x
                    d=describe(x,midi);envelope=d['hold_rms_envelope']
                    d['held_envelope_start_end_dbfs']=[db(envelope[0]),db(envelope[-1])]
                    cases.append(summary(dict(world=world,name=NAMES[world],midi=midi,fx=fx,color=color,**d)))
            for fx in (0,1):old_pcm[(world,50,fx,.5)]=capture(tmp/'previous',world,50,fx,.5,512)[0]
            print(f'{NAMES[world]}: 20 actual matched pitch/Color/dry-FX cases, whole PCM/events/states exact 37/512',flush=True)
        # Check every old proof input, and record reconstruction of both old
        # whole WAV hashes. Hash equality is observed, not required across
        # different compiler/libm versions on another host.
        old_gains=gains_for(old_pcm)
        old_wav_hashes={}
        for fx,name in [(0,'AMBIENT_Worlds_Trocken.wav'),(1,'AMBIENT_Worlds_Mit_FX.wav')]:
            p=tmp/name;save_wav(p,joined(old_pcm,old_gains,fx));old_wav_hashes[name]=sha(p)
        old_hashes_match=all(s==old['files'][n]['sha256'] for n,s in old_wav_hashes.items())
        pairs=[]
        for midi,color in [(m,c) for m in (50,57,62) for c in (0,.5,1)]+[(69,.5)]:
            for fx in (0,1):
                selected=[next(c for c in cases if c['world']==w and c['midi']==midi and c['fx']==fx and c['color']==color) for w in range(3)]
                for i,j in itertools.combinations(range(3),2):
                    a,b=selected[i],selected[j]
                    p=np.sqrt(a['harmonic_energy_fraction']);q=np.sqrt(b['harmonic_energy_fraction'])
                    pairs.append(dict(a=NAMES[i],b=NAMES[j],midi=midi,fx=fx,color=color,
                        harmonic_amplitude_cosine=similarity(p,q),overtone_amplitude_cosine=similarity(p[1:],q[1:]),
                        hold_decay_difference_db=abs(a['late_hold_change_db']-b['late_hold_change_db'])))
        gains=gains_for(pcm)
        p=out/'AMBIENT_Worlds_V2_Trocken.wav';save_wav(p,joined(pcm,gains,0));files.update(encode(p))
        # Render the genuine default music twice too; do not substitute a
        # hand-written MIDI score or hide a changed source behind Nature.
        compile_at(ROOT,'render_sound_finish.c',tmp/'music')
        one=tmp/'music.wav';two=tmp/'music37.wav'
        run([tmp/'music',one,512]);run([tmp/'music',two,37])
        assert one.read_bytes()==two.read_bytes()
        assert Path(str(one)+'.events.csv').read_bytes()==Path(str(two)+'.events.csv').read_bytes()
        music=read(one);assert len(music)==96*SR;frames+=len(music)
        gain=min(2,.49/float(np.max(np.abs(music))));music*=gain
        edge=int(.04*SR)
        for w in range(3):
            start=w*32*SR;end=(w+1)*32*SR
            music[start:start+edge]*=np.linspace(0,1,edge)[:,None]
            music[end-edge:end]*=np.linspace(1,0,edge)[:,None]
        p=out/'AMBIENT_Worlds_V2_Musik.wav';save_wav(p,music);files.update(encode(p))
        (out/'WORLD_MATERIALS_V2_MUSIC_EVENTS.csv').write_bytes(Path(str(one)+'.events.csv').read_bytes())
        after={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)};assert before==after
        proof=dict(material_model=2,parent=BASE,manual_cases=len(cases),whole_stereo_frames_compared=frames,
            actual_hz_owners_events_states_verified=True,old_whole_audition_wav_hashes_reproduced=old_hashes_match,
            previous_audition_wav_sha256=old_wav_hashes,
            nature=0,manual_velocity=.7,attack_release=.5,master=.75,seed=1234,
            dry_seconds=55,dry_cues=['COAST 0:00','WOODLAND 0:18.5','HIGHLANDS 0:37'],
            music_seconds=96,music_cues=['COAST 0:00','WOODLAND 0:32','HIGHLANDS 1:04'],
            fixed_dry_gain_db=[db(g) for g in gains],whole_music_gain_db=db(gain),editorial_edge_ms=40,
            cases=cases,pairs=pairs,previous_pairs=old['pairs'],files=files,inputs_sha256=after,
            hearing_accepted=False,device_accepted=False,
            limitations='Harmonic descriptors are not perceptual identity or ambient quality scores. Listening approval and physical H743 IRQ/stack/DAC/headphones/speakers remain open. 18s manual and 32s musical passages are editorial excerpts, not complete natural tails; full comparisons precede cut fades.')
        # One compact measured case per line keeps a large numeric proof
        # inspectable and avoids thousands of padding-only JSON lines.
        lines=[]
        for k,v in proof.items():
            if k in ('cases','pairs','previous_pairs'):
                lines.append(json.dumps(k)+': [\n'+',\n'.join(json.dumps(x,separators=(',',':')) for x in v)+'\n]')
            else:lines.append(json.dumps(k)+': '+json.dumps(v,indent=2))
        (out/'WORLD_MATERIALS_V2_METRICS.json').write_text('{\n'+',\n'.join(lines)+'\n}\n')
        print(json.dumps({k:v for k,v in proof.items() if k not in ('cases','pairs','previous_pairs','files','inputs_sha256')}),flush=True)
if __name__=='__main__':main()
