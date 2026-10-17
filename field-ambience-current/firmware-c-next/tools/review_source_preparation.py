#!/usr/bin/env python3
"""Interrupt actual source preparation, without modifying any firmware input."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE='7af108cb02c80425c959b58dc8b58ad5299c2465'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(c,**kw):return subprocess.run(c,check=True,text=True,**kw)
def instrument(source,family):
    def replace(old,new):
        nonlocal source
        assert source.count(old)==1,(family,old,source.count(old));source=source.replace(old,new)
    if family!='pluck':
        # Only the Product no-steal path prepares this pending record.
        begin=source.index('static void prepare_note(');end=source.index('\nstatic void start_note(',begin)
        body=source[begin:end]
        anchors=['    memset(v,0,sizeof *v);','    v->freq = freq_hz;',
            '    v->amp  = dsp_clampf(amp, 0.0f, 1.0f);','    v->inc  = freq_hz / SR;',
            '    v->env = 0.0001f;','    v->hold_left =',
            '    v->panL =','    v->stage = V_ATTACK;']
        lines=body.splitlines();new=[]
        for line in lines:
            new.append(line)
            for point,anchor in enumerate(anchors):
                if line.startswith(anchor):new.append(f'    source_preparation_probe({point});')
        source=source[:begin]+'\n'.join(new)+'\n'+source[end:]
        replace('        queued[i] = 1;','        source_preparation_probe(8);\n        queued[i] = 1;\n        source_preparation_probe(9);')
    else:
        anchors=['    bool ambient=atomic_load(&s_ambient);','    p->owner = owner;',
            '    p->stop_left = 0;','    p->dc = 0.0f;',
            '    p->color_a = 1.0f - expf(-6.283185307f * cutoff / SR);',
            '    int n = (int)p->N + 1;','    mean /= (float)n;','    p->widx = n;']
        for point,line in enumerate(anchors):replace(line,line+f'\n    source_preparation_probe({point});')
        replace('        p->buf[k] = x;','        p->buf[k] = x;\n        source_preparation_probe(32u+(unsigned)k);')
        replace('        p->buf[k] -= mean;','        p->buf[k] -= mean;\n        source_preparation_probe(2048u+(unsigned)k);')
        replace('    for (int k = 0; k < n; ++k) p->buf[k] *= gain;',
            '    for (int k = 0; k < n; ++k) { p->buf[k] *= gain; source_preparation_probe(4096u+(unsigned)k); }')
        replace('    p->active = 1;','    source_preparation_probe(8);\n    p->active = 1;\n    source_preparation_probe(9);')
    return 'extern void source_preparation_probe(unsigned);\n'+source
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--fixture-only',action='store_true',help='Native host runner: fixtures only, no historical Git/input audit');a=ap.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if not a.fixture_only:
        repo=Path(subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=ROOT,text=True).strip());prefix=ROOT.relative_to(repo)
        paths=subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo,text=True).splitlines()
        inputs=[p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake') or Path(p).name=='CMakeLists.txt']
        assert len(inputs)==263
        assert all((repo/p).read_bytes()==subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo) for p in inputs)
    metrics=[];traces=[]
    with tempfile.TemporaryDirectory(prefix='source-preparation-') as d:
        tmp=Path(d)
        for number,family in enumerate(['bowed','horn','pluck']):
            original=ROOT/'src'/f'{family}.c';observed=tmp/f'{family}.c';observed.write_text(instrument(original.read_text(),family))
            flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',f'-DFAMILY={number}','-I'+str(ROOT/'include')]
            def compile_at(path,source,extra=()):
                run([*flags,*extra,'-DSOURCE_IMPLEMENTATION="'+str(source)+'"',str(ROOT/'test/test_source_preparation.c'),str(ROOT/'src/dsp.c'),str(ROOT/'src/shape.c'),'-lm','-o',str(path)])
            binary=tmp/'test';compile_at(binary,observed)
            result=json.loads(subprocess.check_output([str(binary)],text=True));metrics.append(result)
            trace=tmp/'observed.pcm';run([str(binary),'--trace',str(trace)])
            raw=tmp/'raw';compile_at(raw,original);reference=tmp/'reference.pcm';run([str(raw),'--trace',str(reference)])
            assert reference.read_bytes()==trace.read_bytes();traces.append({'family':family,'float_bytes':trace.stat().st_size,'sha256':sha(trace)})
            if a.sanitize:
                san=tmp/'san';compile_at(san,observed,['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
                assert json.loads(subprocess.check_output([str(san)],text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))==result
    proof={'baseline_commit':BASE,'baseline_checked':not a.fixture_only,'firmware_inputs_byte_equal':None if a.fixture_only else 263,'runtime_files_changed':None if a.fixture_only else 0,'new_runtime_state_bytes':0,'sanitized':a.sanitize,'families':metrics,'uninstrumented_vs_observed_traces':traces,'instrumentation':'Temporary source copies only; calls after actual statements and every Pluck excitation pass iteration. Main pauses for complete audio halves; no Main inside audio.','limitations':'Source-level directed preemption, not every machine instruction or hardware WCET/stack measurement.','source_sha256':{str(p.relative_to(ROOT)):sha(p) for p in [*[ROOT/'src'/f'{f}.c' for f in ['bowed','horn','pluck']],ROOT/'test/test_source_preparation.c',Path(__file__)]}}
    target=out/'SD47_SOURCE_PREPARATION_METRICS.json';target.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'status':'passed','families':metrics,'proof_sha256':sha(target)}))
if __name__=='__main__':main()
