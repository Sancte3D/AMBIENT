# Schematic Walkthrough — A-Z für PCB-Engineers

Dieses Dokument ist die **menschenlesbare Tour durch das gesamte Schaltbild**.
Wenn du das hier von oben nach unten liest, weißt du am Ende:
- welche 7 Sheets es gibt und was jeder Block macht
- welche Bauteile drin sind und wozu jedes da ist
- wie die Signale fließen
- was kaputtgeht wenn ein Bauteil stirbt
- warum genau diese Bauteile gewählt wurden (nicht andere)

> Diese Datei ist **keine** Datasheet-Verifikation — die liegt in
> `field-ambience-current/docs/component_reviews/` (10-Punkte-Reviews gegen
> Hersteller-PDFs). Dieser Walkthrough ist die *Story*, jene Reviews sind die
> *Belege*. Beide Sichten brauchst du beim PCB-Layout.

## Wie liest man das hier?

1. **Das Schaltbild ist generiert.** Quelle: `kicad/generate_kicad_project.py`
   (~7000 Zeilen Python). **Nie die `.kicad_sch` direkt editieren** — alle
   Änderungen über den Generator. Pro Sheet steht im Walkthrough die
   Python-Funktion + Zeilenbereich, damit du beim Lesen springen kannst.
2. **Refdesigns** (`U1`, `J1`, `R12`) entsprechen genau dem, was im Schematic
   + auf der Platine steht. Alle MPNs + LCSC-Nummern in
   [`BOM_MASTER.md`](../../../BOM_MASTER.md).
3. **Aktive-Low-Netze** haben Suffix `_N` (z. B. `AMP_SHDN_N`) — siehe
   [AI-Ready Schematic Standard](AI_READY_SCHEMATIC_STANDARD.md).
4. **UNVERIFIED**-Markierungen sind echt — wo so steht, ist noch ungeprüft.
   Anti-Guess-Regel (CLAUDE.md) gilt für jedes Bauteil ohne komplette
   Datasheet-Auflösung.

## Übersicht der 7 Sheets

| Sheet | Datei | Generator (`generate_kicad_project.py`) | Was es macht |
|---|---|---|---|
| 1 | `power_tree.kicad_sch` | `make_power_tree_sheet()` ~Z. 1887 | USB-C → Polyfuse → Bulk-Caps → +3V3-Rail, Boost-Konverter für Speakers, Akku-Eingang |
| 2 | `stm32h743.kicad_sch` | `make_stm32h743_sheet()` ~Z. 3424 | MCU + HSE-Crystal + Reset + BOOT + Decoupling + VCAP + VDDA |
| 3 | `lcd.kicad_sch` | `make_lcd_sheet()` ~Z. 3533 | ST7789-Modul-Header J3 + Backlight-FET Q2 + lokale Caps |
| 4 | `mcp.kicad_sch` | `make_mcp_sheet()` ~Z. 4560 | MCP23017 (16 I/O over I²C) + PCA9685 (16 PWM für LEDs) + 5 Hall-Sensoren + 10 Buttons + 10 LEDs |
| 5 | `encoder.kicad_sch` | `make_encoder_sheet()` ~Z. 4845 | 4 EC11-Encoder mit Push + RC-Filter |
| 6 | `audio.kicad_sch` | `make_audio_sheet()` ~Z. 4878 | PCM5102A I²S-DAC + PAM8403 Class-D-Amp + Speaker-Header + 3.5-mm-Line-Out + (DNP) MIDI-Out |
| 7 | `battery.kicad_sch` | `make_battery_sheet()` ~Z. 5956+ | MCP73831 Lade-IC + Akku-JST + Power-Path-Switching (USB ↔ Akku) + Bat-Sense-Divider |

Plus das Top-Level `field_ambience.kicad_sch` — verbindet die 7 Sheets über
Hierarchical Labels.

---

## §1 — Power Tree (Sheet 1)

### Was es macht

