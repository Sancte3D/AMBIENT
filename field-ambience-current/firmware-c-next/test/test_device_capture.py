"""Offline capture guards, real header ABI and the actual GDB command shim."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import device_capture as capture

class CaptureTest(unittest.TestCase):
    def setUp(self):
        self.image=b'fixture-flash'
        self.manifest=dict(schema=1,profile='product',journal_candidate=False,
            image_bytes=len(self.image),image_sha256=capture.sha(self.image),
            elf_sha256='fixture-elf',flash_address=0x08000000,fields=capture.FIELDS,
            symbols={n:dict(address=0x24001000+i*56,bytes=56)
                     for i,n in enumerate(('s_prof','s_irq_prof'))})
        self.values=[480000000,44100,512*480000000//44100,100,0,1000,2000,
                     512,100,0,0,0,.01,.02]
        self.registers=dict(pc=0x08001000,sp=0x2001ff00,xpsr=0x01000000)
        self.reads=[]

    def read(self,address,n):
        self.reads.append((address,n))
        if address==0x08000000:return self.image
        if address==0xe0001000:return b'\1\0\0\0'
        return struct.pack('<12I2f',*self.values)

    def collect(self):
        return capture.collect(self.manifest,self.read,self.registers,'test')

    def test_observation_is_never_acceptance(self):
        r=self.collect()
        self.assertFalse(r['device_accepted']);self.assertIsNone(r['stack_high_water_bytes'])
        self.assertFalse(r['physical_clock_verified']);self.assertEqual(len(self.reads),4)

    def test_bad_performance_is_preserved(self):
        self.values[9]=3;self.values[13]=.8
        r=self.collect();self.assertTrue(r['scope_overrun_observed'])
        self.assertTrue(r['scope_peak_at_or_above_60_percent']);self.assertFalse(r['device_accepted'])

    def test_reject_wrong_image(self):
        self.manifest['image_sha256']='wrong'
        with self.assertRaises(ValueError):self.collect()

    def test_reject_irq_even_when_in_flight_cleared(self):
        self.registers['xpsr']|=16
        with self.assertRaises(ValueError):self.collect()
        self.assertEqual(self.reads,[])

    def test_reject_bad_configuration_layout_or_range(self):
        original=copy.deepcopy(self.manifest)
        for change in [dict(profile='reference'),dict(journal_candidate=True),
                       dict(fields=[]),dict(image_bytes=2**21),dict(flash_address=0)]:
            self.manifest=copy.deepcopy(original);self.manifest.update(change)
            with self.subTest(change=change),self.assertRaises(ValueError):self.collect()
        self.manifest=original
        self.manifest['symbols']['s_irq_prof']['address']=0x24001000
        with self.assertRaises(ValueError):self.collect()

    def test_profiler_rejections(self):
        for index,value in [(0,0),(1,0),(2,0),(4,1),(8,0),(12,float('nan')),(13,-1)]:
            values=self.values.copy();values[index]=value
            with self.subTest(index=index),self.assertRaises(ValueError):
                capture.decode(struct.pack('<12I2f',*values))
        with self.assertRaises(ValueError):capture.decode(b'\0'*55)

    def test_real_header_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)
            code='#include "audio_profiler.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void){printf("%zu",sizeof(audio_profiler_t));'
            for field in capture.FIELDS:code+='printf(" %zu",offsetof(audio_profiler_t,'+field+'));'
            (p/'layout.c').write_text(code+'return 0;}')
            subprocess.run(['cc','-std=c11','-I'+str(ROOT/'include'),str(p/'layout.c'),'-o',str(p/'layout')],check=True)
            self.assertEqual([int(x) for x in subprocess.check_output([str(p/'layout')],text=True).split()],
                             [56,*range(0,56,4)])

    def test_package_rejects_mismatched_build_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)
            cache='FAM_TARGET:STRING=h743\nFAM_SOUND_PROFILE:STRING=product\nFAM_SCENE_JOURNAL_CANDIDATE:BOOL=OFF\n'
            elf=b'fixture-elf';image=b'fixture-bin'
            audit=dict(profile='product',elf_sha256=capture.sha(elf))
            listing='24001000 00000038 b s_prof\n24001038 00000038 b s_irq_prof\n'
            def objcopy(command,**kw):Path(command[-1]).write_bytes(image)
            def original():
                (p/'CMakeCache.txt').write_text(cache)
                (p/'field_ambience_h743').write_bytes(elf)
                (p/'field_ambience_h743.bin').write_bytes(image)
                (p/'PRODUCT_BUILD_AUDIT.json').write_text(json.dumps(audit))
            original()
            with patch.object(capture.subprocess,'run',side_effect=objcopy),patch.object(
                    capture.subprocess,'check_output',return_value=listing):
                self.assertEqual(capture.prepare(p)['image_sha256'],capture.sha(image))
                mutations=[lambda:(p/'CMakeCache.txt').write_text(cache.replace('product','reference')),
                    lambda:(p/'CMakeCache.txt').write_text(cache.replace('OFF','ON')),
                    lambda:(p/'field_ambience_h743.bin').write_bytes(b'wrong-bin'),
                    lambda:(p/'PRODUCT_BUILD_AUDIT.json').write_text('{}')]
                for mutation in mutations:
                    original();mutation()
                    with self.assertRaises(ValueError):capture.prepare(p)

    def test_actual_gdb_command_is_read_only_and_exclusive(self):
        commands=[];state=dict(stopped=True)
        class Command:
            def __init__(self,*args):commands.append(self)
        gdb=types.SimpleNamespace(Command=Command,COMMAND_USER=0,GdbError=ValueError,error=RuntimeError,
            string_to_argv=lambda arg:json.loads(arg),write=lambda text:None,
            parse_and_eval=lambda arg:self.registers[arg[1:]],
            selected_inferior=lambda:types.SimpleNamespace(
                threads=lambda:[types.SimpleNamespace(is_stopped=lambda:state['stopped'])],read_memory=self.read))
        spec=importlib.util.spec_from_file_location('capture_gdb_test',ROOT/'tools/device_capture_gdb.py')
        module=importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules,{'gdb':gdb}):
            spec.loader.exec_module(module)
            self.assertEqual(self.reads,[]) # sourcing only defines the command
            with tempfile.TemporaryDirectory() as directory:
                p=Path(directory);manifest=p/'manifest.json';out=p/'snapshot.json'
                manifest.write_text(json.dumps(self.manifest));args=json.dumps([str(manifest),str(out),'test'])
                state['stopped']=False
                with self.assertRaises(ValueError):commands[0].invoke(args,False)
                self.assertFalse(out.exists());self.assertEqual(self.reads,[])
                state['stopped']=True;commands[0].invoke(args,False)
                self.assertFalse(json.loads(out.read_text())['device_accepted'])
                original=out.read_bytes()
                with self.assertRaises(ValueError):commands[0].invoke(args,False)
                self.assertEqual(original,out.read_bytes())

if __name__=='__main__':unittest.main()
