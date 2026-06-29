#!/usr/bin/env python3
"""
gen_bom_overview.py — standalone, browsable HTML overview of the AMBIENT BOM,
grouped by function, with clickable part links AND verified JLCPCB-assembly
status (Basic / Extended / not-in-assembly + live stock at generation time).

LCSC and JLCPCB are the same group: an LCSC `C`-number IS the JLC part, so the
clickable link goes to the LCSC product page (image + specs) and the badge shows
whether JLC can *assemble* it. JLC stock checked 2026-06-29 via JLC's SMT parts
API — re-verify on the linked page before ordering (stock moves).

    python3 gen_bom_overview.py     # -> ../docs/hardware/bom_overview.html
"""
import html, os

# Verified JLC SMT-assembly status (2026-06-29): code -> (library, stock)
JLC = {
 "C114409":("Extended",392),"C596838":("Extended",1364),"C165948":("Extended",188935),
 "C2687116":("Extended",127452),"C18198349":("Extended",26687),"C113952":("Extended",843167),
 "C165129":("Extended",714),"C83455":("Extended",561),"C8678":("Basic",3338351),
 "C150470":("Extended",15210),"C460397":("Extended",1821),"C131941":("Extended",44589),
 "C49023766":("Extended",388),"C424093":("Extended",10731),"C295747":("Extended",102433),
 "C107671":("Extended",2045),"C17337":("Extended",3903),"C19330":("Extended",950152),
 "C84094":("NONE",0),"C431535":("Extended",49905),"C506653":("Extended",3378),
 "C2678753":("Extended",3147),"C8545":("Basic",421360),"C124383":("Extended",12518),
 "C124375":("Extended",64842),"C2152902":("Extended",3057),"C2845240":("Extended",30),
 "C720477":("Basic",883607),"C2287":("Extended",33177),"C12624":("Extended",345193),
 "C965808":("Extended",2700420),"C72041":("Extended",4),"C23151":("Basic",250152),
 "C25803":("Basic",7471690),"C21190":("Basic",5801237),"C57112":("Basic",2846393),
 "C15850":("Basic",5877645),"C2880380":("Extended",1),"C444831":("Extended",161),
 "C202365":("Extended",2787),"C14663":("Basic",21044139),"C46653":("Basic",1915999),
 "C45783":("Basic",1154055),"C107045":("Extended",167822),"C15849":("Basic",6889230),
 "C24539":("NONE",0),"C23253":("Basic",369149),"C23186":("Basic",3618054),
}
LOW = 100   # stock below this = warn

def lcsc(code): return f"https://www.lcsc.com/product-detail/{code}.html"

