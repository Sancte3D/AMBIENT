#!/usr/bin/env python3
"""Quiet procedural Nature: actual Cells, weather, score and old-PCM evidence."""
import argparse
import array
import hashlib
import io
import json
import math
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import wave
from review_cell_unfold import SOURCES, ROOT

BASE='8381a1dc8ff85cac71d6b4941b94a6ef82d22e17'
SR=44100

def run(cmd,**kw):return subprocess.check_output([str(x) for x in cmd],text=True,**kw)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def data(path):
    with wave.open(str(path)) as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(2,2,SR)
        frames=w.getnframes();pcm=w.readframes(frames)
        assert len(pcm)==frames*4 and path.stat().st_size==44+len(pcm)
        return pcm
def values(pcm):
    a=array.array('h');a.frombytes(pcm)
    if __import__('sys').byteorder!='little':a.byteswap()
    return a
def rms(a):return math.sqrt(sum(float(x)*x for x in a)/len(a))/32768
def db(x):return None if x==0 else 20*math.log10(x)
def notes(path):return Path(str(path)+'.events.csv').read_bytes()

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--native-only',action='store_true');args=ap.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT']
    cc=os.environ.get('CC','cc');tests={};levels=[];mixed=[];solo=[]
    inputs=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']]
    inputs+=[ROOT/'CMakeLists.txt',ROOT/'test/test_nature_accents.c',ROOT/'test/test_nature_handoff.c',
             ROOT/'test/nature_handoff_observer.h',ROOT/'tools/render_nature_accents.c',Path(__file__),
             ROOT/'tools/review_nature_handoff.py',ROOT/'tools/check_product_link.py']
    before={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
    with tempfile.TemporaryDirectory(prefix='nature-accents-') as directory:
        tmp=Path(directory);repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'base',filter='data')
        base=tmp/'base'/prefix
        def compile_at(root,source,binary,finish=True,extra=(),sources=None):
            if sources is None:sources=SOURCES+['gesture']
            run([cc,*flags,*(['-DFAM_SOUND_FINISH_CANDIDATE'] if finish else []),*extra,
                 '-I'+str(root/'include'),source,*[root/'src'/f'{n}.c' for n in sources],'-lm','-o',binary])
        for sanitized in ([False] if args.native_only else [False,True]):
            extra=['-DFAM_NATURE_ACCENTS_TEST']
            if sanitized:extra+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compile_at(ROOT,ROOT/'test/test_nature_accents.c',tmp/'test',extra=extra,sources=['nature','dsp'])
            log=run([tmp/'test'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            name='weather_'+('sanitized' if sanitized else 'native');tests[name]=log.splitlines()[-1]
            (out/(name+'.log')).write_text(log);print(tests[name],flush=True)
        command=['python3',ROOT/'tools/review_nature_handoff.py',out/'handoff','--fixture-only','--finish-candidate']
        if not args.native_only:command+=['--sanitize']
        tests['handoff']=json.loads(run(command));print('Nature target/publication and full-float timelines pass',flush=True)
        driver=ROOT/'tools/render_nature_accents.c'
        for label,root,finish in [('current',ROOT,True),('previous',base,True),('off',ROOT,False),('previous_off',base,False)]:
            compile_at(root,driver,tmp/label,finish)
        captures=0;frames_verified=0
        def capture(binary,world,seed,amount,mode=1,cap=512,seconds=48,room=.5):
            nonlocal captures
            p=tmp/f'{binary}-w{world}-s{seed}-a{amount}-m{mode}-b{cap}-t{seconds}-r{room}.wav'
            run([tmp/binary,p,world,seed,amount,room,mode,cap,seconds]);captures+=1
            assert len(data(p))==seconds*SR*4
            return p
        def exact(a,b):
            nonlocal frames_verified
            first=data(a);assert first==data(b),(a.name,b.name)
            assert notes(a)==notes(b),(a.name,b.name,'notes');frames_verified+=len(first)//4
        for world in range(3):
            for seed in (1234,42):
                off=capture('current',world,seed,0);on=capture('current',world,seed,1)
                exact(off,capture('current',world,seed,0,cap=37))
                exact(on,capture('current',world,seed,1,cap=37))
                assert notes(off)==notes(on)
                (out/f'world{world}_seed{seed}_cells.events.csv').write_bytes(notes(on))
                music=values(data(off));weather=[b-a for a,b in zip(music,values(data(on)))]
                held=slice(10*SR*2,30*SR*2);ratio=rms(weather[held])/rms(music[held]);windows=[]
                for second in range(10,30):
                    sl=slice(second*SR*2,(second+1)*SR*2)
                    windows.append(rms(weather[sl])/rms(music[sl]))
                level=dict(world=world,seed=seed,held_music_rms_dbfs=db(rms(music[held])),
                           held_nature_difference_dbfs=db(rms(weather[held])),
                           nature_to_cells_db=db(ratio),loudest_1s_nature_to_cells_db=db(max(windows)),
                           peak=max(map(abs,values(data(on))))/32768)
                levels.append(level);print(json.dumps(level),flush=True)
                assert ratio<10**(-24/20) and max(windows)<10**(-20/20),level
                assert level['peak']<.75
                if seed==1234:
                    exact(off,capture('previous',world,seed,0))
                    for amount in (0,1):exact(capture('off',world,seed,amount),capture('previous_off',world,seed,amount))
                    audition=capture('current',world,seed,.65);alone=capture('current',world,seed,.65,mode=0)
                    exact(alone,capture('current',world,seed,.65,mode=0,cap=37,room=1))
                    assert notes(off)==notes(audition)
                    # The independent bus really adds the same Nature under
                    # Cells as alone, subject only to final 16-bit rounding.
                    # Nature0 opens the shared 40ms output gate at the first
                    # Cell; positive Nature already opened it at frame0.
                    difference=[b-a for a,b in zip(music,values(data(audition)))]
                    error=[abs(a-b) for a,b in zip(difference,values(data(alone)))]
                    gate_start=8*SR*2;gate_end=(8*SR+SR*40//1000+1)*2
                    assert max(error[:gate_start]+error[gate_end:])<=2
                    mixed.append(data(audition));solo.append(data(alone))
                    for amount in (0,1):
                        p=capture('current',world,seed,amount,mode=2,seconds=64)
                        exact(p,capture('current',world,seed,amount,mode=2,cap=37,seconds=64))
                        if amount==0:generative_notes=notes(p)
                        else:assert notes(p)==generative_notes
                    (out/f'world{world}_seed{seed}_generate.events.csv').write_bytes(generative_notes)
                    silent=capture('current',world,seed,0,mode=0)
                    assert not any(data(silent))
            print(f'World {world}: Cell/Generate traces, levels, old PCM, Room and partition checks pass',flush=True)
        for name,chunks in [('AMBIENT_Nature_mit_Cells.wav',mixed),('AMBIENT_Nature_solo.wav',solo)]:
            expected=b''.join(chunks)
            with (out/name).open('wb') as raw:
                with wave.open(raw,'wb') as w:
                    w.setnchannels(2);w.setsampwidth(2);w.setframerate(SR);w.writeframes(expected)
                raw.flush();os.fsync(raw.fileno())
            assert data(out/name)==expected
        decoded_frames={}
        for mode in ('mit_Cells','solo'):
            src=out/f'AMBIENT_Nature_{mode}.wav';dest=out/f'AMBIENT_Nature_{mode}.mp3'
            # Same fixed audition gain in both files; no individual event,
            # World or pause normalization, compression, added FX or end fade.
            run(['ffmpeg','-nostdin','-v','error','-y','-i',src,'-af','volume=4','-codec:a','libmp3lame','-b:a','160k',dest])
            with dest.open('rb') as f:os.fsync(f.fileno())
            decoded=subprocess.check_output(['ffmpeg','-nostdin','-v','error','-i',str(dest),'-f','s16le','-'])
            assert len(decoded)==144*SR*4,(mode,len(decoded))
            decoded_frames[mode]=len(decoded)//4
        after={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
        assert before==after,'Inputs changed during Nature verification'
        proof=dict(baseline=BASE,model=1,nature_default=0,preview_nature=.65,preview_seconds_each=144,
                   world_order=['COAST 0:00','WOODLAND 0:48','HIGHLANDS 1:36'],
                   physical_cells='Cell0 at8s, Cell2 at18s, release30s, velocity0.7; same in every48s capture',
                   wav_gain_db=0,mp3_shared_gain_db=20*math.log10(4),
                   captures=captures,exact_stereo_frames_compared=frames_verified,tests=tests,levels=levels,
                   complete_zero_nature_matches_previous=True,off_defaults_match_previous=True,
                   complete_note_traces_nature_independent=True,room_independent_nature=True,
                   additive_bus_excludes_first_cell_40ms_output_gate=True,
                   no_phantom_voices=True,all_output_protection_counters_zero=True,
                   mp3_fully_decoded_stereo_frames=decoded_frames,
                   hearing_accepted=False,device_accepted=False,inputs_sha256=after,
                   files={p.name:dict(bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(out.glob('AMBIENT_Nature_*.*'))})
        (out/'NATURE_ACCENTS_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n')
        print(json.dumps({k:v for k,v in proof.items() if k not in ('inputs_sha256','files','tests','levels')}),flush=True)

if __name__=='__main__':main()
