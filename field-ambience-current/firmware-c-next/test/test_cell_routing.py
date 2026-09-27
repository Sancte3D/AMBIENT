#!/usr/bin/env python3
"""Compile actual H743 dispatch/routing functions without hardware registers.
Optional source path permits a red/green test against the original HAL file.
No routing implementation is duplicated or rewritten in the fixture.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else root/'src/hal_h743/main_h743.c'
text = source.read_text()
start = text.find('static void dispatch_cell(')
if start < 0:
    start = text.index('static void route_cell(')
end = text.index('\n/* r19.16', start)
fragment = text[start:end]
assert 'static void route_cell(' in fragment and 'static void hal_set_cell(' in fragment
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    (tmp/'cell_routing_hal.inc').write_text(fragment)
    binary = tmp/'cell_routing'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-O2', '-Wall', '-Wextra',
                    '-Wno-unused-function', '-Wno-unused-const-variable',
                    '-I'+str(root/'include'), '-I'+str(tmp),
                    str(root/'test/test_cell_routing.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
