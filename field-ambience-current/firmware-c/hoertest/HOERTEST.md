# Hörtest — minimaler Aufbau

So hörst du die native Firmware (Steps 1–12a) mit möglichst wenig Aufwand,
**ohne** die fertige Platine. Du brauchst nur einen RP2350-Pico und ein
fertiges I²S-DAC-Modul. Lautsprecher/Verstärker sind **nicht** nötig — der
PCM5102A hat einen eigenen Line-/Kopfhörer-Ausgang.

> Wichtig vorab: Die Firmware ist bisher nur **host-getestet** (DSP-Mathematik,
> Hüllkurven, Filter, Harmonie — 6 Test-Suites grün). Dieser Hörtest ist der
> erste echte On-Device-Lauf. Wenn etwas klemmt, liegt der Verdacht zuerst beim
> I²S-/DMA-Timing, nicht bei der Engine.

---

## TL;DR — zwei Stufen

| Stufe | Hardware | Was du hörst |
|---|---|---|
| **1 — Minimal** | Pico 2 + PCM5102A-Modul + 5 Drähte | Das Idle-Ambience-Bett (Texture + Reverb-Fahne). Bestätigt: Audio-Pfad, I²S, DMA, Engine laufen. **Keine** Tasten nötig. |
| **2 — Spielbar** | + MCP23017-Modul + ein Draht gegen GND | Cell-Taps → Pad auf echter Akkord-Wurzel + Zwei-Schicht-Bass + Reverb. Das eigentliche Instrument. |

Fang mit Stufe 1 an. Wenn da Ton kommt, ist das Schwierigste geschafft.

---

## Einkaufsliste

