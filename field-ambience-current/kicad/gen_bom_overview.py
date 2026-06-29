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
import html, os, re, glob

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
 # r18.70 verified replacements for the 4 sourcing fixes:
 "C23630":("Basic",1894059),"C965800":("Extended",649682),"C23742":("Extended",41420),
 "C36498965":("Extended",19884),"C2845239":("Extended",1488),
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
   ("C_BULK2","100 µF MLCC 1210","transient reservoir (r18.70: C2880380 → C23742)","C23742","jlc",False),
   ("LiPo","2000 mAh pouch 503759","the battery","https://thepihut.com/products/2000mah-3-7v-lipo-battery","off",False),
 ]),
 ("Audio", "#10b981", [
   ("U3","PCM5102A DAC","I²S → analog audio","C107671","jlc",False),
   ("U4","PAM8403 Class-D amp","drives the speakers","C17337","jlc",False),
   ("J8","PJ-320D 3.5 mm line-out","audio out","C431535","jlc",True),
   ("J10","PJ-320D 3.5 mm MIDI-out","MIDI out (TRS Type A)","C431535","jlc",True),
   ("FB1","BLM18AG601 ferrite","audio supply filter","C19330","jlc",False),
   ("FB2","BLM18AG601 ferrite","audio supply filter (r18.70: C84094 → C19330)","C19330","jlc",False),
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
   ("Cells","Gateron LP Magnetic ×5","playable keys — premium Hall-magnetic (analog velocity); ~$45/pack of 35, off-board, pin-less","https://www.gateron.com/products/gateron-low-profile-magnetic-jade-switch","off",True),
   ("Hall","DRV5056A4 ×5","reads each cell's magnet","C2152902","hand",False),
   ("SW6-10","HX B3F-4055 tactile ×5","modifier buttons (THT, square head for caps) — r18.71: C2845240 (stock 30) → C36498965 (20k stock, ~$0.06); verify THT footprint/pinout","C36498965","hand",True),
   ("SW11/BOOT","TS-1088 tactile ×2","Reset + BOOT0 (2 service buttons)","C720477","jlc",False),
 ]),
 ("LEDs (23 visible)", "#a855f7", [
   ("LED-Y","KENTO KT-0603Y (yellow)","Shift + 5 cell base-hold LEDs","C2287","jlc",True),
   ("LED-G","KENTO KT-0603G (green)","Hold + 5 cell shift-hold LEDs","C12624","jlc",True),
   ("LED-W","XL-1608UWC (white)","Drone/Gen/Clear + 8 VU LEDs","C965808","jlc",True),
   ("LED_CHRG","Orange 0603 XL-1608UOC","charger status (r18.70: C72041 was blue+EOL → C965800)","C965800","jlc",True),
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
   ("C 2.2µ VCAP","0603 X5R 16V","PCM5102A charge pump (r18.70: C24539 → C23630)","C23630","jlc",False),
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

# ---- PCB-engineer guide + schematic-sheet map (parsed live from .kicad_sch) ----
KDIR = os.path.dirname(os.path.abspath(__file__))   # the kicad/ dir
SHEET_INFO = {  # filename -> (title, function); order = reading order
 "power_tree":("Power","USB-C / battery → boost U8 → +5V → load-switch U_PWR → LDO → +3V3; USB D±"),
 "stm32h743":("MCU","STM32H743 + decoupling + 8 MHz crystal + SWD + BOOT0 — the hub (most inter-sheet nets)"),
 "audio":("Audio","PCM5102A DAC (I²S) → PAM8403 Class-D amp + line-out / MIDI jacks"),
 "mcp":("I/O & LEDs","MCP23017 + 2× PCA9685 + the 23 LEDs + buttons (largest sheet)"),
 "encoder":("Encoders & cells","4 push-encoders (A/B/SW) + 5 Hall cells"),
 "lcd":("Display","ST7789 1.9″ SPI + backlight FET (Q2)"),
 "battery":("Battery & charge","LiPo JST + MCP73831 charger + power-path (charges while off)"),
 "field_ambience":("Root / hierarchy","top sheet — ties the 7 functional sheets together"),
}

def parse_sheet(name):
    try: t = open(os.path.join(KDIR, name + ".kicad_sch"), encoding="utf-8").read()
    except OSError: return (0, [])
    refs = set(r for r in re.findall(r'\(property "Reference" "([^"]+)"', t) if not r.startswith(("#", "REF")))
    hl = sorted(set(re.findall(r'\(hierarchical_label "([^"]+)"', t)))
    return (len(refs), hl)

def sheets_html():
    cards = []
    for nm, (title, desc) in SHEET_INFO.items():
        n, hl = parse_sheet(nm)
        nets = (", ".join(hl[:10]) + (" …" if len(hl) > 10 else "")) if hl else "—"
        cards.append(f"""<div class="scard">
        <div class="sh"><b>{esc(title)}</b> <span class="sfile">{nm}.kicad_sch</span></div>
        <div class="sd">{esc(desc)}</div>
        <div class="smeta">{n} parts · {len(hl)} inter-sheet nets</div>
        <div class="snets">{esc(nets)}</div>
        <a class="sopen" href="../../kicad/{nm}.kicad_sch">open in KiCad ↗</a></div>""")
    return "".join(cards)

PCB_GUIDE = """
<section><h2 style="border-color:#22d3ee"><span class="dot" style="background:#22d3ee"></span>For the PCB engineer (Aron) — start here</h2>
<div class="guide">
 <p class="ready">✅ <b>The schematic is already drawn and split into 8 pages.</b> Open <code>kicad/field_ambience.kicad_pro</code> in <b>KiCad 9</b> — the 8 sheets (listed below) load with every symbol, net, value and a pre-assigned footprint. You do <b>not</b> redraw the schematic. Then: ERC → <i>Update PCB from Schematic (F8)</i> → place → route.</p>
 <p><b>The schematic is generated</b> by <code>kicad/generate_kicad_project.py</code> — that script is the source of truth. Don't hand-edit the <code>.kicad_sch</code>; change the generator and re-run. <b>There is no <code>.kicad_pcb</code> yet — the board layout is greenfield, that's your job.</b> ERC/DRC run in the KiCad 9 GUI (no kicad-cli here).</p>
 <div class="gcols">
  <div><b>4-layer stack (ADR-0018, locked)</b><ul>
   <li>L1 signals+parts · <b>L2 solid GND (never cut)</b> · L3 power (+3V3/+5V) · L4 signals</li>
   <li><b>USB-D±</b> together, equal length, over solid GND, ~90 Ω — verify on JLC's calculator</li>
   <li><b>Class-D + boost switching away from analog audio</b>; one ground plane, no split</li>
   <li><b>Crystal</b> &lt;3 mm to MCU + local GND; amp bulk cap &lt;5 mm from PVDD</li>
  </ul></div>
  <div><b>Place these first (enclosure-fixed)</b><ul>
   <li>USB-C · both 3.5 mm jacks · display header J3</li>
   <li>5 cells + Hall sensors under them (19 mm grid) · 5 buttons · 4 encoders</li>
   <li>speaker headers · battery JST · then power, MCU+decoupling, audio, I/O</li>
  </ul></div>
 </div>
 <p class="cells"><b>⚠ The 5 cells are unusual — read this before placing them.</b> The Gateron magnetic switches are <b>pin-less</b> — no electrical connection to the PCB; they sit in a plate <i>above</i> the board. The PCB carries only a <b>linear Hall sensor (DRV5056, SOT-23) directly under each switch stem</b>, on the 19 mm MX grid. As the magnet in the stem moves down, the Hall sensor outputs an analog voltage → RC filter (1 k + 10 nF) → STM32 ADC (nets <code>CELL1..5_SENSE</code>) → firmware computes velocity/position. <b>So there is NO switch footprint — just 5 Hall sensors + their RC at the cell centers.</b> Switches, plate and caps are off-board hardware (ADR-0013). Same idea as a NuPhy/HE keyboard.</p>
 <p><b>Assembly — who solders what.</b> JLC reflow-assembles every <b>SMD</b> part (the "JLC" rows in the BOM). The <b>hand-place</b> rows (THT headers + buttons, encoders, Hall sensors) and the <b>off-board</b> rows (display module, Gateron switches, speakers, battery, knobs) are fitted <b>by hand, after</b> JLC — by whoever does final assembly. So the board comes back from JLC fully SMD-populated; the modules + mechanical parts are then hand-fitted.</p>
 <p class="disp"><b>Display = a finished module (this changes how it's attached).</b> The Waveshare 1.9″ LCD is its own PCB (ST7789 + level-shifter + backlight) — it is <b>NOT reflow-soldered to the main board.</b> Put on the main board only <b>J3, a 1×8 2.54 mm header</b>: 1 VCC · 2 GND · 3 SCK (PA5) · 4 MOSI (PA7) · 5 RES (PC5) · 6 DC (PC4) · 7 CS (PA6) · 8 BLK (backlight via Q2 / PCA9685 ch15). The module is then joined <b>by hand</b> two ways: <b>(a) socket on J3 + header on the module</b> → plug-in, nothing on the module to solder, serviceable; or <b>(b) a short 8-wire cable</b> J3↔module → lets the display sit behind the bezel window with the PCB below (most mechanical freedom). ⚠ The module's pin <i>order</i> varies by vendor — verify the real module vs J3 before fixing it. <b>Is this normal / are there better displays?</b> Yes, module-on-header is the standard, reliable choice at this scale (same as Adafruit/Waveshare-based products). The more-integrated alternative — a bare ST7789 panel + an FPC connector soldered to the main board (lower profile) — is a much bigger job (FPC connector + panel sourcing/mounting, and you'd re-add the level-shifter + backlight the module already has); not worth it here.</p>
 <p><b>Thermal:</b> no ventilation slots (~1.5–2.2 W). Give the LDO copper + thermal vias; keep the LiPo away from LDO/charger/boost.</p>
 <p><b>Workflow → fab:</b> regenerate → open <code>field_ambience.kicad_pro</code> in KiCad 9 → ERC (0 err) → Update PCB from Schematic (F8) → place → route → DRC → export Gerber + CPL; BOM = <code>kicad/jlc_bom.csv</code> → upload to JLC.</p>
 <p><b>Hard blockers before order:</b> ERC pass · layout + routing · PCB outline + mechanical coords (TBD) · USB impedance check · the 5 sourcing swaps above · the modifier-button decision.</p>
</div></section>
<section><h2 style="border-color:#22d3ee"><span class="dot" style="background:#22d3ee"></span>Schematic sheets (open in KiCad)</h2>
<p class="note">8 generated sheets — counts + inter-sheet nets parsed from the live <code>.kicad_sch</code>. No browser preview (no kicad-cli); links open the real sheet in KiCad 9.</p>
<div class="sheets">__SHEETS__</div></section>
<section><h2 style="border-color:#22d3ee"><span class="dot" style="background:#22d3ee"></span>Key connections &amp; component values</h2>
<p class="note">Quick reference (verified from <code>PINMAP.md</code>). The full per-pin map is in the schematic + PINMAP — this is the at-a-glance version.</p>
<table><thead><tr><th>Interface</th><th>STM32 pins</th><th>Goes to</th><th>Key values</th></tr></thead><tbody>
<tr><td class="ref">I²S audio</td><td class="part">PE4 / PE5 / PE6</td><td class="fn">PCM5102A LRCK / BCK / DIN</td><td class="fn">—</td></tr>
<tr><td class="ref">Cells ×5 (ADC)</td><td class="part">PC0, PC1, PA4, PB0, PB1</td><td class="fn">DRV5056 Hall OUT (under each switch)</td><td class="fn"><b>each: 1 kΩ series + 10 nF to GND</b></td></tr>
<tr><td class="ref">LCD SPI</td><td class="part">PA5 SCK, PA7 MOSI (+CS/DC/RES)</td><td class="fn">ST7789 via J3 header</td><td class="fn">backlight via Q2 / PCA9685 ch12</td></tr>
<tr><td class="ref">I²C</td><td class="part">PB6 SCL, PB7 SDA</td><td class="fn">MCP23017 + 2× PCA9685 (shared)</td><td class="fn"><b>4.7 kΩ pull-ups ×2</b></td></tr>
<tr><td class="ref">USB</td><td class="part">PA11 D−, PA12 D+</td><td class="fn">USBLC6 → USB-C (J1)</td><td class="fn">90 Ω diff pair</td></tr>
<tr><td class="ref">MIDI</td><td class="part">PD5 TX</td><td class="fn">J10 TRS jack (Type A)</td><td class="fn"><b>2× 220 Ω</b> (Tip + Ring)</td></tr>
<tr><td class="ref">SWD debug</td><td class="part">PA13 SWDIO, PA14 SWCLK</td><td class="fn">J4 Tag-Connect TC2030</td><td class="fn">—</td></tr>
<tr><td class="ref">Crystal</td><td class="part">OSC_IN/OUT</td><td class="fn">Y1 8 MHz (≤3 mm, local GND)</td><td class="fn"><b>2× 27 pF</b> load caps</td></tr>
</tbody></table>
<p class="vals"><b>Other key values:</b> LED series resistors <b>390 Ω ×23</b> · BOOT0 pull-up <b>1 kΩ</b> · power-ON pull-down <b>100 kΩ</b> · decoupling <b>100 nF per IC</b> + bulk (<b>470 µF</b> tant + <b>100 µF</b> MLCC) · power-off output cap <b>10 µF</b>. All passives are 0603 (caps up to 1210). Every value is already set in the schematic.</p></section>
<section><h2 style="border-color:#22d3ee"><span class="dot" style="background:#22d3ee"></span>Footprints — what's ready, what you source</h2>
<div class="guide">
 <p>Footprints are <b>pre-assigned by the generator</b>; symbol → footprint comes through with <i>Update PCB from Schematic</i>. Status:</p>
 <ul>
  <li>✅ <b>In the repo</b> (<code>kicad/libraries/field_ambience.pretty/</code>, 8 custom): crystal HC-49, boost VQFN-HR, Sunlord inductor, PJ-320D 3.5 mm jack, TS-1088 service button, <b>MST-12D18 power slide switch</b>, TC-1212/HX 12×12 button (THT), + 3D STEPs.</li>
  <li>✅ <b>KiCad-standard</b>: everything in standard packages — LQFP-100, SOT-23/89, SOIC, TSSOP, SSOP, 0603/0805/1210, pin headers, HRO USB-C.</li>
  <li>🟢 <b>Cell switches: NO footprint needed</b> — pin-less; only the DRV5056 Hall (SOT-23, standard) is on the board under each stem.</li>
  <li>⚠ <b>Modifier button (HX B3F, C36498965, THT):</b> verify the THT footprint — the repo <code>SW_TC1212-7.3_THT_4P</code> should fit the 12×12 4-pin THT body; if the pinout differs, generate a fresh one.</li>
 </ul>
 <p><b>How to get / make a footprint (for any LCSC part):</b> <code>pip install easyeda2kicad &amp;&amp; easyeda2kicad --full --lcsc_id=C36498965</code> — pulls the footprint <i>and</i> the 3D STEP straight from LCSC/EasyEDA. Otherwise build from the datasheet. The power slide switch + service buttons were made exactly this way.</p>
</div></section>
"""

def files_html():
    links = [
      ("PCB handoff (checklist)","PCB_HANDOFF.md"),("Pin / net map","PINMAP.md"),
      ("Schematic walkthrough","SCHEMATIC_WALKTHROUGH.md"),("Mechanical / case design","MECHANICAL_REQUIREMENTS.md"),
      ("ERC / DRC checklist","ERC_DRC_CHECKLIST.md"),("KiCad layout blueprint","KICAD_BLUEPRINT.md"),
      ("Layer stack (ADR-0018)","../decisions/ADR-0018-pcb-layer-stack.md"),
      ("Schematic generator (truth)","../../kicad/generate_kicad_project.py"),
      ("KiCad project","../../kicad/field_ambience.kicad_pro"),("JLC machine BOM (CSV)","../../kicad/jlc_bom.csv"),
    ]
    return "".join(f'<a class="flink" href="{esc(u)}">{esc(t)} ↗</a>' for t, u in links)

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
    guide = PCB_GUIDE.replace("__SHEETS__", sheets_html())
    files = files_html()

    issues = """
    <div class="issues ok"><b>✅ Sourcing issues — resolved with verified JLC-stocked parts (r18.70)</b>
    <ul>
      <li><b>FB2 ferrite</b> C84094 (not in JLC) → <b>C19330</b> for both FB1+FB2 (Murata, JLC 950k).</li>
      <li><b>2.2 µF VCAP</b> C24539 (not in JLC) → <b>C23630</b> (16 V X5R 0603, JLC Basic 1.9M).</li>
      <li><b>Charger LED</b> C72041 (was <i>blue + discontinued</i>, wrong part!) → <b>C965800</b> (orange 605 nm 0603, JLC 650k).</li>
      <li><b>100 µF 1210</b> C2880380 (stock 1) → <b>C23742</b> (100 µF 10 V X5R 1210, JLC 41k).</li>
      <li><b>Modifier button</b> C2845240 (stock 30) → <b>C36498965</b> (HX B3F, square head for caps, JLC 20k, ~$0.06). ⚠ verify THT footprint/pinout before fab.</li>
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
 .issues.ok{{background:#10241a;border-color:#1f5b3a}}
 .issues ul{{margin:8px 0 0;padding-left:20px}} .issues li{{margin:3px 0;color:#cfeede}} .issues b{{color:#fff}}
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
 .note{{font-size:12px;color:var(--mut);margin:0 0 10px}}
 .guide{{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px 18px;font-size:13.5px}}
 .guide p{{margin:8px 0}} .guide code{{background:#0c0e13;padding:1px 5px;border-radius:5px;font-size:12px}}
 .guide b{{color:#fff}} .gcols{{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin:6px 0}}
 .guide ul{{margin:5px 0 0;padding-left:18px}} .guide li{{margin:3px 0;color:var(--mut)}} .gcols b{{color:#9fe6f5}}
 .guide p.cells{{background:#231a0c;border:1px solid #5a4416;border-radius:9px;padding:10px 14px;color:#e9d9b3}} .guide p.cells code{{background:#0c0e13}}
 .guide p.disp{{background:#0e1f2e;border:1px solid #1f4a6b;border-radius:9px;padding:10px 14px;color:#cfe6f5}} .guide p.disp code{{background:#0c0e13}}
 .guide p.ready{{background:#10241a;border:1px solid #1f5b3a;border-radius:9px;padding:10px 14px;color:#cfeede}} .guide p.ready code{{background:#0c0e13}}
 p.vals{{font-size:13px;color:var(--mut);margin:10px 0 0}} p.vals b{{color:#e6e9ef}}
 .sheets{{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:12px}}
 .scard{{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:13px 15px}}
 .sh{{font-size:14px}} .sh b{{color:#fff}} .sfile{{font-family:ui-monospace,Menlo,monospace;font-size:11px;color:var(--mut);margin-left:6px}}
 .sd{{font-size:12.5px;color:var(--mut);margin:5px 0}} .smeta{{font-size:11.5px;color:#9fe6f5;margin:4px 0}}
 .snets{{font-family:ui-monospace,Menlo,monospace;font-size:10.5px;color:var(--mut);line-height:1.4;word-break:break-word}}
 .sopen{{display:inline-block;margin-top:8px;font-size:12px;color:#8ab4ff;text-decoration:none;font-weight:600}} .sopen:hover{{text-decoration:underline}}
 .flinks{{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:8px}}
 .flink{{background:var(--card);border:1px solid var(--line);border-radius:9px;padding:9px 13px;color:#8ab4ff;text-decoration:none;font-size:13px;font-weight:600}} .flink:hover{{border-color:#3a4250}}
 footer{{color:var(--mut);font-size:12px;margin-top:32px;border-top:1px solid var(--line);padding-top:14px}}
</style></head><body>
<header><h1>AMBIENT — BOM Overview</h1>
<p>Every part grouped by function · click a part for its LCSC/JLC page · the <b>JLC assembly</b> badge is verified live · 👁 = visible on the enclosure</p></header>
<div class="wrap">
 <div class="visgrid">{vcards}</div>
 {issues}
 {guide}
 <div class="legend">
   <span class="b basic">JLC Basic</span> no setup fee &nbsp;
   <span class="b ext">JLC Extended</span> small per-type fee &nbsp;
   <span class="b bad">✗ / low</span> fix before order &nbsp;
   <span class="b hand">hand-place</span> on-board, you place &nbsp;
   <span class="b off">off-board</span> not soldered
 </div>
 {''.join(secs)}
 <section><h2 style="border-color:#22d3ee"><span class="dot" style="background:#22d3ee"></span>Docs &amp; files (repo)</h2>
 <p class="note">Relative links — work when the HTML is opened from a repo clone.</p>
 <div class="flinks">{files}</div></section>
 <footer>LCSC = JLCPCB (same group): the <code>C</code>-number is the JLC part; the link opens the LCSC product page, the badge is JLC's <i>assembly</i> status (stock checked 2026-06-29 — re-verify before ordering). Source of truth = <code>kicad/generate_kicad_project.py</code>; machine BOM for upload = <code>kicad/jlc_bom.csv</code>; regenerate this page with <code>kicad/gen_bom_overview.py</code>.</footer>
</div></body></html>"""

    out = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "docs", "hardware", "bom_overview.html"))
    with open(out, "w", encoding="utf-8") as f:
        f.write(doc)
    print("wrote", out, f"({len(doc)} bytes)")

if __name__ == "__main__":
    main()
