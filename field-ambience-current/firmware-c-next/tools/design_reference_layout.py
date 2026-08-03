"""AMBIENT 320x170 — layout measured from the reference mockup, not invented.

Reference card is 1502 x 970 px; every position below is that measurement
scaled by sx = 320/1502 and sy = 170/970. Comments carry the source number.
"""
from PIL import Image, ImageDraw, ImageFilter
import numpy as np, freetype

W, H = 320, 170
TTF   = "font/Bitcount_Grid_Single/static/BitcountGridSingle_Roman-Regular.ttf"
MINT  = (12, 250, 149)          # sampled from the reference fill
DARK  = (6, 70, 44)             # value text on mint
RIM   = (253, 220, 231)         # sampled card rim

# ---- Bitcount on its grid: monochrome, ppem multiple of 10, blit 1:1 -------
_c = {}
def gl(ppem):
    if ppem in _c: return _c[ppem]
    assert ppem % 10 == 0
    f = freetype.Face(TTF); f.set_pixel_sizes(0, ppem)
    t = {}
    for c in range(32, 127):
        f.load_char(chr(c), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO
                            | freetype.FT_LOAD_MONOCHROME)
        g, bm = f.glyph, f.glyph.bitmap
        t[chr(c)] = ([[ (bm.buffer[r*bm.pitch+(x>>3)]>>(7-(x&7)))&1
                        for x in range(bm.width)] for r in range(bm.rows)],
                     bm.width, bm.rows, g.bitmap_left, g.bitmap_top, g.advance.x>>6)
    _c[ppem] = (t, f.size.ascender>>6)
    return _c[ppem]

def txt(im, x, y, s, ppem, col, a=255):
    t, asc = gl(ppem); px = im.load(); pen = int(x); base = int(y)+asc
    for ch in s:
        bits,w,h,bl,bt,adv = t.get(ch, t[" "])
        for r in range(h):
            yy = base-bt+r
            if not 0 <= yy < im.height: continue
            for c in range(w):
                if bits[r][c]:
                    xx = pen+bl+c
                    if 0 <= xx < im.width:
                        if a >= 255: px[xx,yy] = col
                        else:
                            o = px[xx,yy]; k = a/255
                            px[xx,yy] = tuple(int(o[i]*(1-k)+col[i]*k) for i in range(3))
        pen += adv
    return pen-int(x)
def tw(s, ppem): return len(s)*(ppem*6//10)

# ---- geometry, all derived from the reference ------------------------------
PAD_L   = 23     # ref 126 -> (126-18)*sx
TRACK_R = 209    # ref 1001
LABEL_X = 225    # ref 1076
BAR_H   = 11     # ref 61 * sy
PITCH   = 16     # ref 91 * sy
BAR_Y0  = 55     # ref 335 * sy
HDR_Y   = 15     # ref 111 * sy
TITLE_Y = 34     # ref 242 * sy
PILL_X, PILL_W, PILL_H = 269, 26, 10     # ref 1281 / 124 / 58

# fill fractions measured off the reference mint bands against the 875 px track:
#   Drive 316/875, Echo 635/875, Granular 251/875, Noise/Delay 776/875.
# The reference prints "87%" on a bar that is drawn at 36 % — that inconsistency
# is in the mockup, so the drawn length follows the mockup and the caption with it.
ROWS = [("Drive", .36, True), ("Echo", .73, False), ("Granular", .29, False),
        ("Key", 0, None), ("Noise", .89, False), ("Delay", .89, False)]
SEL_CAPTION = "87%"
KEY_SEG, KEY_ACTIVE = 4, 0     # ref: 4 pills, first one lit

def rr(d, box, r, fill):
    d.rounded_rectangle(box, radius=r, fill=fill)

def render(grad_png="ref_gradient_320.png", rim=True):
    im = Image.open(grad_png).convert("RGB").resize((W, H), Image.LANCZOS)

    # translucent glass tracks -> drawn on an overlay so they lighten the gradient
    ov = Image.new("RGBA", (W, H), (0,0,0,0)); d = ImageDraw.Draw(ov)
    r = BAR_H//2
    for i,(lab,v,sel) in enumerate(ROWS):
        y = BAR_Y0 + i*PITCH
        if sel is None:                                  # segmented (discrete)
            gap = 3
            segw = (TRACK_R-PAD_L - gap*(KEY_SEG-1)) // KEY_SEG
            for k in range(KEY_SEG):
                x = PAD_L + k*(segw+gap)
                rr(d, [x, y, x+segw, y+BAR_H], r, (255,255,255,70))
        else:
            rr(d, [PAD_L, y, TRACK_R, y+BAR_H], r, (255,255,255,70))
    im = Image.alpha_composite(im.convert("RGBA"), ov).convert("RGB")

    # mint glow behind the selected fill (reference: soft halo, no hard edge)
    glow = Image.new("RGB", (W, H), (0,0,0)); gd = ImageDraw.Draw(glow)
    for i,(lab,v,sel) in enumerate(ROWS):
        if sel:
            y = BAR_Y0 + i*PITCH
            fw = int((TRACK_R-PAD_L)*v)
            gd.rounded_rectangle([PAD_L-2, y-2, PAD_L+fw+2, y+BAR_H+2], radius=r+2, fill=MINT)
    glow = glow.filter(ImageFilter.GaussianBlur(2.5))
    im = Image.fromarray(np.clip(np.asarray(im).astype(int)
                                 + np.asarray(glow).astype(int)*0.45, 0, 255).astype(np.uint8))

    d = ImageDraw.Draw(im)
    for i,(lab,v,sel) in enumerate(ROWS):
        y = BAR_Y0 + i*PITCH
        if sel is None:
            gap = 3
            segw = (TRACK_R-PAD_L - gap*(KEY_SEG-1)) // KEY_SEG
            x = PAD_L + KEY_ACTIVE*(segw+gap)
            rr(d, [x, y, x+segw, y+BAR_H], r, MINT)
        else:
            fw = max(BAR_H, int((TRACK_R-PAD_L)*v))
            rr(d, [PAD_L, y, PAD_L+fw, y+BAR_H], r, MINT)
            if sel:                                      # value inside the fill
                s = SEL_CAPTION
                txt(im, PAD_L+fw-tw(s,10)-5, y+2, s, 10, DARK)
        txt(im, LABEL_X, y+2, lab, 10, (255,255,255), 235)

    txt(im, PAD_L, HDR_Y,   "Ambience",      10, (255,255,255), 225)
    txt(im, PAD_L, TITLE_Y, "Crystal Ocean", 20, (255,255,255), 250)
    d.rounded_rectangle([PILL_X, HDR_Y-1, PILL_X+PILL_W, HDR_Y-1+PILL_H],
                        radius=PILL_H//2, fill=MINT)
    txt(im, PILL_X+3, HDR_Y, "100%", 10, DARK)

    if rim:                                              # glass edge, ref ~2 px
        d.rounded_rectangle([1,1,W-2,H-2], radius=13, outline=RIM, width=1)
    return im

if __name__ == "__main__":
    render().save("final_ui.png")
    render(rim=False).save("final_ui_norim.png")
    print("ok")
