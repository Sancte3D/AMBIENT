#!/usr/bin/env python3
"""Prepare a read-only GDB capture package; never certify device acceptance."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import tempfile
import subprocess

FIELDS = ('cpu_hz sample_rate_hz budget_cycles begin_cycles in_flight last_cycles '
          'max_cycles last_frames render_count deadline_miss_count reentry_count '
          'clip_count last_load peak_load').split()

def sha(data):
    return hashlib.sha256(data).hexdigest()

def prepare(build, nm='arm-none-eabi-nm', objcopy='arm-none-eabi-objcopy'):
    build = Path(build)
    cache = (build/'CMakeCache.txt').read_text()
    for key, value in [('FAM_TARGET','h743'),('FAM_SOUND_PROFILE','product'),
                       ('FAM_SCENE_JOURNAL_CANDIDATE','OFF')]:
        matches = re.findall(r'^'+key+r':[^=\n]+=([^\n]+)$', cache, re.M)
        if matches != [value]:
            raise ValueError('Unsupported build configuration: '+key)
    elf = (build/'field_ambience_h743').read_bytes()
    image = (build/'field_ambience_h743.bin').read_bytes()
    audit = json.loads((build/'PRODUCT_BUILD_AUDIT.json').read_text())
    if audit.get('profile') != 'product' or audit.get('elf_sha256') != sha(elf):
        raise ValueError('Product link audit does not match ELF')
    session_values=re.findall(r'^FAM_SOUND_SESSION_CANDIDATE:[^=\n]+=([^\n]+)$',cache,re.M)
    if session_values not in ([],['OFF'],['ON']):
        raise ValueError('Unsupported sound session configuration')
    session=session_values==['ON']
    if session and (audit.get('sound_session') or {}).get('format')!='SCN8/JSS1':
        raise ValueError('Sound session link audit missing')
    if not 0 < len(image) <= 1024*1024:
        raise ValueError('Image does not fit Bank1')
    with tempfile.TemporaryDirectory(prefix='ambient-image-') as directory:
        converted = Path(directory)/'image.bin'
        subprocess.run([objcopy,'-O','binary',str(build/'field_ambience_h743'),
                        str(converted)],check=True,capture_output=True)
        if converted.read_bytes() != image:
            raise ValueError('BIN does not match ELF')
    listing = subprocess.check_output([nm,'-S','--defined-only',
        str(build/'field_ambience_h743')], text=True)
    symbols = {}
    for name in ('s_prof','s_irq_prof'):
        found = re.findall(r'^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+[bBdD]\s+'
                           +name+r'$', listing, re.M)
        if len(found) != 1:
            raise ValueError('Missing/ambiguous profiler: '+name)
        address, size = (int(v,16) for v in found[0])
        if size != 56 or address % 4 or not 0x24000000 <= address <= 0x24080000-size:
            raise ValueError('Unsupported profiler size/location: '+name)
        symbols[name] = {'address':address,'bytes':size}
    if abs(symbols['s_prof']['address'] - symbols['s_irq_prof']['address']) < 56:
        raise ValueError('Aliased profiler symbols')
    manifest = dict(schema=1,profile='product',journal_candidate=False,
        sound_session_candidate=session,
        elf_sha256=sha(elf),image_sha256=sha(image),image_bytes=len(image),
        flash_address=0x08000000,fields=FIELDS,symbols=symbols)
    (build/'DEVICE_CAPTURE_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
    for name in ('device_capture.py','device_capture_gdb.py','DEVICE_CAPTURE_README.md'):
        shutil.copyfile(Path(__file__).with_name(name), build/name)
    return manifest

def decode(raw):
    if len(raw) != 56:
        raise ValueError('Truncated profiler data')
    p = dict(zip(FIELDS,struct.unpack('<12I2f',raw)))
    if p['in_flight'] or not p['render_count']:
        raise ValueError('Profiler unfinished or has no completed samples')
    if (p['cpu_hz'],p['sample_rate_hz'],p['last_frames'],p['budget_cycles']) != (
            480000000,44100,512,512*480000000//44100):
        raise ValueError('Profiler configuration differs from expected H743 setup')
    if p['max_cycles'] < p['last_cycles'] or any(
            not math.isfinite(p[n]) or p[n] < 0 for n in ('last_load','peak_load')):
        raise ValueError('Invalid profiler values')
    return p

def collect(manifest, read_memory, registers, label):
    if (manifest.get('schema'),manifest.get('profile'),manifest.get('journal_candidate')) != (1,'product',False):
        raise ValueError('Unsupported capture manifest')
    if manifest.get('fields') != FIELDS:
        raise ValueError('Unsupported profiler layout')
    if registers['xpsr'] & 0x1ff:
        raise ValueError('Stop in Thread mode, outside all interrupt handlers')
    n = manifest['image_bytes']
    if not 0 < n <= 1024*1024 or manifest['flash_address'] != 0x08000000:
        raise ValueError('Unsupported flash range')
    image = bytes(read_memory(0x08000000,n))
    if len(image) != n or sha(image) != manifest['image_sha256']:
        raise ValueError('Target flash does not match the prepared image')
    addresses = [manifest['symbols'][name]['address'] for name in ('s_prof','s_irq_prof')]
    if abs(addresses[0] - addresses[1]) < 56:
        raise ValueError('Overlapping profiler data')
    profiles = {}
    for name in ('s_prof','s_irq_prof'):
        symbol = manifest['symbols'][name];address = symbol['address']
        if symbol['bytes'] != 56 or address % 4 or not 0x24000000 <= address <= 0x24080000-56:
            raise ValueError('Unsupported profiler memory range')
        profiles[name] = decode(bytes(read_memory(address,56)))
    control = bytes(read_memory(0xe0001000,4))
    if len(control) != 4 or not int.from_bytes(control,'little') & 1:
        raise ValueError('DWT cycle counter is disabled')
    return dict(schema=1,label=label,device_accepted=False,
        target_image_sha256=sha(image),elf_sha256=manifest['elf_sha256'],
        registers=registers,profiles=profiles,
        scope_overrun_observed=any(p['deadline_miss_count'] for p in profiles.values()),
        scope_peak_at_or_above_60_percent=any(p['peak_load'] >= .60 for p in profiles.values()),
        stack_high_water_bytes=None,physical_clock_verified=False,
        limitations='End-of-session halted CPU snapshot. DMA may continue during halt. '
        'No exception/dispatch/stack/clock/output acceptance; no session-duration or prior-debugger-halt proof.')

if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('build',type=Path);ap.add_argument('--nm',default='arm-none-eabi-nm')
    ap.add_argument('--objcopy',default='arm-none-eabi-objcopy')
    args=ap.parse_args();print(json.dumps(prepare(args.build,args.nm,args.objcopy),indent=2))
