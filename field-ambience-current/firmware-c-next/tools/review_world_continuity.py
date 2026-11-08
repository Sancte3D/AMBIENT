#!/usr/bin/env python3
"""Measure current World/Character continuity; export one 28 s device-path clip.

From firmware-c-next: python3 tools/review_world_continuity.py --output DIR
Optional --engine FILE reuses a library from build_engine. Each case uses a
fresh process. This characterizes the current gap; it is not an acceptance
test requiring silence to remain after the architecture is corrected.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import subprocess
import sys

import numpy as np
from render_musical_review import Instrument, build_engine, worlds
from package_musical_review import measure, read, write

SR, SECONDS, SEED = 44100, 28, 1031
CASES = ('dry_control', 'dry_switch', 'world_switch')


def run_case(library, output, case):
    inst = Instrument(library)
    lib = inst.lib
    lib.engine_set_generative.argtypes = [C.c_bool, C.c_int]
    lib.engine_generative_new_field.argtypes = [C.c_uint32]
    lib.engine_generative_tick.argtypes = [C.c_uint32]
    lib.engine_set_user_presence.argtypes = [C.c_bool]
    world = worlds()[3]  # Moss Fields, C Aeolian, native world parameters.
    inst.call('set_world', 3)
    # Autoplay uses engine harmony directly; bloom's cell chord/bass controls
    # are not exercised here. Dusk gestures below are direct engine notes.
    inst.call('set_voice', world['voice'])
    inst.call('set_brightness', world['brightness_hz'])
    for macro, field in [('space', 'space_pct'), ('atmosphere', 'atmos_pct'),
                         ('motion', 'motion_pct'), ('age', 'age_pct'),
                         ('echo', 'echo_pct'), ('blur', 'blur_pct'),
                         ('shimmer', 'shimmer_pct')]:
        inst.call('set_' + macro, world[field] / 100)
    inst.call('set_fx_mode', 8 if case == 'world_switch' else 0)
    inst.call('set_tuning', 0)
    inst.call('generative_new_field', SEED)
    inst.call('set_generative', True, -1)
    events, frame = [], 0
    hook_type = C.CFUNCTYPE(None, C.c_int, C.c_uint8, C.c_float, C.c_float)

    @hook_type
    def hook(on, source, hz, amp):
        events.append(dict(seconds=round(frame / SR, 6), on=on,
                           source=source, hz=round(hz, 4), amp=round(amp, 5)))

    lib.engine_set_note_hook.argtypes = [hook_type]
    lib.engine_set_note_hook(hook)
    actions = {}
    if case != 'dry_control':
        actions[10 * SR] = lambda: inst.call('set_synth', 1)  # Dusk
        actions[20 * SR] = lambda: inst.call('set_synth', 0)
    if case == 'world_switch':
        def note(midi, amp):
            inst.call('set_user_presence', True)
            inst.note(0, midi, amp)

        def off():
            inst.call('note_off', 0)
            inst.call('set_user_presence', False)

        actions[11 * SR] = lambda: note(60, .18)
        actions[14 * SR] = off
        actions[16 * SR] = lambda: note(67, .14)
        actions[18 * SR] = off
    boundaries = sorted([*actions, SECONDS * SR])
    pcm = np.zeros((SECONDS * SR, 2), dtype=np.int16)
    while frame < len(pcm):
        if frame in actions:
            actions[frame]()
        lib.engine_generative_tick(frame * 1000 // SR)
        next_boundary = next(n for n in boundaries if n > frame)
        count = min(512, next_boundary - frame)
        lib.engine_render(pcm[frame:].ctypes.data_as(C.POINTER(C.c_int16)), count)
        frame += count
    x = pcm.astype(np.float64) / 32768
    assert np.isfinite(x).all() and not np.any(np.abs(pcm.astype(np.int32)) >= 32767)
    write(output / (case + '.wav'), x)
    windows = []
    for start, stop in [(2, 10), (12, 20), (22, 28)]:
        segment = x[start * SR:stop * SR]
        rms = float(np.sqrt(np.mean(segment ** 2)))
        windows.append(dict(start_s=start, end_s=stop,
                            rms_dbfs=round(20 * np.log10(rms), 2) if rms else None,
                            nonzero_samples=int(np.count_nonzero(segment))))
    counts = [sum(e['on'] == 1 and e['source'] in (5, 6, 7, 8, 15)
                  and start <= e['seconds'] < stop for e in events)
              for start, stop in [(0, 10), (10, 20), (20, 28)]]
    report = dict(case=case, world=world['name'], seed=SEED, seconds=SECONDS,
                  fx_mode=8 if case == 'world_switch' else 0,
                  automatic_onsets_0_10_20_28=counts, windows=windows,
                  native_measurement=measure(output / (case + '.wav')), events=events)
    (output / (case + '.json')).write_text(json.dumps(report, indent=2) + '\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--engine', type=Path)
    ap.add_argument('--case', choices=CASES)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    engine = args.engine or build_engine(args.output / 'engine.so')
    if args.case:
        run_case(engine, args.output, args.case)
        return
    reports = []
    for case in CASES:
        subprocess.run([sys.executable, str(Path(__file__).resolve()), '--engine',
                        str(engine.resolve()), '--output', str(args.output.resolve()),
                        '--case', case], check=True)
        reports.append(json.loads((args.output / (case + '.json')).read_text()))
    # One constant gain for the entire transition. Never hide the gap or
    # character/world level differences by normalizing individual sections.
    x = read(args.output / 'world_switch.wav')
    native = reports[-1]['native_measurement']
    gain_db = min(-25 - native['integrated_lufs'], -12 - native['true_peak_dbfs'])
    x *= 10 ** (gain_db / 20)
    x[:round(.02 * SR)] *= np.linspace(0, 1, round(.02 * SR))[:, None]
    x[-round(.2 * SR):] *= np.linspace(1, 0, round(.2 * SR))[:, None]
    preview = args.output / 'Ambient_World_Character_28s.wav'
    write(preview, x)
    result = dict(cases=reports, preview=dict(file=preview.name, seconds=SECONDS,
                  constant_gain_db=round(gain_db, 3), **measure(preview)),
                  limits='Host render, no hardware timing or subjective acceptance; '
                         'dry controls have no manual notes; wet clip has two Dusk notes.')
    (args.output / 'WORLD_CONTINUITY_METRICS.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(onsets={r['case']: r['automatic_onsets_0_10_20_28'] for r in reports},
                         windows={r['case']: r['windows'] for r in reports},
                         preview=result['preview']), indent=2))


if __name__ == '__main__':
    main()
