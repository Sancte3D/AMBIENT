#!/usr/bin/env python3
"""SD41 isolated H743 flash bench: native commands, full ELF, vectors and memory."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = '70ddde32dfaeaffe3399a9ee5c1f83b65cde40e3'
BENCH = ROOT/'bench/scene_flash_h743'

def run(command, **kwargs):
    return subprocess.run(command, check=True, text=True, **kwargs)

def capture(command, **kwargs):
    return subprocess.check_output(command, text=True, **kwargs)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--sanitize-model', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    repo = Path(capture(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT).strip())
    prefix = ROOT.relative_to(repo)
    paths = capture(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')],
        cwd=repo).splitlines()
    compiled = [p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake')
                or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 263
    for p in compiled:
        assert (repo/p).read_bytes() == subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo), p
    with tempfile.TemporaryDirectory(prefix='scene-bench-native-') as directory:
        binary = Path(directory)/'test'
        flags = ['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',
                 '-DSCENES_FLASH_H743_HOST_MODEL','-I'+str(ROOT/'include'),
                 '-I'+str(ROOT/'src/hal_h743'),'-I'+str(BENCH)]
        sources = [BENCH/'test_bench.c',BENCH/'bench.c',
            ROOT/'src/hal_h743/scenes_flash_h743_async.c',ROOT/'src/hal_h743/scenes_flash_h743_fault.c',
            ROOT/'src/scenes_journal.c',ROOT/'src/scenes_journal_async.c',ROOT/'src/scenes_journal_recovery.c']
        command = [*flags,*map(str,sources),'-o',str(binary)]
        run(command); native = json.loads(run([str(binary)],capture_output=True).stdout)
        if args.sanitize_model:
            sanitized = command.copy(); sanitized[sanitized.index('-O2')] = '-O1'
            sanitized[1:1] = ['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            run(sanitized)
            run([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),stdout=subprocess.DEVNULL)
    firmware = output/'firmware'
    run(['cmake','-S',str(BENCH),'-B',str(firmware),'-DCMAKE_BUILD_TYPE=Release',
         '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],stdout=subprocess.DEVNULL)
    run(['cmake','--build',str(firmware),'-j2'],stdout=subprocess.DEVNULL)
    elf = firmware/'scene_flash_bench'
    data = elf.read_bytes()
    assert data[:6] == b'\x7fELF\x01\x01' and struct.unpack_from('<H',data,18)[0] == 40
    entry, phoff = struct.unpack_from('<II',data,24)
    phsize, phnum = struct.unpack_from('<HH',data,42)
    segments = []
    for i in range(phnum):
        kind,off,vaddr,paddr,filesz,memsz,flags,align = struct.unpack_from('<8I',data,phoff+i*phsize)
        if kind != 1: continue
        if filesz: assert 0x08000000 <= paddr and paddr+filesz <= 0x08100000
        if flags & 1: assert 0x08000000 <= vaddr and vaddr+memsz <= 0x08100000
        segments.append(dict(vaddr=vaddr,paddr=paddr,file_bytes=filesz,memory_bytes=memsz,flags=flags))
    nm = capture(['arm-none-eabi-nm','-n',str(elf)])
    symbols, types = {}, {}
    for line in nm.splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0],16); types[fields[2]] = fields[1]
    assert not re.search(r'^\s+U ',nm,re.MULTILINE)
    for name in ('FLASH_IRQHandler','BusFault_Handler','HAL_FLASH_EndOfOperationCallback',
                 'HAL_FLASH_OperationErrorCallback'):
        assert types[name] == 'T' and symbols[name] != symbols['Default_Handler']
    assert symbols['g_pfnVectors'] == 0x08000000
    assert 0x20000000 <= symbols['g_scene_bench'] < 0x20020000
    assert symbols['_estack']-symbols['_ebss'] >= 16384
    assert symbols['__bench_journal_start'] == 0x081c0000
    assert symbols['__bench_journal_end'] == 0x08200000
    assert b'.debug_info' in data # typed mailbox is inspectable in GDB
    image = (firmware/'scene_flash_bench.bin').read_bytes()
    assert len(image) == symbols['__bench_load_end']-0x08000000
    header = (ROOT/'src/hal_h743/vendor/CMSIS/Device/ST/STM32H7xx/Include/stm32h743xx.h').read_text()
    irq_index = 16 + int(re.search(r'FLASH_IRQn\s*=\s*(\d+)',header).group(1))
    vectors = {'reset':1,'busfault':5,'systick':15,'flash':irq_index}
    handlers = {'reset':'Reset_Handler','busfault':'BusFault_Handler',
                'systick':'SysTick_Handler','flash':'FLASH_IRQHandler'}
    targets = {}
    for name,index in vectors.items():
        target = struct.unpack_from('<I',image,index*4)[0]
        assert target == (symbols[handlers[name]] | 1)
        targets[name] = target
    assert entry == targets['reset']
    assert struct.unpack_from('<I',image,0)[0] == symbols['_estack'] == 0x20020000
    assert symbols['scenes_flash_h743_guarded_resume']-symbols['scenes_flash_h743_guarded_load'] == 2
    load = symbols['scenes_flash_h743_guarded_load']-0x08000000
    assert image[load:load+4].hex() == '00687047'
    disassembly = capture(['arm-none-eabi-objdump','-d',str(elf)])
    fault_entry = disassembly.split('<BusFault_Handler>:',1)[1].split('\n\n',1)[0]
    assert 'scenes_flash_h743_async_busfault_entry' in fault_entry and '\tbl\t' not in fault_entry
    commands = json.loads((firmware/'compile_commands.json').read_text())
    for c in commands:
        if Path(c['file']).name in ('main.c','interrupts.c','scenes_flash_h743_async.c','scenes_flash_h743_fault.c'):
            assert '-mgeneral-regs-only' in c['command']
    names = [p for p in BENCH.iterdir() if p.suffix in ('.c','.h','.S','.ld','.gdb') or p.name == 'CMakeLists.txt']
    names.append(Path(__file__).resolve())
    frames = {}
    for p in sorted(firmware.rglob('*.su')):
        if p.name not in ('main.c.su','bench.c.su','interrupts.c.su'): continue
        for line in p.read_text().splitlines():
            function,size,kind = line.rsplit('\t',2)
            assert kind == 'static'
            frames[p.name+':'+function.rsplit(':',1)[-1]] = int(size)
    assert frames and 'bench.c.su:scene_bench_tick' in frames
    manifest = dict(scope='Standalone H743 journal test image only; not Product Scene/UI/audio integration. '
        'Boot reads only; explicit one-shot SWD arming precedes pattern writes.',
        baseline_commit=BASE,existing_firmware_inputs_byte_equal=len(compiled),
        device_accepted=False,hardware_timing_measured=False,audio_load_measured=False,
        native=native,sanitized_model=args.sanitize_model,elf_machine='ELF32 little-endian ARM',
        executable_bank='Bank1 only',mailbox_memory='DTCM',load_segments=segments,
        vector_indices=vectors,vector_targets=targets,initial_stack=symbols['_estack'],bin_bytes=len(image),
        bin_sha256=hashlib.sha256(image).hexdigest(),dtcm_static_bytes=symbols['_ebss']-0x20000000,
        reserved_stack_bytes=16384,dtcm_spare_after_reserve=symbols['_estack']-symbols['_ebss']-16384,
        static_frames_bytes=frames,frames_scope='individual compiler frames, not measured stack high-water',
        source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(names)})
    proof = output/'SD41_H743_SCENE_BENCH_METRICS.json'
    proof.write_text(json.dumps(manifest,indent=2)+'\n')
    artifact = output/'artifact'; artifact.mkdir(exist_ok=True)
    for name in ('scene_flash_bench','scene_flash_bench.bin','scene_flash_bench.hex','scene_flash_bench.map'):
        shutil.copyfile(firmware/name,artifact/name)
    for name in ('commands.gdb','README.md'):
        shutil.copyfile(BENCH/name,artifact/name)
    shutil.copyfile(proof,artifact/proof.name)
    (artifact/'scene_flash_bench.nm').write_text(nm)
    (artifact/'scene_flash_bench.disassembly').write_text(disassembly)
    for p in sorted(firmware.rglob('*.su')):
        if p.name in ('main.c.su','bench.c.su','interrupts.c.su'): shutil.copyfile(p,artifact/p.name)
    print(json.dumps(dict(status='passed',native=native,bin_bytes=len(image),
        bin_sha256=manifest['bin_sha256'],proof_sha256=hashlib.sha256(proof.read_bytes()).hexdigest())))

if __name__ == '__main__': main()
