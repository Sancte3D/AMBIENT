#!/usr/bin/env python3
"""Short live-tuning A/B. Args: before.so after.so output.wav.
Each six-second section holds E5 over a quiet C5 reference. Start Just,
change to Equal at 2 s, return to Just at 4 s. One-second pause between.
"""
import ctypes as C
import sys
import wave
import numpy as np
from render_musical_review import Instrument

SR = 44100

def section(library):
    inst = Instrument(library)
    for key, value in [('set_tuning', 1), ('set_key', 60), ('set_fx_mode', 0),
                       ('set_synth', 2), ('set_release', 1.0),
                       ('set_master_volume', 0.55)]:
        inst.call(key, value)
    def render(n):
        data = np.zeros((n, 2), dtype=np.int16)
        inst.lib.engine_render(data.ctypes.data_as(C.POINTER(C.c_int16)), n)
        return data.astype(np.float64) / 32768
    render(SR)
    inst.lib.engine_note_on(0, 523.2511306 * 1.25, 0.65)
    parts = []
    for mode in (1, 0, 1):
        inst.call('set_tuning', mode)
        parts.append(render(SR * 2))
    data = np.concatenate(parts)
    t = np.arange(len(data)) / SR
    fade = np.minimum(t / .03, 1) * np.minimum((6 - t) / .1, 1)
    data += (.04 * np.sin(2 * np.pi * 523.2511306 * t))[:, None]
    return data * fade[:, None]

if __name__ == '__main__':
    data = np.concatenate([section(sys.argv[1]), np.zeros((SR, 2)), section(sys.argv[2])])
    assert np.isfinite(data).all() and np.max(np.abs(data)) < 1
    with wave.open(sys.argv[3], 'wb') as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(SR)
        out.writeframes((data * 32767).astype('<i2').tobytes())
    print(f'13 s A/B; peak {np.max(np.abs(data)):.4f}; fixed gain')
