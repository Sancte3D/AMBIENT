# FIELD AMBIENCE — Pitch (Draft v0.1)

> *Working document for crowdfund / donation copy. Tighten the language before
> publishing. Every technical number in here is verified against the schematic,
> the SPEC, and the live JLCPCB parts library as of 2026-05-30.*

---

## Hook

**Tap eine Zelle. Lass eine Welt entstehen.**
Ein langsames, tiefes Ambient-Instrument — Hardware mit massiver Optik, oder
kostenfrei im Browser — das **per Konstruktion keinen häßlichen Klang machen kann**.

*One-line EN: A slow, deep ambient instrument — as dedicated hardware or as a
free browser instrument — engineered so it cannot make an ugly sound.*

---

## Zwei Wege, es zu erleben

### 1) Web-Edition — kostenfrei, sofort, in jedem Browser

Eine einzelne HTML-Datei (`field_ambience_webapp.html`, ~98 KB). Öffnen,
spielen — kein Install, kein Server, keine Abhängigkeit. Die komplette
Klang-Engine ist via **Web Audio API** im Browser implementiert: Pad-Voices,
Sub-Bass + Deep-Bass, Texture-Bed aus Rauschen, **Convolution-Reverb** mit
pre-Reverb-tanh-Drive, Tonic-Drone, opt-in Generative-Bed. Voll responsive
Touch-/Maus-UI.

Diese Web-Edition ist die **Demo + Funnel**: jeder kann sofort hören, was die
Sound Constitution bedeutet — bevor er die Hardware backt.

### 2) Hardware-Edition — das Crowdfund-Ziel

Ein dediziertes Performance-Instrument mit eigenem Display, eigenen Tasten,
eigenen Drehgebern, eigenen Lautsprechern. Kein Laptop. Kein App-Wechsel.
Ein Knopf zum Einschalten (USB-C), dann **spielen**.

| Block | Spec |
|---|---|
| **Audio-Host** | Raspberry Pi Zero 2 W (onboard WLAN für Updates) |
| **I/O-Controller** | RP2350 (Raspberry Pi Pico 2) — Dual-Core Cortex-M33 @ 150 MHz |
| **DAC** | Texas Instruments PCM5102A (I²S, ground-centered, ~2,1 Vrms full-scale) |
| **Amplifier** | Diodes PAM8403DR-H — Stereo Class-D, 2× 3 W @ 4 Ω, pop-suppressed |
| **Speakers** | 2× PUI AS04008PS — 40 mm down-firing, mit Bass-Reflex-Ports |
| **Line-Out** | 3,5 mm TRS mit Insertion-Detect (Speaker mute on plug-in) |
| **Display** | 256×64 weißes OLED (SSD1322), Zwei-Seiten-Performance-Menü |
| **Cells** | 5× Kailh Choc V2 Hot-Swap, 2u, mit Stabilizer — austauschbare Tasten |
| **Modifier** | 5× Choc V2 1u: SHIFT · HOLD · DRONE · GENERATE · CLEAR |
| **Encoder** | 4× EC11 mit Push: DRIVE · BRIGHTNESS · DISPLAY · VOLUME |
| **PCB** | 4-Layer (Signal/GND/+5V/Signal), 320 × 130 mm, ENIG, JLCPCB-bestellbar |
| **Power** | USB-C 5 V / 3 A, ESD- + TVS-geschützt |
| **MIDI** | USB-MIDI-Out (über Pi), für DAW-Integration |

---

## Was es besonders macht — die Sound Constitution

Das ist die Differenzierung. Nicht „mehr Sounds", nicht „AI-generated". Sondern
ein **Regelwerk im Code**, das physikalisch verhindert, dass das Gerät häßlich klingt.

> **NEVER**: Glocken, schnelle Zufallsmelodien, Dissonanz, harte FM, aggressive
> Resonanz, schnelle Arpeggios, Glitter, melodisches Chaos, destabilisierende
> Modi, harsche Vibes, Tempi > 90 BPM, Brightness > +2700 Hz.
>
> **ALWAYS**: tief, dunkel, langsam, harmonisch stabil, Tiefen-Mitten-Wärme,
> großer Hall, Skalen-gebundene Tonhöhen, graduelle Modulation. Defaults:
> 54 BPM, Drive 0.18, Brightness -0.2.
>
> **Constitutional safety in code**: jeder Macro-Encoder hat einen Safe-Range,
> jeder Parameter klemmt auf musikalische Werte. Der User darf — egal wie er
> dreht — keinen häßlichen Klang erzeugen.
>
> — `field_ambience_skill.md`

Konkret in der Engine:
- **Instrument, kein Generator.** Beim Einschalten: Stille. Erst wenn du eine
  Zelle drückst, entsteht Klang. Generative-Bed und Drone sind opt-in Toggles.
- **Scale-locked & voice-led.** Jede Note bleibt in der gewählten Tonart (7
  Kirchentonarten, ohne Locrian — die wurde als zu instabil entfernt).
  Akkordwechsel finden automatisch die nächstgelegene Stimmlage.