Aus dem USB-C-Stecker (5 V von extern) oder dem Akku (3.0–4.2 V LiPo) macht
dieser Block die einzige Logik-Versorgung des Geräts: **+3V3** auf der ganzen
PCB. Zusätzlich erzeugt ein **Boost-Konverter** aus dem Akku +5 V für die
Speaker-Endstufe (PAM8403 mag keinen direkten LiPo-Range). Eingangs-Schutz
gegen Überstrom + ESD ist hier verortet.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `J1` | USB-C Receptacle TYPE-C-31-M-12 | Eingang 5 V + USB-D±-Daten + CC (Power-Negotiation 5 V default) | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` |
| `F1` | Polyfuse 1812L300 (3 A hold, 6 A trip) | Hard-Short-Schutz auf VBUS — bei Bass-Peaks (2,45 A worst case) hält, bei Kurzschluss trippt | `Fuse:Fuse_1812_4532Metric` |
| `D1` | USBLC6-2SC6 (TVS) | ESD-Diodenarray für VBUS + D+ + D− (8 kV Kontakt, 15 kV Air) | KiCad-Standard SOT-23-6 |
| `Q1` | DMG2305UX (P-MOSFET) | Power-Path: schaltet automatisch zwischen USB-VBUS (bevorzugt) und Akku — Body-Diode invertiert, Vgs durch USB-Anwesenheit gesteuert | `Package_TO_SOT_SMD:SOT-23` |
| `U8` | TPS61089RNR (Boost-Konverter) | Akku 3,0–4,2 V → +5 V für PAM8403 (Speaker-Amp). Wenn USB anliegt: direkter Pfad ohne Boost | `field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` (Custom) |
| `L1` | Sunlord SWPA6045 1 µH | Boost-Speicherdrossel für `U8` | `field_ambience:L_Sunlord_SWPA6045` (Custom) |
| `U5` | AP7361A-33ER (LDO) | +5 V → +3V3, 1 A, low-noise (für Analog-Audio kritisch) | KiCad-Standard SOT-223 |
| `C_BULK` | 1000 µF 16 V Alu-Elko | Reservoir-Kondensator für Class-D-Bass-Peaks. *Wichtigster anti-Brumm-Hebel* (ADR-0010 §4) | KiCad-Standard 8 mm-Elko |
| diverse | 4,7 µF + 100 nF X7R | Lokales Decoupling pro IC | 0603 |

### Wie es fließt

```
USB-C VBUS ──F1── Power-Path Q1 ──┬── +5V_RAIL ──┬── PAM8403 (Speakers)
                                  │              └── AP7361A LDO ──┬── +3V3_RAIL
LiPo-Akku ────────TPS61089 Boost──┘                                 │
                                                                    └── alle Logik (MCU, MCP, PCA, DAC, LCD)
