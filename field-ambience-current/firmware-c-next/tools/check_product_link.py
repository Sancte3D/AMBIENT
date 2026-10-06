#!/usr/bin/env python3
"""Audit actual ARM ELF and compilation inputs, not host sizeof estimates."""
import json
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
assert not [s[3] for s in symbols if s[3].startswith(prefixes)], 'Retired DSP symbol retained'
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
