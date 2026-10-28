#!/usr/bin/env python3
"""SD52: run actual H743 input routing against whole Product PCM controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = 'b29246fde0c66618bbfc25742f929dc1f362ebf6'

def run(cmd, **kw):
    return subprocess.check_output(cmd, text=True, **kw)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output', type=Path)
    ap.add_argument('--sanitize', action='store_true')
    ap.add_argument('--fixture-only', action='store_true')
    ap.add_argument('--finish-candidate', action='store_true')
    a = ap.parse_args()
    out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    hal = (ROOT/'src/hal_h743/main_h743.c').read_text()
    # The routing fixture has many test includes. Independently compile the
    # real entry point's engine-header prelude so those includes cannot hide
    # a missing dependency in Basic Product versus Full-Sound builds.
    engine_headers = hal[hal.index('#include "engine.h"'):hal.index('#ifndef FAM_SOUND_PRODUCT')]
    with tempfile.TemporaryDirectory(prefix='ui-header-scope-') as temp:
        probe = Path(temp)/'headers.c'
        probe.write_text(engine_headers+'\nvoid probe(void) {\n#ifdef FAM_SOUND_PRODUCT\n'
            'engine_note_block_t reason=engine_note_block_reason();\n'
            'uint32_t count=engine_admission_rejections();\n(void)reason;(void)count;\n#endif\n}\n')
        for profile in ('reference','product','full'):
            defs=[] if profile=='reference' else ['-DFAM_SOUND_PRODUCT']
            if profile=='full':defs+=['-DFAM_SOUND_FINISH_CANDIDATE']
            run([os.environ.get('CC','cc'),'-std=c11','-Wall','-Wextra','-Werror',*defs,
                 '-I'+str(ROOT/'include'),'-c',str(probe),'-o',str(Path(temp)/'headers.o')])
    routing = hal[hal.index('static void dispatch_cell('):hal.index('\n/* r19.16')]
    poll = hal[hal.index('static ui_idle_t s_ui_idle;'):hal.index('\nint main(void)')]
    inputs = [ROOT/'src/ui_idle.c', ROOT/'include/ui_idle.h', ROOT/'src/leds.c',
              ROOT/'include/leds.h', ROOT/'src/hal_h743/main_h743.c',
              ROOT/'CMakeLists.txt', ROOT/'test/test_ui_idle.c',
              ROOT/'test/test_ui_h743.c', Path(__file__)]
    baseline_equal = None
    if not a.fixture_only:
        repo = Path(run(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT).strip())
        prefix = ROOT.relative_to(repo)
        paths = run(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
            str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'),
            str(prefix/'CMakeLists.txt')], cwd=repo).splitlines()
        paths = [p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake')
                 or Path(p).name == 'CMakeLists.txt']
        assert len(paths) == 263
        changed = {str(prefix/p) for p in ('src/leds.c','include/leds.h',
                    'src/hal_h743/main_h743.c','CMakeLists.txt')}
        for p in paths:
            if p not in changed:
                assert (repo/p).read_bytes() == subprocess.check_output(
                    ['git','show',f'{BASE}:{p}'], cwd=repo), p
        baseline_equal = len(paths) - len(changed)
    logs = []
    for sanitized in ([False, True] if a.sanitize else [False]):
        with tempfile.TemporaryDirectory(prefix='ambient-ui-') as temp:
            tmp = Path(temp)
            (tmp/'ui_h743_routing.inc').write_text(routing + '\n' + poll)
            flags = ['-std=c11','-O2','-Wall','-Wextra','-Werror']
            if a.finish_candidate:flags += ['-DFAM_SOUND_FINISH_CANDIDATE']
            if sanitized:
                flags += ['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
            cc = os.environ.get('CC','cc')
            run([cc, *flags, '-I'+str(ROOT/'include'), str(ROOT/'test/test_ui_idle.c'),
                 str(ROOT/'src/ui_idle.c'), '-o', str(tmp/'unit')])
            unit = run([str(tmp/'unit')], env=env)
            run(['cmake','-S',str(ROOT),'-B',str(tmp/'build'),'-DFAM_TARGET=host',
                 '-DFAM_SOUND_PROFILE=product','-DCMAKE_BUILD_TYPE=Release',
                 '-DFAM_SOUND_FINISH_CANDIDATE='+('ON' if a.finish_candidate else 'OFF'),
                 '-DCMAKE_C_FLAGS='+' '.join(flags)])
            run(['cmake','--build',str(tmp/'build'),'-j2'])
            run([cc, *flags, '-Wno-unused-function', '-DFAM_SOUND_PRODUCT',
                 '-I'+str(ROOT/'include'), '-I'+str(tmp), str(ROOT/'test/test_ui_h743.c'),
                 str(tmp/'build/libfield_ambience_dsp.a'), '-lm', '-o', str(tmp/'integration')])
            integration = run([str(tmp/'integration')], env=env)
            logs.append(dict(sanitized=sanitized, unit=unit.strip(), integration=integration.strip()))
            print(logs[-1], flush=True)
    proof = dict(baseline=BASE, unchanged_existing_compiled_inputs=baseline_equal,
        actual_h743_routing_extracted=True, real_engine_header_scope_profiles=['reference','product','full'], comparisons_per_mode=126,
        whole_stereo_frames_compared_per_mode=126*12*44100,
        audio_seconds_per_mode=132*12, logs=logs,
        scope='21 wake routes, all3 Worlds,64/512 render caps; cap512 uses441-frame Main grid; normal/wrapping Mainclock. Entire12-second PCM equality against untouched awake controls, actual UI setters and subsequent matching musical/volume input. Separate exact900000-ms boundary/held/duplicate/boot-held/wrap cases.',
        limitations='Hardware queues injected; not physical switch bounce, DMA latency, display current, H743 whole IRQ/stack, flash power-cut or hearing acceptance. Idle boundary uses elapsed Mainclock; no15-minute real-device timing claim. Default live Save remains synchronous; journal OFF.',
        sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs})
    (out/'SD52_UI_IDLE_METRICS.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')

if __name__ == '__main__':
    main()
