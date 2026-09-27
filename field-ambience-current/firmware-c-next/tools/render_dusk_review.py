#!/usr/bin/env python3
"""25 s tonal A/B: before.so after.so output.wav. Constant gain per half.
Five original Just C-major notes, C3/C4/E4/G4/C5, light shared room.
"""
import json
import sys
import tempfile
from pathlib import Path
import numpy as np
from render_musical_review import Instrument
from render_tuning_review import render_pcm
from package_musical_review import measure, write

def phrase(lib):
    inst=Instrument(lib)
    for k,v in [('set_synth',1),('set_key',60),('set_tuning',1),
                ('set_fx_mode',1),('set_space',.3),('set_master_volume',.4)]:
        inst.call(k,v)
    render_pcm(inst,44100)
    chunks=[]
    for ratio in (.5,1,1.25,1.5,2):
        inst.lib.engine_note_on(0,261.625565*ratio,.5)
        chunks.append(render_pcm(inst,74970))  # 1.7 s held
        inst.call('note_off',0)
        chunks.append(render_pcm(inst,30870))  # .7 s release
    x=np.concatenate(chunks)
    x[-8820:]*=np.linspace(1,0,8820)[:,None]
    assert x.shape==(12*44100,2) and np.isfinite(x).all()
    return x

if __name__=='__main__':
    halves=[phrase(lib) for lib in sys.argv[1:3]]
    with tempfile.TemporaryDirectory() as tmp:
        path=Path(tmp)/'probe.wav';raw=[]
        for x in halves:
            write(path,x);raw.append(measure(path))
        target=min([-25]+[m['integrated_lufs']-m['true_peak_dbfs']-6 for m in raw])
        gain=[target-m['integrated_lufs'] for m in raw]
        x=np.concatenate([halves[0]*10**(gain[0]/20),np.zeros((44100,2)),halves[1]*10**(gain[1]/20)])
        write(sys.argv[3],x)
        print(json.dumps(dict(duration_s=25,gain_db=gain,raw=raw,output=measure(sys.argv[3])),indent=2))