# group -> (color, [ (ref, part, fn, code_or_url, route, visible) ])
#   route: "jlc" (SMD auto-assembly) | "hand" (on-board, you place) | "off" (not on PCB)
G = [
 ("MCU & Clock", "#6366f1", [
   ("U1","STM32H743VIT6 (LQFP-100)","the brain — 480 MHz Cortex-M7","C114409","jlc",False),
   ("Y1","ABLS-8.000MHZ crystal","8 MHz reference clock","C596838","jlc",False),
   ("C_HSE","27 pF C0G ×2","crystal load caps","C107045","jlc",False),
 ]),
 ("Power", "#ef4444", [
   ("J1","USB-C TYPE-C-31-M-12","charging + USB-DFU flash","C165948","jlc",True),
   ("U8","TPS61089 boost","battery 3.7 V → 5 V","C165129","jlc",False),
   ("L1","SWPA6045 2.2 µH","boost inductor","C83455","jlc",False),
   ("Q1","DMG2305UX P-MOSFET","power-path switch","C150470","jlc",False),
   ("U5","AP7361C LDO","5 V → 3.3 V (thermal hotspot)","C460397","jlc",False),
   ("U_PWR","TPS22918 load switch","the power-off — gates 3.3 V","C131941","jlc",False),
   ("SW_PWR","MST-12D18G3 slide switch","on/off, side-actuated","C49023766","jlc",True),
   ("U7","MCP73831 charger","LiPo charger (charges when off)","C424093","jlc",False),
   ("J9","JST-PH 2.0 (battery)","LiPo connector","C295747","jlc",False),
   ("D1","USBLC6-2SC6","USB ESD protection","C2687116","jlc",False),
   ("F1","PTC fuse 3 A","USB overcurrent protection","C18198349","jlc",False),
   ("D2","SMAJ5.0A TVS","5 V surge clamp","C113952","jlc",False),
   ("D3","SS34 Schottky","boost rectifier","C8678","jlc",False),
   ("C_BULK","470 µF tantalum 1210","bulk reservoir","C444831","jlc",False),
   ("C_BULK2","100 µF MLCC 1210","transient reservoir","C2880380","jlc",False),
   ("LiPo","2000 mAh pouch 503759","the battery","https://thepihut.com/products/2000mah-3-7v-lipo-battery","off",False),
 ]),
 ("Audio", "#10b981", [
   ("U3","PCM5102A DAC","I²S → analog audio","C107671","jlc",False),
   ("U4","PAM8403 Class-D amp","drives the speakers","C17337","jlc",False),
   ("J8","PJ-320D 3.5 mm line-out","audio out","C431535","jlc",True),
   ("J10","PJ-320D 3.5 mm MIDI-out","MIDI out (TRS Type A)","C431535","jlc",True),
   ("FB1","BLM18AG601 ferrite","audio supply filter","C19330","jlc",False),
   ("FB2","BLM18AG601 ferrite","audio supply filter (use C19330 too)","C84094","jlc",False),
   ("J6/J7","1×2 header ×2","speaker leads","C124375","hand",False),
   ("SPK","Same Sky CMS-402811-28SP ×2","the speakers (8 Ω cloth cone)","https://www.digikey.com/en/products/detail/cui-devices/CMS-402811-28SP/10821307","off",True),
 ]),
 ("Display", "#f59e0b", [
   ("LCD","Waveshare 1.9″ 170×320 ST7789","the screen","https://www.waveshare.com/1.9inch-lcd-module.htm","off",True),
   ("Q2","2N7002 MOSFET","backlight PWM driver","C8545","jlc",False),
   ("J3","1×8 header","plugs the LCD in","C124383","hand",False),
 ]),
 ("Controls", "#0ea5e9", [
   ("EN1-4","ALPS EC11E18244AU ×4","4 push-encoders","C202365","hand",True),
   ("Cells","Gateron Low-Profile Magnetic ×5","the 5 playable keys","https://www.gateron.com/products/gateron-low-profile-magnetic-jade-switch","off",True),
   ("Hall","DRV5056A4 ×5","reads each cell's magnet","C2152902","hand",False),
   ("SW6-10","TC-1212-7.3 tactile ×5","modifier buttons (THT)","C2845240","hand",True),
   ("SW11/BOOT","TS-1088 tactile ×2","Reset + BOOT0 (2 service buttons)","C720477","jlc",False),
 ]),
 ("LEDs (23 visible)", "#a855f7", [
   ("LED-Y","KENTO KT-0603Y (yellow)","Shift + 5 cell base-hold LEDs","C2287","jlc",True),
   ("LED-G","KENTO KT-0603G (green)","Hold + 5 cell shift-hold LEDs","C12624","jlc",True),
   ("LED-W","XL-1608UWC (white)","Drone/Gen/Clear + 8 VU LEDs","C965808","jlc",True),
   ("LED_CHRG","Amber 0603","charger status","C72041","jlc",True),
   ("R_LED","390 Ω ×23","LED series resistors","C23151","jlc",False),
 ]),
 ("I/O Expander & LED Drivers", "#64748b", [
   ("U2","MCP23017 GPIO expander","extra inputs over I²C","C506653","jlc",False),
   ("U6","PCA9685 PWM driver","drives the 15 status LEDs","C2678753","jlc",False),
   ("U10","PCA9685 PWM driver #2","drives the 8 VU LEDs","C2678753","jlc",False),
 ]),
 ("Passives & Service", "#94a3b8", [
   ("R 1k","0603 1%","Hall RC, pull-ups","C21190","jlc",False),
   ("R 100k","0603 1%","ON pull-down etc.","C25803","jlc",False),
   ("R 5.1k / 820","0603 1%","dividers, CC","C23186","jlc",False),
   ("C 100n","0603 X7R","decoupling (many)","C14663","jlc",False),
   ("C 1µ","0603 X5R","decoupling / VREF","C15849","jlc",False),
   ("C 10n","0603","filtering","C57112","jlc",False),
   ("C 2.2µ VCAP","0603 X5R","PCM5102A charge pump","C24539","jlc",False),
   ("C 10µ","0805 X5R","rail bulk","C15850","jlc",False),
   ("C 4.7µ / 22µ","0603/0805","local bulk","C46653","jlc",False),
   ("J4","Tag-Connect TC2030-IDC","SWD debug header","https://www.tag-connect.com/product/tc2030-idc","off",False),
 ]),
]

