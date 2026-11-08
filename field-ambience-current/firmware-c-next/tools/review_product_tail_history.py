#!/usr/bin/env python3
"""SD12: actual source/Room history, complete rejected PCM and bounded state."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE='659cb04df4d9e145c6de1f7aea9ecd17fe2b8db6'
SOURCES=['world_grammar','bowed','horn','pluck','ambient_room','nature','dsp','shape','tuning','brain','cells']
def capture(cmd,**kw):return subprocess.check_output(cmd,text=True,**kw)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--sanitize',action='store_true');ap.add_argument('--fixture-only',action='store_true')
    a=ap.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    count=None
    if not a.fixture_only:
        repo=Path(capture(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        paths=capture(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo).splitlines()
        paths=[p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake') or Path(p).name=='CMakeLists.txt']
        assert len(paths)==263
        for p in paths:assert (repo/p).read_bytes()==subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo),p
        count=len(paths)
    with tempfile.TemporaryDirectory(prefix='product-tail-history-') as directory:
        binary=Path(directory)/'fixture'
        def fixture(sanitize):
            flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-I'+str(ROOT/'include')]
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            subprocess.run([*flags,str(ROOT/'test/test_product_tail_history.c'),*[str(ROOT/'src'/f'{s}.c') for s in SOURCES],'-lm','-o',str(binary)],check=True)
            return json.loads(capture([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))
        native=fixture(False)
        if a.sanitize:assert fixture(True)==native
    proof=dict(baseline_commit=BASE,baseline_checked=not a.fixture_only,existing_compiled_inputs_byte_equal=count,
        runtime_files_changed=None if count is None else 0,new_runtime_state_bytes=0,sanitized=a.sanitize,native=native,
        scope='Real Equal-to-Just rejection while held/releasing, all three current sources, release/Room endpoints,64/512, complete continuation controls and audio frame/epoch wrap; common Room tone and cancelled preparation. Separate synthetic ledger capacity/exact-zero deadline checks.',
        limitations='Acks/admission observed on a10-ms Main grid; last-block Room peak/quiet can shift acceptance within a block plus control tick. No arbitrary control-rate or universal psychoacoustic threshold, hardware or hearing approval.',
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'src/engine_product.c',*[ROOT/'src'/f'{s}.c' for s in SOURCES],ROOT/'test/test_product_tail_history.c',Path(__file__)]})
    target=out/'SD12_TAIL_HISTORY_METRICS.json';target.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(status='passed',cases=native['real_tail_cases'],frames=native['whole_pcm_stereo_frames_compared'],partition_delta=native['max_partition_accept_delta_frames'],proof_sha256=sha(target))))
if __name__=='__main__':main()
