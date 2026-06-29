#!/usr/bin/env python3
"""
gen_bom_overview.py — build a standalone, browsable HTML overview of the AMBIENT
components, grouped by function, with clickable LCSC/JLCPCB links so you can see
each part (image + specs) on the supplier page.

Self-contained: no external CSS/JS. Open the produced .html in any browser.
Data mirrors BOM_MASTER.md / PCB_BOM.md — regenerate after a BOM change.

    python3 gen_bom_overview.py            # writes ../docs/hardware/bom_overview.html
"""
import html, os

def lcsc(code):  return f"https://www.lcsc.com/product-detail/{code}.html"
def jlc(code):   return f"https://jlcpcb.com/partdetail/{code}"

# group -> list of dicts: ref, part, fn (function), link, vis (visible? count str)
GROUPS = [
 ("MCU & Clock", "#6366f1", [
   ("U1", "STM32H743VIT6 (LQFP-100)", "the brain — 480 MHz Cortex-M7, runs everything", lcsc("C114409")),
   ("Y1", "ABLS-8.000MHZ crystal", "8 MHz reference clock for the MCU", lcsc("C596838")),
 ]),
 ("Power", "#ef4444", [
   ("J1", "USB-C TYPE-C-31-M-12", "charging + USB-DFU firmware flash · VISIBLE (edge)", lcsc("C165948")),
   ("U8", "TPS61089 boost", "battery 3.7 V → 5 V system rail", lcsc("C165129")),
   ("L1", "SWPA6045 2.2 µH", "boost inductor", lcsc("C83455")),
   ("Q1", "DMG2305UX P-MOSFET", "power-path switch (USB vs battery)", lcsc("C150470")),
   ("U5", "AP7361C LDO", "5 V → 3.3 V for the MCU/logic (the thermal hotspot)", lcsc("C460397")),
   ("U_PWR", "TPS22918 load switch", "the actual power-off — gates the 3.3 V domain", lcsc("C131941")),
   ("SW_PWR", "MST-12D18G3 slide switch", "on/off, side-actuated · VISIBLE (side)", jlc("C49023766")),
   ("U7", "MCP73831 charger", "LiPo charger (charges even when 'off')", lcsc("C424093")),
   ("J9", "JST-PH 2.0 (battery)", "LiPo connector", lcsc("C295747")),
   ("D1", "USBLC6-2SC6", "USB ESD protection", lcsc("C2687116")),
   ("F1", "PTC fuse 3 A", "overcurrent protection on USB 5 V", lcsc("C18198349")),
   ("D2", "SMAJ5.0A TVS", "surge clamp on the 5 V rail", lcsc("C113952")),
   ("D3", "SS34 Schottky", "boost rectifier", lcsc("C8678")),
 ]),
 ("Audio", "#10b981", [
   ("U3", "PCM5102A DAC", "I²S → analog audio (the sound quality part)", lcsc("C107671")),
   ("U4", "PAM8403 Class-D amp", "drives the two speakers", lcsc("C17337")),
   ("J8", "PJ-320D 3.5 mm line-out", "audio out to headphones/mixer · VISIBLE (side)", lcsc("C431535")),
   ("J10", "PJ-320D 3.5 mm MIDI-out", "MIDI out (TRS Type A) · VISIBLE (side)", lcsc("C431535")),
   ("SPK", "Same Sky CMS-402811-28SP ×2", "the speakers (cloth cone, 8 Ω) · VISIBLE (grilles)",
    "https://www.digikey.com/en/products/detail/cui-devices/CMS-402811-28SP/10821307"),
   ("FB1/2", "BLM18AG601 ferrite bead", "audio supply filtering", lcsc("C19330")),
 ]),
 ("Display", "#f59e0b", [
   ("LCD", "Waveshare 1.9″ 170×320 ST7789", "the screen · VISIBLE (window)",
    "https://www.waveshare.com/1.9inch-lcd-module.htm"),
   ("Q2", "2N7002 MOSFET", "backlight PWM driver", lcsc("C8545")),
   ("J3", "1×8 header", "plugs the LCD module in", lcsc("C124383")),
 ]),
 ("Controls — Encoders & Cells & Buttons", "#0ea5e9", [
   ("EN1-4", "ALPS EC11E18244AU ×4", "4 push-encoders — global params + push · VISIBLE (top)", lcsc("C202365")),
   ("Cells", "Gateron Low-Profile Magnetic ×5", "the 5 playable keys (note + velocity) · VISIBLE (top)",
    "https://www.gateron.com/products/gateron-low-profile-magnetic-jade-switch"),
   ("Hall", "DRV5056A4 Hall sensor ×5", "reads each cell's magnet (velocity)", lcsc("C2152902")),
   ("SW6-10", "TC-1212-7.3 tactile ×5", "modifier buttons Shift/Hold/Drone/Generate/Clear · VISIBLE (top)", jlc("C2845240")),
   ("SW11/BOOT", "TS-1088 tactile ×2", "Reset + BOOT0 (bottom service pinholes)", lcsc("C720477")),
 ]),
 ("LEDs (23 visible indicators)", "#a855f7", [
   ("Mod-Y", "KENTO KT-0603Y (yellow)", "Shift modifier LED + 5 cell base-hold LEDs · VISIBLE", lcsc("C2287")),
   ("Mod-G", "KENTO KT-0603G (green)", "Hold modifier LED + 5 cell shift-hold LEDs · VISIBLE", lcsc("C12624")),
   ("White", "XL-1608UWC (white)", "Drone/Generate/Clear LEDs + 8 VU-meter LEDs · VISIBLE", lcsc("C965808")),
   ("LED_CHRG", "Amber 0603", "charger status (optional, visible)", lcsc("C72041")),
 ]),
 ("I/O Expander & LED Drivers", "#64748b", [
   ("U2", "MCP23017 GPIO expander", "extra buttons/inputs over I²C", lcsc("C506653")),
   ("U6", "PCA9685 PWM driver", "drives the 15 status LEDs", lcsc("C2678753")),
   ("U10", "PCA9685 PWM driver #2", "drives the 8 VU-meter LEDs", lcsc("C2678753")),
 ]),
 ("Service", "#94a3b8", [
   ("J4", "Tag-Connect TC2030-IDC", "SWD debug/flash header", "https://www.tag-connect.com/product/tc2030-idc"),
 ]),
]

