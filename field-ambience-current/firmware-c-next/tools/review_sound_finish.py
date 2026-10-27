#!/usr/bin/env python3
"""Complete sound candidate: real Product PCM, ownership, tail and FX evidence."""
import argparse
import array
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import wave
from review_cell_unfold import SOURCES,ROOT
BASE='c974584ab24dcb51696d85888ce4125deda20d6f'
def run(cmd,**kw):return subprocess.check_output([str(x) for x in cmd],text=True,**kw)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--native-only',action='store_true');a=ap.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT']
    cc=os.environ.get('CC','cc');tests={}
    def compile_at(root,source,exe,finish=False,sanitize=False,sources=None):
        extra=['-DFAM_SOUND_FINISH_CANDIDATE'] if finish else []
        if sanitize:extra+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if sources is None:sources=SOURCES+(['gesture'] if finish else [])
        run([cc,*flags,*extra,'-I'+str(root/'include'),source,*[root/'src'/f'{n}.c' for n in sources],'-lm','-o',exe])
    guard=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']]
    guard+=[ROOT/'test'/f'{n}.c' for n in ['test_sound_finish_fx','test_sound_finish','test_world_character_stress','test_sound_finish_music']]
    guard+=[Path(__file__),ROOT/'tools/render_sound_finish.c',ROOT/'src/hal_h743/main_h743.c']
    initial_hashes={str(p.relative_to(ROOT)):sha(p) for p in sorted(guard)}
    with tempfile.TemporaryDirectory(prefix='sound-finish-') as directory:
        tmp=Path(directory);repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'base',filter='data')
        base=tmp/'base'/prefix
        for sanitized in ([False] if a.native_only else [False,True]):
            for name in ['test_sound_finish_fx','test_sound_finish','test_world_character_stress','test_sound_finish_music','test_woodland_resonator','test_parameter_foundation']:
                compile_at(ROOT,ROOT/'test'/f'{name}.c',tmp/'test',True,sanitized,
                           sources=['dsp'] if name=='test_sound_finish_fx' else ['dsp','shape'] if name=='test_woodland_resonator' else None)
                log=run([tmp/'test'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')).strip()
                label=name+('_sanitized' if sanitized else '_native');tests[label]=log.splitlines()[-1]
                (out/(label+'.log')).write_text(log+'\n')
                print(label,tests[label],flush=True)
        driver=ROOT/'tools/render_sound_finish.c'
        compile_at(base,driver,tmp/'previous');compile_at(ROOT,driver,tmp/'off')
        compile_at(ROOT,driver,tmp/'candidate',True)
        run([tmp/'previous',tmp/'base.wav','512']);run([tmp/'off',tmp/'off.wav','512'])
        assert (tmp/'base.wav').read_bytes()==(tmp/'off.wav').read_bytes()
        run([tmp/'previous',tmp/'neutral-base.wav','512','neutral']);run([tmp/'off',tmp/'neutral-off.wav','512','neutral'])
        assert (tmp/'neutral-base.wav').read_bytes()==(tmp/'neutral-off.wav').read_bytes()
        run([tmp/'candidate',tmp/'on.wav','512']);run([tmp/'candidate',tmp/'partition.wav','37'])
        assert (tmp/'on.wav').read_bytes()==(tmp/'partition.wav').read_bytes()
        assert Path(str(tmp/'on.wav')+'.events.csv').read_bytes()==Path(str(tmp/'partition.wav')+'.events.csv').read_bytes()
        def pcm(path):
            with wave.open(str(path)) as w:
                assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(2,2,44100,96*44100)
                return w.readframes(w.getnframes())
        before=pcm(tmp/'base.wav');after=pcm(tmp/'on.wav');size=32*44100*4
        assert all(before[w*size:(w+1)*size]!=after[w*size:(w+1)*size] for w in range(3))
        preview=out/'AMBIENT_Sound_Complete_Candidate.wav';pair=out/'AMBIENT_Sound_Complete_AB.wav'
        for path,data in [(preview,after),(pair,b''.join(before[w*size:(w+1)*size]+after[w*size:(w+1)*size] for w in range(3)))]:
            with wave.open(str(path),'wb') as w:
                w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100);w.writeframes(data)
        samples=array.array('h',after);peak=max(map(abs,samples))/32768
        proof=dict(model=1,world_material_model=2,baseline=BASE,default_profile_unchanged_pcm=True,compared_stereo_frames=2*96*44100,
                   candidate_pcm_exact_37_512=True,actual_event_traces_partition_exact=True,
                   tests=tests,preview_seconds=96,ab_seconds=192,
                   order='COAST, WOODLAND, HIGHLANDS; each 32s before then 32s candidate in AB',
                   preview_sha256=sha(preview),ab_sha256=sha(pair),sample_peak=peak,
                   hearing_accepted=False,device_accepted=False)
        inputs=[ROOT/'src'/f'{n}.c' for n in SOURCES+['gesture']]+list((ROOT/'include').glob('*.h'))
        inputs+=[ROOT/'test'/f'{n}.c' for n in ['test_sound_finish_fx','test_sound_finish','test_world_character_stress','test_sound_finish_music']]
        inputs+=[Path(__file__),driver,ROOT/'src/hal_h743/main_h743.c']
        proof['inputs_sha256']={str(p.relative_to(ROOT)):sha(p) for p in sorted(inputs)}
        assert proof['inputs_sha256']==initial_hashes, 'Inputs changed while proof was running'
        (out/'SOUND_FINISH_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n')
        (out/'SOUND_FINISH_EVENTS.csv').write_text(Path(str(tmp/'on.wav')+'.events.csv').read_text())
        print(json.dumps({k:v for k,v in proof.items() if k!='inputs_sha256'}),flush=True)
if __name__=='__main__':main()
