#!/usr/bin/env python3
"""SD41 prepared pending/confirmed Scene Save API and complete real PCM."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from check_prepared_scenes import compare_inputs

ROOT=Path(__file__).resolve().parents[1]
BASE='6c1316bea9dc5ced80dafcde29872ac4eb2b784e'
SOURCES=('engine_product world_grammar worlds_product bowed horn pluck ambient_room nature dsp '
         'shape tuning brain cells controls scenes menu params battery oled_draw oled_color '
         'baked_font baked_font_data font_8x8 scenes_journal scenes_journal_async '
         'scenes_journal_recovery scenes_save_async').split()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('--sanitize-model',action='store_true')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    repo=Path(subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=ROOT,text=True).strip())
    prefix=ROOT.relative_to(repo)
    tracked=subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,'--',
        str(prefix/'src'),str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo,text=True).splitlines()
    compiled=[p for p in tracked if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake') or Path(p).name=='CMakeLists.txt']
    assert len(compiled)==255
    guard=compare_inputs(repo,ROOT,BASE,compiled)
    assert guard['existing_inputs_byte_equal']==254 and guard['changed_inputs']==['src/scenes.c']
    with tempfile.TemporaryDirectory(prefix='scene-save-review-') as folder:
        tmp=Path(folder);rows={}
        flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
               '-DFAM_SCENE_SAVE_ASYNC_PREPARED','-I'+str(ROOT/'include')]
        for name in ('test_product_scene_save','test_product_scene','test_product_boot_recall'):
            binary=tmp/name
            subprocess.run([*flags,str(ROOT/'test'/f'{name}.c'),*[str(ROOT/'src'/f'{s}.c') for s in SOURCES],
                            '-lm','-o',str(binary)],check=True)
            result=subprocess.run([str(binary)],capture_output=True,text=True)
            if result.returncode:raise RuntimeError(result.stderr or f'{name} exited {result.returncode}')
            if name=='test_product_scene':rows[name]=dict(exit_code=0,stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest())
            else:rows[name]=[json.loads(line) for line in result.stdout.splitlines()]
        if args.sanitize_model:
            import os
            binary=tmp/'sanitized'
            subprocess.run([*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                str(ROOT/'test/test_product_scene_save.c'),*[str(ROOT/'src'/f'{s}.c') for s in SOURCES],
                '-lm','-o',str(binary)],check=True)
            env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
            subprocess.run([str(binary),'--model'],check=True,env=env,stdout=subprocess.DEVNULL)
    inputs=['include/scenes_save_async.h','src/scenes_save_async_internal.h','src/scenes_save_async.c','src/scenes.c',
            'test/test_product_scene_save.c','tools/review_scene_save.py','tools/check_prepared_scenes.py']
    manifest=dict(scope='Prepared Product-only Scene Save API. Begin snapshots genuine SCN7 without IO, '
        'then optional yielding recovery and writer with <=1 IO/32B read per Main step. '
        'RAM/active Scene publish only after complete verification; cancellation drains IO. '
        'FAILED may leave a recoverable physical commit. No UI/H743/CMake/default activation or device timing proof.',
        baseline_commit=BASE,default_scene_guard=guard,existing_runtime_inputs_byte_equal=250,
        previous_prepared_inputs_byte_equal=4,scn7_jsc1_unchanged=True,prepared_flag_default=False,
        journal_candidate_default='OFF',device_accepted=False,hearing_accepted=False,
        source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs},
        frontend=rows['test_product_scene_save'][0],actual_pcm=rows['test_product_scene_save'][1:],
        prepared_scene_legacy_regression=rows['test_product_scene'],
        prepared_boot_recall_regression=rows['test_product_boot_recall'][-1])
    path=out/'SD41_SCENE_SAVE_METRICS.json';path.write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(status='passed',frontend=manifest['frontend'],pcm_summary=manifest['actual_pcm'][-1],
                         proof_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),sanitized_model=args.sanitize_model)))

if __name__=='__main__':main()
