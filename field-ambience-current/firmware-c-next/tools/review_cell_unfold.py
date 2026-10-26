#!/usr/bin/env python3
"""Current Cell expression, matched PR182 neutral PCM and true Product A/B."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = 'bea7f5765125b2d55c7bb28832626dce7956b382'
SOURCES = ['engine_product', 'world_grammar', 'controls', 'bowed', 'horn',
           'pluck', 'ambient_room', 'nature', 'dsp', 'shape', 'tuning', 'brain', 'cells']

def run(cmd, **kwargs):
    return subprocess.check_output([str(c) for c in cmd], text=True, **kwargs)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output', type=Path)
    a = ap.parse_args()
    out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    cc = os.environ.get('CC', 'cc')
    flags = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-DFAM_SOUND_PRODUCT']
    def compile_at(root, source, binary, extra=()):
        run([cc, *flags, *extra, '-I'+str(root/'include'), source,
             *[root/'src'/f'{n}.c' for n in SOURCES], '-lm', '-o', binary])
    with tempfile.TemporaryDirectory(prefix='cell-unfold-') as directory:
        tmp = Path(directory)
        repo = Path(run(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT).strip())
        prefix = ROOT.relative_to(repo)
        archive = subprocess.check_output(['git', 'archive', BASE, str(prefix)], cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
            tar.extractall(tmp/'baseline', filter='data')
        base_root = tmp/'baseline'/prefix
        compile_at(ROOT, ROOT/'test/test_cell_unfold.c', tmp/'test',
                   ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])
        test = run([tmp/'test'], env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0')).strip()
        renderer = ROOT/'tools/render_cell_unfold.c'
        compile_at(ROOT, renderer, tmp/'current', ['-DHAS_CELL_UNFOLD'])
        compile_at(base_root, renderer, tmp/'base')
        run([tmp/'current', tmp/'neutral.wav', 'neutral'])
        run([tmp/'base', tmp/'baseline.wav', 'neutral'])
        assert (tmp/'neutral.wav').read_bytes() == (tmp/'baseline.wav').read_bytes()
        wav = out/'AMBIENT_Entfaltung_AB.wav'
        run([tmp/'current', wav])
        assert wav.stat().st_size == 44+72*44100*4
        proof = dict(baseline=BASE, neutral_pcm_byte_identical=True,
                     neutral_stereo_frames=72*44100, sanitizer_test=test,
                     seconds=72, worlds=['COAST', 'WOODLAND', 'HIGHLANDS'],
                     order='Each World: 12s neutral, then 12s physical unfold. No file gain.',
                     wav_sha256=hashlib.sha256(wav.read_bytes()).hexdigest(),
                     neutral_sha256=hashlib.sha256((tmp/'neutral.wav').read_bytes()).hexdigest(),
                     device_accepted=False, hearing_accepted=False)
        (out/'CELL_UNFOLD_METRICS.json').write_text(json.dumps(proof, indent=2)+'\n')
        print(json.dumps(proof))

if __name__ == '__main__':
    main()
