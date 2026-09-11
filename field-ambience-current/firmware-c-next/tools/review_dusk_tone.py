#!/usr/bin/env python3
"""Compare Dusk dry partials at C4, velocity .5; before.so after.so report.json.
Hann-weighted projections of the 1..2 s body, amplitudes relative to root.
This measures harmonic balance, not subjective calmness or all-register aliasing.
"""
import json
import sys
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm

def partials(lib):
    inst=Instrument(lib)
    inst.call('set_synth',1);inst.call('set_fx_mode',0);inst.call('set_tuning',0)
    render_pcm(inst,44100);inst.note(0,60,.5)
    x=render_pcm(inst,2*44100)[44100:].mean(axis=1)
    w=np.hanning(len(x));t=np.arange(len(x))/44100
    h=np.array([abs(np.sum(x*w*np.exp(-2j*np.pi*261.625565*n*t))) for n in range(1,9)])
    return dict(midi=60,velocity=.5,harmonic_amplitudes_db_relative_root=(20*np.log10(h/h[0])).tolist())

if __name__=='__main__':
    r=dict(before=partials(sys.argv[1]),after=partials(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps(r,indent=2))
