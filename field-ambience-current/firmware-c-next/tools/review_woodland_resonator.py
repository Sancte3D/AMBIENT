#!/usr/bin/env python3
"""Check independent long WOODLAND modal replacement against exact PR185."""
import argparse
import array
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import wave
from review_cell_unfold import SOURCES, ROOT

BASE='b6b62dbf3f33230ea4c659a8f5976efbf4ff31d2'
def run(cmd,**kw):return subprocess.check_output([str(x) for x in cmd],text=True,**kw)
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    out=ap.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    cc=os.environ.get('CC','cc');flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT']
    def compile_at(root,source,binary,extra=(),sources=SOURCES):
        run([cc,*flags,*extra,'-I'+str(root/'include'),source,*[root/'src'/f'{n}.c' for n in sources],'-lm','-o',binary])
    with tempfile.TemporaryDirectory(prefix='woodland-resonator-') as directory:
        tmp=Path(directory);repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'base',filter='data')
        base=tmp/'base'/prefix;tests={}
        for name in ['test_woodland_resonator','test_world_character','test_world_character_stress','test_woodland_generator']:
            compile_at(ROOT,ROOT/'test'/f'{name}.c',tmp/name,
                       ['-DFAM_WOODLAND_RESONATOR_CANDIDATE','-DFAM_WOODLAND_BODY_CANDIDATE','-O1','-g',
                        '-fsanitize=address,undefined','-fno-omit-frame-pointer'],
                       sources=['dsp','shape'] if name=='test_woodland_resonator' else SOURCES)
            log=run([tmp/name],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')).strip()
            (out/(name+'.log')).write_text(log+'\n');tests[name]=log.splitlines()[-1]
        renderer=ROOT/'tools/render_world_character.c'
        compile_at(ROOT,renderer,tmp/'current',['-DHAS_WORLD_CHARACTER','-DHAS_WOODLAND_BODY','-DHAS_WOODLAND_RESONATOR'])
        compile_at(base,renderer,tmp/'previous',['-DHAS_WORLD_CHARACTER','-DHAS_WOODLAND_BODY'])
        for mode in ['off','on','body-on','resonator-off']:
            run([tmp/'previous',tmp/'previous.wav',mode]);run([tmp/'current',tmp/'off.wav',mode])
            assert (tmp/'previous.wav').read_bytes()==(tmp/'off.wav').read_bytes(),mode
        whole=tmp/'all-worlds.wav';run([tmp/'current',whole,'resonator-pair','512'])
        run([tmp/'current',tmp/'partition.wav','resonator-pair','37'])
        assert whole.read_bytes()==(tmp/'partition.wav').read_bytes()
        assert Path(str(whole)+'.events.csv').read_bytes()==Path(str(tmp/'partition.wav')+'.events.csv').read_bytes()
        events=list(csv.DictReader(Path(str(whole)+'.events.csv').open()))
        for world in range(3):
            def rows(variant):return [{k:v for k,v in r.items() if k!='variant'} for r in events if int(r['world'])==world and int(r['variant'])==variant]
            assert rows(0)==rows(1),world
        with wave.open(str(whole)) as w:
            assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(2,2,44100,144*44100)
            pcm=w.readframes(w.getnframes())
        size=24*44100*4
        for world in range(3):assert (pcm[world*2*size:(world*2+1)*size]==pcm[(world*2+1)*size:(world*2+2)*size])==(world!=1)
        pair=out/'AMBIENT_Woodland_Resonator_AB.wav';preview=out/'AMBIENT_Woodland_Resonator.wav'
        for path,data in [(pair,pcm[2*size:4*size]),(preview,pcm[3*size:4*size])]:
            with wave.open(str(path),'wb') as w:
                w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100);w.writeframes(data)
        samples=array.array('h',pcm[2*size:4*size]);peak=max(abs(x) for x in samples)/32768
        proof=dict(baseline=BASE,model=1,default_enabled=False,
                   baseline_off_pcm_exact_character_and_body_modes=True,baseline_compared_stereo_frames=4*144*44100,
                   candidate_partition_exact_37_512=True,actual_event_traces_equal=True,other_worlds_pcm_exact=True,
                   sanitizer_tests=tests,ab_seconds=48,preview_seconds=24,
                   order='24s PR185 Character+Body, 24s harmonic modal replacement, actual generator, no file gain',
                   sample_peak=peak,ab_sha256=digest(pair),preview_sha256=digest(preview),
                   hearing_accepted=False,device_accepted=False)
        inputs=[ROOT/'src'/f'{n}.c' for n in SOURCES]+list((ROOT/'include').glob('*.h'))
        inputs+=[ROOT/'test/test_woodland_resonator.c',Path(__file__),renderer]
        proof['inputs_sha256']={str(p.relative_to(ROOT)):digest(p) for p in sorted(inputs)}
        (out/'WOODLAND_RESONATOR_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n')
        (out/'WOODLAND_RESONATOR_EVENTS.csv').write_text(Path(str(whole)+'.events.csv').read_text())
        print(json.dumps({k:v for k,v in proof.items() if k!='inputs_sha256'}))
if __name__=='__main__':main()
