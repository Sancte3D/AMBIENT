#!/usr/bin/env python3
"""SD45: continuous real Product/Scene sessions and complete Main-wrap controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BASE='fe0551c3f0ca1bbf2fc7d1693a2a8d337c5e77c2'
SOURCES=['engine_product','world_grammar','worlds_product','bowed','horn','pluck','ambient_room','nature',
         'dsp','shape','tuning','brain','cells','controls','scenes','menu','params','battery',
         'oled_draw','oled_color','baked_font','baked_font_data','font_8x8']
def capture(cmd,**kw):return subprocess.check_output(cmd,text=True,**kw)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--sanitize',action='store_true');ap.add_argument('--fixture-only',action='store_true')
    a=ap.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);count=None
    if not a.fixture_only:
        repo=Path(capture(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        rows=capture(['git','ls-tree','-r',BASE,'--',str(prefix/'src'),str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo).splitlines()
        paths=[]
        for row in rows:
            info,p=row.split('\t',1)
            if Path(p).suffix.lower() not in ('.c','.h','.s','.ld','.cmake') and Path(p).name!='CMakeLists.txt':continue
            raw=(repo/p).read_bytes()
            assert hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()==info.split()[2],p
            paths.append(p)
        assert len(paths)==263;count=len(paths)
    with tempfile.TemporaryDirectory(prefix='product-long-recovery-') as directory:
        binary=Path(directory)/'fixture'
        def fixture(sanitize):
            flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-I'+str(ROOT/'include')]
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            subprocess.run([*flags,str(ROOT/'test/test_product_long_recovery.c'),*[str(ROOT/'src'/f'{s}.c') for s in SOURCES],'-lm','-o',str(binary)],check=True)
            return json.loads(capture([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))
        native=fixture(False)
        if a.sanitize:assert fixture(True)==native
    proof=dict(baseline_commit=BASE,baseline_checked=not a.fixture_only,existing_compiled_inputs_byte_equal=count,
        runtime_files_changed=None if count is None else 0,new_runtime_state_bytes=0,sanitized=a.sanitize,native=native,
        scope='Two seeds,64/512 render caps,800 seconds per uninterrupted engine session; repeated live Scenes/Worlds, failed/retried native Saves, corrupt SCN7 reload/recovery, Stop/restart/Mute and long manual WOODLAND across World selection. Whole PCM and hooks compare normal and wrapping Main clocks within each partition.',
        limitations='RAM Scene backend verifies the actual native synchronous Scene frontend, not H743 flash/ECC, power-cut timing or prepared async integration. No whole64/512 PCM-equality promise across known block-peak admission boundaries; only complete per-partition Main-wrap controls. Existing separate phrase-memory checks supply long uninterrupted musical-form coverage. No CPU, stack, physical-output or hearing approval.',
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in [*[ROOT/'src'/f'{s}.c' for s in SOURCES],ROOT/'test/test_product_long_recovery.c',Path(__file__)]})
    target=out/'SD45_LONG_RECOVERY_METRICS.json';target.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(status='passed',frames=native['whole_wrap_pcm_stereo_frames_compared'],captures=native['captures'],proof_sha256=sha(target))))
if __name__=='__main__':main()
