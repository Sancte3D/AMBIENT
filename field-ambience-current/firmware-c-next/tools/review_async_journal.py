#!/usr/bin/env python3
"""Prepared SD41 async writer proof; RAM NOR, actual Product PCM, no device activation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from check_prepared_scenes import compare_inputs

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('engine_product world_grammar worlds_product bowed horn pluck ambient_room nature dsp '
           'shape tuning brain cells controls scenes menu params battery oled_draw oled_color '
           'baked_font baked_font_data font_8x8 scenes_journal scenes_journal_async').split()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    base = '6e5be3d663a54d3e1e08d77683b23563fae0da09'
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', base, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')],
        cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in ('.c', '.h', '.s', '.ld', '.cmake')
                or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 251
    default_guard = compare_inputs(repo, ROOT, base, compiled)
    with tempfile.TemporaryDirectory(prefix='async-journal-review-') as directory:
        tmp = Path(directory)
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'include')]
        rows = {}
        for name, sources, product in (
            ('test_scenes_journal_async', ['scenes_journal', 'scenes_journal_async'], False),
            ('test_product_async_journal', SOURCES, True)):
            binary = tmp/name
            subprocess.run([*flags, *(['-DFAM_SOUND_PRODUCT'] if product else []),
                str(ROOT/'test'/f'{name}.c'), *[str(ROOT/'src'/f'{s}.c') for s in sources],
                *(['-lm'] if product else []), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stderr or f'{name} failed: {result.returncode}')
            rows[name] = [json.loads(line) for line in result.stdout.splitlines()]
    inputs = ['include/scenes_journal_async.h', 'src/scenes_journal_async.c',
              'src/scenes_journal.c', 'include/scenes_journal.h',
              'test/test_scenes_journal_async.c', 'test/test_product_async_journal.c',
              'tools/review_async_journal.py']
    manifest = dict(scope='SD41 prepared Main-only writer. No H743 flash, Scene frontend or UI '
        'integration; no ECC/IRQ/timing/device guarantee. Callbacks must be nonblocking and '
        'classify safe reads; cancellation drains in-flight operations. Boot recovery remains separate.',
        baseline_commit=base, existing_firmware_inputs_byte_equal=default_guard['existing_inputs_byte_equal'],
        default_scene_guard=default_guard,
        existing_jsc1_scn7_format_unchanged=True, device_default_changed=False,
        audio_code_changed=False, journal_candidate_default='OFF',
        source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs},
        model=rows['test_scenes_journal_async'][-1],
        actual_pcm=rows['test_product_async_journal'])
    path = out/'SD41_ASYNC_JOURNAL_METRICS.json'
    path.write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(status='passed', model=manifest['model'], actual_pcm=manifest['actual_pcm'],
                         proof_sha256=hashlib.sha256(path.read_bytes()).hexdigest())))

if __name__ == '__main__':
    main()