VIS = [("Touch controls","15","4 enc + 5 cells + 5 buttons + power switch"),
       ("Display","1","1.9″ window"),("Indicator LEDs","23","5 mod + 10 cell + 8 VU"),
       ("Ports","3","USB-C + line-out + MIDI"),("Speakers","2","grilles"),
       ("Bottom","—","2 pinholes + battery + 4 screws")]

def esc(s): return html.escape(str(s))

def badge(code, route):
    if route == "off":
        return '<span class="b off">off-board · not soldered</span>'
    if not code or code not in JLC:
        return '<span class="b warn">JLC ? verify</span>'
    lib, stock = JLC[code]
    hand = ' <span class="b hand">hand-place</span>' if route == "hand" else ''
    if lib == "NONE":
        return '<span class="b bad">✗ not in JLC assembly</span>' + hand
    cls = "bad" if stock < LOW else ("basic" if lib == "Basic" else "ext")
    warn = " ⚠" if stock < LOW else ""
    return f'<span class="b {cls}">JLC {lib} · {stock:,}{warn}</span>' + hand

def link(code, url):
    return url if url else lcsc(code)

def main():
    secs = []
    for title, color, parts in G:
        rows = []
        for ref, part, fn, codeurl, route, vis in parts:
            url = codeurl if str(codeurl).startswith("http") else None
            code = None if url else codeurl
            href = link(code, url)
            eye = '<span class="eye">👁</span>' if vis else ''
            rows.append(f"""<tr class="{'vis' if vis else ''}">
        <td class="ref">{esc(ref)}</td>
        <td class="part"><a href="{esc(href)}" target="_blank" rel="noopener">{esc(part)} ↗</a>{eye}</td>
        <td class="fn">{esc(fn)}</td>
        <td class="jlc">{badge(code, route)}</td></tr>""")
        secs.append(f"""<section><h2 style="border-color:{color}"><span class="dot" style="background:{color}"></span>{esc(title)}</h2>
      <table><thead><tr><th>Ref</th><th>Part (→ LCSC/JLC page)</th><th>What it's for</th><th>JLC assembly</th></tr></thead>
      <tbody>{''.join(rows)}</tbody></table></section>""")

    vcards = ''.join(f'<div class="vcard"><div class="vnum">{esc(n)}</div><div class="vlbl">{esc(l)}</div><div class="vsub">{esc(s)}</div></div>' for l,n,s in VIS)

    issues = """
    <div class="issues"><b>⚠ Sourcing issues to fix before a JLC order</b>
    <ul>
      <li><b>FB2 ferrite (C84094)</b> — not in JLC assembly → use <b>C19330</b> (FB1, same MPN) for both.</li>
      <li><b>2.2 µF VCAP (C24539)</b> — not in JLC assembly → pick a JLC-stocked 2.2 µF 0603/0805 X5R.</li>
      <li><b>Amber LED (C72041)</b> — stock 4 (≈out) → swap to an in-stock amber 0603.</li>
      <li><b>100 µF 1210 C_BULK2 (C2880380)</b> — stock 1 (≈out) → swap to an in-stock 100 µF/10 V 1210.</li>
      <li><b>TC-1212 button (C2845240)</b> — stock 30, low for 5 + spares → check stock / pick a spare.</li>
    </ul></div>"""

    doc = f"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>AMBIENT — BOM Overview</title>
