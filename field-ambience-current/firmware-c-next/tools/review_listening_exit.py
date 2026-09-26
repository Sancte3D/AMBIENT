#!/usr/bin/env python3
"""26.5-second A/B: old exit, 0.5 s gap, new exit. One shared export gain.

--before baseline.so --after candidate.so --output /tmp/exit-review
Both clips: real Open Sea generator, stop at 5 s, manual Dusk D4 at 6.5..8.5 s.
20 s preroll is rendered only in memory; no audio export exceeds 30 seconds.
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


def render(library, output):
    inst = Instrument(library)
    w = worlds()[1]
    inst.call('set_world', 1)
    inst.call('set_voice', w['voice'])
    inst.call('set_brightness', w['brightness_hz'])
    inst.call('set_master_volume', .5)
    inst.call('set_fx_mode', 8)
    for macro, field in [('space','space_pct'), ('atmosphere','atmos_pct'),
                         ('motion','motion_pct'), ('age','age_pct'),
                         ('echo','echo_pct'), ('blur','blur_pct'), ('shimmer','shimmer_pct')]:
        inst.call('set_' + macro, w[field]/100)
    inst.call('set_synth', 1)
    inst.call('set_gen_seed', 0x5EEDBA55)
    inst.call('set_generative', True, -1)
    data = np.empty((33*SR, 2), dtype=np.int16)
    events = [(25, 'set_generative', (False,-1)), (26.5, 'note_on', (0,293.6648,.12)),
              (28.5, 'note_off', (0,))]
    cursor = 0
    for frame in range(0,len(data),BLOCK):
        while cursor < len(events) and frame >= events[cursor][0]*SR:
            _, name, args = events[cursor]
            inst.call(name,*args); cursor += 1
        inst.call('generative_tick', frame*1000//SR)
        n = min(BLOCK,len(data)-frame)
        inst.lib.engine_render(data[frame:].ctypes.data_as(C.POINTER(C.c_int16)),n)
    write(output, data[20*SR:].astype(np.float64)/32768)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before',type=Path)
    p.add_argument('--after',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--render',type=Path,help=argparse.SUPPRESS)
    args=p.parse_args()
    if args.render:
        render(args.render,args.output); return
    if not args.before or not args.after: p.error('--before and --after required')
    args.output.mkdir(parents=True,exist_ok=True)
    raw=[]
    for tag,lib in [('before',args.before),('after',args.after)]:
        path=args.output/(tag+'.wav')
        # Fresh process per version: isolate engine globals and random seeds.
        subprocess.run([sys.executable,__file__,'--render',str(lib),'--output',str(path)],check=True)
        raw.append(read(path))
    metrics=[measure(args.output/(tag+'.wav')) for tag in ['before','after']]
    gain=min(-25-max(m['integrated_lufs'] for m in metrics),
             -12-max(m['true_peak_dbfs'] for m in metrics))
    for x in raw:
        x*=10**(gain/20)
        n=round(.12*SR)
        x[:n]*=np.linspace(0,1,n)[:,None]; x[-n:]*=np.linspace(1,0,n)[:,None]
    combined=np.concatenate([raw[0],np.zeros((SR//2,2)),raw[1]])
    path=args.output/'AMBIENT_Generate_Exit_AB_26s5.wav'
    write(path,combined)
    report={'seconds':len(combined)/SR,'order':['before','after'],
            'second_clip_s':13.5,'exit_s':[5,18.5],
            'manual_note_s':[6.5,20.0],'constant_gain_db':gain,
            'raw_metrics':metrics,'export':measure(path)}
    (args.output/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
