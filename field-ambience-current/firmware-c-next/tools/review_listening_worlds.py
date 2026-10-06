#!/usr/bin/env python3
"""Render one real autonomous World: 24 s playing + 4 s release, never >30 s.

python3 tools/review_listening_worlds.py --world 1 --output /tmp/open-sea
Uses firmware defaults, real scheduler and shared FX; no scripted notes.
Requires the existing host renderer dependencies (numpy, cc, ffmpeg).
"""
import argparse
import ctypes as C
import json
from pathlib import Path

import numpy as np

from render_musical_review import Instrument, build_engine, worlds, SR, BLOCK
from package_musical_review import measure, write


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--world', type=int, choices=range(5), default=1)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--engine', type=Path)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    library = args.engine or build_engine(args.output / 'engine.so')
    inst = Instrument(library)
    w = worlds()[args.world]
    inst.call('set_world', args.world)
    inst.call('set_voice', w['voice'])
    inst.call('set_brightness', w['brightness_hz'])
    inst.call('set_master_volume', .5)
    inst.call('set_fx_mode', 8)
    inst.call('set_tuning', 0)
    for macro, field in [('space', 'space_pct'), ('atmosphere', 'atmos_pct'),
                         ('motion', 'motion_pct'), ('age', 'age_pct'),
                         ('echo', 'echo_pct'), ('blur', 'blur_pct'),
                         ('shimmer', 'shimmer_pct')]:
        inst.call('set_' + macro, w[field] / 100)
    events, clock = [], [0]
    hook_type = C.CFUNCTYPE(None, C.c_int, C.c_uint8, C.c_float, C.c_float)

    @hook_type
    def hook(on, source, hz, amp):
        events.append({'ms': clock[0], 'on': on, 'source': source,
                       'hz': round(hz, 3), 'amp': round(amp, 5)})

    inst.lib.engine_set_note_hook.argtypes = [hook_type]
    inst.lib.engine_set_note_hook(hook)
    inst.call('set_gen_seed', 0x5EEDBA55)
    inst.call('set_generative', True, -1)
    data = np.empty((28 * SR, 2), dtype=np.int16)
    stopped = False
    for frame in range(0, len(data), BLOCK):
        clock[0] = frame * 1000 // SR
        if clock[0] >= 24000 and not stopped:
            inst.call('set_generative', False, -1)
            stopped = True
        inst.call('generative_tick', clock[0])
        n = min(BLOCK, len(data) - frame)
        inst.lib.engine_render(data[frame:].ctypes.data_as(C.POINTER(C.c_int16)), n)
    x = data.astype(np.float64) / 32768
    raw = args.output / 'raw.wav'
    write(raw, x)
    original = measure(raw)
    # One constant gain for the entire performance; no limiting or section
    # normalization. A conservative audition level cannot predict headphone SPL.
    gain_db = min(-25 - original['integrated_lufs'], -12 - original['true_peak_dbfs'])
    x *= 10 ** (gain_db / 20)
    fade = round(.2 * SR)
    x[:fade] *= np.linspace(0, 1, fade)[:, None]
    x[-fade:] *= np.linspace(1, 0, fade)[:, None]
    output = args.output / ('AMBIENT_' + w['name'].replace(' ', '_') + '_28s.wav')
    write(output, x)
    report = {'world': w['name'], 'seconds': 28, 'generate_stop_s': 24,
              'raw': original, 'gain_db': round(gain_db, 2),
              'audition': measure(output), 'raw_clipped_samples': int(np.sum(np.abs(data.astype(np.int32)) >= 32767)),
              'raw_dc': np.mean(data.astype(np.float64) / 32768, axis=0).tolist(),
              'events': events}
    (args.output / 'measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'events'}, indent=2))


if __name__ == '__main__':
    main()
