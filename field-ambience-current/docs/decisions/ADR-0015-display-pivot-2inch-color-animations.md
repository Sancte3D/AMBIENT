# ADR-0015: Display-Pivot 1.9″ → 2.0″ + Farb-/Animations-Architektur

**Status:** PARTIALLY IMPLEMENTED — Panel-Pivot PROPOSED (User-Verify offen);
**Farb-Schritt 1 (Akzent-Tint) implementiert r18.44**.
**Date:** 2026-06-22
**Supersedes for display panel:** ADR-implicit r16 (SSD1322 → Waveshare 1.9″ ST7789V2)

> **Implementation note r18.44** — Schritt 1 von D2 ist gebaut, aber *schlauer*
> als ursprünglich skizziert: statt sofort auf einen vollen RGB565-Framebuffer
> zu wechseln (154 KB, Rewrite jeder `oled_*`-Primitive), bleibt der 4-bit-
> Grau-Framebuffer und der **Akzent-Tint passiert in der Grau→RGB565-Umwandlung**
> (`src/oled_color.c`). Default-Akzent = Weiß ⇒ exakt der bisherige Mono-Look
> (bit-genau, per Test bewiesen). Pro World eine dezente Akzentfarbe
> (Tokyo=kühlblau, Coast=aqua, Drive=violett, AfterHours=amber). Null FB-Bloat,
> null Draw-Layer-Rewrite, läuft schon auf dem 1.9″-Panel — der 2.0″-Pivot ist
> davon entkoppelt. Der volle Per-Pixel-RGB565-FB bleibt **Schritt 2**, nur
> nötig falls die UI mal beliebige Mehrfarb-Inhalte (nicht nur einen Cast)
> braucht.

## Kontext

User-Frage (2026-06-22): *„Kann das display minimal größer sein? ich will ja
eigentlich später bisschen animationen drauf haben. wie mache ich das mit
speicher und leistung. auch wichtig. muss trotzdem farbiges display sein."*

Drei verschränkte Punkte: **(1)** mehr Fläche, **(2)** Farbe behalten,
**(3)** flüssige UI-Animationen ohne Audio-Aussetzer.

### Ist-Zustand (r18)

- **Panel:** Waveshare 1.9″ 170×320 ST7789V2 IPS, SPI (BOM_MASTER §5)
- **Farbe physisch:** ja — ST7789V2 ist RGB565. **Genutzt:** nein.
  Framebuffer in `include/oled.h` ist 4-bit Grau (OP-1-Sprache); der Treiber
  wandelt beim Streamen nach RGB565 (`src/oled.h:9` Header-Kommentar).
- **Framebuffer-Größe:** 27,2 KB (170×320 × 4 bpp)
- **SPI:** 24 MHz Bench-proven (`src/hal_h743/lcd_st7789_h743.c:30`)
- **DMA:** im H7-Skeleton noch TODO (`lcd_st7789_h743.c:39`)
- **Audio-Constraint:** 1 kHz Audio-IRQ darf nicht aussetzen (ADR-0010)

## Optionen

| Option | Auflösung | Controller | Pixel-Δ | Firmware-Aufwand | Mech.-Δ |
|---|---|---|---|---|---|
| A. bleiben (1.9″ 170×320) | 54.400 px | ST7789V2 | 0 % | 0 | 0 |
| **B. 2.0″ 240×320 ST7789** ⭐ | 76.800 px | ST7789(V) | **+41 %** | klein (Init-Bytes + Maße + Pixel-Offset entfällt) | Bezel-Fenster größer, J3-Pin-Order neu prüfen |
| C. 2.4″ 240×320 ILI9341 | 76.800 px | ILI9341 | +41 % (nur Fläche) | mittel (neue Init-Sequenz) | Gehäuse wächst spürbar; Modul-Ökosystem schlechter |
| D. 2.8″/3.2″ 240×320 ILI9341 | 76.800 px | ILI9341 | +41 % (nur Fläche) | mittel | Gehäuse unverhältnismäßig groß für handheld |

**Empfehlung: Option B.**

Begründung:
1. **Mehr Fläche *und* +41 % Pixel** — Option C/D geben nur Fläche bei gleicher
   Pixelzahl.