```

USB-D+/D− gehen durch `D1` zum MCU (USB-OTG-FS für Firmware-Updates per DFU).

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `F1` polyfuse trippt | Gerät komplett tot bis Polyfuse abgekühlt (~30 s) | warten oder echte Ursache (Kurzschluss?) finden |
| `Q1` P-MOSFET | USB lädt, Akku-Betrieb tot — oder umgekehrt | Q1 ersetzen |
| `U8` Boost | Speakers tot, Logik (3V3) lebt | U8 + L1 prüfen |
| `U5` LDO | Gerät komplett tot (kein 3V3) | LDO prüfen — kann thermisch aus, wenn Layout-Ground schlecht |
| `C_BULK` | Brummt bei Bass, klingt schlecht, evtl. Boost-Aussetzer | Elko tauschen, Polarität prüfen |

### Warum gerade diese Wahl?

- **TYPE-C-31-M-12** statt 6-Pin-Variante: Brauchen Daten (USB-DFU), nicht nur
  Power. 16-Pin-Original (r18.19-Revert) garantiert D+/D−/CC am Pad.
- **3 A Polyfuse statt 2 A**: 2,45 A Bass-Peak gemessen → 2 A würde trippen
  bei lauter Wiedergabe. 3 A hat ~25 % Reserve unter 50 °C-Innentemp.
- **TPS61089 + Sunlord-Drossel**: Boost-Konverter mit niedrigem Schaltrauschen
  unter Audio-Band. Wichtig: Bulk-Cap muss **< 5 mm** vom PAM8403-PVDD-Pin
  liegen, sonst koppelt Class-D-Switching auf den DAC-Output (ADR-0010 §4).
- **AP7361A-33ER LDO** statt einfacher Boost-direkt-zu-3,3V: LDO ist *low-noise*,
  Boost ist *switcher*. Audio-Analog hängt am LDO-3V3, nicht am Boost-Output —
  sonst hört man das Switching im Headphone-Out.
- **Power-Path-MOSFET Q1**: hält den Akku frisch, wenn USB angeschlossen ist
  (lädt + speist gleichzeitig).

### Sleep-Architektur (ADR-0016 — geplant)

Geplante Hardware-Erweiterung in diesem Sheet:
- `U9` TPS22918DBVR Load-Switch (SOT-23-6) gated den 3V3-Pfad zu den 5
  Hall-Sensoren (HALL_VDD-Net) — diese ziehen ~17,5 mA und sind im Sleep der
  echte Stromfresser, nicht der MCU. Mit Load-Switch fallen wir auf <100 µA
  Idle. Siehe [ADR-0016](../decisions/ADR-0016-power-sleep-architecture.md).

---

## §2 — STM32H743VIT6 (Sheet 2)

### Was es macht

Der MCU ist das **Herz**: hostet die komplette Audio-DSP-Engine (Pad / Reverb
/ Bass / Texture / Drone — siehe `firmware-c-next/src/*.c`), liest Cells +
Encoder + Buttons, treibt das Display, spuckt I²S an den DAC raus. ARM
Cortex-M7 480 MHz mit FPU + 1 MB SRAM + 2 MB Flash, LQFP-100-Gehäuse.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `U1` | STM32H743VIT6 | Der MCU selbst | `Package_QFP:LQFP-100_14x14mm_P0.5mm` |
| `Y1` | ABLS-8.000MHZ-B4-T (Crystal) | HSE-Takt für PLL → SYSCLK 480 MHz; Audio-PLL3 für saubere SAI-Clock (ADR-0010 §3) | `field_ambience:Crystal_HC49-US-SMD_ABLS` (Custom) |
| `C_HSE_1/2` | 18 pF C0G | Load-Caps für Y1, auf den ESR + GM=2,97 abgestimmt (Crystal-Review F-4) | 0603 |
| `C_VCAP1/2` | 2× 2,2 µF X5R (ESR < 100 mΩ) | Interner SMPS-Mode-Bypass (Datasheet Table 24) | 0805 |
| `R_NRST` | 10 kΩ Pull-Up | externer NRST-Pull-Up (interner ~30–50 kΩ wäre ausreichend, externer dominiert + EMI) | 0603 |
| `C_NRST` | 100 nF | NRST-Spike-Killer | 0603 |
| `R_BOOT0` | 10 kΩ Pull-Down | BOOT0 default = 0 → Boot aus Flash. Drücken von `SW_BOOT` zieht hoch → Boot in DFU-Bootloader | 0603 |
| `SW_BOOT` | XUNPU TS-1088-AR02016 (Mini-SMD-Tact) | BOOT0-Override für USB-DFU-Flash. **Wird ab ADR-0016 zusätzlich Wake/Sleep-Taster** | `field_ambience:SW_TS1088_SMD` (Custom) |
| `SW11` | gleiches Teil wie `SW_BOOT` | Manueller Reset auf NRST | gleicher FP |
| Decoupling | 5× (4,7 µF + 100 nF) auf VDD-Pins | Lokales Bypassing, je 1 Paar pro VDD-Pin (Pins 11, 27, 50, 75, 100) | 0603 |
| VDDA-Filter | Ferrit + 1 µF + 100 nF | VDD → VDDA Trennung, dämpft Digital-Switching auf den Analog-Pfad | Ferrit 0805, Caps 0603 |

### Pin-Allocation (Stand SPEC v0.7 §5 — verifiziert in U1-Review)

| Funktion | Pin | Net |
|---|---|---|
| Audio I²S out | 4/5/6 | I2S_BCK, I2S_LRCK, I2S_DOUT (SAI1-A AF6) |
| LCD SPI | 29/31/30 + 32/33 | LCD_SCK, LCD_MOSI, LCD_CS, LCD_DC, LCD_RES |
| I²C (MCP+PCA) | 92/93 | I2C_SCL, I2C_SDA (AF4) |
| MIDI TX | 86 | MIDI_TX (UART, DNP-Variante) |
| Encoder TIMs | div. | DRIVE_A/B, BRIGHT_A/B, VOL_A/B, DISPLAY_A/B (Hardware-Quadratur über TIM1–5) |
| USB-OTG-FS | 70/71 | USB_DM, USB_DP (AF10) |
| SWD | 72/76 | SWDIO (PA13), SWCLK (PA14) |
| HSE | 12/13 | OSC_IN, OSC_OUT |
| BAT_SENSE | 25 | PA3 (ADC1_INP15) — Akku-Spannungsteiler |

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `Y1` Crystal | MCU bootet, läuft auf HSI (intern, 64 MHz), Audio-Clock falsch → Pitch + Sample-Rate verstellt | Crystal-Lötstellen prüfen; im Datasheet GM=2,97 ist Indoor-OK aber knapp |
| `C_HSE_1/2` | Crystal startet evtl. nicht (Lastkapazität fehlt) | Caps nachlöten, Wert prüfen |
| Decoupling fehlt | Random Resets unter Last, USB-DFU schlägt fehl, Audio knackt | Phase-5-Profiling-Phase erkennt das; Layout-Constraint: jedes Paar < 3 mm vom VDD-Pin |
| `SW_BOOT` defekt | DFU-Flash nicht möglich (Workaround: BOOT0 mit Drahtbrücke nach 3V3 ziehen) | Taster ersetzen |
| VDDA-Ferrit Kurzschluss | ADC + Audio voll von Digital-Noise → hörbar als Grießeln | Ferrit ersetzen |

### Warum gerade diese Wahl?

- **STM32H743 statt H7B0 / H750**: 2 MB Flash + 1 MB SRAM gibt Reverb-Tails +
  Brain-State problemlos Platz; SAI1 + PLL3 ergeben saubere Audio-Clock (PLL3
  speist SAI direkt → kein SYSCLK-Jitter, ADR-0010 §3). 480 MHz mit VOS0
  reicht für die DSP-Last + UI.
- **LQFP-100 statt BGA**: handlötbar, JLC kann es bestücken, kein X-Ray
  nötig. BGA wurde bewusst verworfen.
- **8 MHz HSE statt MEMS-Oszillator**: ABLS-Crystal mit 80 Ω ESR, GM=2,97
  Worst-Case — Indoor-Use real ~5–6 (Crystal-Review F-4). Bewusst akzeptiert,
  spart 1–2 $.
- **Interner SMPS-Mode**: nutzt VCAP-Caps statt externer DC/DC. Kleiner +
  einfacher. ADR-0010 §3 bestätigt: Audio-Clock kommt aus PLL3 (nicht
  SYSCLK), daher koppelt der SMPS nicht auf BCK.

---

## §3 — Audio Chain (Sheet 6)

### Was es macht

Die zweite Hälfte der Sound-Pipeline. MCU spuckt **I²S 16-Bit 44,1 kHz** an
einen externen DAC (`U3`), der DAC fährt entweder die Class-D-Speaker-Endstufe
(`U4`) ODER die Line-Out-Buchse `J8`. **Sub-Bass-Layer geht NUR an Line-Out**
(ADR-0010 §6) — die 40-mm-Speaker können keinen Sub-Bass, das wäre nur
Geklapper.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `U3` | PCM5102APWR | I²S → Stereo-DAC, 32-Bit-Resolution, interne PLL (synct sich auf BCK ohne MCLK). Eigene AVDD-Versorgung über Ferrit-Bead. | TSSOP-20 KiCad-Standard |
| `U4` | PAM8403DR-H | Stereo Class-D-Amp, 3 W/ch @ 4 Ω, BTL-Output. `AMP_SHDN_N` (active-low Shutdown) vom MCP23017 gated. | SO-16-150mil KiCad-Standard |
| `J7` | Speaker-Header 2×2 Pin (PUI AS04008PS, 8 Ω, 40 mm) | Speaker-Anschluss BTL — 2 Drähte pro Kanal | Pin-Header 2,54 mm |
| `J8` | PJ-320D 3,5 mm TRS (mit Insertion-Detect) | Line-Out / Kopfhörer. Insertion-Detect-Pin gated optional die Speaker-Amp (Auto-Mute beim Einstecken) | `field_ambience:Jack_3.5mm_PJ-320D_SMT` (Custom EasyEDA-CAD) |
| `J9` | PJ-320D MIDI-OUT — **DNP für 5er-Run** (ADR-0004 r18.30) | 2× 220 Ω Resistor pair + UART-TX. Reaktivierbar durch Bestücken + `midi_tx_init()` | gleicher FP, DNP |
| `FB1` | BLM18AG601 (Ferrit-Bead) | AVDD-Trennung DAC (Digital-Rail → Analog-Rail) | 0603 |
| `C_AVDD` | 10 µF + 100 nF X7R am Ferrit-Output | DAC-AVDD-Decoupling | 0603 |
| `C_AMP` | 1 µF + 100 nF auf PAM8403 PVDD | Amp-lokales Decoupling | 0603 |

### Wie es fließt

```
STM32H743 SAI1
   │ BCK/LRCK/DOUT
   ▼
PCM5102A (U3)
   │ Analog L/R
   ├──► PAM8403 (U4) ──► J7 Speaker-Header ──► 2× PUI AS04008PS 40 mm
   │           ▲
   │           │ AMP_SHDN_N (MCP23017 GPA4) — Mute aus Firmware
   │
   └─────────► J8 3,5 mm Line-Out (TRS) ──► Kopfhörer / Mixer / Aktiv-Box
                   │
                   └─ Insertion-Detect ──► MCP23017 (optional Auto-Speaker-Mute)
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U3` PCM5102A | Komplett stumm an allen Outs (Line-Out + Speakers) | DAC oder I²S-Verkabelung prüfen |
| `U4` PAM8403 | Speakers stumm, Line-Out lebt | Amp prüfen — oft thermisch oder Strapping-Pin falsch |
| `FB1` Ferrit | Digital-Switching grießelt im Headphone-Out | Ferrit tauschen |
| `J7` Speaker-Header lose | Speaker brüllt, Brummen, evtl. Amp thermisch | Header neu löten |
| Speaker fällt aus Mesh | Hörbar dünn — und mechanisch oft Folge eines lose gewordenen Mesh-Klebepunkts | Membran + Mesh checken (ADR-0007) |

### Akustik — was die PCB **nicht** löst (mechanische Aufgaben)

Klingt das Gerät am Ende wie Plastik-Spielzeug? Hängt **nicht** an der PCB,
sondern an drei mechanischen Dingen:

1. **Geschlossene Akustikkammer pro Speaker** — der wichtigste Hebel. 40 mm
   Driver in offener Plastikhülle = blechern. 40 mm Driver in versiegelter
   ~5 cm³-Kammer + Dust-Mesh (ADR-0007) = überraschend gut, OP-1-Niveau. **Im
   Gehäuse-CAD vorsehen — kein BOM-Eintrag, aber kritisch.**
2. **EQ-Pre-Filter im Pad-Render-Pfad** — der DSP rollt unter 80 Hz ab fürs
   Speaker-Routing (ADR-0010 §6 + `engine.c`), damit der Treiber nicht
   mechanisch überlastet wird. Sub-Bass landet nur am Line-Out.
3. **Passive Membran** — *optional* on top der geschlossenen Kammer. Zusatz-
   Bauteil (25–30 mm PR-Membran pro Seite, ~1–3 $/Stück), erweitert f3 von
   ~250 Hz auf ~150 Hz. Nicht critical-path; entscheiden nach Hörtest auf
   Prototyp.

**Honest take:** Line-Out (`J8`) ist die *echte* Hörerfahrung. Speakers sind
"convenience ohne Kopfhörer" — das Gerät richtig zu positionieren ist Punkt
des Sound-Designs, nicht ein Versuch, 40 mm zu HiFi zu prügeln.

### Warum gerade diese Wahl?

- **PCM5102A statt billigerer PT8211**: Interne DAC-PLL synct sich auf BCK
  → kein externer MCLK nötig → ein SAI-Pin weniger. Saubere 112 dB SNR.
- **PAM8403 statt MAX98357A I²S-Amp**: PAM8403 ist analoger Class-D — wir
  *wollen* den Analog-Pfad zwischen DAC + Amp, damit der Line-Out gleichzeitig
  möglich ist (MAX98357 hat keinen Analog-Output).
- **8 Ω / 40 mm PUI statt 4 Ω / 28 mm**: 40 mm gibt physikalisch mehr Membran-
  Fläche → mehr Mid-Lautstärke (alles unter 250 Hz ist sowieso nur am
  Line-Out). 8 Ω passt zur PAM8403-Optimierung.
- **PJ-320D-Jack mit Insertion-Detect**: Auto-Mute der Speakers beim
  Einstecken — Standard-User-Erwartung.

---

## §4 — MCP23017 + PCA9685 + Cells + LEDs (Sheet 4)

> **In Arbeit.** Inhalt folgt in der nächsten Walkthrough-PR. Bis dahin:
> - Generator: `make_mcp_sheet()` ~Z. 4560 in `generate_kicad_project.py`
> - Bauteile: `U2` MCP23017 (I²C-GPIO-Expander, 16 Kanäle für Buttons +
>   Mux-Signale), `U6` PCA9685 (I²C-PWM, 16 Kanäle für die LEDs +
>   Backlight-FET-Steuerung), 5× DRV5056A4 Hall-Sensoren (Cells), 10 LEDs,
>   10 Mini-Buttons.
> - Sleep-Detail: HALL_VDD wird durch `U9` (ADR-0016) gegated — die
>   3,5 mA/Sensor sind der Sleep-Strom-Killer.

## §5 — Encoders (Sheet 5)

> **In Arbeit.** Inhalt folgt.
> - Generator: `make_encoder_sheet()` ~Z. 4845
> - Bauteile: 4× ALPS EC11E18244AU (mit Push), RC-Filter pro Kanal, alle vier
>   Encoder identisch (ADR-0012 — uniformer Part wegen NRND-Familie).

## §6 — LCD (Sheet 3)

> **In Arbeit.** Inhalt folgt.
> - Generator: `make_lcd_sheet()` ~Z. 3533
> - Bauteile: `J3` 1×8 Pin-Header für Waveshare 1.9" ST7789V2-Modul (PROPOSED
>   Pivot 2.0″ in ADR-0015), `Q2` 2N7002 Backlight-FET, lokale Caps.

## §7 — Battery & Power-Path (Sheet 7)

> **In Arbeit.** Inhalt folgt.
> - Generator: `make_battery_sheet()` ~Z. 5956
> - Bauteile: `U7` MCP73831T-2ACI/OT (LiPo-Charger), `J_BAT` JST-PH 2.0,
>   2000-mAh-LiPo-Pouch, Charge-LED, Akku-Sense-Divider auf ADC1_INP15.

---

## Maschinenlesbare Anker

Damit ein Tool (oder ein anderer Engineer) das hier durchsuchen kann, gilt:
- Jeder Refdes erscheint **genau wie auf der Platine** (Großbuchstaben + Zahl,
  keine Variation).
- Aktive-Low-Netze haben `_N`-Suffix; Differentialpaare `_P`/`_N`.
- Footprints sind als `<Library>:<Name>` notiert wie in KiCad. Custom-Lib =
  `field_ambience:*`, sonst KiCad-Standard.
- Cross-Refs zu BOM, ADRs und Component-Reviews als relative Markdown-Links.

## Erweiterung dieses Dokuments

- **Neuer Block?** Sektion mit "Was es macht / Bauteile / Wie es fließt / Was
  kaputt geht / Warum" einfügen. Format ist absichtlich rigide.
- **Bauteil-Wechsel?** Hier korrigieren UND
  [`BOM_MASTER.md`](../../../BOM_MASTER.md) UND den Generator. Anti-Drift-
  Regel: drei Stellen, alle gleich.
- **UNVERIFIED → verified:** Wenn du Datasheet-Belege hast, im
  [`component_reviews/`](../component_reviews/) ein 10-Punkte-Review
  anlegen + hier die UNVERIFIED-Markierung entfernen.