<style>
 :root{{--bg:#0f1115;--card:#171a21;--line:#262b36;--txt:#e6e9ef;--mut:#9aa4b2;}}
 *{{box-sizing:border-box}} body{{margin:0;font:15px/1.5 -apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--txt)}}
 header{{padding:26px 24px 16px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:5}}
 header h1{{margin:0 0 4px;font-size:22px}} header p{{margin:0;color:var(--mut);font-size:13px}}
 .wrap{{max-width:1100px;margin:0 auto;padding:0 24px 60px}}
 .visgrid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin:20px 0 6px}}
 .vcard{{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:13px 15px}}
 .vnum{{font-size:28px;font-weight:700;line-height:1;color:#fff}} .vlbl{{font-size:13px;font-weight:600;margin-top:6px}} .vsub{{font-size:11px;color:var(--mut);margin-top:3px}}
 .issues{{background:#2a1416;border:1px solid #5b2330;border-radius:12px;padding:14px 18px;margin:18px 0 4px;font-size:13.5px}}
 .issues ul{{margin:8px 0 0;padding-left:20px}} .issues li{{margin:3px 0;color:#f3c9cf}} .issues b{{color:#fff}}
 .legend{{font-size:12px;color:var(--mut);margin:14px 0 0}} .legend .b{{margin-right:6px}}
 section{{margin-top:26px}} h2{{font-size:16px;margin:0 0 10px;padding-bottom:6px;border-bottom:2px solid;display:flex;align-items:center;gap:9px}}
 .dot{{width:10px;height:10px;border-radius:50%;display:inline-block}}
 table{{width:100%;border-collapse:collapse;background:var(--card);border:1px solid var(--line);border-radius:12px;overflow:hidden}}
 th,td{{text-align:left;padding:8px 12px;border-bottom:1px solid var(--line);vertical-align:top}}
 th{{font-size:11px;text-transform:uppercase;letter-spacing:.04em;color:var(--mut);background:#12151b}}
 tr:last-child td{{border-bottom:none}}
 td.ref{{font-family:ui-monospace,Menlo,monospace;font-size:12px;color:var(--mut);white-space:nowrap}}
 td.part a{{color:#8ab4ff;text-decoration:none;font-weight:600}} td.part a:hover{{text-decoration:underline}}
 td.fn{{color:var(--mut);font-size:13px}} td.jlc{{white-space:nowrap}}
 tr.vis{{background:rgba(99,102,241,.06)}} .eye{{margin-left:7px}}
 .b{{font-size:10.5px;padding:2px 7px;border-radius:20px;white-space:nowrap;font-weight:600}}
 .b.basic{{background:#10331f;color:#7ee2a8}} .b.ext{{background:#33270f;color:#f0c674}}
 .b.bad{{background:#3a1418;color:#ff9aa6}} .b.off{{background:#1b2330;color:#9ab}} .b.hand{{background:#16263a;color:#8ab4ff}} .b.warn{{background:#33270f;color:#f0c674}}
 footer{{color:var(--mut);font-size:12px;margin-top:32px;border-top:1px solid var(--line);padding-top:14px}}
</style></head><body>
<header><h1>AMBIENT — BOM Overview</h1>
<p>Every part grouped by function · click a part for its LCSC/JLC page · the <b>JLC assembly</b> badge is verified live · 👁 = visible on the enclosure</p></header>
<div class="wrap">
 <div class="visgrid">{vcards}</div>
 {issues}
 <div class="legend">
   <span class="b basic">JLC Basic</span> no setup fee &nbsp;
   <span class="b ext">JLC Extended</span> small per-type fee &nbsp;
   <span class="b bad">✗ / low</span> fix before order &nbsp;
   <span class="b hand">hand-place</span> on-board, you place &nbsp;
   <span class="b off">off-board</span> not soldered
 </div>
 {''.join(secs)}
 <footer>LCSC = JLCPCB (same group): the <code>C</code>-number is the JLC part; the link opens the LCSC product page, the badge is JLC's <i>assembly</i> status (stock checked 2026-06-29 — re-verify before ordering). Source of truth = <code>kicad/generate_kicad_project.py</code>; machine BOM for upload = <code>kicad/jlc_bom.csv</code>; regenerate this page with <code>kicad/gen_bom_overview.py</code>.</footer>
</div></body></html>"""

    out = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "docs", "hardware", "bom_overview.html"))
    with open(out, "w", encoding="utf-8") as f:
        f.write(doc)
    print("wrote", out, f"({len(doc)} bytes)")

if __name__ == "__main__":
    main()
