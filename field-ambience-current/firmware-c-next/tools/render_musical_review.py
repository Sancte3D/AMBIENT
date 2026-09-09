#!/usr/bin/env python3
"""Render the real C device path, with reproducible notes and native 16-bit PCM.

python3 tools/render_musical_review.py --output /tmp/ambient-audition
--engine /path/to/baseline.so reuses a library built from the comparison commit.
--group worlds|synths|effects|all selects the listening material.
Only the offline analysis needs numpy; the firmware gains no dependency.
"""
import argparse
import ctypes as C
import json
import math
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SR, BLOCK = 44100, 512


def build_engine(output):
    runner = (ROOT / 'test/run_tests.sh').read_text()
    at = runner.index('"$here/test_synth_device.c"')
    command = runner[runner.rfind('"$CC"', 0, at):runner.index('"$tmp/synth_device_test"', at)]
    command = command.replace('\\\n', ' ').replace('$src', str(ROOT)).replace('$here', str(ROOT/'test'))
    sources = [s for s in shlex.split(command) if s.endswith('.c') and '/test/' not in s]
    subprocess.run(['cc', '-std=c11', '-O2', '-shared', '-fPIC', '-I'+str(ROOT/'include'),
                    *sources, str(ROOT/'src/params.c'), '-lm', '-o', str(output)], check=True)
    return output


class Instrument:
    def __init__(self, library):
        self.lib = C.CDLL(str(Path(library).resolve()))
        self.backend = (C.c_void_p * 9)()
        for i, name in enumerate(['synth_host_select', 'synth_host_note_on', 'synth_host_note_off',
                                  'synth_host_panic', 'synth_host_render', 'synth_host_render_mix',
                                  'synth_host_set_param', 'synth_host_note_on_hz', 'synth_host_set_macro']):
            if hasattr(self.lib, name):
                self.backend[i] = C.cast(getattr(self.lib, name), C.c_void_p).value
        self.lib.engine_set_synth_backend.argtypes = [C.c_void_p]
        self.lib.engine_render.argtypes = [C.POINTER(C.c_int16), C.c_int]
        self.lib.engine_note_on.argtypes = [C.c_uint8, C.c_float, C.c_float]
        for name in ['master_volume','drive','brightness','space','atmosphere','motion','age',
                     'echo','blur','shimmer','bass_depth','attack','release','send']:
            getattr(self.lib, 'engine_set_'+name).argtypes = [C.c_float]
        if hasattr(self.lib, 'engine_set_synth_param'):
            self.lib.engine_set_synth_param.argtypes = [C.c_int, C.c_float]
        self.lib.dsp_init()
        self.lib.engine_init()
        self.lib.synth_host_init()
        self.lib.engine_set_synth_backend(self.backend)

    def call(self, name, *args):
        getattr(self.lib, 'engine_'+name)(*args)

    def note(self, source, midi, amp):
        self.call('note_on', source, 440*2**((midi-69)/12), amp)


def worlds():
    text = (ROOT/'src/worlds.c').read_text()
    result = []
    for block in re.findall(r'\{\s*\.name\s*=.*?\n\s*\}', text, re.S):
        row = {k: int(v) for k, v in re.findall(r'\.(\w+)\s*=\s*(-?\d+)', block)}
        row['name'] = re.search(r'\.name\s*=\s*"([^"]+)"', block)[1]
        result.append(row)
    assert len(result) == 5
    return result


