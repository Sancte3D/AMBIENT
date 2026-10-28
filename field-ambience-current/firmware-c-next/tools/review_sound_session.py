#!/usr/bin/env python3
"""Actual effect controls and complete SCN8/JSS1/H743 storage ownership proof."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('engine_product world_grammar worlds_product bowed horn pluck ambient_room nature dsp '
         'shape tuning brain cells controls menu params battery oled_draw oled_color '
         'baked_font baked_font_data font_8x8 gesture scenes_journal scenes_journal_async '
         'scenes_journal_recovery').split()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output',type=Path);ap.add_argument('--sanitize',action='store_true')
    args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    inputs=list((ROOT/'include').glob('*.h'))+[ROOT/'src'/f'{s}.c' for s in SOURCES]
    inputs+=[ROOT/p for p in ('src/scenes.c','src/scenes_save_async.c',
        'src/hal_h743/scenes_storage_h743.c','src/hal_h743/scenes_flash_h743_async.c',
        'test/test_effect_parameters.c','test/test_sound_session.c','tools/review_sound_session.py')]
    def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)}
    before=hashes();rows={}
    with tempfile.TemporaryDirectory(prefix='sound-session-') as folder:
        tmp=Path(folder)
        flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
               '-DFAM_SOUND_FINISH_CANDIDATE','-I'+str(ROOT/'include')]
        for sanitized in ([False,True] if args.sanitize else [False]):
            extra=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitized else []
            for name in ('test_effect_parameters','test_sound_session'):
                binary=tmp/name
                if name=='test_effect_parameters':defs=[];sources=[ROOT/'src/dsp.c']
                else:
                    defs=['-DFAM_SOUND_SESSION_CANDIDATE','-DFAM_SCENE_SAVE_ASYNC_PREPARED','-DSCENES_FLASH_H743_HOST_MODEL']
                    sources=[ROOT/'src'/f'{s}.c' for s in SOURCES]+[ROOT/'src/hal_h743/scenes_flash_h743_async.c']
                subprocess.run([*flags,*extra,*defs,str(ROOT/'test'/f'{name}.c'),*map(str,sources),'-lm','-o',str(binary)],check=True)
                label=name+('_sanitized' if sanitized else '_native')
                result=subprocess.run([str(binary)],text=True,capture_output=True,
                    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
                (out/(label+'.log')).write_text(result.stdout+result.stderr)
                if result.returncode:raise RuntimeError(label+': '+result.stderr)
                rows[label]=result.stdout.strip().splitlines()
                print(label,*rows[label],sep='\n',flush=True)
    assert before==hashes(),'Source changed during proof'
    manifest=dict(status='passed',effect_parameter_codec='FXP1, 52 bytes, CRC32, 10 float32 targets plus flags',
        session_format='SCN8, five 1168-byte slots, 5848-byte transaction',journal='JSS1, 8192-byte payload, two 128KiB sectors',
        tests=rows,inputs_sha256=before,encoder_mapping_changed=False,candidate_default=False,
        device_accepted=False,hearing_of_new_parameter_ranges_accepted=False,
        limitations='Host NOR/ECC/IRQ model checks real owners and algorithms. Actual H743 flash/audio timing, power interruption, cache/ECC and stack high-water need device bench acceptance.')
    path=out/'SOUND_SESSION_METRICS.json';path.write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(status='passed',manifest_sha256=hashlib.sha256(path.read_bytes()).hexdigest())),flush=True)

if __name__=='__main__':main()
