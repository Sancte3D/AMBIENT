#!/usr/bin/env python3
"""
gen_bom_handoff.py — the human-facing "Field Ambience — Bill of Materials"
handoff (the Aron PDF). Plain-language "what it does" per part, grouped by
function, quantities + footprints taken LIVE from the authoritative
jlc_bom.csv so it can never drift from the orderable BOM again (the previous
hand-made PDF went stale on the r19.37 PAM8406 gain-staging — amp part AND the
surrounding passive counts).

    python3 gen_bom_handoff.py            # -> ../docs/hardware/bom_handoff.html
    # then render to PDF (headless Chromium):
    #   chromium --headless=new --no-sandbox --no-pdf-header-footer \
    #            --print-to-pdf=Aron_Field_Ambience_BOM.pdf file://…/bom_handoff.html

Quantities/footprints/MPN/LCSC = jlc_bom.csv (authoritative). Titles +
descriptions = curated here. Links go to the LCSC product page (which hosts the
datasheet, specs and land pattern); the KiCad footprint is printed under each
part. Off-board parts are hand-fitted and not on the JLC list.
"""
import csv, html, os, datetime

HERE = os.path.dirname(__file__)
CSV  = os.path.join(HERE, "jlc_bom.csv")
OUT  = os.path.normpath(os.path.join(HERE, "..", "docs", "hardware", "bom_handoff.html"))

REV  = "r19.37"
REV_NOTE = "PAM8406 speaker amp + gain-staging (ADR-0025)"

