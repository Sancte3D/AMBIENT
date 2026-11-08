#!/usr/bin/env python3
"""Opt-in World designs: exact previous PCM, actual musical traces and A/B."""
import argparse
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
BASE='49169462c1705a4238bd2194559689b07e22d8c5'

def run(cmd,**kw):return subprocess.check_output([str(x) for x in cmd],text=True,**kw)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);a=ap.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    cc=os.environ.get('CC','cc');flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT']
    def compile_at(root,source,binary,extra=()):
        run([cc,*flags,*extra,'-I'+str(root/'include'),source,*[root/'src'/f'{n}.c' for n in SOURCES],'-lm','-o',binary])
    with tempfile.TemporaryDirectory(prefix='world-character-') as directory:
        tmp=Path(directory);repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'base',filter='data')
        base=tmp/'base'/prefix
        compile_at(ROOT,ROOT/'test/test_world_character.c',tmp/'test',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
        test=run([tmp/'test'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')).strip()
        compile_at(ROOT,ROOT/'test/test_world_character_stress.c',tmp/'stress',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
        stress=run([tmp/'stress'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')).strip()
        renderer=ROOT/'tools/render_world_character.c'
        compile_at(ROOT,renderer,tmp/'current',['-DHAS_WORLD_CHARACTER'])
        compile_at(base,renderer,tmp/'baseline')
        run([tmp/'current',tmp/'off.wav','off']);run([tmp/'baseline',tmp/'base.wav','off'])
        assert (tmp/'off.wav').read_bytes()==(tmp/'base.wav').read_bytes()
        wav=out/'AMBIENT_Welten_Charakter_AB.wav';run([tmp/'current',wav,'pair','512'])
        run([tmp/'current',tmp/'partition.wav','pair','37'])
        assert wav.read_bytes()==(tmp/'partition.wav').read_bytes()
        events=list(csv.DictReader(Path(str(wav)+'.events.csv').open()))
        for world in range(3):
            def rows(variant):return [{k:v for k,v in r.items() if k!='variant'} for r in events if int(r['world'])==world and int(r['variant'])==variant]
            assert rows(0)==rows(1)
        with wave.open(str(wav)) as w:
            assert w.getnchannels()==2 and w.getframerate()==44100 and w.getnframes()==144*44100
            pcm=w.readframes(w.getnframes());size=24*44100*4
            for world in range(3):assert pcm[world*2*size:(world*2+1)*size]!=pcm[(world*2+1)*size:(world*2+2)*size]
        preview=out/'AMBIENT_Welten_Charakter_V1.wav'
        with wave.open(str(preview),'wb') as w:
            w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100)
            w.writeframes(b''.join(pcm[(world*2+1)*size:(world*2+2)*size] for world in range(3)))
        assert preview.stat().st_size==44+72*44100*4
        proof=dict(baseline=BASE,model=1,default_enabled=False,baseline_off_pcm_exact=True,neutral_stereo_frames=144*44100,
                   candidate_partition_exact_37_512=True,actual_event_traces_equal=True,sanitizer_test=test,sanitizer_endpoint_test=stress,
                   seconds=144,wav_sha256=hashlib.sha256(wav.read_bytes()).hexdigest(),preview_seconds=72,preview_sha256=hashlib.sha256(preview.read_bytes()).hexdigest(),hearing_accepted=False,device_accepted=False)
        (out/'WORLD_CHARACTER_METRICS.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof))
if __name__=='__main__':main()
