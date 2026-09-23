#!/usr/bin/env python3
"""Open Sea insert balance: real voice/pad/FX, 27.5 s level-matched A/B.

--before baseline.so --after candidate.so --output /tmp/calm-motion
Controlled D3 bed + D4 held Bowed, not an autonomous composition. Natural
layers are muted to isolate the musical bus; the World FX send is preserved.
Four 14 s ablations per version, fresh processes; no exported file >30 s.
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

LAYERS = ['dream', 'no_chorus', 'no_blur', 'room']


def render(library, layer, path):
    inst=Instrument(library); w=worlds()[1]
    inst.call('set_world',1); inst.call('set_voice',0)
    inst.call('set_master_volume',.5); inst.call('set_brightness',w['brightness_hz'])
    for name,field in [('space','space_pct'),('motion','motion_pct'),('age','age_pct'),
                       ('echo','echo_pct'),('blur','blur_pct'),('shimmer','shimmer_pct')]:
        inst.call('set_'+name,w[field]/100)
    inst.call('set_atmosphere',0.)
    inst.lib.fx_master_set_atmosphere.argtypes=[C.c_float]
    inst.lib.fx_master_set_atmosphere(w['atmos_pct']/100)
    inst.call('set_fx_mode',8)
    # Motion's FX setter also affects tape/reverb modulation. This ablation
    # is therefore labelled as FX-Motion removal in the report, not as a
    # mathematically pure chorus bypass. Pad Motion remains unchanged.
    for macro,enabled in [('motion',layer in ['no_chorus','room']),
                          ('blur',layer in ['no_blur','room'])]:
        if enabled:
            setter=getattr(inst.lib,'fx_master_set_'+macro)
            setter.argtypes=[C.c_float]; setter(0.)
    inst.lib.bowed_note_on.argtypes=[C.c_int,C.c_float,C.c_float]
    inst.note(8,50,.10)
    data=np.empty((14*SR,2),np.int16);on=off=False
    for frame in range(0,len(data),BLOCK):
        if frame>=SR//2 and not on:
            inst.note(15,62,.07)
            inst.lib.bowed_note_on(15,440*2**((62-69)/12),.42);on=True
        if frame>=11*SR and not off:
            inst.call('note_off',15);inst.call('note_off',8);off=True
        count=min(BLOCK,len(data)-frame)
        inst.lib.engine_render(data[frame:].ctypes.data_as(C.POINTER(C.c_int16)),count)
    write(path,data.astype(np.float64)/32768)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before',type=Path);p.add_argument('--after',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--render',type=Path,help=argparse.SUPPRESS)
    p.add_argument('--layer',choices=LAYERS,help=argparse.SUPPRESS)
    a=p.parse_args()
    if a.render: render(a.render,a.layer,a.output);return
    if not a.before or not a.after: p.error('--before and --after required')
    a.output.mkdir(parents=True,exist_ok=True)
    report={'notes':{'bed_midi':50,'melody_midi':62,'note_on_s':.5,'note_off_s':11},
            'ablation_caveat':'no_chorus removes FX Motion (also tape/reverb modulation); room removes FX Motion+Blur. Pad Motion unchanged.'}
    for tag,lib in [('before',a.before),('after',a.after)]:
        report[tag]={}
        for layer in LAYERS:
            path=a.output/f'{tag}_{layer}.wav'
            subprocess.run([sys.executable,__file__,'--render',str(lib.resolve()),
                            '--layer',layer,'--output',str(path)],check=True)
            x=read(path);active=x[3*SR:10*SR]
            report[tag][layer]={'rms_dbfs':float(20*np.log10(np.sqrt(np.mean(active**2))+1e-12)),
                                'lr_correlation':float(np.corrcoef(active.T)[0,1]),
                                'peak_dbfs':float(20*np.log10(np.max(np.abs(x))+1e-12)),
                                'clipped_samples':int(np.sum(np.abs(x)>=32767/32768))}
    excerpts=[];levels=[]
    for tag in ['before','after']:
        x=read(a.output/f'{tag}_dream.wav')[SR//2:].copy()
        n=round(.15*SR);x[:n]*=np.linspace(0,1,n)[:,None];x[-n:]*=np.linspace(1,0,n)[:,None]
        path=a.output/f'{tag}_excerpt.wav';write(path,x)
        excerpts.append(x);levels.append(measure(path))
    target=min([-25.] + [-12-v['true_peak_dbfs']+v['integrated_lufs'] for v in levels])
    gains=[target-v['integrated_lufs'] for v in levels]
    result=np.concatenate([excerpts[0]*10**(gains[0]/20),np.zeros((SR//2,2)),
                           excerpts[1]*10**(gains[1]/20)])
    output=a.output/'AMBIENT_Calm_Motion_AB_27s5.wav';write(output,result)
    report['comparison']={'seconds':len(result)/SR,'new_start_s':14,
                          'fixed_gains_db':gains,'target_lufs_per_excerpt':target,**measure(output)}
    (a.output/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