# LCSC -> (title, what-it-does).  Titles are the human name; the ×N suffix is
# appended automatically from the live quantity.
META = {
 # Processor & memory
 "C114409": ("Main processor", "Runs everything: the sound engine, screen, buttons, MIDI. The brain of the instrument."),
 "C3028887": ("8 MB memory chip", "Extra memory for the sound engine — holds audio samples, reverb impulses and wavetables that don't fit inside the processor."),
 "C596838": ("8 MHz crystal", "The processor's clock reference. Everything is timed from this."),
 "C107045": ("Crystal capacitors (27 pF)", "Help the crystal oscillate cleanly."),
 # Audio
 "C107671": ("Audio DAC", "Turns the digital sound from the processor into an analog audio signal."),
 "C86270": ("Speaker amplifier", "Amplifies the DAC's signal to drive the two speakers. Class-D (MODE tied to +5 V); input resistors set +4.3 dB gain, input caps give a ~91 Hz speaker high-pass. Replaces the NRND PAM8403 (ADR-0025)."),
 "C69901": ("Headphone amplifier", "Drives headphones (16 Ω and up) and line inputs from the audio jack — pop-free, short-circuit protected. Before this, the jack was line-level only."),
 "C431535": ("3.5 mm jacks", "One is the PHONES / LINE OUT (headphones and line inputs, driven by the headphone amplifier), one is the MIDI output. Plugging into the audio jack mutes the speakers; unplugging brings them back."),
 "C19330": ("Audio supply filters", "Ferrite beads that keep digital noise out of the audio power supply."),
 # Power
 "C165129": ("Boost converter", "Raises the 3.7 V battery voltage to 5 V for the amplifier and LEDs."),
 "C36500": ("Boost inductor (2.2 µH)", "Energy-storage coil the boost converter needs to work."),
 "C460397": ("3.3 V regulator", "Makes the clean 3.3 V that the processor, screen and chips run on. Gets warm — needs copper area on the board."),
 "C131941": ("Power switch chip", "Electronic switch that turns the whole 3.3 V system on and off."),
 "C109335": ("Slide switch", "The physical on/off switch on the side of the device."),
 "C54313": ("Battery charger with power path", "Manages USB power, battery charging and the system supply in one chip. The system is powered first, the battery gets what's left; charging works while the device is off. Charge current 0.89 A, USB input limited to 1.34 A."),
 "C438899": ("Battery fuse (resettable)", "Sits in the battery + wire. Trips at 5 A if the battery is ever shorted, then resets itself. Backup protection — the battery pouch must still have its own protection."),
 "C295747": ("Battery connector", "JST plug the LiPo battery connects to. Pin 1 = battery plus."),
 "C2687116": ("USB protection", "Protects the USB data lines from static discharge."),
 "C18198349": ("USB fuse (3 A)", "Cuts power if something shorts on the USB input. Resets itself."),
 "C113952": ("Surge protector", "Clamps voltage spikes on the 5 V rail."),
 "C8678": ("Power diode", "Sits between the 5 V boost converter and the big rail capacitors — keeps the regulator stable and blocks reverse current."),
 "C165948": ("USB-C connector", "For charging and flashing firmware."),
 # Controls & lights
 "C506653": ("Button expander", "Reads the 5 playing keys and 5 modifier buttons over a 2-wire bus, so the processor doesn't need 10 extra pins."),
 "C2678753": ("LED driver", "Controls the brightness of all 15 status LEDs and the screen backlight."),
 "C202365": ("Rotary encoders", "The four knobs: sound drive, brightness, volume and menu navigation. Each can also be pressed."),
 "C400229": ("Playing keys (Kailh Choc)", "The five main keys you play notes on. Real mechanical keyswitches. Hand-soldered."),
 "C36498965": ("Modifier buttons", "Shift, Hold, Drone, Generate, Clear. Hand-soldered."),
 "C720477": ("Service buttons", "Reset and boot-mode — only needed when flashing/debugging."),
 "C8545": ("Backlight transistor", "Switches the screen backlight, dimmed by the LED driver."),
 "C2287": ("Yellow LEDs", "Status lights: Shift + one per playing key."),
 "C12624": ("Green LEDs", "Status lights: Hold + one per playing key (shift layer)."),
 "C965808": ("White LEDs", "Status lights for Drone, Generate, Clear."),
 "C965800": ("Orange LED", "Charging indicator. Lit while the battery charges."),
 "C23151": ("LED resistors 390 Ω", "Set the current through each status LED."),
 # Connectors
 "C124383": ("Screen header (1×8)", "The screen module plugs into this via a short cable."),
 "C124375": ("Speaker headers", "The two speakers plug into these."),
 # Capacitors
 "C444831": ("Bulk capacitor 470 µF", "Big energy reservoir that keeps the 5 V rail steady under load."),
 "C23742": ("Bulk capacitor 100 µF", "Second reservoir for fast load changes."),
 "C14663": ("100 nF capacitors", "Small decoupling capacitors — one next to every chip to keep its supply clean."),
 "C15849": ("1 µF capacitors", "Decoupling and audio-path capacitors, including the headphone amplifier charge pump and input coupling."),
 "C1607": ("2.2 µF capacitors", "Supply and internal-rail capacitors for the headphone amplifier."),
 "C15850": ("10 µF capacitors", "Local supply reservoirs for the chips."),
 "C45783": ("22 µF capacitors", "Supply reservoirs, including the boost converter output and the charger system/battery pins."),
 "C46653": ("4.7 µF capacitors", "Input/output capacitors for the regulator and charger."),
 "C23630": ("2.2 µF capacitors", "Required by the processor's internal power supply (VCAP pins)."),
 "C57112": ("10 nF capacitors", "Filtering, the boost converter's compensation, and the speaker-amp input coupling."),
 # Resistors
 "C25804": ("10 kΩ resistors", "Pull-ups/pull-downs that define safe default states everywhere (reset, mute, boot, buttons, charger temperature pin…)."),
 "C25803": ("100 kΩ resistors", "Pull-downs on the power-on and USB-detect lines."),
 "C21190": ("1 kΩ resistors", "Series resistors for the boot button and charge LED; one also programs the battery charge current (0.89 A)."),
 "C22962": ("220 Ω resistors", "Required series resistors on the MIDI output."),
 "C23345": ("22 Ω resistors", "Series resistors on the line output."),
 "C114605": ("1.2 kΩ resistor", "Sets the maximum current the charger may draw from USB (1.34 A)."),
 "C25809": ("121 kΩ resistor", "Sets the boost converter's output to 5 V (with the 39 kΩ)."),
 "C23153": ("39 kΩ resistor", "Second half of the boost voltage divider."),
 "C22890": ("174 kΩ resistors", "The boost converter's current limit, plus the speaker amplifier's input / gain resistors (+4.3 dB)."),
 "C23146": ("360 kΩ resistor", "Sets the boost converter's switching frequency."),
 "C4260": ("6.2 kΩ resistor", "Boost converter loop compensation — keeps it stable under load."),
 "C23162": ("4.7 kΩ resistors", "Pull-ups the I²C bus needs to work."),
 "C23186": ("5.1 kΩ resistors", "USB-C configuration resistors — tell the charger this is a device."),
}