- **Markov-Progressionen mit Sicherheits-Klemmen.** Die Generative-Logik darf
  bestimmte Übergänge nicht erzeugen (z. B. den verminderten 7. Grad — er
  ist überall mit Gewicht 0 hinterlegt und zusätzlich auf 1–6 hart begrenzt).
- **Performance-Macros mit musikalischen Grenzen.** SPACE (Raumgröße), DEPTH
  (Tiefen-Gewicht), TEXTURE (Atmosphären-Bed), BLOOM (Pad-Swell) sind
  4-Macros, die jeweils mehrere Engine-Parameter koordiniert verschieben —
  alle Schreibwege durch *eine* Reconciliation-Funktion, damit kein Macro
  ein anderes still überschreibt.

**Referenz-Klang:** State Azure Live-Performances — slow two-chord beds,
modular-driven, harmonische Stabilität.

---

## Für wen ist das?

- **Ambient-/Soundscape-Musiker**, die ein dediziertes Performance-Werkzeug
  wollen, kein App-Plugin. Wegspielen vom Bildschirm.
- **Meditations- und Sound-Healing-Praktiker** — die „never ugly"-Garantie ist
  hier Kernfeature, nicht Kosmetik. Augen zu, Hand auf das Gerät, kein
  Fehlgriff möglich.
- **Producer**, die ein Hardware-Hilfsmittel suchen, das **Pad-Beds und Drones
  in Studio-Qualität** liefert, MIDI-Out für DAW-Integration.
- **Synth-Sammler**, die ein **Open-Hardware-Stück** wollen, das nicht in 3
  Jahren von einer App-Update-Politik abgewürgt wird.

**Nicht für**: Beat-Maker, Speed-Arp-Fans, „möglichst viele Presets"-Sucher.
Wir sind explizit anti-Kompromiss in diese Richtung.

---

## Tech-Details — die echten Zahlen

**Komponenten** (alles per JLCPCB-Live-DB 2026-05-30 verifiziert):
- **97 platzierte Bauteile** im Schaltplan
- **25 distinct LCSC-Teile** — *alle lagernd*, *0 Wert-/Package-Fehler*,
  14 JLC Basic / 11 Extended (ICs + Spezialteile sind unvermeidbar Extended)
- **~$3,70 SMT-Bauteilkosten pro Board** (ohne die Module unten)
- **Eigenbeschaffung** (JLC kann das prinzipbedingt nicht bestücken): Pico 2,
  Pi Zero 2 W, OLED-Modul, 2× Lautsprecher, 10× Choc-Hotswap-Sockets,
  5× Stabilizer, 4× EC11-Encoder

**Was öffentlich (FOSS / Open Hardware) ist:**
- **KiCad 9 Schaltplan**, vollständig + ERC-konform, plus Python-Generator
  (`generate_kicad_project.py`), der das Schematic reproduzierbar erzeugt
- **MicroPython-Firmware** für den Pico 2 (OLED-Treiber, Encoder, MCP23017,
  Amp-Power-Sequencing)
- **SuperCollider-Engine** (`field_ambience_v29o.scd`) für die Hardware-Edition
- **Web-Audio-Engine** (`field_ambience_webapp.html`) für die Browser-Edition
  — Single-File, kein Build-Step
- **Python-Bridge** (WebSocket ↔ OSC ↔ UART) für die Hardware-Variante
- **SPEC**, **CHANGELOG**, **PCB-TODO**, **mechanische Koordinaten** —
  vollständige Build-Doku, in der Repo

**Software-Architektur:**
```
HARDWARE-EDITION                       WEB-EDITION
─────────────────────────────          ─────────────
Choc-Switches / Encoder                Browser (Tap / Maus)
        │                                     │
   Pico 2 (MicroPython)                       │
   OLED + Buttons + Amp-Sequenz               │
        │ UART/JSON                           │
   Pi Zero 2 W                                │
   ├─ SuperCollider-Engine ──┐                │
   └─ Python-Bridge ─OSC─────┤        Web-Audio-Engine
                             │        (im Browser)
                       PCM5102A DAC
                             │
                       PAM8403 Amp
                             │
                       2× 40 mm Speaker / Line-Out
```

Beide Audio-Engines implementieren **dieselbe Sound Constitution** und reden
dieselbe OSC-Sprache (`/fam/cell`, `/fam/hold`, `/fam/space`, …). Beide
enthalten die v0.8-Reverb-Reconciliation: SPACE/Mode/Vibe/Mood schreiben
nicht mehr gegeneinander auf den Reverb-Bus.

---

## Status — was fertig ist, was Crowdfund-Geld baut

### ✅ Heute fertig

- Schaltplan komplett, alle Pin-Outs gegen Datenblatt verifiziert
- BOM zu 100 % JLCPCB-orderbar (diese Session: 7 Fehler in den LCSC-Nummern
  korrigiert, darunter 3 mit *falschem Wert* — ohne den Audit hätte JLC
  ein kaputtes Board bestückt)
- Pico-Firmware in MicroPython: OLED, 10 Buttons, 4 Encoder, Amp-Sequencing
- SuperCollider-Engine: voll spielbar, Sound Constitution durchgesetzt,
  Reverb-Architektur konsolidiert