VISIBLE = [
  ("Touch controls", "15", "4 encoders + 5 cell keys + 5 modifier buttons + 1 power switch"),
  ("Display", "1", "1.9″ window"),
  ("Indicator LEDs", "23", "5 modifier + 10 cell + 8 VU-meter"),
  ("Ports", "3", "USB-C + line-out + MIDI"),
  ("Speakers", "2", "grilles"),
  ("Bottom service", "—", "2 pinholes + battery access + 4 screws"),
]

def esc(s): return html.escape(s)

def main():
    rows = []
    for title, color, parts in GROUPS:
        cards = []
        for ref, part, fn, link in parts:
            vis = "vis" if "VISIBLE" in fn else ""
            fn_disp = fn.replace(" · VISIBLE", "")
            badge = '<span class="visbadge">👁 visible</span>' if vis else ''
            cards.append(f"""
        <tr class="{vis}">
          <td class="ref">{esc(ref)}</td>
          <td class="part"><a href="{esc(link)}" target="_blank" rel="noopener">{esc(part)} ↗</a>{badge}</td>
          <td class="fn">{esc(fn_disp)}</td>
        </tr>""")
        rows.append(f"""
    <section>
      <h2 style="border-color:{color}"><span class="dot" style="background:{color}"></span>{esc(title)}</h2>
      <table>
        <thead><tr><th>Ref</th><th>Part (→ supplier page)</th><th>What it's for</th></tr></thead>
        <tbody>{''.join(cards)}</tbody>
      </table>
    </section>""")

    vis_cards = ''.join(
      f'<div class="vcard"><div class="vnum">{esc(n)}</div><div class="vlbl">{esc(lbl)}</div>'
      f'<div class="vsub">{esc(sub)}</div></div>' for lbl, n, sub in VISIBLE)

    doc = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AMBIENT — Component Overview</title>
