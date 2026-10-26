"""Pixel contracts: actual export bytes, row edges, color order and rejection."""
import importlib.util
from pathlib import Path
import unittest
import subprocess
import sys
import tempfile
from PIL import Image
spec=importlib.util.spec_from_file_location('assets',Path(__file__).resolve().parents[1]/'tools/prepare_ui_asset.py')
assets=importlib.util.module_from_spec(spec);spec.loader.exec_module(assets)

class PixelContract(unittest.TestCase):
    def test_565_wire_order(self):
        im=Image.new('RGB',(5,1));colors=[(255,0,0),(0,255,0),(0,0,255),(255,255,255),(0,0,0)]
        im.putdata(colors)
        for fmt,wanted in [('rgb565be',bytes.fromhex('f800 07e0 001f ffff 0000')),
                           ('rgb565le',bytes.fromhex('00f8 e007 1f00 ffff 0000'))]:
            data,stride=assets.encode(im,fmt);self.assertEqual(data,wanted);self.assertEqual(stride,10)
            self.assertEqual(assets.decode(data,5,1,fmt,stride).convert('RGB').tobytes(),bytes(v for p in colors for v in p))
    def test_gray_odd_rows(self):
        im=Image.new('L',(3,2));im.putdata([0,17,34,221,238,255])
        data,stride=assets.encode(im,'gray4');self.assertEqual(data,bytes.fromhex('01 20 de f0'));self.assertEqual(stride,2)
        self.assertEqual(assets.decode(data,3,2,'gray4',stride).convert('L').tobytes(),im.tobytes())
    def test_alpha_and_rejections(self):
        im=Image.new('RGBA',(2,1));im.putdata([(255,0,0,0),(0,0,255,255)])
        data,stride=assets.encode(im,'alpha4');self.assertEqual(data,b'\x0f')
        self.assertEqual(assets.decode(data,2,1,'alpha4',stride).tobytes(),bytes([255,255,255,0,255,255,255,255]))
        for fmt in ('gray4','rgb565le','rgb565be'):
            with self.assertRaises(ValueError):assets.encode(im,fmt)
        with self.assertRaises(ValueError):assets.encode(Image.new('RGB',(1,1),(255,0,0)),'gray4')
    def test_native_frame_size(self):
        im=Image.new('L',(320,170))
        self.assertEqual(len(assets.encode(im,'gray4')[0]),27200)
        self.assertEqual(len(assets.encode(im,'rgb565le')[0]),108800)
    def test_cli_and_generated_c(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);im=Image.new('RGB',(3,2),(255,0,0));im.save(p/'source.png')
            cmd=[sys.executable,str(assets.__file__),str(p/'source.png'),str(p/'out'),
                 '--name','sample','--format','rgb565be','--size','3','2']
            subprocess.run(cmd,check=True,capture_output=True)
            (p/'out/check.c').write_text('#include "sample.h"\nint main(void){return sample_width != 3 || sample_height != 2 || sizeof(sample_data) != 12 || sample_data[0] != 0xf8 || sample_data[1] != 0;}\n')
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p/'out/check.c'),'-o',str(p/'check')],check=True)
            subprocess.run([str(p/'check')],check=True)
            cmd[-1]='3'
            self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0)
if __name__=='__main__':unittest.main()
