#!/usr/bin/env python3
"""Audit actual ARM ELF and compilation inputs, not host sizeof estimates."""
import json
import hashlib
from pathlib import Path
import re
import subprocess
import sys

build = Path(sys.argv[1])
elf = build / 'field_ambience_h743'
compiled = json.loads((build / 'compile_commands.json').read_text())
forbidden = {
    'engine.c','pad.c','padsynth.c','bass.c','drone.c','body.c','choir.c','guembri.c','ember.c',
    'texture.c','ambience.c','reverb.c','reverb_presets.c','fx_master.c','ambient_effects.c',
    'tape.c','echo.c','blur.c','shimmer.c','generative.c','composer.c','harmony.c','voices.c','dsp_ladder.c'
}
sources = [Path(c['file']) for c in compiled]
bad = [str(s) for s in sources if s.name in forbidden or '/v2/' in str(s)]
assert not bad, f'Archived/retired sources compiled: {bad}'
required = {'engine_product.c','ambient_room.c','nature.c','world_grammar.c','worlds_product.c'}
assert required.issubset({s.name for s in sources})
nm = subprocess.check_output(['arm-none-eabi-nm','-S','--defined-only',str(elf)], text=True)
symbols = []
for line in nm.splitlines():
    m=re.match(r'([0-9a-f]+)\s+([0-9a-f]+)\s+(\S)\s+(.+)$',line)
    if m: symbols.append((int(m[1],16),int(m[2],16),m[3],m[4]))
prefixes = ('pad_','padsynth_','bass_','drone_','body_','choir_','guembri_','ember_','texture_',
            'ambience_','reverb_','fx_master_','synth_host_','engine_v2_','tape_','echo_','blur_','shimmer_')
bad_symbols=[s[3] for s in symbols if s[2].lower()=='t' and s[3].startswith(prefixes)]
assert not bad_symbols, f'Retired DSP function retained: {bad_symbols}'
def one(name):
    found=[s for s in symbols if s[3]==name]
    assert len(found)==1, (name,found)
    return found[0]
for name in ('tank','diffusion'):
    address,size,_,_=one(name)
    assert 0x30000000<=address and address+size<=0x30048000,(name,hex(address),size)
    print(f'PRODUCT LINK {name}: 0x{address:08x}, {size} B, internal D2')
address,size,_,_=one('s_buffer')
assert 0x24000000<=address and address+size<=0x24080000 and address%32==0,(hex(address),size)
print(f'PRODUCT LINK audio DMA: 0x{address:08x}, {size} B, aligned internal D1')
assert all('FAM_SOUND_PRODUCT' in c['command'] for c in compiled if Path(c['file']).name in required)
print('PRODUCT LINK PASS: reference DSP/archive excluded, one internal room, DMA placement verified')

# Compiler-reported individual frames. This is deliberately not a call-chain,
# ISR nesting or runtime high-water claim (those are SD48 on real hardware).
frames=[]
for path in build.rglob('*.su'):
    for line in path.read_text().splitlines():
        fields=line.split('\t')
        if len(fields)!=3:continue
        match=re.match(r'(.+):(\d+):(\d+):(.+)',fields[0])
        if not match:continue
        source=Path(match[1]);function=match[4]
        if '/vendor/' in str(source):continue
        frames.append({'source':source.name,'function':function,
                       'bytes':int(fields[1]),'kind':fields[2]})
assert frames,'-fstack-usage evidence missing'
core={'engine_product.c','world_grammar.c','bowed.c','horn.c','pluck.c','nature.c','ambient_room.c'}
core_frames=[r for r in frames if r['source'] in core]
assert core_frames and max(r['bytes'] for r in core_frames)<=1024,core_frames
assert all('unbounded' not in r['kind'] for r in core_frames),core_frames
for function in ('engine_render','ambient_room_process','nature_render','engine_generative_tick','pluck_note_on','scenes_save'):
    rows=[r for r in frames if r['function']==function or r['function'].startswith(function+'.')]
    assert rows,function
    print(f"PRODUCT FRAME {function}: {max(r['bytes'] for r in rows)} B (compiler, not stack high-water)")
report={'elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest(),
        'compiler':subprocess.check_output(['arm-none-eabi-gcc','--version'],text=True).splitlines()[0],
        'profile':'product','individual_frames':sorted(frames,key=lambda r:r['bytes'],reverse=True),
        'core_max_frame_bytes':max(r['bytes'] for r in core_frames),
        'limitations':'Individual compiler frames only. Library callees, call chains, exception context, ISR nesting and actual stack high-water remain SD48.'}
(build/'PRODUCT_BUILD_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n')
print(f"PRODUCT FRAME PASS: {len(core_frames)} core frames, maximum {report['core_max_frame_bytes']} B")