- **Raspberry Pi Pico 2** (RP2350) — zwingend RP2350, nicht der alte RP2040-Pico. ~5 €
- **PCM5102A-DAC-Modul** (oft „GY-PCM5102" / lila Board) — ~3–8 €
- Steckbrett + Jumper-Kabel
- Kopfhörer **oder** Aktivboxen mit 3.5-mm-Klinke
- USB-Kabel (Daten, nicht nur Laden)
- *(Stufe 2)* **MCP23017-Modul** (~2 €) + optional 5 Taster (oder einfach ein Draht zum Antippen gegen GND)

---

## Stufe 1 — Verdrahtung Pico ↔ PCM5102A

Fünf Drähte für Audio, plus drei Konfig-Pins am DAC.

| Pico-Pin | → | PCM5102A-Modul | Zweck |
|---|---|---|---|
| GP0 (Pin 1) | → | **BCK** | I²S Bit-Clock |
| GP1 (Pin 2) | → | **LCK** (LRCK) | I²S Word-Select |
| GP4 (Pin 6) | → | **DIN** | I²S Daten |
| 3V3 OUT (Pin 36) | → | **VIN / 3.3V** | DAC-Versorgung |
| GND (z. B. Pin 38) | → | **GND** | Masse |

**Konfig-Pins am DAC-Modul** (entscheidend — sonst bleibt es stumm):

| PCM5102A-Pin | auf | Warum |
|---|---|---|
| **SCK** | **GND** | 3-Wire-Mode: der DAC erzeugt seine Clock selbst aus BCK. (Auf vielen Lila-Boards ist das ein Lötjumper auf der Rückseite — oft schon gesetzt.) |
| **XMT / XSMT** | **3V3** | Soft-Mute aus = DAC entstummt. Auf der fertigen Platine macht das der MCP23017; in Stufe 1 legst du es fest auf High. |
| **FMT** | **GND** | I²S-Format (Standard). |

Audio abgreifen: **am 3.5-mm-Ausgang des PCM5102A-Moduls** (Kopfhörer/Aktivboxen).
Der PAM8403-Verstärker und die Onboard-Speaker werden für den Hörtest komplett
umgangen — der Pico steuert zwar GP27/GP28 (Amp-Enable) an, aber ohne
angeschlossenen Amp ist das wirkungslos.

> Wenn dein DAC-Board einen separaten **FLT**- und **DEMP**-Pin hat: beide auf
> GND (oder offen lassen) ist in Ordnung.

---

## Toolchain — geringster Aufwand

**Option A (empfohlen, am wenigsten Handarbeit): VS-Code-Erweiterung**
„Raspberry Pi Pico" installieren. Sie zieht Pico-SDK + ARM-Toolchain + CMake
automatisch. Dann „Import Project" auf den Ordner `firmware-c/`, Board auf
**pico2** stellen, „Compile". Fertige `.uf2` liegt unter `build/`.

**Option B (CLI):** Pico SDK 2.x + `arm-none-eabi-gcc` + CMake selbst
installieren, `PICO_SDK_PATH` setzen, dann:

```bash
cd field-ambience-current/firmware-c
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
# erzeugt: field_ambience_native.uf2
```

---

## Flashen

1. Pico vom USB trennen.
2. **BOOTSEL-Taste** gedrückt halten und einstecken — er meldet sich als
   USB-Laufwerk `RPI-RP2`.
3. `field_ambience_native.uf2` auf das Laufwerk ziehen. Der Pico bootet neu und
   startet sofort (< 1 s).

---

## Was du hören solltest

**Stufe 1 (nur Pico + DAC):**
- Nach dem Boot blendet über ~2 s ein **leises, dunkles Ambience-Bett** ein
  (gefiltertes Rauschen + langsam atmender Bandpass, durch den Reverb breit
  gezogen). Konstant, nicht-melodisch. Das ist `famTexture` (Idle-Pegel 0.20)
  durch `famReverbMaster`.
- **Kein Knacks** beim Einschalten (Pop-Suppression-Sequenz).
- Hörst du das sauber in Stereo → I²S, DMA, Mix-Bus und Reverb funktionieren.

**Stufe 2 (mit Cell-Trigger):**
- Ein Cell-Tap lässt eine **warme, verstimmte Saw-Pad-Stimme** auf der
  Akkord-Wurzel der Skalenstufe aufblühen (langsamer Bloom ~1.6 s), mit
  langer Reverb-Fahne und einem **Zwei-Schicht-Bass** darunter.
- Defaults: **C-Dur ionisch, „warm"/add9**. Cell 1→Stufe I, … Cell 5→Stufe V.
- Beim Loslassen klingt es lang aus (3 s Pad-Release + Reverb-Tail).
- Mehrere Cells gleichzeitig: Bass folgt der tiefsten gehaltenen Note (Legato-
  Glide), Pad-Stimmen stapeln sich (8 Stimmen, dann Voice-Stealing).

---

## Stufe 2 — Cell-Trigger (MCP23017)

Verdrahtung Pico ↔ MCP23017-Modul:

| Pico-Pin | → | MCP23017 | Zweck |
|---|---|---|---|
| GP2 (Pin 4) | → | **SDA** | I²C1 Daten |
| GP3 (Pin 5) | → | **SCL** | I²C1 Clock |
| GP22 (Pin 29) | → | **INTA** | Interrupt bei Tastendruck |
| 3V3 | → | **VCC** | Versorgung |
| GND | → | **GND, A0, A1, A2** | Masse + Adresse 0x20 |
| 3V3 | → | **RESET** | (sonst bleibt der Chip im Reset) |

XSMT kann in Stufe 2 weiterhin fest auf 3V3 bleiben (einfacher) — oder du
verdrahtest **GPA5 → PCM5102A XMT**, dann übernimmt die Firmware das Muting.

**Cells auslösen:** GPA0…GPA4 sind die Cells 1…5, aktiv-low mit internem
Pull-Up. Ein Tastendruck = Pin kurz **gegen GND**. Minimal ohne Taster: ein
Jumper an GPA0, dessen freies Ende du kurz an GND antippst → Cell 1 spielt.
Mit 5 Tastern (je GPA-Pin ↔ GND) hast du das volle Instrument.

> OLED und Encoder sind für den Hörtest **nicht** nötig. Ohne OLED zeigt die
> Firmware nichts an, spielt aber normal; ohne MCP23017 startet sie mit einer
> Warnung auf USB-Serial und läuft trotzdem (= Stufe 1).

---

## Wenn es still bleibt — Checkliste

1. **XMT/XSMT wirklich auf 3V3?** Häufigste Ursache. Ohne High bleibt der DAC
   stumm.
2. **SCK auf GND?** Ohne 3-Wire-Mode kommt kein Ton.
3. **GP0/GP1/GP4 richtig?** BCK=GP0, LCK=GP1, DIN=GP4 — leicht vertauscht.
4. **RP2350?** Mit `-DPICO_BOARD=pico2` gebaut? Ein RP2040-Pico läuft nicht.
5. **USB-Serial mitlesen** (115200 Baud, oder einfach das CDC-Gerät öffnen):
   beim Boot kommt `audio: I2S pump live, engine ready …`. Bei Tastendruck
   `cell N ON (degree …, midi …)`. Kommt der Boot-Print, aber kein Ton →
   Verdacht DAC-Verdrahtung/XSMT. Kommt kein Print → Flash/Board-Problem.
6. **DAC-Masse = Pico-Masse?** Gemeinsame GND ist Pflicht.

---

## Optional: Null-Verdrahtungs-Demo

Wenn du Stufe 2 ganz ohne MCP23017/Taster hören willst, kann ich einen kleinen
**Demo-Auto-Trigger** einbauen (~5 Min Arbeit): die Firmware spielt dann beim
Boot selbsttätig eine langsame Akkordfolge über die Cells, sodass du Pad + Bass
+ Brain + Reverb mit **nur Pico + DAC** hörst. Sag Bescheid, dann mache ich das
als kleinen, leicht wieder entfernbaren Schalter (`#define DEMO_AUTOPLAY`).

---

## Was dieser Test (noch) nicht abdeckt

- **Step 12b** ist nicht gebaut: kein Menü, kein USB-MIDI, keine Encoder→Param-
  Bindings. Key/Mode/Vibe stehen fest auf den Defaults (C-Dur ionisch, warm).
- Die finale Akustik (Sealed + Top-Firing, SPEC §8 r14) betrifft die Platine,
  nicht diesen DAC-Line-Out-Test — über Kopfhörer hörst du den **vollen**
  Frequenzumfang inkl. des Bass-Layers, der onboard über die 380-Hz-Speaker
  bewusst wegfällt.

---

# Anhang A — konkret für die bestellten Teile

Bezug: Pico 2 (Waveshare, pre-soldered), GY-PCM5102 DAC (3er), MCP23017
CJMCU-2317 (3er), 120er-Jumper-Set (M-F / M-M / F-F). Mit einem zusätzlichen
**830er Full-Size-Breadboard** (~3–5 €, „830 Tie-Points" / „MB-102") und
**5× Tactile-Push-Buttons** (6×6 mm oder 12×12 mm DIP, ~2 € im Pack) wird
Tier 2 deutlich sauberer als Pin-zu-Pin-Jumper.

## Was gelötet werden muss

Nur **Stiftleisten anbringen** — keine Kabel löten.

| Board | Löten | Was |
|---|---|---|
| Pico 2 (Waveshare) | ❌ | Header bereits verlötet |
| GY-PCM5102 (DAC) | ✅ | mitgelieferte Stiftleisten in die Pin-Löcher am Rand. 3.5-mm-Klinke ist schon dran. |
| MCP23017 (nur Tier 2) | ✅ | mitgelieferte Stiftleisten in die zwei Pin-Reihen |

Technik: Leiste durchstecken (Pins zeigen weg von der Bauteilseite), einen
Eck-Pin zuerst löten, Ausrichtung prüfen, Rest löten. Pro Pin Kolben an Pad +
Pin (~1 s), Zinn zuführen bis es fließt, Zinn weg, Kolben weg → kleiner
glänzender Kegel. Werkzeug: Lötkolben ~320–350 °C, Lötzinn 0.8 mm mit
Flussmittelseele, etwas zum Fixieren.

## Pico-2-Pinbelegung (physische Pinnummern)

USB oben → Pin 1 oben links, linke Seite abwärts.

| Signal | GPIO | phys. Pin |
|---|---|---|
| I²S BCK | GP0 | **1** |
| I²S LRCK | GP1 | **2** |
| GND | – | **3** |
| I²C SDA *(Tier 2)* | GP2 | **4** |
| I²C SCL *(Tier 2)* | GP3 | **5** |
| I²S DIN | GP4 | **6** |
| MCP INT *(Tier 2)* | GP22 | **29** |
| 3V3 OUT | – | **36** |
| GND | – | **38** |

## Tier 1 — Pico → GY-PCM5102

| Pico phys. Pin | → | DAC-Label |
|---|---|---|
| 1 (GP0) | → | **BCK** |
| 2 (GP1) | → | **LCK** (ggf. „LRCK") |
| 6 (GP4) | → | **DIN** |
| 36 (3V3) | → | **VIN** (ggf. „3.3V"/„VCC") |
| 38 (GND) | → | **GND** |

Konfig-Brücken **auf dem DAC selbst** (F-F-Jumper zwischen zwei Pins desselben
Boards), Labels stehen aufgedruckt:

| DAC-Label | auf | Hinweis |
|---|---|---|
| **SCK** | **GND** | 3-Wire-Mode. Kein SCK-Pin rausgeführt? → ignorieren (intern gebrückt). |
| **XMT** (XSMT/Mute) | **3V3** | **Pflicht** — ohne High bleibt der DAC stumm. |
| **FMT** | **GND** | I²S-Format. |

FLT/DEMP: offen lassen oder GND. Sind die vier nur als Lötbrücken auf der
Rückseite (L/H): **XSMT auf „H"** sicherstellen.

Hören: Kopfhörer/Aktivboxen in die 3.5-mm-Klinke des DAC.

## Tier 2 — zusätzlich Pico → MCP23017

| Pico phys. Pin | → | MCP-Label |
|---|---|---|
| 4 (GP2) | → | **SDA** |
| 5 (GP3) | → | **SCL** |
| 29 (GP22) | → | **INTA** (ggf. „INT") |
| 36 (3V3) | → | **VCC** |
| 38 (GND) | → | **GND** |

Brücken **auf dem MCP-Modul selbst**:

| MCP-Label | auf | warum |
|---|---|---|
| **RST** | **VCC/3V3** | sonst bleibt der Chip im Reset |
| **A0, A1, A2** | **GND** | → I²C-Adresse 0x20 (von der Firmware erwartet) |

Cell auslösen: **GPA0…GPA4** = Cell 1…5 (aktiv-low, interner Pull-Up).
Quick-and-dirty: Draht an GPA0, freies Ende kurz an **GND** antippen → Cell 1
spielt. Sauber mit Tasten siehe nächster Abschnitt.

## Tactile-Buttons als Cells

Tactile-Push-Buttons (4-Bein-DIP, 6×6 mm oder 12×12 mm) sind elektrisch
korrekt — die finalen Kailh-Choc-Cells sind ebenfalls nur SPST-Momentary-
Schalter. Pro Cell:

```
   GPA-Pin  ──┐                ┌── GND
              │                │
              └─── [Button] ───┘
```

**4-Bein-Pinout-Falle**: Die zwei Beine **derselben Seite** sind intern
dauerhaft kurzgeschlossen; gedrückt wird Seite A mit Seite B verbunden. Nimm
also **ein Bein von einer Seite und eines von der anderen** (über die *lange*
Achse hinweg, nicht die kurze). Auf dem Breadboard wird der Button so
eingesteckt, dass die **lange Achse die Mittelfuge überquert** — dann sitzen
die zwei elektrischen Seiten automatisch in getrennten Reihen, richtig herum.

5 Buttons in einer Reihe nebeneinander: je eine Seite an die GND-Schiene, die
andere Seite per Jumper an GPA0/1/2/3/4 → fertige Tier-2-Tastatur.

Modifier-Buttons (SHIFT/HOLD/DRONE/GENERATE/CLEAR auf GPB0–4) sind in der
aktuellen Firmware **noch ohne Funktion** (kommt erst mit Step 12b) — also
jetzt keine kaufen.

## Breadboard-Layout (830 Tie-Points)

Mit dem 830er-Breadboard wird die Stromverteilung trivial — die zwei roten
und zwei blauen Power-Schienen am Rand übernehmen 3V3 und GND, und alle drei
Module passen mit Platz für die 5 Buttons nebeneinander.

```
+rail (rot)   ←── Pico Pin 36 (3V3)
−rail (blau)  ←── Pico Pin 38 (GND)

   [Pico 2, USB links, über die Mittelfuge]    Spalten ~1–20
   [GY-PCM5102, über die Mittelfuge]           Spalten ~22–32
   [MCP23017,    über die Mittelfuge]          Spalten ~35–48
   [5 Tactile-Buttons in Reihe, über die Fuge] Spalten ~50–60
       eine Seite → blaue (GND) Schiene
       andere     → GPA0..GPA4 am MCP
```

Module ziehen ihre Versorgung dann direkt aus den Schienen:
- DAC VIN → +rail, DAC GND → −rail, DAC XMT → +rail (Brücke am DAC)
- MCP VCC → +rail, MCP GND → −rail, MCP RST → +rail (Brücke am MCP),
  MCP A0/A1/A2 → −rail

Kein Ketten-Brücken-Salat, alle Signal-Jumper sind kurz und gerichtet.

Falls du nur ein **400er Half-Size-Breadboard** hast: passt auch noch, aber
sehr eng. Dann eher zwei nebeneinander.

