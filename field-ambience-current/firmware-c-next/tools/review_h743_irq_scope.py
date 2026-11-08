#!/usr/bin/env python3
"""SD48 preparation: actual H743 C handler, synthetic cycles, complete Product PCM."""
import argparse, hashlib, json, os, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='de732f42d4c1973a82f256d9cdec129394dba4b1'
SOURCES=['engine_product','world_grammar','bowed','horn','pluck','ambient_room','nature','dsp','shape','tuning','brain','cells']
CHANGED=['src/hal_h743/'+n for n in ['audio_h743.c','stm32h7xx_it.c','diag_h743.c','h743_hal.h']]
def run(cmd,**kw):return subprocess.check_output(cmd,text=True,**kw)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--sanitize',action='store_true');ap.add_argument('--fixture-only',action='store_true')
    a=ap.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);equal=None
    if not a.fixture_only:
        repo=Path(run(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
        paths=run(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo).splitlines()
        paths=[p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake') or Path(p).name=='CMakeLists.txt'];assert len(paths)==265
        for p in paths:
            if str(Path(p).relative_to(prefix)) not in CHANGED:
                assert (repo/p).read_bytes()==subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo),p
        equal=261
    s=(ROOT/'src/hal_h743/audio_h743.c').read_text()
    audio=s[s.index('static audio_render_fn'):s.index('/* Enable the Cortex-M7')]+s[s.index('const audio_profiler_t *audio_profiler_state'):s.index('/* ---- MspInit')]
    s=(ROOT/'src/hal_h743/stm32h7xx_it.c').read_text();irq=s[s.index('void DMA1_Stream0_IRQHandler'):s.index('/* ---- SPI1-TX')]
    s=(ROOT/'src/hal_h743/diag_h743.c').read_text();diag=s[s.index('static char *u2s'):];results=[]
    for sanitize in ([False,True] if a.sanitize else [False]):
        with tempfile.TemporaryDirectory(prefix='h743-irq-') as directory:
            tmp=Path(directory)
            for name,text in [('scope_audio.inc',audio),('scope_irq.inc',irq),('scope_diag.inc',diag)]: (tmp/name).write_text(text)
            flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-I'+str(ROOT/'include'),'-I'+str(tmp)]
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            cc=os.environ.get('CC','cc')
            run([cc,*flags,'-Daudio_profiler_scan_clips=real_scan_clips','-c',str(ROOT/'src/audio_profiler.c'),'-o',str(tmp/'profiler.o')])
            run([cc,*flags,str(ROOT/'test/test_h743_irq_scope.c'),str(tmp/'profiler.o'),*[str(ROOT/'src'/f'{n}.c') for n in SOURCES],'-lm','-o',str(tmp/'fixture')])
            results.append(json.loads(run([str(tmp/'fixture')],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))))
    assert all(x==results[0] for x in results)
    paths=[*CHANGED,'src/audio_profiler.c','test/test_h743_irq_scope.c','tools/review_h743_irq_scope.py',*['src/'+n+'.c' for n in SOURCES]]
    proof=dict(baseline=BASE,unchanged_existing_inputs=equal,changed_inputs=CHANGED,sanitized=a.sanitize,native=results[0],
        scope='Actual H743 reset/IRQ/refill/diagnostic functions with synthetic DWT/HAL/scan/cache costs;0/1/2 pending halves, counter wrap, real full Product PCM in3 Worlds at64/512 render caps.',
        limitations='No device timing/stack/dispatch-latency/DMA-error/output acceptance. C window excludes exception entry/exit and final profiler bookkeeping. Overruns describe scope budget, not measured hardware underruns. Diagnostic fields are individual observations, not a coherent multi-field snapshot.',
        sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths})
    (out/'SD48_IRQ_SCOPE_METRICS.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(status='passed',sanitized=a.sanitize,**results[0])))
if __name__=='__main__':main()
