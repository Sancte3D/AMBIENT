# Field Ambience — ROADMAP

Stand: **v29-o (instrument mode)**. Was funktioniert, was als nächstes kommt, was später.

---

## Kernprinzip (kritisch)

**Field Ambience ist ein INSTRUMENT, kein Soundgenerator.**

Default-Verhalten:
- `~famStart.()` startet die Engine, aber **nichts spielt**
- Cells sind die Klangquelle — der User spielt aktiv
- Generative Auto-Progression ist **opt-in** über `/fam/generative` (Generate-Button)
- Tonika-Drone ist **opt-in** über `/fam/drone` (Drone-Button)
- Beide standardmäßig OFF

---

## Aktueller Stand (v29-o)

### Sound-Engine

**Sound Constitution** als Grundlage:
- Defaults sind **deep, dark, slow, harmonisch stabil**
- Tempo: 54 BPM (nur in generative-mode relevant)
- Akkord-Wechsel: alle 8 Bars (nur generative-mode)
- Default Progression in generative-mode: `\slow_bed [I, IV]`
- Drive Default: 0.18 (warmer Körper)
- Brightness Default: -0.2 (leicht dunkler)
- Reverb t60: 4.5s

**Layer**:
- 5-stimmiges Pad mit Tape-Wow Pitch-Drift + Haas-Stereo (nur bei aktivem Cell-Hold oder generative-mode)
- Two-layer Bass: SubBass (sub region, gentle breath) + DeepBass (warm body, no harsh harmonics)
- Two-layer Texture: Rumble + Breath (Sparkle entfernt — zu unruhig)
- JPverb Premium-Reverb (oder FreeVerb2-Fallback)
- Drone-Voice: einzelner sustained Pad-Voice auf Tonika (nur wenn drone-mode aktiv)

**Macro-Encoders im Display-Menu**:
- KEY · MODE · CHORDS · MOOD · VOICE · OCTAVE · TEMPO · TEXTURE · SPACE · DEPTH · BLOOM
- Alle Werte safe-clamped — keine Kombination kann hässlich werden

**Modifier-Buttons**:
- SHIFT (Cells 6-10), HOLD (hold-mode für cells), DRONE (toggle), GENERATE (toggle), CLEAR

### Hardware (Konzept)
- Raspberry Pi Pico, PCM5102A I²S DAC, PAM8403 Class-D-Amp
- 2× PUI AS04008PS-4W-WR-R 40mm Speaker (rechteckig, hinter rundem Front-Panel-Grille)
- 5 Cell-Buttons (Silikon, MX-style), 5 Modifier-Buttons, 4 Encoder mit LED-Punkt-Anzeige, OLED 128×64

### Software-Stack
- SuperCollider als Audio-Engine (`field_ambience_v29o.scd`)
- Python-Bridge (`field_ambience_bridge.py`) — 30 State-Felder (inkl. generative + drone)
- HTML-Panel — Industrial-Mockup-Design mit 4 Encoder, 5 Cells, 5 Modifier (Generate + Drone), runde Speakers

---

## Constitutional Audit (v29-o)

Alles was der User über das Menü erreichen kann ist **garantiert nicht hässlich**:

| Parameter | Audit-Ergebnis |
|---|---|
| KEY (12 Tonarten) | ✓ alle safe |
| MODE (6 Modi: Major, Dorian, Phrygian, Lydian, Mixolydian, Minor) | ✓ Locrian entfernt (war dissonant) |
| CHORDS (5 Progressionen + Markov) | ✓ Default Slow Bed, Markov nur bewusst |
| MOOD (4 Vibes: Warm, Bright, Deep, Floating) | ✓ Sharp entfernt (war harsh) |
| VOICE (Warm, Strings, Brass) | ✓ alle safe (Brightness clamped) |
| OCTAVE (Low, Mid, High) | ✓ alle safe |
| TEMPO (40-90 BPM) | ✓ Range begrenzt (war 40-140) |
| TEXTURE (0-100%) | ✓ BPF Q=0.4, max amp 0.12 |
| SPACE (0-100%) | ✓ Reverb-Werte alle in safe ranges |
| DRIVE Encoder | ✓ tanh self-limit |
| BRIGHTNESS Encoder | ✓ clamped -1000..+2700Hz |
| VOLUME | ✓ kann nur leiser/lauter machen |

