#!/usr/bin/env python3
"""Native startup paint/actual HAL error route plus read-only capture guards."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--sanitize',action='store_true');args=ap.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    paths=['src/device_bench.c','include/device_bench.h','src/scenes_save_async.c','src/hal_h743/main_h743.c',
        'src/hal_h743/audio_h743.c','src/hal_h743/stm32h743_flash.ld','CMakeLists.txt',
        'tools/device_capture.py','tools/device_capture_gdb.py','tools/check_product_link.py',
        'tools/review_device_bench.py','test/test_device_bench.c','test/test_device_bench_capture.py','test/test_device_capture.py']
    def hashes():return {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    before=hashes();audio=(ROOT/'src/hal_h743/audio_h743.c').read_text()
    start=audio.index('void HAL_SAI_ErrorCallback(');end=audio.index('\n}',start)+2
    main=(ROOT/'src/hal_h743/main_h743.c').read_text();boot=main[main.index('int main(void)'):]
    assert boot.index('device_bench_init')<boot.index('HAL_Init()')<boot.index('audio_init()')
    frontend=(ROOT/'src/scenes_save_async.c').read_text()
    assert frontend.index('info=(scenes_save_info_t){.status=SCENES_SAVE_BUSY')<frontend.index('device_bench_save_begin();')<frontend.index('return true;',frontend.index('device_bench_save_begin();'))
    logs=[]
    with tempfile.TemporaryDirectory(prefix='device-bench-') as folder:
        temp=Path(folder);(temp/'device_bench_callback.inc').write_text(audio[start:end]+'\n')
        for sanitize in ([False,True] if args.sanitize else [False]):
            flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_DEVICE_BENCH_CANDIDATE',
                '-DDEVICE_BENCH_ROUTING_FIXTURE','-I'+str(ROOT/'include'),'-I'+str(temp)]
            if sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
            subprocess.run([*flags,str(ROOT/'test/test_device_bench.c'),str(ROOT/'src/device_bench.c'),'-o',str(temp/'test')],check=True)
            r=subprocess.check_output([str(temp/'test')],text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            logs.append(dict(sanitized=sanitize,output=r.strip()))
        subprocess.run(['python3',str(ROOT/'test/test_device_bench_capture.py')],check=True)
    assert before==hashes(),'Bench sources changed during proof'
    manifest=dict(status='passed',inputs_sha256=before,logs=logs,candidate_default=False,
        device_accepted=False,stack_method='Startup MSP write watermark; not minimum SP or per-context reserve',
        capture_read_only=True,physical_clock_verified=False)
    (out/'DEVICE_BENCH_METRICS.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(status='passed',native_and_sanitized=args.sanitize,capture_guards='passed')))
if __name__=='__main__':main()