2. **Gleiche Controller-Familie** (ST7789) → bestehender SPI-Init-Stream
   funktioniert; der Y-Offset-Trick (SPEC §6, GRAM-Offset 35) entfällt sogar,
   weil das 2.0″-Die *ist* 240×320 ohne Beschnitt → **Treiber wird einfacher**.
3. **Gleicher Vendor (Waveshare 2″ LCD Module)** → Level-Shifter +
   Backlight-Driver-Layout bleiben kompatibel. Q2 (2N7002) + PCA9685-PWM-Kette
   unverändert (BOM_MASTER §5).
4. **Speicher-Budget bequem:** 240×320 × 16 bpp = 154 KB; doppelt gepuffert
   308 KB. STM32H743 hat **512 KB AXI-SRAM** am Stück — passt locker.

## Entscheidungen

### D1 — Panel-Pivot 1.9″ → 2.0″ 240×320 ST7789

**Vendor-Kandidat:** Waveshare 2inch LCD Module (240×320 SPI ST7789), gleiche
Familie wie das aktuelle 1.9″-Modul, gleicher 8-Pin-SPI-Header.

> ⚠ **UNVERIFIED — NEEDS HUMAN CHECK** bevor Bestellung:
> 1. Genaue LCSC-Part-Number des Waveshare-2.0″-Moduls (das 1.9″-Modul hat
>    keinen LCSC-Eintrag, wurde via PiHut / Waveshare-Direct beschafft).
>    Anti-Guess-Regel (CLAUDE.md): keine LCSC-Nummer eintragen, bevor sie
>    am realen Bestellpfad bestätigt ist.
> 2. **8-Pin-Header-Reihenfolge** gegen das tatsächlich gelieferte Modul.
>    Der `PCB_FOOTPRINT_RISK_AUDIT.md` listet diese Verifikation schon für
>    das 1.9″-Modul als offen — derselbe Schritt gilt für 2.0″.
> 3. **Mechanische Außenmaße** des 2.0″-Moduls (PCB-Größe + Active-Area-
>    Position) gegen Bezel-Ausschnitt — die User-seitige Gehäuse-CAD muss
>    angepasst werden, bevor das Modul bestellt wird.

### D2 — Farbe firmware-seitig aktivieren (RGB565 nativ)

Heutiger Pfad: 4-bit Grau → RGB565-Konvertierung im Treiber. Das war OK fürs
monochrome OP-1-Look. Für Animationen mit Verlaufen, Akzentfarben (z. B.
World-Akzent pro Preset: blau/orange/violet/gold) brauchen wir **nativen
RGB565-Framebuffer**.

- Neuer FB: `uint16_t fb[OLED_WIDTH * OLED_HEIGHT]` in AXI-SRAM (154 KB)
- Bestehende Primitives (`oled_rect_fill`, `oled_rrect_fill`, AA-Text …) werden
  von „grey 0..15" auf „RGB565" generalisiert. AA bleibt über Alpha-Blend in
  den ARGB-Quellfarben (statt max-blend grau).
- **OP-1-Look bleibt Default:** der WORLD-Preset für Tokyo / Coast / Drive /
  After Hours definiert eine Akzentfarbe; das restliche UI bleibt
  schwarz/weiß/grau. Farbe ist *Akzent*, nicht Dekoration.

### D3 — Framebuffer in AXI-SRAM, nicht DTCM

Korrektur zum aktuellen Skeleton-Kommentar (`lcd_st7789_h743.c:34` sagt DTCM):
- **DTCM ist von H7-DMA1/DMA2 nicht erreichbar** (nur über MDMA). SPI-DMA
  fällt auf Polling zurück → CPU-Last → Audio-Risiko.
- **AXI-SRAM (512 KB, ab 0x24000000)** ist die richtige Region für FB. SPI-DMA
  + DMA2D können direkt darauf operieren.

### D4 — Animations-Architektur (3 Hebel, in Anwendungs-Reihenfolge)

1. **Dirty-Rectangles** — das Menü tut das de facto schon (nur die Slot-
   Region wird neu gerendert). Eine Animation animiert eine *Region*, nicht
   das ganze Bild. Beispiel: 80×80-Tween = 12,8 KB → 4 ms @ 24 MHz SPI →
   mühelos 60 fps für das animierte Element.
