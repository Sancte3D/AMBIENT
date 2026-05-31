# Field Ambience PCB — CHANGELOG

Vollständige Änderungshistorie der PCB-Spec und des KiCad-Schematic.
Die Spec-Body selbst (`field_ambience_pcb_SPEC_v0.6.md`) beschreibt
**immer den aktuellen Stand** — diese Datei trackt wie wir dahin kamen.

Aktuelle Rev: **v0.6.3-r7.1** (Industrial-Design-Spec ergänzt — Knob-Style, Cap-Strategie, USB-C-Upgrade-Intent). Pi-frei (v0.9) bleibt weiterhin der maßgebliche Audio-Stand — r7/r7.1 sind orthogonal zur Audio-Architektur.

---

## v0.6.3-r7.1 (2026-05-31) — Industrial-Design-Spec (Knobs / Caps / USB-C)

**Was**: Premium-Feel-Decisions für die user-touchable Komponenten, basierend
auf einem Aesthetik-Audit (Studio-Instrument als Design-Sprache, nicht Kirmes).
Keine elektrische SPEC-Änderung — die Decisions sind Industrial-Design
+ Procurement-Spec für user-supplied Teile.

**SPEC §2.1 NEU**:
1. **Knobs für EN1-EN4** (4×): brushed Aluminium 20 mm Ø, 6 mm flatted bore,
   matt-schwarz / gun-metal eloxiert, warm-weißer Top-Linie-Indikator
   (matched LED-Farbschema r7). 30 mm Encoder-Pitch lässt 10 mm Gap →
   ergonomisch greifbar. Mass gibt EC11-Detent das „teure" Gefühl unabhängig
   vom Encoder-Hersteller.
2. **Caps für SW1-10** (10×): uniform warm-gray / charcoal (RAL 7016 oder
   ähnlich), KEINE Farbcodierung pro Funktion — die LEDs (PCA9685) sind die
   semantische Schicht. Cells = MBK Bigseat 2u Choc V2; Modifier = 3D-Print
   Custom für 12×12-Plunger. Labels SHIFT/HOLD/DRONE/GEN/CLR lasergraviert.
3. **USB-C-Premium-Upgrade-Intent**: für die Produktions-Charge JAE
   DX07S016JJ1 / Amphenol-Equivalent (≥10000 Cycles vs. ~5000 Generic).
   Sourcing-Pass + JLC-Stock-Verify als r7.1-B4 dokumentiert. Prototyp läuft
   weiter mit C165948.

**Begründung als Block** (Aesthetik-Konsistenz):
- Existierende Choices definieren bereits einen Studio-Look: 256×64 mono-white
  OLED + warm/amber LEDs + schwarzer Soldermask + weiße Silkscreen.
- Wood-Knobs oder bunte Caps würden diesen Look brechen.
- Premium-USB-C ist der höchste UX-Hebel pro $-Aufpreis (täglich gesteckt,
  $1.20 mehr, 2× Lebensdauer).
- Cap-Farbcodierung wäre redundant mit den LED-States → uniform-Caps =
  visuelle Ruhe.

**Files**: SPEC §2.1 (~80 Zeilen NEU), mechanical_coordinates.md §6 (Knob-
Stack-Höhe), MEINE_TODO (r7.1-Block: 3 Procurement-Decisions).

---

## v0.6.3-r7 (2026-05-31) — Modifier-Switches: momentary tactile + LED-Statusanzeige

**Was sich ändert (UX-Treiber)**: SW6-SW10 waren in v0.6 als 1u Choc V2 Hot-Swap
geplant — also latching-Caps mit physisch sichtbarem ON/OFF-Zustand. Problem
beim Preset-Recall: ein latching-Switch in „falscher" Position widerspräche dem
geladenen Snapshot. Lösung: **alle 5 Modifier sind jetzt momentary tactile
(12×12×7.3 mm) mit integrierter LED**. State lebt in Firmware, LED zeigt den
Zustand an, Snapshot-Recall setzt Firmware-Var + PCA9685-Kanal kongruent.

**Hardware-Änderungen:**
- **+ U6 PCA9685PW,118** (NXP, TSSOP-28, LCSC C2678753, JLC Extended, ~1605 pcs)
  als 16-Kanal-PWM-LED-Driver. I²C-Adresse 0x40, gleicher Bus wie MCP23017.
  Symbol `Driver_LED:PCA9685PW`, Footprint `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm`.
- **+ R_LED6 .. R_LED10**: 5× 390 Ω 0603 LED-Series (open-drain Sink-Config: LED-Anode
  → R → +5 V → LED → PCA9685-Kanal → GND).
