#!/usr/bin/env python3
"""SD47 remaining Nature publications and final unchanged-runtime inventory."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BASE='b352dbf9799b3fa618eeb01a833ac2a776f1c64e'

def run(cmd,**kw):return subprocess.run(cmd,check=True,text=True,**kw)
def capture(cmd,**kw):return subprocess.check_output(cmd,text=True,**kw)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output',type=Path);ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--fixture-only',action='store_true')
    ap.add_argument('--finish-candidate',action='store_true')
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
    with tempfile.TemporaryDirectory(prefix='nature-handoff-') as directory:
        tmp=Path(directory)
        def fixture(sanitize):
            flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include')]
            if args.finish_candidate:flags+=['-DFAM_SOUND_FINISH_CANDIDATE']
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            obj=tmp/'observed.o';binary=tmp/'fixture'
            names=['init','clear','set_amount','set_world','set_seed','render']
            run([*flags,*['-Dnature_'+n+'=observed_nature_'+n for n in names],
                '-include',str(ROOT/'test/nature_handoff_observer.h'),'-c',str(ROOT/'src/nature.c'),'-o',str(obj)])
            run([*flags,str(ROOT/'test/test_nature_handoff.c'),str(obj),str(ROOT/'src/nature.c'),
                str(ROOT/'src/dsp.c'),'-lm','-o',str(binary)])
            return json.loads(capture([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))
        native=fixture(False)
        if args.sanitize:assert fixture(True)==native
    proof=dict(baseline_commit=BASE,baseline_checked=not args.fixture_only,
        existing_compiled_inputs_byte_equal=None if inputs is None else len(inputs),
        runtime_files_changed=None if inputs is None else 0,new_runtime_state_bytes=0,
        sanitized=args.sanitize,native=native,
        instrumentation='Real nature.c compiled twice; only the observed host object wraps its three atomic stores/loads and SVF coefficient calls. Real original object supplies the full-float control.',
        limitations='Directed Main-store boundary preemption and identical-command-frame partition checks; not every machine instruction, arbitrary control-rate invariance, hardware WCET, total stack or hearing acceptance.',
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'src/nature.c',ROOT/'test/nature_handoff_observer.h',ROOT/'test/test_nature_handoff.c',Path(__file__)]})
    target=out/'SD47_NATURE_HANDOFF_METRICS.json';target.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(status='passed',native=native,proof_sha256=sha(target))))

if __name__=='__main__':main()
