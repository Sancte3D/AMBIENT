#!/usr/bin/env python3
"""SD41 bounded actual-PCM boot/recall/seed proof; no flash or device simulation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from check_prepared_scenes import compare_inputs

ROOT = Path(__file__).resolve().parents[1]
BASE = '67b4ac83f42e5af7b5190db812005ed8c4d693ae'
TREE = '73cac3ef4682f19dc74d1fe1c29f6311bc2f6d6a'
SOURCES = ('engine_product world_grammar worlds_product bowed horn pluck ambient_room nature '
           'dsp shape tuning brain cells controls scenes menu params battery oled_draw oled_color '
           'baked_font baked_font_data font_8x8').split()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    repo = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True).strip())
    prefix = ROOT.relative_to(repo)
    assert subprocess.check_output(['git', 'rev-parse', BASE+'^{tree}'], cwd=repo, text=True).strip() == TREE
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
        str(prefix/'src'), str(prefix/'include'), str(prefix/'cmake'), str(prefix/'CMakeLists.txt')],
        cwd=repo, text=True).splitlines()
    compiled = [p for p in tracked if Path(p).suffix.lower() in ('.c', '.h', '.s', '.ld', '.cmake')
                or Path(p).name == 'CMakeLists.txt']
    assert len(compiled) == 251
    default_guard = compare_inputs(repo, ROOT, BASE, compiled)
    with tempfile.TemporaryDirectory(prefix='boot-recall-review-') as directory:
        binary = Path(directory)/'contract'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT',
            '-I'+str(ROOT/'include'), str(ROOT/'test/test_product_boot_recall.c'),
            *[str(ROOT/'src'/f'{name}.c') for name in SOURCES], '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr.strip() or f'Contract exited {result.returncode}')
    rows = [json.loads(line) for line in result.stdout.splitlines()]
    summary = rows[-1]
    assert summary['category'] == 'summary' and summary['status'] == 'passed'
    expected = dict(boot_restarts=6, bad_store_fallbacks=380, legacy_recalls=10, off_recalls=20,
                    manual_pcm_triplets=12, failed_save_pcm_triplets=3, seed_pcm_triplets=9,
                    seed_wrap_pairs=9, pending_cancellations=3, live_seed_recalls=3, autoplay_gates=3)
    assert all(summary[key] == value for key, value in expected.items())
    manifest = dict(scope='SD41 only: current real Product boot, Scene recall and bounded eight-second '
        'seed openings. Same pre-DMA API initialization order as H743; RAM persistence backend. '
        'Not HAL/flash/ECC/IRQ, full-phrase, arbitrary corruption or acoustic acceptance.',
        baseline_commit=BASE, baseline_tree=TREE, default_runtime_changed=False, prepared_scene_hooks_changed=True,
        unchanged_firmware_input_count=default_guard['existing_inputs_byte_equal'],
        default_scene_guard=default_guard,
        firmware_input_sha256={str(Path(p).relative_to(prefix)): sha(repo/p) for p in compiled},
        audit_input_sha256={p: sha(ROOT/p) for p in ('test/test_product_boot_recall.c', 'tools/review_boot_recall.py')},
        block_sizes=[64, 512], control_tick_ms=10, sample_rate_hz=44100,
        product_scene_bytes=368, device_boot_target_percent=30, recalled_volume_is_preserved=True,
        saved_generate_and_held_owners_are_not_restored=True, optional_nature_recall_is_explicit=True,
        whole_pcm_and_events_compared_in_c=True, fingerprint_note='FNV-1a64 is a diagnostic on host bytes; '
        'proof uses direct complete memcmp, not hashes alone. Not a portable ARM event-ABI fingerprint.',
        generated_live_recall_preserves_room=True, journal_candidate_default='OFF',
        hardware_accepted=False, hearing_accepted=False, summary=summary, seed_openings=rows[:-1])
    path = out/'SD41_BOOT_RECALL_METRICS.json'
    path.write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(status='passed', summary=summary, proof_path=str(path), proof_sha256=sha(path))))


if __name__ == '__main__':
    main()