# Section order and which LCSC parts fall in each (drives grouping + order).
SECTIONS = [
 ("Processor & Memory", ["C114409","C3028887","C596838","C107045"]),
 ("Audio", ["C107671","C86270","C69901","C431535","C19330"]),
 ("Power", ["C165129","C36500","C460397","C131941","C109335","C54313","C438899",
            "C295747","C2687116","C18198349","C113952","C8678","C165948"]),
 ("Controls & Lights", ["C506653","C2678753","C202365","C400229","C36498965",
            "C720477","C8545","C2287","C12624","C965808","C965800","C23151"]),
 ("Connectors", ["C124383","C124375"]),
 ("Capacitors", ["C444831","C23742","C14663","C15849","C1607","C15850","C45783",
            "C46653","C23630","C57112"]),
 ("Resistors", ["C25804","C25803","C21190","C22962","C23345","C114605","C25809",
            "C23153","C22890","C23146","C4260","C23162","C23186"]),
]

# Board parts that are intentionally NOT fitted by the assembler (no LCSC line).
DNP = {"Connectors": [("Debug pads (not fitted)", "SWD 1×3 · 1.27 mm pads · DNP",
        "Three bare pads for programming/debugging. Nothing is soldered here at "
        "the factory — solder a small header or wires by hand when needed.", 0, None)]}

# Off-board, hand-fitted parts (not on the JLC assembly list).
OFFBOARD = [
 ("Screen", "Waveshare 1.9″ LCD, 170×320, ST7789V2",
  "Colour display. Plugs into the 1×8 header via a short cable — wire by signal "
  "name, the module pin order differs from the header.",
  "https://eckstein-shop.de/WaveShare-19inch-LCD-Display-Module-170320-IPS-262K-Colors-SPI-Interface-EN"),
 ("Battery", "LiPo 2000 mAh, JST-PH (Adafruit 2011)",
  "Powers the device. Check plug polarity before first connect: connector "
  "pin 1 = battery plus.", "https://www.adafruit.com/product/2011"),
 ("Speakers ×2", "Same Sky CMS-402811-28SP, 8 Ω",
  "The built-in speakers. Plug into the two speaker headers.",
  "https://www.digikey.com/en/products/detail/cui-devices/CMS-402811-28SP/10821307"),
 ("Debug cable", "Tag-Connect TC2030-IDC",
  "Presses onto the SWD pads for flashing/debugging. A tool, not soldered.",
  "https://www.tag-connect.com/product/tc2030-idc"),
]

def esc(s): return html.escape(str(s))
def lcsc_url(code): return f"https://www.lcsc.com/product-detail/{code}.html"

def clean_mpn(comment):
    """Human MPN: drop trailing parenthetical / comma notes from the CSV comment."""
    s = comment.strip()
    for cut in (" (", ","):
        i = s.find(cut)
        if i > 0: s = s[:i]
    return s.strip()

def short_fp(fp):
    return fp.split(":", 1)[1] if ":" in fp else fp

def load_csv():
    rows = {}   # lcsc -> (mpn, footprint, qty)
    total_placements = 0
    with open(CSV, encoding="utf-8") as f:
        for r in csv.reader(f):
            if not r or r[0].strip() == "Comment" or len(r) < 4:
                continue
            comment, desig, fp, code = r[0], r[1], r[2], r[3].strip()
            qty = len([d for d in desig.split(",") if d.strip()])
            rows[code] = (clean_mpn(comment), short_fp(fp), qty)
            total_placements += qty
    return rows, total_placements