**Was entfernt wurde** als Constitution-Verletzungen:
- Mode `\locrian` — diminished, instabil, unaufgelöst → dissonant
- Vibe `\sharp` (maj7#11) — kann mit Brightness=hoch dissonant werden
- Tempo > 90 BPM — bricht Slow-Bed-Konzept (Constitution: never fast)

---

**Brightness Encoder**:
- Range -1..+1, intern auf -1000Hz..+2700Hz Hz-Offset clamped
- Maximum erreichbarer Cutoff ~3000Hz
- Keine Resonanz-Spitzen über 3kHz

**Drive Encoder**:
- Range 0..1, mappt zu tanh-Pre-Gain 1x..5x
- tanh self-limits, niemals digital harsh

**Pad-Voices**:
- Attack minimum 0.8s, Release minimum 3s
- Pitch immer scale-locked
- Keine Cell-Tap-Sprünge über +1 Oktave

**Engine-Defaults**:
- AUTO-Markov OFF
- 8 Bars pro Akkord
- Glitter-Layer komplett aus
- Texture mit BPF Q=0.4 (keine Resonanz-Peaks)

---

## TO DO — kurzfristig

### Sound-Tests
- [ ] **Erstes komplettes Anhören mit Constitution-Defaults**
  - Empfohlene Start-Settings: Drive=0.18, Brightness=-0.2, Texture=0.45, Vol=0.8
  - Score-Format: Deepness, Epicness, Chaos, Harshness, Warmth, Movement, Emotional pull
- [ ] **TEXTURE-Layer feinabstimmen** (3-Layer field-recording-style)
- [ ] **Pad-Bloom feinabstimmen** (fenvAmount 0.25 ggf. zu wenig oder zu viel)
- [x] **Pitch-Drift Pad** — Tape-Wow LFO (0.18Hz, ±0.3% / ~5 cent)
- [x] **Sub-Bass Atmung** — sehr langsame Amp-LFO (0.07Hz, 18% depth)
- [x] **Haas-Stereo Pad** — 8/14ms L/R Delay für massive Stereo-Breite
- [x] **Texture als 3-Layer** — Rumble + Breath + Sparkle mit asynchronen Zeitskalen
- [x] **Markov constitutional** — vii-Stufe ausgeschlossen, weiche Voice-Leading-Übergänge

### Software-Strukturierung
- [x] **HTML-Panel matching v29-o** — Texture im Display-Menü hinzugefügt (8. Eintrag)
- [x] **Markov-Modus zugänglich machen** über AUTO-Toggle: AUTO ON = Markov, AUTO OFF = slow_bed
- [ ] **MIDI-Hardware-Erkennung in Bridge**

### Performance-Controls erweitern (Macro-Controls)
- [x] **SPACE Encoder** (Reverb-Send + Tail + Size, 0..1) — als Display-Menü-Eintrag
- [x] **DEPTH Encoder** (Sub-Amp + DeepBass-Amp + Pad-Release) — Display-Menü
- [x] **BLOOM Encoder** (Pad-Filter-Swell + Pad-Attack + Reverb-Wash) — Display-Menü

---

## TO DO — mittelfristig

### Hardware-Build
- [ ] PCB-Layout in KiCad
- [ ] PCB bei JLCPCB bestellen (5er Pack ~50€)
- [ ] BOM-Bestellung bei DigiKey (95-110€ Premium pro Gerät)
- [ ] 3D-Druck-Mockup
- [ ] Erstes Bauen + Akustik-Test

### Pi-4-Migration
- [ ] **SuperCollider headless auf RPi 4** (sclang + scsynth als systemd-Service)
- [ ] **Performance-Profiling** auf RPi 4
- [ ] **Auto-Recovery** falls SC abstürzt

### Bridge + Pico
- [ ] **Pico-Firmware testen** mit echter Hardware
- [ ] Encoder-Quadratur-Decoding
- [ ] OLED-Layout finalisieren

---

## TO DO — langfristig (Ideenpool)

### Sound-Erweiterungen
- [x] **Tape-Wow LFO** auf Pad-Pitch (0.18Hz, ±5 cent) — implementiert v29-o
- [x] **Multi-layer Texture** (rumble/breath/sparkle, asynchrone Zeitskalen) — implementiert v29-o
- [x] **Haas-Stereo Pad** — implementiert v29-o
- [x] **Sub-Bass Breathing** (LFO 0.07Hz auf Amp) — implementiert v29-o
- [x] **Side-Chain duck** Texture wenn Cells getriggert werden — implementiert v29-o
- [ ] **Dedicated Strings/Brass-SynthDefs**
- [ ] **Sample-Layer** (Felt-Piano, Glas-Klänge)
- [ ] **Convolution-Reverb** mit Lo-Fi-Cassette-IR statt JPverb
- [ ] **Field-Recording-Texture** (echte Samples statt Synthese — größerer Schritt)

### Hardware-Erweiterungen
- [ ] Photosensor für ambient brightness control
- [ ] Touch-Plates für mute-per-voice
- [ ] Akku-Betrieb (LiPo + Boost)

### Komposition/Logik
- [ ] **Tonart-Modulation** über Zeit (sanfte Transposition)
- [ ] **Pause-Phasen** zwischen Akkord-Wechseln
- [ ] **Sequenzer-Modus**

---

## Wichtige Erkenntnisse

### Was funktioniert hat
- **Sustained Pad-Layer** (v29-i) — Wendepunkt
- **Glitter-Layer komplett aus** (v29-m)
- **Cells als Pad-Voices** (v29-l)
- **DRIVE/BRIGHTNESS additiv** (v29-n)
- **Sound Constitution + Safe-Range-Clamping** (v29-o) — Regeln statt Geschmack

### Was nicht funktioniert hat
- **Filter-ADSR-Schwellen v29-j** zu schnell — v29-o ULTRA-slow Version
- **Prime-Bar-Längen** zu unruhig
- **VerbSend 0.6** zu nass
- **FM-Glocken** kitschig
- **Markov-Akkord-Progression standardmäßig** chaotisch

### Design-Prinzipien (Sound Constitution)
- **Defaults müssen schön klingen** ohne User-Action
- **Charakter erhalten beim Erweitern** — neue Funktionen bei Default = Original
- **Performance-Controls additiv** — Gesten, keine Engine-Changes
- **Spielbarkeit > Komplexität** — wenig sichtbare Parameter, klare Wirkung
- **Niemals hässlich werden können** — Safe Ranges für jeden Encoder
- **Regeln statt Geschmack** — "Verbote + sichere Bereiche" sind reproduzierbar

### Klang-Achsen-Übersetzung
| Gefühl | Technische Übersetzung |
|---|---|
| deep | mehr Sub, weniger Höhen, lange Release |
| episch | großer Raum, breite Pads, langsame Swells |
| harmonisch | feste Skala, wenige Akkorde |
| lo-fi | Sättigung, Noise-Bed (TEXTURE), dunklere Filter |
| nicht chaotisch | wenige Stimmen, langsame Modulation |
| nicht hässlich | keine extremen Resonanzen, kein FM, keine Sprünge |
| lebendig | subtile LFOs, Drift, leichte Variationen |

### Score-Format für zukünftiges Feedback
```
Version: v29-o
Gut:
  - ...
Schlecht:
  - ...
Scores (0-10):
  Deepness:
  Epicness:
  Chaos (lower=better):
  Harshness (lower=better):
  Warmth:
  Movement:
  Emotional pull:
Bitte ändern:
  - ...
```
