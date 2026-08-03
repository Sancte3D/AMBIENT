"""Render ONLY the UI layer at native 320x170 as RGBA. Never scaled.
Geometry measured from the reference (card 1502x970), scaled 320/1502 and 170/970."""
from PIL import Image, ImageDraw, ImageFilter
import numpy as np, freetype, sys

W,H=320,170
TTF="font/Bitcount_Grid_Single/static/BitcountGridSingle_Roman-Regular.ttf"
MINT=(12,250,149); DARK=(6,70,44); RIM=(253,220,231)
PAD_L,TRACK_R,LABEL_X=23,209,225
BAR_H,PITCH,BAR_Y0=11,16,55
HDR_Y,TITLE_Y=15,34
PILL_X,PILL_W,PILL_H=269,26,10
ROWS=[("Drive",.36,True),("Echo",.73,False),("Granular",.29,False),
      ("Key",0,None),("Noise",.89,False),("Delay",.89,False)]
SEL_CAPTION="87%"; KEY_SEG,KEY_ACTIVE=4,0

_c={}
def gl(p):
    if p in _c: return _c[p]
    assert p%10==0
    f=freetype.Face(TTF); f.set_pixel_sizes(0,p); t={}
    for c in range(32,127):
        f.load_char(chr(c), freetype.FT_LOAD_RENDER|freetype.FT_LOAD_TARGET_MONO|freetype.FT_LOAD_MONOCHROME)
        g,bm=f.glyph,f.glyph.bitmap
        t[chr(c)]=([[(bm.buffer[r*bm.pitch+(x>>3)]>>(7-(x&7)))&1 for x in range(bm.width)]
                    for r in range(bm.rows)],bm.width,bm.rows,g.bitmap_left,g.bitmap_top,g.advance.x>>6)
    _c[p]=(t,f.size.ascender>>6); return _c[p]

def txt(im,x,y,s,p,col,a=255):
    t,asc=gl(p); px=im.load(); pen=int(x); base=int(y)+asc
    for ch in s:
        bits,w,h,bl,bt,adv=t.get(ch,t[" "])
        for r in range(h):
            yy=base-bt+r
            if not 0<=yy<im.height: continue
            for c in range(w):
                if bits[r][c]:
                    xx=pen+bl+c
                    if 0<=xx<im.width: px[xx,yy]=col+(a,)
        pen+=adv
    return pen-int(x)
def tw(s,p): return len(s)*(p*6//10)

im=Image.new("RGBA",(W,H),(0,0,0,0)); d=ImageDraw.Draw(im); r=BAR_H//2

# glass tracks — translucent white, gradient reads through
for i,(lab,v,sel) in enumerate(ROWS):
    y=BAR_Y0+i*PITCH
    if sel is None:
        gap=3; segw=(TRACK_R-PAD_L-gap*(KEY_SEG-1))//KEY_SEG
        for k in range(KEY_SEG):
            x=PAD_L+k*(segw+gap)
            d.rounded_rectangle([x,y,x+segw,y+BAR_H],radius=r,fill=(255,255,255,70))
    else:
        d.rounded_rectangle([PAD_L,y,TRACK_R,y+BAR_H],radius=r,fill=(255,255,255,70))

# mint halo on the selected row, built as its own soft layer
halo=Image.new("RGBA",(W,H),(0,0,0,0)); hd=ImageDraw.Draw(halo)
for i,(lab,v,sel) in enumerate(ROWS):
    if sel:
        y=BAR_Y0+i*PITCH; fw=int((TRACK_R-PAD_L)*v)
        hd.rounded_rectangle([PAD_L-2,y-2,PAD_L+fw+2,y+BAR_H+2],radius=r+2,fill=MINT+(190,))
halo=halo.filter(ImageFilter.GaussianBlur(2.5))
im=Image.alpha_composite(halo,im); d=ImageDraw.Draw(im)

for i,(lab,v,sel) in enumerate(ROWS):
    y=BAR_Y0+i*PITCH
    if sel is None:
        gap=3; segw=(TRACK_R-PAD_L-gap*(KEY_SEG-1))//KEY_SEG
        x=PAD_L+KEY_ACTIVE*(segw+gap)
        d.rounded_rectangle([x,y,x+segw,y+BAR_H],radius=r,fill=MINT+(255,))
    else:
        fw=max(BAR_H,int((TRACK_R-PAD_L)*v))
        d.rounded_rectangle([PAD_L,y,PAD_L+fw,y+BAR_H],radius=r,fill=MINT+(255,))
        if sel: txt(im,PAD_L+fw-tw(SEL_CAPTION,10)-5,y+2,SEL_CAPTION,10,DARK)
    txt(im,LABEL_X,y+2,lab,10,(255,255,255),235)

txt(im,PAD_L,HDR_Y,"Ambience",10,(255,255,255),225)
txt(im,PAD_L,TITLE_Y,"Crystal Ocean",20,(255,255,255),250)
d.rounded_rectangle([PILL_X,HDR_Y-1,PILL_X+PILL_W,HDR_Y-1+PILL_H],radius=PILL_H//2,fill=MINT+(255,))
txt(im,PILL_X+3,HDR_Y,"100%",10,DARK)
d.rounded_rectangle([1,1,W-2,H-2],radius=13,outline=RIM+(150,),width=1)
im.save(sys.argv[1] if len(sys.argv)>1 else "ui_overlay.png")
print("UI-Overlay nativ 320x170 geschrieben")