<style>
  :root {{ --bg:#0f1115; --card:#171a21; --line:#262b36; --txt:#e6e9ef; --mut:#9aa4b2; --acc:#6366f1; }}
  * {{ box-sizing:border-box; }}
  body {{ margin:0; font:15px/1.5 -apple-system,Segoe UI,Roboto,sans-serif; background:var(--bg); color:var(--txt); }}
  header {{ padding:28px 24px 18px; border-bottom:1px solid var(--line); position:sticky; top:0; background:var(--bg); z-index:5; }}
  header h1 {{ margin:0 0 4px; font-size:22px; }}
  header p {{ margin:0; color:var(--mut); font-size:13px; }}
  .wrap {{ max-width:1040px; margin:0 auto; padding:0 24px 60px; }}
  .visgrid {{ display:grid; grid-template-columns:repeat(auto-fit,minmax(150px,1fr)); gap:12px; margin:22px 0 8px; }}
  .vcard {{ background:var(--card); border:1px solid var(--line); border-radius:12px; padding:14px 16px; }}
  .vnum {{ font-size:30px; font-weight:700; line-height:1; color:#fff; }}
  .vlbl {{ font-size:13px; font-weight:600; margin-top:6px; }}
  .vsub {{ font-size:11.5px; color:var(--mut); margin-top:3px; }}
  section {{ margin-top:30px; }}
  h2 {{ font-size:16px; margin:0 0 10px; padding-bottom:6px; border-bottom:2px solid; display:flex; align-items:center; gap:9px; }}
  .dot {{ width:10px; height:10px; border-radius:50%; display:inline-block; }}
  table {{ width:100%; border-collapse:collapse; background:var(--card); border:1px solid var(--line); border-radius:12px; overflow:hidden; }}
  th,td {{ text-align:left; padding:9px 12px; border-bottom:1px solid var(--line); vertical-align:top; }}
  th {{ font-size:11px; text-transform:uppercase; letter-spacing:.04em; color:var(--mut); background:#12151b; }}
  tr:last-child td {{ border-bottom:none; }}
  td.ref {{ font-family:ui-monospace,Menlo,monospace; font-size:12.5px; color:var(--mut); white-space:nowrap; }}
  td.part a {{ color:#8ab4ff; text-decoration:none; font-weight:600; }}
  td.part a:hover {{ text-decoration:underline; }}
  td.fn {{ color:var(--mut); font-size:13px; }}
  tr.vis {{ background:rgba(99,102,241,.07); }}
  .visbadge {{ font-size:10px; color:#c7b3ff; margin-left:8px; white-space:nowrap; }}
  footer {{ color:var(--mut); font-size:12px; margin-top:36px; border-top:1px solid var(--line); padding-top:14px; }}
  a.docref {{ color:#8ab4ff; }}
</style></head>
<body>
<header>
  <h1>AMBIENT — Component Overview</h1>
  <p>Every orderable part, grouped by function · click any part to open its supplier page (LCSC / JLCPCB / vendor) · rows marked 👁 are visible on the enclosure</p>
</header>
<div class="wrap">
  <div class="visgrid">{vis_cards}</div>
  {''.join(rows)}
  <footer>
    Generated from <code>BOM_MASTER.md</code> / <code>PCB_BOM.md</code>. Source of truth = the
    schematic generator (<code>kicad/generate_kicad_project.py</code>). Machine BOM for JLC assembly:
    <code>kicad/jlc_bom.csv</code>. Regenerate this page with <code>kicad/gen_bom_overview.py</code>.
    Prices/stock: verify on the linked pages before ordering.
  </footer>
</div>
</body></html>"""

    out = os.path.join(os.path.dirname(__file__), "..", "docs", "hardware", "bom_overview.html")
    out = os.path.normpath(out)
    with open(out, "w", encoding="utf-8") as f:
        f.write(doc)
    print("wrote", out, f"({len(doc)} bytes)")

if __name__ == "__main__":
    main()