def render(library, output, kind, index, wet=False):
    inst = Instrument(library)
    events = []
    def event(t, action, *args):
        events.append((round(t*SR/BLOCK)*BLOCK, action, args))
    def note(t, length, source, midi, amp):
        event(t, 'note', source, midi, amp)
        event(t+length, 'note_off', source)
    inst.call('set_master_volume', 0.5)
    inst.call('set_drive', 0.10)
    inst.call('set_bass_depth', 0.25)
    if kind == 'world':
        w = worlds()[index]
        inst.call('set_world', index)
        inst.call('set_voice', w['voice'])
        inst.call('set_brightness', w['brightness_hz'])
        for macro, field in [('space','space_pct'),('atmosphere','atmos_pct'),('motion','motion_pct'),
                              ('age','age_pct'),('echo','echo_pct'),('blur','blur_pct'),('shimmer','shimmer_pct')]:
            inst.call('set_'+macro, w[field]/100)
        root = w['key_midi']
        # A sparse, original pentatonic performance. Three phrases and a true
        # final rest; identical events/velocities for before and after.
        third = 4 if index < 2 else 3
        seventh = 9 if index < 2 else 10
        for t, degree, length in [(0.8,0,9),(13,7,8),(25,0,12)]:
            note(t,length,8,root+degree,0.085)
        for t, length, degree, amp in [(2,5.5,12,.10),(5.2,4,19,.16),(9,1.1,third+12,.08),
                                      (14,6.4,19,.17),(18,3,24,.12),(23,1.5,seventh+12,.09),
                                      (26,6,third+12,.14),(30,6,19,.18),(35,3,12,.11)]:
            source = 0 if t in [2,9,14,23,26,35] else 1
            note(t,length,source,root+degree,amp)
        duration = 50
    elif kind == 'synth':
        inst.call('set_synth', index+1)
        inst.call('set_atmosphere', .62 if wet else 0)
        inst.call('set_space', .64); inst.call('set_echo', .48)
        inst.call('set_age', .16); inst.call('set_blur', .12)
        inst.call('set_motion', .25); inst.call('set_shimmer', .08)
        inst.call('set_fx_mode', 8 if wet else 0)
        lengths=[1.2,2.0,3.7,1.5,2.8,.2]
        for t,midi,amp in [(1,57,.36),(5.5,64,.54),(10,60,.42),(14.5,57,.62)]:
            note(t,lengths[index],0,midi,amp)
        # Last two notes audition a real core-control change.
        if hasattr(inst.lib,'engine_set_synth_param'):
            event(9,'set_synth_param',2 if index==4 else 0,.52)
        duration = 23
    else:
        inst.call('set_voice', 1)
        inst.call('set_synth',6)  # short LPG excitation leaves audible tails
        inst.call('set_atmosphere', .85); inst.call('set_space', .8)
        inst.call('set_echo', .72); inst.call('set_motion', .62)
        inst.call('set_age', .5); inst.call('set_blur', .58); inst.call('set_shimmer', .6)
        inst.call('set_fx_mode',index)
        for t,midi in [(1,57),(2.3,64),(4,69),(5.2,72)]: note(t,.18,0,midi,.55)
        if index==5:  # Swell is scheduled look-ahead, never faked on live taps.
            inst.lib.fx_master_trigger_swell.argtypes=[C.c_float,C.c_float,C.c_float]
            event(2.5,'swell',440.0,.18,1.5)
        duration = 17
    events.sort(key=lambda e:e[0])
    path=output
    data=np.empty((int(duration*SR),2),dtype=np.int16)
    cursor=0
    for frame in range(0,len(data),BLOCK):
        while cursor<len(events) and events[cursor][0]<=frame:
            _,name,args=events[cursor]
            if name=='note': inst.note(*args)
            elif name=='swell': inst.lib.fx_master_trigger_swell(*args)
            else: inst.call(name,*args)
            cursor+=1
        n=min(BLOCK,len(data)-frame)
        inst.lib.engine_render(data[frame:].ctypes.data_as(C.POINTER(C.c_int16)),n)
    with wave.open(str(path),'wb') as f:
        f.setnchannels(2);f.setsampwidth(2);f.setframerate(SR);f.writeframes(data.tobytes())
    x=data.astype(np.float64)/32768
    active=x[SR: min(38*SR,len(x))]
    mono=active.mean(axis=1)
    spec=np.abs(np.fft.rfft(mono))**2
    freq=np.fft.rfftfreq(len(mono),1/SR)
    return {'file':path.name,'seconds':duration,'peak_dbfs':round(20*math.log10(max(np.max(np.abs(x)),1e-9)),2),
            'rms_dbfs':round(20*math.log10(max(np.sqrt(np.mean(active**2)),1e-9)),2),
            'clipped_samples':int(np.sum(np.abs(data.astype(np.int32))>=32767)),
            'dc':float(x.mean()),'energy_below_100hz_pct':round(float(spec[freq<100].sum()/max(spec.sum(),1e-12)*100),2),
            'events':[{'sample':t,'action':name,'args':args} for t,name,args in events]}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--engine',type=Path);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--group',choices=['all','worlds','synths','effects'],default='all')
    args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    # Fresh subprocess per clip also resets every file-static PRNG/state. Shared
    # library globals otherwise survive CDLL reloads in the same interpreter.
    with tempfile.TemporaryDirectory(prefix='ambient-review-') as tmp:
        library=args.engine or build_engine(Path(tmp)/'engine.so')
        jobs=[]
        if args.group in ['all','worlds']:
            for i,w in enumerate(worlds()): jobs.append(('world',i,False,f'World_{i+1}_{w["name"].replace(" ","_")}.wav'))
        if args.group in ['all','synths']:
            for i,name in enumerate(['Acid','FM_Glass','Mist','Storm','Orbit','Bamboo']):
                for wet in [False,True]: jobs.append(('synth',i,wet,f'Synth_{i+1}_{name}_{"Dream" if wet else "Dry"}.wav'))
        if args.group in ['all','effects']:
            for i,name in enumerate(['Bypass','Reverb','Delay','Chorus','Tape','Swell','Shimmer','Blur','Dream']):
                jobs.append(('effect',i,True,f'FX_{i}_{name}.wav'))
        import sys
        manifest=[]
        for kind,index,wet,name in jobs:
            code='import sys,json; sys.path.insert(0,sys.argv[1]); from render_musical_review import render; from pathlib import Path; print(json.dumps(render(sys.argv[2],Path(sys.argv[3]),sys.argv[4],int(sys.argv[5]),sys.argv[6]=="1")))'
            result=subprocess.run([sys.executable,'-c',code,str(ROOT/'tools'),str(library),str(args.output/name),kind,str(index),'1' if wet else '0'],capture_output=True,text=True,check=True)
            metrics=json.loads(result.stdout);manifest.append(metrics)
            print(f'{name}: peak {metrics["peak_dbfs"]} dBFS, clips {metrics["clipped_samples"]}',flush=True)
        (args.output/'render_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__': main()