- **+ R_OE**: 10 kΩ 0603 Pull-Up an /OE zu +3V3 (Default-disabled bis Firmware enabled).
- **+ C_PCA_VDD / C_PCA_VDD_HF**: 10 µF X5R 0805 + 100 nF X7R 0603 Decoupling an Pin 28.
- **SW6-SW10 umdefiniert**: weg von Kailh Choc V2 1u Hot-Swap → **12×12×7.3 mm
  momentary tactile MIT integrierter LED**, Generic China / AliExpress
  („Momentary Touch LED 12*12*7.3 mm"). User-supplied + hand-soldered
  (analog zu SW1-5 Cells). **Custom-Footprint** in Projekt-PCB-Lib nötig
  (siehe `mechanical_coordinates.md` §5).

**Firmware-Verträge (nicht implementiert, dokumentiert):**
- SW6 SHIFT: momentary Modifier während gedrückt → Cells liefern Degrees 6-10
  statt 1-5. LED6 leuchtet solange Switch gedrückt.
- SW7 HOLD: Press toggelt HOLD-Mode. LED7 = HOLD aktiv.
- SW8 DRONE: Press toggelt Drone. LED8 = Drone spielt, optionaler Fade-In/Out.
- SW9 GENERATE: Press toggelt Generative-Mode. LED9 = generative aktiv.
- SW10 CLEAR: Press = one-shot „release all holds + reset patterns". LED10
  flasht ~200 ms als Visual-Confirmation.

**Power-Budget**: +5 mA Idle / +25 mA Worst-Case (PCA9685 + 5 LEDs @ 100 %
PWM). Worst-Case-Summe steigt 2480 mA → 2525 mA — Polyfuse F1 (3 A) bleibt
ausreichend dimensioniert.

**Verifikations-Blocker für r7 (vor PCB-Layout)**:
- Pin-Pitch der AliExpress-12×12-Switches real vermessen (Generic-Parts haben
  keinen herstellergemeinsamen Standard — Annahme im SPEC: 6.5×4.5 mm
  Switch-Raster + ~8 mm zum LED-Pin-Paar).
- `Driver_LED:PCA9685PW` Symbol-Pin-Map gegen NXP-Datasheet Rev. 4 S.6 prüfen.

**Aktive Aufgabe**: Schematic-Generator + KiCad-Sheets müssen die r7-
Änderungen abbilden (U6 hinzufügen, SW6-10 Symbol-Form umstellen, LED6-10
hinzu). Wird in eigenem Commit nachgezogen.

---

## v0.9 (2026-05-30) — Pi-frei-Transition (Schaltplan, Step 6 von 12)

Teil der nativen Portierung (`NATIVE_PORT_PLAN.md`). Nachdem die C-Firmware
auf dem RP2350 Display, Buttons, Encoder und **I²S-Audio** (Step 1–5) selbst
beherrscht, fällt der Raspberry Pi Zero 2 W aus dem Gerät.

**Schaltplan-Änderungen (`generate_kicad_project.py`):**
- `pi.kicad_sch` (Sheet 7, Pi-Header) **gelöscht**; aus kicad_pro-Sheetliste + Root entfernt.
- **5 Bauteile raus**: J2 (40-pin Pi-Header), R1 (UART-RX-Serie), R_BCK/R_LRCK/R_DOUT
  (Pi-seitige I²S-Serien-R). Reale Bauteilzahl **97 → 92**.
- GP0/GP1/GP4 im Pico-Sheet von UART/MISO → **I²S_BCK/LRCK/DOUT** umgewidmet
  (RP2350 PIO-I²S-Master). Root-Sheet brückt Pico-I²S-Outputs → Audio-Sheet-Inputs.
- D2 (SMAJ5.0A TVS) **bleibt** — sitzt auf der +5V-Hauptschiene (nicht am Pi),
  dient weiter als allgemeiner Rail-Surge-Schutz.

**Verifikation:** Generator regeneriert sauber, alle Sheets paren-balanced,
entfernte Nets (PICO_TX_PI_RX/PI_TX_PICO_RX/OLED_MISO_NC) verschwunden,
I²S-Nets durchgehend Pico→Root→Audio. kicad-happy-Analyzer: 6 VM-001-Blocker,
**alle die bekannte False-Positive-Klasse** (Pico-VBUS-Pin → Heuristik tagged
GPIOs als 5V; real GP0/1/4 @ 3V3 → PCM5102A @ 3V3, kein Level-Shifter nötig).
Warnings 19 → 16. GUI-ERC (B3) bleibt maßgeblich.

**Folge-Arbeit (offen):** SPEC §1/§3 (Architektur-Diagramm, Power-Budget mit
Pi-Zeile) sind noch Pi-zentrisch → SPEC-v0.9-Überarbeitung; Power-Budget sinkt
~700 mA, F1 darf optional kleiner werden.

---

## v0.8 (2026-05-28) — Komponenten-Audit: BOM ↔ Schematic abgeglichen

Vollständiger Cross-Check der Stückliste (SPEC §3 Decoupling-Tabelle + §4 BOM)
gegen jede tatsächlich im Generator/Schaltplan platzierte Komponente. Drei
Diskrepanzen gefunden und korrigiert:

- **🔴 C1 + C2 fehlten im Schaltplan** (Fix): SPEC §3 listet C1 (10µF X5R 0805)
  und C2 (100nF X7R 0603) als +5V-Hauptrail-HF-Decoupling, und §4 zählt sie mit
  (6× 10µF / 8× 100nF) — aber `power_tree_sheet()` platzierte sie nie (nur 5×/7×
  real). Die Rail hatte nur C_BULK (1000µF Elko, hohe ESR/ESL → schlechte HF-
  Antwort) + D2 TVS, aber keinen Keramik-HF-Bypass. **C1/C2 jetzt platziert**
  (Rail-Taps x=80/83, GND-Drop, Auto-Junction). 10µF-Count jetzt = 6 (BOM-konform).
- **🟠 C_audio_filt aus BOM gestrichen**: Der §4-Eintrag (2× 220nF "PCM5102A output
  filter") war nie im Schaltplan. PCM5102A hat internen Rekonstruktions-Filter,
  TI-Referenz nutzt keinen Output-Cap, und §8 Line-Out sagt "keine Koppel-Caps
  nötig". Doku an Realität angeglichen (§4 + §8 Notiz).
- **🟡 R_RUN in BOM ergänzt**: Pico RUN-Pull-up (10k 0603) war im Schaltplan,
  fehlte aber in §4. Jetzt gelistet.
- **Stale F1-BOM-Wert gefixt**: §4-Misc-Tabelle zeigte noch "2.0A/4.0A" — auf
  v0.7-Stand 3.0A/6.0A (Littelfuse 1812L300) gebracht (war in §3 schon korrekt).

Methodik-Hinweis: Reiner Audit-/Reconcile-Pass, keine neue Funktion. ERC im
KiCad-GUI (Blocker B3) bleibt offen — headless nicht durchführbar.

---

## v0.7 (2026-05-22) — Alle offenen Engineering-Entscheidungen geschlossen

Durchgang zum Schließen sämtlicher offener Auslegungs-Fragen, damit kein
"TBD" / keine Entscheidung mehr im Weg steht. Design und Doku reconciled.

**Power (I1/I2) — final entschieden:**
- 5V/3A-USB-C-Netzteil als **harte Anforderung** (kein PD-Controller im
  Prototyp). Worst-Case 2.45A < 3A → Board darf voll aussteuern, Volume-Clamp
  ist keine Power-Schutz-Pflicht mehr (nur optionale Akustik-Maßnahme).
- **F1 Polyfuse 2A/4A → 3A/6A** (Littelfuse 1812L300, LCSC C18198349). Das alte
  2A-hold derated bei ~50°C Innentemp auf ~1.5A → trug 1.4A-Typical-Audio nicht
  zuverlässig. 3A-hold derated ~2.3A deckt Typical; Bass-Peaks reiten den Bulk.

**Inrush (I3) — final entschieden:**
- Erkenntnis: Inrush-**Peak** ist widerstandsbegrenzt, nicht kapazitätsbegrenzt
  — ein kleinerer Cap senkt nur Dauer/Energie, nicht den Spitzenstrom. Da tiefer
  Bass das Produktziel ist und der Bulk genau die Bass-Transienten puffert,
  **bleibt C_BULK bei 1000µF**. Polyfuse ist thermisch → trippt nicht auf den
  <1ms-Inrush-Spike. Produktion bekommt einen Soft-Start-Load-Switch.
- **Footprint-Bug gefixt**: EEE-FK1A102P ist ein D10×10.2mm-Becher, der Generator
  hatte `CP_Elec_8x6.7` (zu klein) → korrigiert auf `CP_Elec_10x10.5`.
- **Doku/Design-Konflikt aufgelöst**: SPEC nannte fälschlich eine Polymer-Cap
  6.3V (PCV1A102), die nie ins Design kam. SPEC zeigt jetzt die reale Alu-10V.

**Stack-Up (I4):** final Signal/GND/+5V/Signal (SPEC §9).

**Mechanik (I5):** alle TBD-Positionen festgelegt — OLED-J3 @ (80,95) auf
Standoffs, Pi-J2 @ (160,90), Pico-U1 @ (270,80). `mechanical_coordinates.md`
von "Template/Draft" auf "alle Positionen definiert" hochgestuft.

**Bereits erledigt, nur noch als ✅ markiert:** I7 (UART-Naming eindeutig),
N1 (XSMT via MCP GPA5), N2 (I²S 33Ω Serien-R), N4 (Titleblocks → rev 0.7),
B4 (SHDN/MUTE Pull-Downs vorhanden).

**Verbleibend offen (nur noch GUI/physisch, nicht headless lösbar):**
B0-B2 (Footprint-Pad-Mapping im KiCad-Footprint-Editor gegen Datenblatt),
B3 (GUI-ERC-Lauf), sowie der eigentliche PCB-Layout-Schritt.

---

## v0.7-pre (2026-05-16) — Choc-V2-Footprint-Fix + Line-Out/Kopfhörer

Erste echte Funktionserweiterung seit v0.6. Zwei Themen.

### 1. Switch-Footprint-Mismatch behoben (Choc V2)

- **Bug**: Generator referenzierte `Button_Switch_Keyboard:SW_Hotswap_Kailh_MX_*`
  (MX-Hotswap) während BOM/SPEC durchgehend **Kailh Choc V2** sagen. MX und
  Choc V2 sind physikalisch inkompatibel (Pin-Spacing, Höhe, Keycap-Stem).
- **Fix**: Footprints auf `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_*`
  geändert (1.00u Modifier, 2.00u Cells).
- **Library**: Diese Footprints sind NICHT in der KiCad-Standard-Lib. Benötigt
  die **kiswitch keyswitch-kicad-library**:
  - KiCad → Plugin & Content Manager → Libraries → "Keyswitch Kicad Library" → Install
  - GitHub: https://github.com/kiswitch/keyswitch-kicad-library
  - **Footprint-Namen verifiziert gegen kiswitch v2.4** (jsDelivr-Tree-API):
    `SW_Hotswap_Kailh_Choc_V1V2_1.00u` und `_2.00u` existieren beide im
    Ordner `Switch_Keyboard_Hotswap_Kailh.pretty`. Bei abweichender
    Library-Version Namen erneut prüfen.
- **Warum V1V2 statt V2-spezifisch**: Die Lib hat auch `SW_Hotswap_Kailh_Choc_V2_*`.
  V1V2 bohrt die Alignment-Löcher für V1 UND V2 → die Hot-Swap-Buchse nimmt
  jede Choc-Generation auf. Genau das ist der Sinn eines Hot-Swap-Boards
  (End-User kann V1 oder V2 stecken). V2 wird voll unterstützt.
- **2u Cells**: Choc-Stabilizer (CPG1353G24D01) als separate mechanische
  Footprint-Platzierung im Layout — der Switch selbst ist 1u, der 2u-Keycap
  braucht den Stabilizer.

### 2. Line-Out / Kopfhörer-Buchse (J8) — löst das Tiefen-Problem

**Warum**: Die 40mm-Onboard-Speaker können den 30-60Hz-SubBass des
Sound-Constitution-Konzepts physikalisch nicht abstrahlen. Ohne externen
Ausgang verhungert der tiefe Charakter im Gehäuse.

**Hardware (Sheet 6 Audio erweitert)**:
- J8: 3.5mm TRS-Buchse mit Insertion-Detect-Switch (PJ-320-Klasse, LCSC C2884109)
- Passiver Tap an PCM5102A VOUTL/VOUTR (vor dem PAM8403). PCM5102A-Output ist
  ground-centered (interne Charge-Pump) → keine Koppel-Caps nötig.
- R_LO_L / R_LO_R: 22Ω Serien-Widerstände (Schutz / Kurzschluss-Limit)
- Jack-Detect: J8 DET-Switch → MCP23017 GPA6 (Pull-Up + IRQ). Idle=LOW
  (kein Plug), eingesteckt=HIGH.

**Firmware-Verhalten**:
- Plug eingesteckt → Pico mutet NUR den PAM8403 (Speaker), PCM5102A-DAC +
  Line-Out bleiben live. Neue `AudioPower.speakers(on)`-Methode trennt
  Speaker-Mute von vollem System-Mute.
- Pico sendet `{"event":"jack","inserted":true}` an die Bridge; die Bridge
  echo't `{"set":"amp","enabled":0}` (war im Protokoll schon vorgesehen).

**Empfehlung**: Für Line-Out an Aktivboxen/Interface reicht der direkte
PCM5102A-Tap. Für niederohmige Kopfhörer (<32Ω) wäre ein dedizierter
Kopfhörer-Amp (TPA6132 o.ä.) besser — kann in v0.8 nachgerüstet werden.

---

## v0.6.3-r6 (2026-05-15) — Stabilization-Pass (Senior-Review Findings)

Kein neues Feature. Reine Stabilisierung nach Senior-Engineer-Review.

### HIGH-Fixes

**PVDD-Decoupling (PAM8403H pins 4, 13)**
- Vorher: nur C9 10µF + C9b 100nF an VDD (pin 6). PVDD-Pins 4 (links) und
  13 (rechts) hatten KEIN lokales Decoupling.
- Datasheet-Anforderung (PAM8403H.PDF "Application Information"): "1.0µF
  ceramic close to VDD" (HF) + "20µF or greater" (bulk).
- Fix:
  - Links (VDD pin 6 + PVDD-L pin 4): C9 10µF→**22µF** (C45783), C9b 100nF→**1µF**
  - Rechts (PVDD-R pin 13): NEU **C_PVDDR 22µF + C_PVDDR_HF 1µF** lokal
- Verhindert: Class-D-Switching-Transienten (250kHz) modulieren PVDD →
  Distortion bei hoher Lautstärke + EMI.

**Startup-Sequenz (Pop-Suppression-Reihenfolge)**
- Vorher (SPEC §8): /MUTE=HIGH zuerst, dann /SHDN=HIGH 100ms später. FALSCH —
  beide active-low, un-mute VOR chip-wakeup → Pop wenn /SHDN HIGH wird.
- Fix: /SHDN=HIGH zuerst (chip wacht auf, Referenzen settlen), dann ~50ms
  später /MUTE=HIGH (un-mute). Shutdown umgekehrt.

### Cross-Domain-Korrekturen
- SPEC-Body §8 vollständig auf korrekte Startup-Sequenz umgeschrieben (nicht
  nur Errata).
- Errata-Historie aus SPEC-Body extrahiert → diese CHANGELOG.md (single
  source of truth: Body = aktueller Stand, CHANGELOG = Historie).

---

## Errata-Historie

### v0.6.3-r5 (2026-05-15) — N1: XSMT via MCP23017 GPA5 (statt statischem Pull-Up)

Aus User-Wunsch "lieber direkt haben als nicht haben" — Pop-Suppression
für PCM5102A jetzt explizit per GPIO statt nur durch PAM8403-Pull-Downs.

**Hardware-Änderung in audio.kicad_sch (U3 PCM5102A Pin 17 XSMT):**

| Alt (v0.6.3-r4) | Neu (v0.6.3-r5) |
|---|---|
| R_XSMT 10k pull-up zu +3V3 (statisch un-muted) | R_XSMT_PD 10k pull-down zu GND (default LOW = muted) |
| — | Hier-Input `PCM_XSMT` von MCP23017 |

**Hardware-Änderung in mcp.kicad_sch (U2 MCP23017 GPA5 = Pin 26):**

| Alt | Neu |
|---|---|
| GPA5 als NC_GPA5 Reserve-Label | GPA5 → Hier-Output `PCM_XSMT` zu Audio-Sheet |

**Cross-Sheet im Root**: Label-Bridge zwischen MCP-Sheet-Box rechts und
Audio-Sheet-Box links für PCM_XSMT-Net.

**Firmware-Konsequenz** (Pico-Init Reihenfolge):

1. Boot: Alle MCP23017-Pins default Input → R_XSMT_PD zieht XSMT LOW → PCM5102A stumm
2. I²C-Init: MCP23017 GPA5 als Output, default 0 (LOW = stumm bleibt)
3. Power-Sequence: +5V/+3V3-Rails stabilisieren (~50 ms)
4. PAM8403 enable: Pico setzt GP28 (MUTE) HIGH, 100 ms später GP27 (SHDN) HIGH
5. PCM5102A un-mute: MCP23017 GPA5 auf HIGH (PCM_XSMT = +3V3) → DAC liefert Audio
6. → kein Pop am Speaker, kein DAC-Hochfahr-Knack

**Bei Shutdown** umgekehrt: MCP GPA5 LOW → PCM stumm, dann PAM8403 muten/shutdown.

### v0.6.3-r3 (2026-05-14) — Important-Items aus 2nd-Review adressiert

Doc-side updates (kein KiCad-GUI nötig). Hardware-Schema unverändert
außer N2 (I²S Series-Resistoren am Pi-Header).

#### I1 — Power-Budget realistisch rekalkuliert

Der v0.6-PAM8403-Wert (350 mA peak) war zu optimistisch. Für 2× 4Ω
Speaker bei 3W BTL: ~6W Audio out, ~85% Class-D-Effizienz → ~7W Eingang
→ ~1.4 A nur für Amp.

| Last | Idle | Typical Audio | Worst Case (loud, 4Ω) |
|---|---|---|---|
| Pi Zero 2 W (SuperCollider) | 250 mA | 500 mA | 700 mA |
| Pico 2 (RP2350) | 30 mA | 50 mA | 50 mA |
| OLED SSD1322 256×64 | 50 mA | 150 mA | 250 mA |
| MCP23017 + Pull-Ups | 5 mA | 20 mA | 25 mA |
| PCM5102A DAC | 20 mA | 30 mA | 30 mA |
| **PAM8403H @ 4Ω, 6W Out** | 80 mA | 600 mA | **1400 mA** |
| EC11-Encoder + LED + Modifier | 5 mA | 15 mA | 25 mA |
| **TOTAL** | **440 mA** | **1365 mA** | **2480 mA** |

**Konsequenz:** 2A-Hold-Polyfuse (F1) trippt im Worst-Case.
Optionen:
- (a) **Firmware Volume-Clamp** auf ~50% max → Worst Case ~1.7 A,
  passt unter 2A-Hold knapp. SAFE für USB-C 5V/3A-Source.
- (b) Polyfuse höher dimensionieren auf 3A-Hold (z.B. MF-MSMF300),
  aber dann USB-C-Spec-Verletzung wenn Source nur Default-Current liefert.
- (c) **USB-PD-Sink-Controller** (siehe I2) + 3A-Polyfuse.

**Aktuelle Empfehlung für Prototyp**: (a) Firmware Volume-Clamp,
2A-Polyfuse beibehalten. Final-Produkt-Entscheidung mit (c).

#### I2 — USB-C Power-Negotiation

5.1kΩ CC-Pulldowns signalisieren "Sink", aber NICHT "berechtigt 3A".
USB-C Source-Port kontrolliert via Rp (Pull-Up an CC) ob Default
(~500 mA), 1.5 A oder 3 A erlaubt.

**Decision für v0.6.3**: Variante A (Konservativ) für Prototyp.
- Pi: Firmware-side Volume-Clamp damit Worst-Case Peak < 1.5 A
- BOM-Note: Mit Standard 5V/3A-Netzteil (z.B. Apple 20W) garantiert OK
- Mit altem 5V/0.5A-Hub-Port: Risiko Brown-out bei lauten Passagen

**Future Hardware-Option**: TPS25750 oder CYPD3177 als USB-PD-Sink-Controller.
Kosten: ~$2 IC + ein paar passives. Rechtfertigt sich für Produkt-Stage.

#### I3 — Inrush-Strategie für 1000µF Bulk-Cap

Roher Elko hinter USB-C ohne Strombegrenzung kann USB-C-Source
unhappy machen (Spike beim Hot-Plug). Polyfuse limitiert NICHT
elegant — sie wird träge warm, lässt den Spike aber durch.

**Mitigation v0.6.3**:
- C_BULK MPN-Spec auf **Low-ESR Polymer** geändert: nicht generischer Elko,
  sondern z.B. Nichicon **PCV1A102MCL1GS** (1000µF / 6.3V / Polymer / ESR < 20mΩ).
  Höhere Lebensdauer, definierter ESR begrenzt Charge-Strom selbst etwas.
- ADD-ON für Final-Build (DNP für Prototyp): **NTC-Inrush-Limiter** (CL-130
  Ametherm, 5Ω cold / 0.5Ω hot, in Serie zwischen Polyfuse und +5V-Rail).

#### I4 — 4-Layer Stack-Up überdacht

Alt: Signal / GND / **+3V3** / Signal
Neu: Signal / GND / **+5V** / Signal

**Begründung:** Auf diesem Board ist +5V die Hochstrom-Schiene:
- Pi Zero 2 W (700 mA peak)
- PAM8403H (1.4 A peak)
- OLED VBAT (250 mA peak)
- Summe: bis 2.4 A auf +5V-Plane

+3V3 trägt nur ~80 mA (OLED VDDIO + MCP + Pull-Ups + EC11). Kann lokal
gepoured werden.

**GND-Plane-Regel**: Layer 2 muss **eine ungeteilte Fläche** sein, KEINE
Splits unter USB-C-Bereich, I²S-Strecke (Pi→PCM5102A), oder Audio-Out
(PAM8403→Speakers). Trace-Loops über GND-Splits sind die Hauptquelle
für EMC-Probleme und Audio-Brumm.

#### I5 — Mechanische Koordinaten

Siehe neue Datei `mechanical_coordinates.md` für die vollständige
Tabelle. Hier nur Außen-Maße:

| Element | Dim |
|---|---|
| Gehäuse | 333 × 143.3 × 40 mm |
| PCB-Outline | 320 × 130 mm |
| Edge-Rails | 5 mm rundherum (entfernen nach Bestückung) |
| Component-to-Edge | min 2.5 mm |

#### I6 — BOM-Split JLC-fitted vs Manual

**SECTION A: JLCPCB SMT-bestückt (Full-PCBA)** — ~70 SKUs:

Discrete passives + Power-ICs + Connectors:
- C_BULK (Polymer 1000µF), C1-C9b, C10-C17, C_VREF, C_VCOM, C_VNEG, C_LDOO, C_FLY,
  C_CPVDD_*, C_in_L/R, C6/C6b/C6c — ALLE 0603/0805 SMD-Caps
- R1-R20, R_VOL_L/R, R_RUN, R_XSMT, R_MUTE_PD, R_SHDN_PD, R_BCK, R_LRCK, R_DOUT — ALLE 0603 SMD
- F1 Polyfuse 1812 (MF-MSMF200)
- FB1 Ferrite-Bead 0603 (BLM18AG601)
- D1 USBLC6-2SC6 ESD (SOT-23-6)
- D2 SMAJ5.0A TVS (SMA)
- LED1 0805 warm white

ICs (alle JLC Extended Stock):
- U2 MCP23017-E/SS (SSOP-28, C506653)
- U3 PCM5102APWR (TSSOP-20, C107671)
- U4 PAM8403H (SOIC-16, C17337)

Connectors (SMD):
- J1 USB-C TYPE-C-31-M-12 (C165948)
- J3 OLED-Header 2.54mm 16-pin
- J4 SWD-Header 1.27mm 3-pin
- J5 VSYS-Bridge 2-pin (DNP default)
- J6, J7 Speaker-Headers 2.54mm 2-pin

Switches:
- SW11 Reset 6mm SMD (TL3342)
- SW12 BOOTSEL 6mm SMD (TL3342) — DNP für THT-Pico-Variante

**SECTION B: Manuell zu bestücken (du lieferst)** — ~15 Items:

Mikrocontroller/Compute-Module:
- U1 Pico 2 (RP2350) als 40-pin THT Pin-Header (Empfehlung Prototyp)
- Pi Zero 2 W mit GPIO-Header J2 2×20 durchgesteckt

Switches (Hot-Swap-Sockets nicht im JLC-Stock):
- SW1-SW5 Kailh Choc V2 Hot-Swap-Socket 2u (Cells) — 5×
- SW6-SW10 Kailh Choc V2 Hot-Swap-Socket 1u (Modifier) — 5×
- 5× Kailh 2u Choc V2 Stabilizer (CPG1353G24D01)

Module:
- OLED ER-OLEDM032-1W 3.2" 256×64 SSD1322 mit 16-pin Header
- 4× EC11-Encoder (wenn nicht SMD-Variante gewählt)

Speaker:
- 2× PUI AS04008PS-4W-WR-R Lautsprecher (mit Drähten an J6/J7)

**SECTION C: TBD (noch zu sourcen oder mechanisch)**:
- Custom MX-Stem Silikon-Cell-Caps (5×, DIY/print)
- BOOTSEL- und Reset-Caps (klein)
- Gehäuse-Schraubdome + M3-Schrauben (4×)
- Bass-Reflex-Ports (Bottom-Case 2× Port 8×25 mm)
- USB-C-Source-Netzteil (5V/3A) — User liefert separat

#### N2 — I²S Series-Resistoren am Pi-Header (DONE)

3× 33Ω 0603 Series-Resistoren am Pi-GPIO-Header in Sheet 7 hinzugefügt:
- **R_BCK** an Pi pin 12 (GPIO18 PCM_CLK)
- **R_LRCK** an Pi pin 35 (GPIO19 PCM_FS)
- **R_DOUT** an Pi pin 40 (GPIO21 PCM_DOUT)

Dämpft Overshoot/Reflexionen auf I²S-Strecke (~100mm über Layer 1/4).
LCSC C23138 (RC0603FR-0733RL).

#### N3 — ERC/DRC Report-Workflow

Convention für die nächsten Iterationen:

1. Nach jedem Major-Pinout-Fix oder Architektur-Änderung:
   ```
   cd kicad/
   kicad-cli sch erc --severity-all --output ../reports/ERC_$(date +%Y-%m-%d).txt field_ambience.kicad_sch
   ```
2. Datei format `reports/ERC_YYYY-MM-DD.md`:
   ```
   KiCad Version: X.Y.Z
   Date: YYYY-MM-DD
   Schematic Commit: <git-sha>
   Errors: N
   Warnings: M
   Accepted warnings (with reason): list
   ```
3. Committen ins Repo. Vor jedem Merge in `main`: aktueller ERC-Report
   muss vorhanden sein und 0 unaccepted Errors zeigen.

### v0.6.3-r2 (2026-05-14, gleicher Tag) — Hardware-Pull-Downs + UART-Naming

Nach zweiter Review-Iteration ergänzt:

**B4-Fix: PAM8403 /SHDN und /MUTE Hardware-Defaults**

PAM8403H /SHDN (Pin 12) und /MUTE (Pin 5) sind ACTIVE LOW. Während
Pico-Reset/Boot floaten GPIOs unbestimmt — Amp könnte un-defined oder
voll-on starten → Pop/Klick. Fix:

- **R_MUTE_PD = 10 kΩ 0603** Pull-Down von Pin 5 (MUTE) zu GND
- **R_SHDN_PD = 10 kΩ 0603** Pull-Down von Pin 12 (SHDN) zu GND

Default-State (während Boot, Reset, Pico-down): beide LOW → Amp ist
GESHUTDOWN und GEMUTED. Pico-Firmware zieht beide aktiv HIGH erst
NACH Power-Sequencing (per Spec v0.6 §5 GP27/GP28 Pop-Suppression).

**I7-Fix: UART-Net-Naming disambiguiert**

Alt: UART0_TX / UART0_RX — perspektivisch mehrdeutig (TX/RX hängt
ab welches Bauteil schaut).

Neu (eindeutige Direction):
- `PICO_TX_PI_RX` — Pico GP0 (UART0 TX) → Pi GPIO15 (RX) auf pin 10
- `PI_TX_PICO_RX` — Pi GPIO14 (TX) auf pin 8 → R1 1k → Pico GP1 (UART0 RX)

Geändert in: pico_sheet, pi_sheet, root_sheet (sheet-pins + cross-sheet labels).

### v0.6.3 (2026-05-14) — CRITICAL: USB-C VBUS/GND-Pinout korrigiert

Externer Review fand: USB-C-Symbol hatte VBUS auf Pin-Numbers A1/A4/B1/B4
und GND auf A12/A9/B9/B12. Das ist **falsch gegen USB Type-C Spec
Rev 2.1 Table 3-1**. Die korrekte Pin-Belegung ist:

```
GND  = A1, A12, B1, B12  (NICHT was im v0.6 Symbol stand)
VBUS = A4, A9, B4, B9
CC1  = A5
CC2  = B5
D+   = A6 (+ B6 für reversed orientation)
D-   = A7 (+ B7 für reversed orientation)
SBU1 = A8
SBU2 = B8
```

**Kritische Konsequenz** ohne Fix: Beim PCB-Layout hätte das Pad A1
des USB-C-Footprints +5V bekommen — Standard-Pad A1 ist aber GND.
Beim Einstecken eines USB-C-Kabels wäre **VBUS direkt auf GND
kurzgeschlossen** worden (Pad A1 vom Kabel = GND, hier am PCB = +5V).

Fix in `generate_kicad_project.py`: Symbol pin numbers für die 8
Power-Pins korrigiert (Positionen bleiben, nur Numbers geswappt):
- local +10.16: number A1 → **A4** (name VBUS)
- local +7.62:  number A4 → **B4** (name VBUS)
- local +5.08:  number B4 → **A9** (name VBUS)
- local +2.54:  number B1 → **B9** (name VBUS)
- local -2.54:  number A12 → **A1** (name GND)
- local -5.08:  number A9 → **B1** (name GND)
- local -7.62:  number B9 → **A12** (name GND)
- local -10.16: number B12 → **B12** (name GND, unchanged)

Power-Tree-Sheet-Wiring bleibt unverändert da Y-Positionen identisch.
Analyzer `pin_nets`-Dump verifiziert nun alle 17 Pins (16 + Shield S1)
korrekt gegen USB Type-C Spec.

**Verbleibender PCB-Layout-Check**: Footprint des USB-C-Receptacle
(JLCPCB C165948 = TYPE-C-31-M-12) muss in KiCad gegen das Symbol
gegengecheckt werden — Pad-Number-Zuordnung muss zur Pin-Number im
Symbol passen. Standard-KiCad-Footprints `USB_C_Receptacle_HRO_TYPE-
C-31-M-12` haben die korrekte Spec-Belegung; trotzdem zwingend
verifizieren bevor Bestellung.

### v0.6.2 (2026-05-13) — PAM8403H-Pinout per PDF im Repo verifiziert

Der PAM8403H.PDF im Repo-Root ist das offizielle Diodes-Inc-Datasheet
für LCSC C17337. Damit ist die Pin-Belegung jetzt zu 100% gesichert.

Bug in v0.6.1 (zwischenzeitlicher Fix-Versuch ohne PDF): basierte auf
falscher DS31295-Vermutung. Pin 8 (VREF), 9 (NC), 10 (INR), 11 (GND),
12 (SHDN), 13 (PVDD), 14 (+OUT_R), 15 (PGND), 16 (-OUT_R) waren alle
um eine Position verschoben. **+OUT_L/−OUT_L Polarität war auch
vertauscht** (Pin 1 ist −OUT_L, nicht +OUT_L wie in v0.6.1 angenommen).

Verifizierte PAM8403H-Pin-Belegung (Diodes Inc, Nov 2012):

| Pin | Name | Funktion |
|---|---|---|
| 1 | **-OUT_L** | Left negative output (BTL) |
| 2 | PGND | Power Ground |
| 3 | **+OUT_L** | Left positive output (BTL) |
| 4 | PVDD | Power VDD |
| 5 | MUTE | Mute Control (ACTIVE LOW) |
| 6 | VDD | Analog VDD |
| 7 | INL | Left Channel Input |
| 8 | **VREF** | Internal analog ref — Bypass-Cap zu GND **REQUIRED** |
| 9 | NC | No connected |
| 10 | INR | Right Channel Input |
| 11 | GND | Analog GND |
| 12 | **SHDN** | Shutdown Control (ACTIVE LOW) |
| 13 | PVDD | Power VDD |
| 14 | **+OUT_R** | Right positive output (BTL) |
| 15 | PGND | Power Ground |
| 16 | **-OUT_R** | Right negative output (BTL) |

NEUER Component in v0.6.2: **C_VREF 1µF X7R 0603** an PAM8403 Pin 8
(VREF Bypass-Cap, per Datasheet zwingend). War in v0.6.1 vergessen.

### v0.6.1 (2026-05-13) — Erster Pinout-Fix-Versuch (PCM5102A korrekt, PAM8403 noch falsch)

| Errata | Ursprünglich (v0.6) | Korrigiert (v0.6.1) |
|---|---|---|
| PCM5102A pinout | LRCK/BCK/DIN auf Pin 1/2/3 (alle falsch) | Per TI Datasheet SLAS859C: CPVDD=1, OUTL=6, OUTR=7, AVDD=8, BCK=13, DIN=14, LRCK=15, DVDD=20 |
| BOM LCSC-Nummern | PCM5102A=C9900003814 (existiert nicht), PAM8403=C84368 (existiert nicht) | PCM5102A=**C107671** (verified, Stock 6726), PAM8403=**C17337** (verified, Stock 8962) |

---