- Web-Audio-Engine: Standalone-HTML, läuft in jedem modernen Browser
- Zwei-Seiten-Performance-Menü (v30): PLAY-Page mit Macros, SETUP-Page mit Config

### ⏳ Was Crowdfund-/Donation-Mittel finanzieren

1. **PCB-Layout & Fertigung.** Es gibt noch kein `.kicad_pcb`. Layout (Routing
   320×130 mm 4-Layer mit GND-Plane unter den Audio-Pfaden), DRC, JLCPCB-
   Prototyp-Charge, Stencil.
2. **Gehäuse & Front-Plate.** Industrial-Design-CAD (FreeCAD/Fusion), Top-Plate
   mit Cutouts für USB-C/OLED/Cells/Modifier/Encoder, Bottom-Case mit
   Speaker-Grille-Pattern, Bass-Reflex-Ports.
3. **Erste Produktions-Charge.** JLCPCB-Bestückung Section A (~70 SMT-Bauteile)
   + Eigenbeschaffung Section B (Module + Choc-Sockets + Lautsprecher + Caps)
   + Assembly.
4. **Verteilung & Versand** an Backer / erste Käufer.

### 🚀 Stretch Goals

- **„Pi-frei"-Hardware-Revision** mit dedizierter ARM-Audio-Engine in C/Rust,
  die direkt auf dem RP2350 oder einem Teensy-4-Klasse-SoC läuft. Würde die
  Stückliste verkleinern und den Boot komplett deterministisch machen.
  Eigenes PCB-Rev, eigene Engineering-Phase — nur sinnvoll wenn der erste
  Prototyp ausgeliefert ist.
- **Cell-Cap-System** in Silikon mit eigenem MX-Stem (für noch besseres
  Tap-Feeling).
- **Web-Editor** für eigene Vibes (Chord-Familien, Reverb-Presets) — direkt
  in der Web-Edition, exportierbar als JSON-Profile, die auf die Hardware
  geladen werden können.

---

## Was wir aus Prinzip *nicht* tun

- **Kein „AI-powered"**. Das Gerät ist explizit anti-zufälliges-AI-Chaos.
  Markov-Übergänge sind hart begrenzt, jede Note ist scale-locked.
- **Keine harten Superlative** („world's first", „revolutionary", „endless
  possibilities"). Die Sound Constitution ist besonders genug. Das reicht.
- **Keine Lock-in-Cloud-Konten**. Das Gerät funktioniert offline, lebenslang.
  Updates über lokale WLAN-Verbindung zum Pi, ohne Pflicht-Account.
- **Keine geschlossenen Treiber**. KiCad-Files, Firmware, beide Audio-Engines
  sind im Repo.

---

## Aufruf zum Mitmachen

- **Sofort spielen:** öffne `field_ambience_webapp.html` im Browser. Kein
  Install, kein Login. Spiel 10 Minuten, dann entscheide.
  *(Hosting auf GitHub Pages folgt — bis dahin: HTML herunterladen + öffnen.)*
- **Hardware vorbestellen:** Crowdfund-Tier ab [TBD] €. Erste Charge:
  [TBD] Stück, geplante Auslieferung [TBD].
- **Open-Hardware unterstützen:** Donation-Tier ab [TBD] €. Geld fließt 1:1
  in PCB-Layout, Gehäuse-Engineering, JLCPCB-Charge.
- **Mitbauen:** das KiCad-Projekt ist offen. Pull Requests willkommen.
  Issue-Tracker für Bug-Reports / Feature-Wünsche.

---

## Anhang: was im Repo liegt

| Ordner / Datei | Inhalt |
|---|---|
| `field-ambience-current/field_ambience_webapp.html` | **Standalone Web-Edition** (Web Audio, ein File) |
| `field-ambience-current/field_ambience_panel.html` | älteres Remote-UI (braucht Bridge + SC; v.a. historisch / Hardware-UI) |
| `field-ambience-current/field_ambience_v29o.scd` | SuperCollider-Engine (Hardware-Audio) |
| `field-ambience-current/field_ambience_bridge.py` | Python WebSocket↔OSC↔UART-Bridge |
| `field-ambience-current/firmware/*.py` | MicroPython auf Pico 2 |
| `field-ambience-current/kicad/` | KiCad-9-Projekt + Schaltplan-Generator |
| `field-ambience-current/field_ambience_pcb_SPEC_v0.6.md` | vollständige Hardware-Spec |
| `field-ambience-current/MEINE_TODO.md` | aktuelle Bau-Schritte (Abschnitt 0 = Komponenten-Status) |
| `field-ambience-current/CHANGELOG.md` | komplette Designgeschichte v0.5 → v0.8 |

---

*Diese Datei ist Draft v0.1. Vor Veröffentlichung: konkrete Crowdfund-Plattform
festlegen (Kickstarter / Crowd Supply / Indiegogo / eigene Donation-Page),
Pricing-Tiers ergänzen, Foto-/Render-Material des Hardware-Prototyps einbinden,
Web-Edition auf GitHub Pages hosten und Link in Abschnitt „Sofort spielen"
einsetzen.*
