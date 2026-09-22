#!/usr/bin/env python3
"""Open Sea source/mix/FX diagnosis; exports a 28.25 s six-part A/B.

--before baseline.so --after candidate.so --output /tmp/open-sea-review
Order: voice old/new, voice+pad old/new, voice+pad+Dream old/new.
Each part is 4.5 s; 0.25 s gaps. Same notes and one shared export gain.
These are controlled diagnostic notes, not an autonomous performance.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import subprocess
import sys
import numpy as np
from render_musical_review import Instrument, worlds, SR, BLOCK
from package_musical_review import measure, read, write

LAYERS=['voice','pad','mix','dream','no_blur','no_motion','no_shimmer']


def render(library, layer, path):
    x=Instrument(library); w=worlds()[1]
    x.call('set_world',1); x.call('set_voice',0)
    x.call('set_master_volume',.5); x.call('set_brightness',w['brightness_hz'])
    for macro,field in [('space','space_pct'),('motion','motion_pct'),('age','age_pct'),
                        ('echo','echo_pct'),('blur','blur_pct'),('shimmer','shimmer_pct')]:
        x.call('set_'+macro,w[field]/100)
    # Isolate the musical layers from landscape noise while retaining the
    # exact World reverb-send control. Do not change the device's World preset.
    x.call('set_atmosphere',0.)
    x.lib.fx_master_set_atmosphere.argtypes=[C.c_float]
    x.lib.fx_master_set_atmosphere(w['atmos_pct']/100)
    x.call('set_fx_mode',0 if layer in ['voice','pad','mix'] else 8)
    if layer.startswith('no_'):
        name=layer[3:]
        setter=getattr(x.lib,'fx_master_set_'+name); setter.argtypes=[C.c_float]; setter(0.)
    x.lib.bowed_note_on.argtypes=[C.c_int,C.c_float,C.c_float]
    if layer!='voice': x.note(8,50,.10)
    data=np.empty((12*SR,2),np.int16); on=off=False
    for frame in range(0,len(data),BLOCK):
        if frame>=SR and not on:
            if layer!='voice': x.note(15,71,.07)
            if layer!='pad': x.lib.bowed_note_on(15,440*2**((71-69)/12),.42)
            on=True
        if frame>=9*SR and not off:
            x.call('note_off',15); x.call('note_off',8); off=True
        n=min(BLOCK,len(data)-frame)
        x.lib.engine_render(data[frame:].ctypes.data_as(C.POINTER(C.c_int16)),n)
    write(path,data.astype(np.float64)/32768)


def metrics(path):
    x=read(path)[2*SR:8*SR]; mono=x.mean(axis=1)
    energy=np.abs(np.fft.rfft(mono*np.hanning(len(mono))))**2
    f=np.fft.rfftfreq(len(mono),1/SR)
    return {'rms_dbfs':round(20*np.log10(np.sqrt(np.mean(x*x))+1e-12),3),
            'centroid_hz':round(float(np.sum(energy*f)/np.sum(energy)),1),
            'energy_above_2khz_pct':round(float(100*np.sum(energy[f>2000])/np.sum(energy)),3),
            'lr_correlation':round(float(np.corrcoef(x.T)[0,1]),4)}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before',type=Path);p.add_argument('--after',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--render',type=Path,help=argparse.SUPPRESS)
    p.add_argument('--layer',choices=LAYERS,help=argparse.SUPPRESS)
    a=p.parse_args()
    if a.render: render(a.render,a.layer,a.output);return
    if not a.before or not a.after: p.error('--before and --after required')
    a.output.mkdir(parents=True,exist_ok=True); report={}
    for tag,lib in [('before',a.before),('after',a.after)]:
        report[tag]={}
        for layer in LAYERS:
            path=a.output/(tag+'_'+layer+'.wav')
            subprocess.run([sys.executable,__file__,'--render',str(lib),'--layer',layer,
                            '--output',str(path)],check=True)
            report[tag][layer]=metrics(path)
    segments=[];order=[]
    for layer in ['voice','mix','dream']:
        for tag in ['before','after']:
            x=read(a.output/(tag+'_'+layer+'.wav'))[SR:round(5.5*SR)].copy()
            n=round(.08*SR)
            x[:n]*=np.linspace(0,1,n)[:,None];x[-n:]*=np.linspace(1,0,n)[:,None]
            if segments: segments.append(np.zeros((SR//4,2)))
            segments.append(x);order.append(layer+' '+tag)
    combined=np.concatenate(segments); raw=a.output/'comparison_raw.wav'; write(raw,combined)
    level=measure(raw);gain=min(-25-level['integrated_lufs'],-12-level['true_peak_dbfs'])
    result=a.output/'AMBIENT_Open_Sea_Stems_AB_28s25.wav'
    write(result,combined*10**(gain/20))
    report['comparison']={'seconds':len(combined)/SR,'order':order,
                          'shared_gain_db':round(gain,2),**measure(result)}
    (a.output/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__': main()
