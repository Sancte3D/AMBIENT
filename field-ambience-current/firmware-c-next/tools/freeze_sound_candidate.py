#!/usr/bin/env python3
"""Fingerprint the current own-source candidate and compile its FX registry.

This freezes software inputs only. It cannot freeze listening choices or
certify device acceptance. Vendor/toolchain and actual ELF remain separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
FIELDS='delay grain drive crush phaser delay_left_ms delay_right_ms feedback grain_ms phaser_hz'.split()
def manifest():
    paths=[ROOT/'CMakeLists.txt']
    for folder in ('include','src','cmake'):
        paths += [p for p in (ROOT/folder).rglob('*') if p.is_file() and p.suffix.lower() in ('.c','.h','.s','.ld','.cmake')]
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(set(paths))}
    with tempfile.TemporaryDirectory(prefix='candidate-registry-') as folder:
        tmp=Path(folder);source=tmp/'registry.c'
        assign=';'.join('p.'+f+'=v' for f in FIELDS)+';'
        args=','.join('(double)p.'+f for f in FIELDS)
        source.write_text('#include "sound_parameters.h"\n#include <float.h>\n#include <stdio.h>\n'
            'static void row(sound_parameters_t p){printf("['+','.join(['%.9g']*10)+']",'+args+');}\n'
            'static sound_parameters_t bounded(float v){sound_parameters_t p=sound_parameters_default(),q;'+assign+
            'if(!sound_parameters_normalize(&p,&q))return p;return q;}\n'
            'int main(void){sound_parameters_t p=sound_parameters_default();printf("{\\"default\\":");row(p);'
            'printf(",\\"minimum\\":");row(bounded(-FLT_MAX));printf(",\\"maximum\\":");row(bounded(FLT_MAX));'
            'printf(",\\"freeze\\":%s,\\"plate\\":%s}",p.freeze?"true":"false",p.plate?"true":"false");return 0;}')
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include'),str(source),'-lm','-o',str(tmp/'registry')],check=True)
        values=json.loads(subprocess.check_output([str(tmp/'registry')],text=True))
    return dict(software_candidate='0.11',world_material_model=2,fx_model=1,base_commit='ddb3cef2bb9af29027531f80140f3705a10b75c1',
        configuration=dict(FAM_TARGET='h743',FAM_SOUND_PROFILE='product',FAM_SOUND_FINISH_CANDIDATE='ON',
            FAM_SOUND_SESSION_CANDIDATE='ON',FAM_DEVICE_BENCH_CANDIDATE='OFF',FAM_SCENE_JOURNAL_CANDIDATE='OFF',
            CMAKE_BUILD_TYPE='Release',FAM_LCD_PANEL='1_9inch'),
        default_profile='reference',new_flags_default='OFF',encoder_mapping_deferred=True,
        effect_targets={f:{k:values[k][i] for k in ('minimum','default','maximum')} for i,f in enumerate(FIELDS)},
        effect_flags=dict(freeze=values['freeze'],plate=values['plate']),effect_wire_bytes=52,
        scene_store_bytes=5848,scene_slot_bytes=1168,gesture_events_per_slot=128,journal_payload_bytes=8192,
        inputs=hashes,scope='Own C/header/startup/linker/CMake source files, including reference paths. Vendor, compiler, tests and tool scripts are separate evidence.',
        final_sound_frozen=False,hearing_fully_accepted=False,device_accepted=False)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);ap.add_argument('--check',action='store_true')
    args=ap.parse_args();data=manifest()
    if args.check:
        if json.loads(args.output.read_text())!=data:raise SystemExit('Candidate sources/registry differ; regenerate and review the software snapshot')
    else:args.output.write_text(json.dumps(data,indent=2)+'\n')
    print(json.dumps(dict(status='matched' if args.check else 'written',own_source_inputs=len(data['inputs']),effect_targets=len(data['effect_targets']),final_sound_frozen=False)))
if __name__=='__main__':main()
