#!/usr/bin/env python3
"""Compare real Bowed DSP across both colours, four pitches and three levels.
Requires numpy, scipy and ffmpeg; --before/--after host shared libraries.
Exports a 27.5-second dry A/B and JSON measurements. Host diagnostics only.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import numpy as np
from scipy.signal import butter, sosfiltfilt, hilbert
from package_musical_review import write, measure

SR=44100

def render(lib, colour, midi, amp, seconds=6, release=None):
    lib.dsp_init(); lib.shape_init(); lib.bowed_init(); lib.bowed_set_colour(colour)
    hz=440*2**((midi-69)/12)
    lib.bowed_note_on.argtypes=[C.c_int,C.c_float,C.c_float]
    lib.bowed_render_mix.argtypes=[C.POINTER(C.c_float)]*4+[C.c_int,C.c_float]
    lib.bowed_note_on(15,hz,amp)
    x=np.zeros((round(seconds*SR),2));off=False
    for start in range(0,len(x),256):
        if release is not None and start>=release*SR and not off:
            lib.bowed_note_off(15);off=True
        n=min(256,len(x)-start);buf=[np.zeros(n,np.float32) for _ in range(4)]
        lib.bowed_render_mix(*[b.ctypes.data_as(C.POINTER(C.c_float)) for b in buf],n,0.)
        x[start:start+n]=np.column_stack(buf[:2])
    return x,hz

def metrics(x,hz):
    mono=x.mean(axis=1)
    # Isolate the root and its nearby companion, then demodulate pitch.
    root=sosfiltfilt(butter(4,[hz-20,hz+20],btype='bandpass',fs=SR,output='sos'),mono)
    phase=np.unwrap(np.angle(hilbert(root)))
    f=np.gradient(phase)*SR/(2*np.pi)
    t=np.arange(len(f))/SR;mask=(t>=2)&(t<5)
    cents=1200*np.log2(np.maximum(f[mask],1e-9)/hz);t=t[mask]
    # Fit the known detune beat independently; do not attribute it to vibrato.
    freqs=[5.1,hz*.0041,2*hz*.0041,3*hz*.0041]
    basis=[np.ones_like(t),t-t.mean()]
    for rate in freqs:basis.extend([np.sin(2*np.pi*rate*t),np.cos(2*np.pi*rate*t)])
    fit=np.linalg.lstsq(np.column_stack(basis),cents,rcond=None)[0]
    segment=mono[2*SR:5*SR];power=abs(np.fft.rfft(segment*np.hanning(len(segment))))**2
    bins=np.fft.rfftfreq(len(segment),1/SR)
    rootpower=power[abs(bins-hz)<12].sum()
    # Between harmonics, away from 1.5x and 2x sympathetic resonators.
    grain=power[(bins>2.25*hz)&(bins<2.75*hz)].sum()
    return dict(vibrato_5hz_cents=float(np.hypot(fit[2],fit[3])),
                grain_to_root_db=float(10*np.log10((grain+1e-30)/rootpower)),
                root_rms=float(np.sqrt(np.mean(root[2*SR:5*SR]**2))))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before',required=True,type=Path);p.add_argument('--after',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    libs=[C.CDLL(str(path.resolve())) for path in [a.before,a.after]]
    report={'probes':[]};failed=0
    for colour in range(2):
        for midi in [50,55,62,69]:
            for amp in [.18,.42,.58]:
                vals=[metrics(*render(lib,colour,midi,amp)) for lib in libs]
                old,new=vals;root_db=20*np.log10(new['root_rms']/old['root_rms'])
                drop=new['grain_to_root_db']-old['grain_to_root_db']
                ok=(all(np.isfinite(list(v.values())).all() for v in vals)
                    and new['root_rms']>.01*amp and abs(root_db)<1
                    and new['vibrato_5hz_cents']<.7 and drop< -10)
                failed+=not ok
                report['probes'].append(dict(colour=colour,midi=midi,amp=amp,before=old,after=new,
                    root_change_db=float(root_db),grain_change_db=float(drop),passed=bool(ok)))
    clips=[];levels=[]
    for colour in range(2):
        for tag,lib in zip(['old','new'],libs):
            x,_=render(lib,colour,62,.42,6.5,4.5)
            fade=round(.1*SR);x[:fade]*=np.linspace(0,1,fade)[:,None];x[-fade:]*=np.linspace(1,0,fade)[:,None]
            path=a.output/f'colour{colour}_{tag}.wav';write(path,x);clips.append(x);levels.append(measure(path))
    target=min([-25.]+[-12-l['true_peak_dbfs']+l['integrated_lufs'] for l in levels]);parts=[]
    gains=[target-l['integrated_lufs'] for l in levels]
    for clip,gain in zip(clips,gains):
        if parts:parts.append(np.zeros((SR//2,2)))
        parts.append(clip*10**(gain/20))
    audio=a.output/'AMBIENT_Bowed_Calm_AB_27s5.wav';write(audio,np.concatenate(parts))
    report['audio']=dict(seconds=27.5,starts_s=[0,7,14,21],order=['Open Sea old','Open Sea new','Fjords old','Fjords new'],
        fixed_gains_db=gains,**measure(audio))
    report['failures']=int(failed)
    (a.output/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    for key in ['vibrato_5hz_cents','grain_to_root_db','root_rms']:
        print(key, {tag:[min(r[tag][key] for r in report['probes']),max(r[tag][key] for r in report['probes'])] for tag in ['before','after']})
    print('24 probes, failures:',failed);print(report['audio'])
    return bool(failed)

if __name__=='__main__':raise SystemExit(main())