2. **SPI per DMA** (Step 13.3 TODO im Skeleton schon vermerkt) — Transfer im
   Hintergrund, CPU + 1 kHz Audio-IRQ ungestört. Pflicht für Animationen.
3. **DMA2D (Chrom-ART)** — H743-Hardware-Beschleuniger für Rect-Fill,
   Sprite-Blit, Alpha-Blend, Farbformat-Konvertierung. Genau die Operationen,
   die Animationen normalerweise CPU-belasten, sind damit „gratis".

Optional später: **SPI-Takt 24 → ~40 MHz** wenn Vollbild-Refresh nötig wird
(20 ms statt 51 ms für 240×320 RGB565). Anfangs nicht nötig — Dirty-Rects
reichen für UI-Tweens.

### D5 — Performance-Budget

| Operation | RAM-Read | SPI-Out @ 24 MHz | Verdict |
|---|---|---|---|
| Vollbild 240×320 RGB565 | 154 KB | 51 ms (~19 fps) | nur Boot-Splash / World-Wechsel |
| 80×80-Animations-Tile RGB565 | 12,8 KB | 4 ms (~250 fps Headroom) | 60 fps UI-Tweens trivial |
| Slot-Region (typ. 280×40 RGB565) | 22 KB | 7,4 ms (~135 fps Headroom) | Encoder-Drehen reaktionsfrei |

Bei DMA-getriebenem SPI ist die CPU für *keine* dieser Operationen blockiert
— die obigen Zeiten sind reine Übertragungs-Dauer im Hintergrund.

## Consequences

**Positiv:**
- +41 % Pixel + spürbar mehr Fläche bei minimaler Firmware-Änderung
- Y-Offset-Trick verschwindet → Treiber-Code einfacher
- Farbpfad wird real (Akzentfarben pro World), OP-1-Look bleibt als Default
- Animations-Architektur (DMA + Dirty-Rects + DMA2D) wird *jetzt* festgelegt,
  bevor das Display-Driver-Skeleton finalisiert wird (Step 13.3) — keine
  Doppelarbeit später

**Negativ:**
- Gehäuse-Fenster muss neu gezeichnet werden (User-CAD-Aufwand)
- BOM-Eintrag mit `UNVERIFIED`-Marker, bis Lieferant + LCSC + Pin-Order
  bestätigt sind (Anti-Guess)
- Framebuffer-Refactor von 4-bit-Grau auf RGB565 ist mehrere `oled_*`-
  Primitives — nicht riesig, aber non-trivial
- Mehr SRAM-Verbrauch (+127 KB FB-only; +281 KB falls Double-Buffer) — bleibt
  weit unter 512 KB AXI-SRAM

**Bewusst nicht geändert:**
- Backlight-Architektur (PCA9685-PWM + Q2/2N7002 Low-Side) bleibt
- SPI-Pins (PA5/PA6/PA7/PC4/PC5) bleiben
- Audio-Pfad bleibt unangetastet — die DMA-Trennung ist gerade *der Grund*,
  warum Animation und Audio sich nicht in die Quere kommen

## Open Items (vor Implementierung)

1. **User-side:** Waveshare-2.0″-Modul realer Quelle + Maße + Pin-Order
   verifizieren (siehe D1 UNVERIFIED-Block).
2. **Firmware:** Refactor-Spike: `oled_*`-API von `uint8_t gs` (4-bit Grau)
   auf `uint16_t rgb565` migrieren — ein Compile, alle Aufrufer wandern. Small
   commit, vor dem realen Panel-Wechsel.
3. **Generator:** Wenn die Pin-Order des 2.0″-Moduls vom 1.9″-Modul abweicht
   → `kicad/generate_kicad_project.py` Display-Sheet anpassen, **niemals**
   `.kicad_sch` direkt editieren (CLAUDE.md).

## Related

- ADR-0010 — Audio-Buffer / SAI-PLL (Audio-IRQ ist die heilige Kuh, die der
  DMA-Pfad schützt)
- SPEC §6 — ST7789-Konfiguration (1.9″) — wird durch D1 ersetzt sobald Pivot
  hard
- `field-ambience-current/PCB_FOOTPRINT_RISK_AUDIT.md` Punkt 7 — Pin-Order-
  Verification (gilt für 2.0″ identisch)
- CLAUDE.md — Anti-Guess-Regel für Footprints / LCSC-Nummern
