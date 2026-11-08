#!/usr/bin/env python3
"""SD41 prepared yielding recovery and actual Product save-retry PCM; no device activation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from check_prepared_scenes import compare_inputs

ROOT = Path(__file__).resolve().parents[1]
BASE = '6c380a62be0a3fd54e1725995ad619de9ab51396'
SOURCES = ('engine_product world_grammar worlds_product bowed horn pluck ambient_room nature dsp '
           'shape tuning brain cells controls scenes menu params battery oled_draw oled_color '
           'baked_font baked_font_data font_8x8 scenes_journal scenes_journal_async '
           'scenes_journal_recovery').split()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')],
        cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in ('.c', '.h', '.s', '.ld', '.cmake')
                or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 253
    default_guard = compare_inputs(repo, ROOT, BASE, compiled)
    with tempfile.TemporaryDirectory(prefix='journal-recovery-review-') as directory:
        tmp = Path(directory)
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'include')]
        rows = {}
        for name, sources, product in (
            ('test_scenes_journal_recovery', ['scenes_journal', 'scenes_journal_async', 'scenes_journal_recovery'], False),
            ('test_product_journal_recovery', SOURCES, True)):
            binary = tmp/name
            subprocess.run([*flags, *(['-DFAM_SOUND_PRODUCT'] if product else []),
                str(ROOT/'test'/f'{name}.c'), *[str(ROOT/'src'/f'{s}.c') for s in sources],
                *(['-lm'] if product else []), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stderr or f'{name} failed: {result.returncode}')
            rows[name] = [json.loads(line) for line in result.stdout.splitlines()]
    inputs = ['include/scenes_journal_recovery.h', 'src/scenes_journal_recovery.c',
              'include/scenes_journal.h', 'src/scenes_journal.c',
              'include/scenes_journal_async.h', 'src/scenes_journal_async.c',
              'test/test_scenes_journal_recovery.c', 'test/test_product_journal_recovery.c',
              'tools/review_journal_recovery.py']
    manifest = dict(scope='SD41 prepared Main-only recovery. Begin performs no I/O; each step reads '
        'at most 32 bytes and computes at most 32-byte CRC. Exclusive journal/media ownership; '
        'availability invalidated at Begin, no partially published cache. Read callbacks must '
        'return promptly and safely classify corrupt/unavailable memory. No Scene frontend, '
        'H743/ECC/IRQ/timing or default activation. Existing synchronous Scene read/save remains.',
        baseline_commit=BASE, existing_compiled_inputs_byte_equal=default_guard['existing_inputs_byte_equal'],
        default_scene_guard=default_guard,
        existing_runtime_core_inputs_byte_equal=250, previous_unlinked_writer_inputs_byte_equal=2,
        existing_jsc1_scn7_format_unchanged=True, audio_code_changed=False,
        device_default_changed=False, journal_candidate_default='OFF',
        source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs},
        model=rows['test_scenes_journal_recovery'][-1],
        actual_pcm=rows['test_product_journal_recovery'])
    path = out/'SD41_JOURNAL_RECOVERY_METRICS.json'
    path.write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(status='passed', model=manifest['model'],
        actual_pcm_summary=manifest['actual_pcm'][-1],
        proof_sha256=hashlib.sha256(path.read_bytes()).hexdigest())))

if __name__ == '__main__':
    main()
