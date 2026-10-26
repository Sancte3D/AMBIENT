#!/usr/bin/env python3
"""Bake a native-size PNG into explicit, renderer-neutral C pixel data.

No resize, font substitution, SVG execution or firmware integration. RGB565
endianness and 4-bit row stride are explicit. This is not an LVGL .bin file.
Requires Pillow; intended for offline authoring, never the audio/MCU runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image

def encode(im,fmt):
    im=im.convert('RGBA');w,h=im.size;raw=im.tobytes()
    pixels=[tuple(raw[i:i+4]) for i in range(0,len(raw),4)];data=bytearray()
    if fmt not in ('gray4','alpha4','rgb565le','rgb565be'):
        raise ValueError('unknown format')
    if fmt!='alpha4' and any(p[3]!=255 for p in pixels):
        raise ValueError('opaque asset required; composite deliberately or export an alpha4 mask')
    if fmt=='gray4' and any(r!=g or g!=b for r,g,b,a in pixels):
        raise ValueError('gray4 requires a grayscale source; full color needs RGB565/LVGL')
    stride=(w+1)//2 if fmt.endswith('4') else w*2
    for y in range(h):
        nibbles=[]
        for x in range(w):
            r,g,b,a=pixels[y*w+x]
            if fmt in ('gray4','alpha4'):
                nibbles.append(((a if fmt=='alpha4' else r)*15+127)//255)
            else:
                value=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
                data.extend(value.to_bytes(2,'little' if fmt=='rgb565le' else 'big'))
        for x in range(0,len(nibbles),2):
            data.append((nibbles[x]<<4)|(nibbles[x+1] if x+1<w else 0))
    return bytes(data),stride

def decode(data,w,h,fmt,stride):
    im=Image.new('RGBA',(w,h));out=[]
    for y in range(h):
        for x in range(w):
            if fmt in ('gray4','alpha4'):
                value=(data[y*stride+x//2]>>(4 if x%2==0 else 0))&15
                out.append((255,255,255,value*17) if fmt=='alpha4' else (value*17,)*3+(255,))
            else:
                pos=y*stride+x*2
                c=int.from_bytes(data[pos:pos+2],'little' if fmt=='rgb565le' else 'big')
                r=(c>>11)&31;g=(c>>5)&63;b=c&31
                out.append(((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255))
    im.putdata(out);return im

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source',type=Path);ap.add_argument('output',type=Path)
    ap.add_argument('--name',required=True)
    ap.add_argument('--format',required=True,choices=['gray4','alpha4','rgb565le','rgb565be'])
    ap.add_argument('--size',nargs=2,type=int,required=True,metavar=('WIDTH','HEIGHT'))
    a=ap.parse_args()
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*',a.name):ap.error('name must be a nonreserved C identifier')
    if a.source.suffix.lower()!='.png':ap.error('supply a native-size PNG export')
    if not 1<=a.size[0]<=320 or not 1<=a.size[1]<=170:ap.error('asset must fit the current 320x170 display')
    with Image.open(a.source) as source:
        if source.size!=tuple(a.size):ap.error(f'expected {tuple(a.size)}, got {source.size}; no automatic scaling')
        try:data,stride=encode(source,a.format)
        except ValueError as e:ap.error(str(e))
        preview=decode(data,*a.size,a.format,stride)
    a.output.mkdir(parents=True,exist_ok=True)
    header=a.output/(a.name+'.h')
    lines=['/* Generated offline by prepare_ui_asset.py. Renderer-neutral pixel bytes.',
           f' * Format {a.format}; stride {stride} bytes. No LVGL descriptor/header. */',
           '#pragma once','#include <stdint.h>',
           f'enum {{ {a.name}_width={a.size[0]}, {a.name}_height={a.size[1]}, {a.name}_stride={stride} }};',
           f'static const uint8_t {a.name}_data[{len(data)}] = {{']
    lines += ['    '+','.join(f'0x{x:02x}' for x in data[i:i+20])+',' for i in range(0,len(data),20)]
    lines+=['};',''];header.write_text('\n'.join(lines))
    (a.output/(a.name+'.raw')).write_bytes(data)
    preview.save(a.output/(a.name+'_preview.png'))
    manifest=dict(schema=1,name=a.name,source=a.source.name,width=a.size[0],height=a.size[1],
                  format=a.format,stride_bytes=stride,size_bytes=len(data),
                  source_sha256=hashlib.sha256(a.source.read_bytes()).hexdigest(),
                  data_sha256=hashlib.sha256(data).hexdigest(),
                  nibble_order='left pixel high, each row padded low zero' if a.format.endswith('4') else None,
                  status='prepared asset only; not integrated into current renderer or LVGL')
    (a.output/(a.name+'.json')).write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest))
if __name__=='__main__':main()