def main():
    rows, placements = load_csv()
    seen = set()

    part_types = len(rows)
    for sec in DNP.values():
        part_types += len(sec)

    body = []
    for title, codes in SECTIONS:
        trs = []
        for code in codes:
            if code not in rows:
                continue
            seen.add(code)
            mpn, fp, qty = rows[code]
            name, desc = META.get(code, (mpn, ""))
            suffix = f" ×{qty}" if qty > 1 else ""
            trs.append(f"""
      <tr>
        <td class="part"><span class="pn">{esc(name)}{suffix}</span>
            <span class="mpn">{esc(mpn)} · {esc(fp)}</span></td>
        <td class="fn">{esc(desc)}</td>
        <td class="qty">{qty}</td>
        <td class="lnk"><a href="{lcsc_url(code)}">LCSC {esc(code)}</a></td>
      </tr>""")
        # DNP rows for this section
        for (name, sub, desc, qty, url) in DNP.get(title, []):
            link = f'<a href="{url}">link</a>' if url else "—"
            trs.append(f"""
      <tr>
        <td class="part"><span class="pn">{esc(name)}</span>
            <span class="mpn">{esc(sub)}</span></td>
        <td class="fn">{esc(desc)}</td>
        <td class="qty">{qty}</td>
        <td class="lnk">{link}</td>
      </tr>""")
        if trs:
            body.append(f"""
    <h2>{esc(title.upper())}</h2>
    <table>
      <tr class="hd"><th>PART</th><th>WHAT IT DOES</th><th>QTY</th><th>LINKS</th></tr>
      {''.join(trs)}
    </table>""")

    # Off-board section
    offrows = []
    for (name, mpn, desc, url) in OFFBOARD:
        offrows.append(f"""
      <tr>
        <td class="part"><span class="pn">{esc(name)}</span>
            <span class="mpn">{esc(mpn)}</span></td>
        <td class="fn">{esc(desc)}</td>
        <td class="lnk"><a href="{esc(url)}">Product page</a></td>
      </tr>""")
    body.append(f"""
    <h2>OFF-BOARD PARTS (HAND-FITTED, NOT SOLDERED BY THE ASSEMBLER)</h2>
    <table class="off">
      <tr class="hd"><th>PART</th><th>WHAT IT DOES</th><th>LINKS</th></tr>
      {''.join(offrows)}
    </table>""")

    # sanity: any curated part missing from the CSV?
    missing = [c for _, codes in SECTIONS for c in codes if c not in rows]
    if missing:
        print("WARNING: curated LCSC not found in jlc_bom.csv:", missing)
    uncur = [c for c in rows if c not in seen]
    if uncur:
        print("WARNING: CSV parts with no section/description:", uncur)

    date = datetime.date.today().isoformat()
    sub = (f"Handheld ambient synthesizer · {part_types} board part-types "
           f"({placements} placements) + {len(OFFBOARD)} off-board parts · "
           f"generated from jlc_bom.csv · {date} (rev {REV} — {REV_NOTE}) · "
           f"links open the LCSC page (datasheet · specs · footprint)")

    doc = f"""<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Field Ambience — Bill of Materials</title>
<style>
  @page {{ size: A4; margin: 15mm 13mm; }}
  * {{ box-sizing: border-box; }}
  body {{ font: 12px/1.45 -apple-system,'Helvetica Neue',Arial,sans-serif;
         color:#1a1a1a; margin:0; padding:26px 30px; -webkit-print-color-adjust:exact; }}
  h1 {{ font-size:24px; font-weight:800; letter-spacing:-.01em; margin:0 0 6px; }}
  .sub {{ color:#6b7280; font-size:12px; margin:0 0 22px; max-width:52em; }}
  h2 {{ font-size:12.5px; font-weight:800; letter-spacing:.09em; color:#111;
        margin:26px 0 0; padding-bottom:6px; border-bottom:2px solid #111;
        page-break-after:avoid; }}
  table {{ width:100%; border-collapse:collapse; }}
  tr {{ page-break-inside:avoid; }}
  th {{ text-align:left; font-size:9.5px; font-weight:700; letter-spacing:.07em;
        color:#9ca3af; padding:8px 10px 6px 0; border-bottom:1px solid #e5e7eb; }}
  td {{ padding:9px 10px 9px 0; border-bottom:1px solid #eef0f2; vertical-align:top; }}
  .part {{ width:31%; }}
  .qty  {{ width:5%; color:#374151; }}
  .lnk  {{ width:18%; }}
  .off .lnk {{ width:22%; }}
  .pn  {{ display:block; font-weight:700; color:#111; }}
  .mpn {{ display:block; font-family:'SFMono-Regular',Consolas,monospace;
          font-size:10px; color:#9ca3af; margin-top:2px; }}
  .fn  {{ color:#374151; }}
  a {{ color:#2563eb; text-decoration:none; }}
</style></head>
<body>
  <h1>Field Ambience — Bill of Materials</h1>
  <p class="sub">{esc(sub)}</p>
  {''.join(body)}
</body></html>"""

    with open(OUT, "w", encoding="utf-8") as f:
        f.write(doc)
    print(f"wrote {OUT}")
    print(f"  {part_types} board part-types · {placements} placements · {len(OFFBOARD)} off-board")

if __name__ == "__main__":
    main()
