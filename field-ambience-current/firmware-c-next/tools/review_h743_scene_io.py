#!/usr/bin/env python3
"""Prepared H743 Scene IO: exact native fault model and real ARM/HAL partial link."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = '45a71dc9615271e596aa984a4c89cb8403e91686'
NEW = ['include/scenes_flash_h743_async.h', 'src/hal_h743/scenes_flash_h743_async.c',
       'src/hal_h743/scenes_flash_h743_fault.h', 'src/hal_h743/scenes_flash_h743_fault.c',
       'src/hal_h743/scenes_flash_h743_guard.S', 'test/test_scenes_flash_h743_async.c',
       'tools/review_h743_scene_io.py']

def run(command, **kwargs):
    return subprocess.run(command, check=True, text=True, **kwargs)

def capture(command):
    return subprocess.check_output(command, text=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--sanitize-model', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    repo = Path(capture(['git', 'rev-parse', '--show-toplevel']).strip())
    prefix = ROOT.relative_to(repo)
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')],
        cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in
        ('.c', '.h', '.s', '.ld', '.cmake') or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 258
    changed = [p for p in compiled if (repo/p).read_bytes() !=
        subprocess.check_output(['git', 'show', f'{BASE}:{p}'], cwd=repo)]
    assert not changed, f'existing firmware input changed: {changed}'

    with tempfile.TemporaryDirectory(prefix='h743-scene-io-review-') as folder:
        model = Path(folder)/'model'
        native = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
            '-DSCENES_FLASH_H743_HOST_MODEL', '-I'+str(ROOT/'include'),
            '-I'+str(ROOT/'src/hal_h743'), str(ROOT/'test/test_scenes_flash_h743_async.c'),
            *[str(ROOT/p) for p in ['src/hal_h743/scenes_flash_h743_async.c',
                'src/hal_h743/scenes_flash_h743_fault.c', 'src/scenes_journal.c',
                'src/scenes_journal_async.c', 'src/scenes_journal_recovery.c']], '-o', str(model)]
        run(native)
        host = json.loads(run([str(model)], capture_output=True).stdout)
        if args.sanitize_model:
            san = native.copy(); san[san.index('-O2')] = '-O1'
            san[1:1] = ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            run(san)
            run([str(model)], env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'),
                stdout=subprocess.DEVNULL)

    vendor = ROOT/'src/hal_h743/vendor'
    flags = ['arm-none-eabi-gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-ffreestanding', '-mcpu=cortex-m7', '-mthumb', '-mfpu=fpv5-d16', '-mfloat-abi=hard',
        '-DSTM32H743xx', '-DUSE_HAL_DRIVER', '-fstack-usage',
        '-DFAM_SOUND_PRODUCT', '-DFAM_SCENE_SAVE_ASYNC_PREPARED', '-I'+str(ROOT/'include'),
        '-I'+str(ROOT/'src/hal_h743'), '-I'+str(vendor/'CMSIS/Include'),
        '-I'+str(vendor/'CMSIS/Device/ST/STM32H7xx/Include'),
        '-I'+str(vendor/'STM32H7xx_HAL_Driver/Inc')]
    sources = ['src/hal_h743/scenes_flash_h743_async.c', 'src/hal_h743/scenes_flash_h743_fault.c',
        'src/hal_h743/scenes_flash_h743_guard.S', 'src/scenes.c', 'src/scenes_save_async.c',
        'src/scenes_journal.c', 'src/scenes_journal_async.c', 'src/scenes_journal_recovery.c',
        'src/hal_h743/vendor/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c',
        'src/hal_h743/vendor/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c']
    objects = []
    for source in sources:
        obj = output/(Path(source).stem+'.o')
        extra = ['-w'] if '/vendor/' in source else []
        if source in ('src/hal_h743/scenes_flash_h743_async.c', 'src/hal_h743/scenes_flash_h743_fault.c'):
            extra.append('-mgeneral-regs-only')
        run([*flags, *extra, '-c', str(ROOT/source), '-o', str(obj)]); objects.append(str(obj))
    arm = output/'prepared_h743_scene_io.o'
    run(['arm-none-eabi-ld', '-r', *objects, '-o', str(arm)])
    nm = capture(['arm-none-eabi-nm', '-S', str(arm)])
    disassembly = capture(['arm-none-eabi-objdump', '-d', str(arm)])
    (output/'prepared_h743_scene_io.nm').write_text(nm)
    (output/'prepared_h743_scene_io.disassembly').write_text(disassembly)
    undefined = capture(['arm-none-eabi-nm', '-u', str(arm)])
    assert not re.search(r' U (?:scenes_save_|scenes_flash_h743_|HAL_FLASH|pFlash)', undefined)
    guard_object = output/'scenes_flash_h743_guard.o'
    guard_nm = capture(['arm-none-eabi-nm', '-n', str(guard_object)])
    symbols = {line.split()[-1]: int(line.split()[0], 16) for line in guard_nm.splitlines()
               if len(line.split()) == 3 and line.split()[1] != 'U'}
    assert symbols['scenes_flash_h743_guarded_load'] == 0
    assert symbols['scenes_flash_h743_guarded_resume'] == 2
    raw = output/'guarded_read.bin'
    run(['arm-none-eabi-objcopy', '-O', 'binary', '-j', '.text.scenes_flash_h743_guarded_read',
         str(guard_object), str(raw)])
    assert raw.read_bytes() == bytes.fromhex('00687047') # ldr r0,[r0]; bx lr
    entry = capture(['arm-none-eabi-objdump', '-dr', str(guard_object)])
    assert 'mrs' in entry and 'scenes_flash_h743_busfault_dispatch' in entry
    frames = {}
    for source in ('scenes_flash_h743_async', 'scenes_flash_h743_fault'):
        for line in (output/(source+'.su')).read_text().splitlines():
            function, size, kind = line.rsplit('\t', 2)
            assert kind == 'static'
            frames[function.rsplit(':', 1)[-1]] = int(size)
    manifest = dict(scope='Prepared standalone H743 Bank-2 IO; no CMake/main/vector/UI activation. '
        'Actual HAL interrupt erase/program, quiescent terminal poll, <=32B guarded read. '
        'No real-silicon ECC/latency/power-cut proof.', baseline_commit=BASE,
        existing_firmware_inputs_byte_equal=len(compiled), changed_existing_firmware_inputs=changed,
        default_connected=False, ui_connected=False, device_accepted=False,
        busfault_scope='active fixed load + precise-only CFSR + exact BFAR/PC + Thread basic/FP frame + matching Bank2 double-ECC word',
        host_model=host, sanitized_model=args.sanitize_model, component_sources=sources,
        guard_instructions_hex=raw.read_bytes().hex(), arm_partial_link_bytes=arm.stat().st_size,
        arm_static_frames_bytes=frames, stack_scope='individual compiler frames; excludes transitive/IRQ/device use',
        source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in NEW})
    proof = output/'SD41_H743_SCENE_IO_METRICS.json'
    proof.write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(status='passed', host=host, arm_bytes=arm.stat().st_size,
                         proof_sha256=hashlib.sha256(proof.read_bytes()).hexdigest())))

if __name__ == '__main__':
    main()
