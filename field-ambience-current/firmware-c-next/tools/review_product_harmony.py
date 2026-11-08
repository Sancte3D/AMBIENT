#!/usr/bin/env python3
"""SD11: actual public Product admission, independent Hz and complete rejects."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BASE='e7c16650c10f466cb602ab2f3b24643a83f014a9'
SOURCES=['engine_product','world_grammar','bowed','horn','pluck','ambient_room',
         'nature','dsp','shape','tuning','brain','cells']
def capture(cmd,**kw):return subprocess.check_output(cmd,text=True,**kw)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output',type=Path);ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--fixture-only',action='store_true')
    args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    inputs=None
    if not args.fixture_only:
        repo=Path(capture(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip())
        prefix=ROOT.relative_to(repo)
        paths=capture(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),
            str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo).splitlines()
        inputs=[p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake')
                or Path(p).name=='CMakeLists.txt']
        assert len(inputs)==263
        for p in inputs:
            assert (repo/p).read_bytes()==subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo),p
    with tempfile.TemporaryDirectory(prefix='product-harmony-') as directory:
        tmp=Path(directory)
        def fixture(sanitize):
            flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror',
                   '-DFAM_SOUND_PRODUCT','-I'+str(ROOT/'include')]
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            binary=tmp/'fixture'
            subprocess.run([*flags,str(ROOT/'test/test_product_harmony.c'),
                *[str(ROOT/'src'/f'{s}.c') for s in SOURCES],'-lm','-o',str(binary)],check=True)
            return json.loads(capture([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))
        native=fixture(False)
        if args.sanitize:assert fixture(True)==native
    proof=dict(baseline_commit=BASE,baseline_checked=not args.fixture_only,
        existing_compiled_inputs_byte_equal=None if inputs is None else len(inputs),
        runtime_files_changed=None if inputs is None else 0,new_runtime_state_bytes=0,
        sanitized=args.sanitize,native=native,
        scope='Current public Manual/World APIs only, 140..470 Hz and MIDI50..69. Internal phrase registers are covered by existing real all-key generator tests, not opened through these public APIs.',
        oracle='Independent double-precision Equal/Just reference, fixed Core bitsets and declarative forbidden cent bands; no call to production private admission helpers. Full 2048-frame real PCM continuation after rejected pairs matches its untouched first-note control.',
        limitations='Applied Hz, admission, real DSP acks and complete rejected PCM, not a universal measured source-F0/partial/roughness or hearing acceptance. Source selection, partial/register audibility and actual output remain hearing/device gates.',
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in [*[ROOT/'src'/f'{s}.c' for s in SOURCES],ROOT/'test/test_product_harmony.c',Path(__file__)]})
    target=out/'SD11_PUBLIC_HARMONY_METRICS.json';target.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(status='passed',native=native,proof_sha256=sha(target))))
if __name__=='__main__':main()
