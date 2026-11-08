#!/usr/bin/env python3
"""27.5 s Alps source A/B: dry old/new, with pad and Dream old/new.

--before baseline.so --after candidate.so --output /tmp/horn-body
Real source/engine DSP, controlled D4 at .5 s, release at 4.5 s. World bed
G3 only in the room comparison. Diagnostic notes, not autonomous Generate.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import subprocess
import sys
import numpy as np
from render_musical_review import Instrument, worlds, SR, BLOCK
from package_musical_review import read, write, measure


def render(library,layer,path):
    inst=Instrument(library);w=worlds()[0]
    inst.call('set_world',0);inst.call('set_voice',0);inst.call('set_master_volume',.5)
    inst.call('set_brightness',w['brightness_hz'])
    for name,field in [('space','space_pct'),('motion','motion_pct'),('age','age_pct'),
                       ('echo','echo_pct'),('blur','blur_pct'),('shimmer','shimmer_pct')]:
        inst.call('set_'+name,w[field]/100)
    inst.call('set_atmosphere',0.) # isolate musical layers from nature texture
    inst.lib.fx_master_set_atmosphere.argtypes=[C.c_float]
    inst.lib.fx_master_set_atmosphere(w['atmos_pct']/100)
    inst.call('set_fx_mode',0 if layer=='dry' else 8)
    inst.lib.horn_note_on.argtypes=[C.c_int,C.c_float,C.c_float]
    if layer=='room':inst.note(8,55,.10)
    x=np.empty((8*SR,2),np.int16);on=off=False
    for frame in range(0,len(x),BLOCK):
        if frame>=SR//2 and not on:
            if layer=='room':inst.note(15,62,.07)
            inst.lib.horn_note_on(15,440*2**((62-69)/12),.38);on=True
        if frame>=round(4.5*SR) and not off:
            inst.call('note_off',15);inst.call('note_off',8);off=True
        n=min(BLOCK,len(x)-frame)
        inst.lib.engine_render(x[frame:].ctypes.data_as(C.POINTER(C.c_int16)),n)
    write(path,x.astype(np.float64)/32768)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before',type=Path);p.add_argument('--after',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--render',type=Path,help=argparse.SUPPRESS)
    p.add_argument('--layer',choices=['dry','room'],help=argparse.SUPPRESS)
    a=p.parse_args()
    if a.render:render(a.render,a.layer,a.output);return
    if not a.before or not a.after:p.error('--before and --after required')
    a.output.mkdir(parents=True,exist_ok=True)
    clips=[];levels=[];report={'baseline':'8f8bdb266bf6de241253b7de7304e1cce293d822','raw':{}}
    for layer in ['dry','room']:
        for tag,lib in [('before',a.before),('after',a.after)]:
            path=a.output/f'{layer}_{tag}.wav'
            subprocess.run([sys.executable,__file__,'--render',str(lib.resolve()),
                            '--layer',layer,'--output',str(path)],check=True)
            raw=read(path);x=raw[:round(6.5*SR)].copy()
            fade=round(.1*SR);x[:fade]*=np.linspace(0,1,fade)[:,None];x[-fade:]*=np.linspace(1,0,fade)[:,None]
            clip=a.output/f'{layer}_{tag}_excerpt.wav';write(clip,x)
            clips.append(x);levels.append(measure(clip))
            report['raw'][f'{layer}_{tag}']={'peak':float(np.max(np.abs(raw))),
                'rms_dbfs_2_4s':float(20*np.log10(np.sqrt(np.mean(raw[2*SR:4*SR]**2))+1e-12)),
                'clipped_samples':int(np.sum(np.abs(raw)>=32767/32768))}
    target=min([-25.] + [-12-l['true_peak_dbfs']+l['integrated_lufs'] for l in levels])
    gains=[target-l['integrated_lufs'] for l in levels];parts=[]
    for x,g in zip(clips,gains):
        if parts:parts.append(np.zeros((SR//2,2)))
        parts.append(x*10**(g/20))
    combined=np.concatenate(parts);output=a.output/'AMBIENT_Alps_Body_AB_27s5.wav'
    write(output,combined)
    report['comparison']={'seconds':len(combined)/SR,'starts_s':[0,7,14,21],
        'order':['dry old','dry new','pad+Dream old','pad+Dream new'],
        'fixed_gains_db':gains,'target_lufs_per_excerpt':target,**measure(output)}
    (a.output/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
