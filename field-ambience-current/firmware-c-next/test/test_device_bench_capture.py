"""The actual read-only capture path: real stack paint ABI, errors and rejects."""
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import device_capture as capture
from test_device_capture import CaptureTest

class BenchCaptureTest(CaptureTest):
    def setUp(self):
        super().setUp()
        self.manifest.update(sound_session_candidate=True,device_bench=dict(address=0x24002000,
            bytes=64,fields=capture.BENCH_FIELDS,stack_low=0x2001c000,stack_high=0x20020000))
        self.registers.update(control=0,msp=self.registers['sp'])
        self.values_bench=[capture.BENCH_MAGIC,1,0x2001c000,0x20020000,0x2001fe00,capture.BENCH_PATTERN,
                           0xfffffff0,100,2,3,5,7,1,2,3,480000000]
        self.stack=bytearray(struct.pack('<I',capture.BENCH_PATTERN)*(0x3e00//4))
        self.stack[0x3800:0x3804]=b'\0'*4

    def read(self,address,n):
        if address==0x24002000:
            self.reads.append((address,n));return struct.pack('<16I',*self.values_bench)
        if address==0x2001c000:
            self.reads.append((address,n));return self.stack
        return super().read(address,n)

    # Base invariants remain covered independently by CaptureTest. The bench
    # observation has two extra reads and a non-null measured watermark.
    def test_observation_is_never_acceptance(self):
        r=self.collect();self.assertFalse(r['device_accepted']);self.assertFalse(r['physical_clock_verified'])
        self.assertEqual(r['stack_high_water_bytes'],2048);self.assertEqual(len(self.reads),6)
        b=r['device_bench'];self.assertTrue(b['audio_error_observed'])
        self.assertEqual(b['raw']['last_dma_error'],5);self.assertEqual(b['raw']['saves_done'],7)
        self.assertEqual(b['observed_main_elapsed_ms'],116)

    def test_paint_exhaustion_is_preserved(self):
        self.stack[:4]=b'\0'*4;r=self.collect()
        self.assertTrue(r['device_bench']['stack_paint_exhausted'])
        self.assertEqual(r['stack_high_water_bytes'],16384);self.assertFalse(r['device_accepted'])

    def test_reject_bad_bench_without_arbitrary_memory_reads(self):
        original=self.values_bench.copy()
        for index,value in [(0,0),(1,2),(2,0),(3,0),(4,0x20030000),(4,0x2001ff04),(5,0),(14,1),(15,0)]:
            self.values_bench=original.copy();self.values_bench[index]=value;self.reads=[]
            with self.subTest(index=index,value=value),self.assertRaises(ValueError):self.collect()
            self.assertNotIn(0x2001c000,[a for a,n in self.reads])
        self.values_bench=original;self.registers['control']=2
        with self.assertRaises(ValueError):self.collect()

    def test_real_bench_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory);code='#include "device_bench.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void){printf("%zu",sizeof(device_bench_t));'
            for field in capture.BENCH_FIELDS:code+='printf(" %zu",offsetof(device_bench_t,'+field+'));'
            (p/'layout.c').write_text(code+'return 0;}')
            subprocess.run(['cc','-std=c11','-I'+str(ROOT/'include'),str(p/'layout.c'),'-o',str(p/'layout')],check=True)
            self.assertEqual(list(map(int,subprocess.check_output([str(p/'layout')],text=True).split())),[64,*range(0,64,4)])

    def test_prepare_requires_bench_audit_and_actual_symbols(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory);image=b'bench-image';elf=b'bench-elf'
            (p/'CMakeCache.txt').write_text('FAM_TARGET:STRING=h743\nFAM_SOUND_PROFILE:STRING=product\n'
                'FAM_SCENE_JOURNAL_CANDIDATE:BOOL=OFF\nFAM_SOUND_SESSION_CANDIDATE:BOOL=ON\nFAM_DEVICE_BENCH_CANDIDATE:BOOL=ON\n')
            (p/'field_ambience_h743').write_bytes(elf);(p/'field_ambience_h743.bin').write_bytes(image)
            audit=dict(profile='product',elf_sha256=capture.sha(elf),sound_session={'format':'SCN8/JSS1'})
            (p/'PRODUCT_BUILD_AUDIT.json').write_text(json.dumps(audit))
            listing=('24001000 00000038 b s_prof\n24001038 00000038 b s_irq_prof\n'
                     '24002000 00000040 B g_device_bench\n2001c000 A __ambient_stack_low\n20020000 A __ambient_stack_high\n')
            def objcopy(command,**kw):Path(command[-1]).write_bytes(image)
            with patch.object(capture.subprocess,'run',side_effect=objcopy),patch.object(capture.subprocess,'check_output',return_value=listing):
                with self.assertRaises(ValueError):capture.prepare(p)
                audit['device_bench']={'schema':1};(p/'PRODUCT_BUILD_AUDIT.json').write_text(json.dumps(audit))
                self.assertEqual(capture.prepare(p)['device_bench']['stack_low'],0x2001c000)

if __name__=='__main__':unittest.main()
