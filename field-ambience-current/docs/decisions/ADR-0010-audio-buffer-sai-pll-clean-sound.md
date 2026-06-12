# ADR-0010: Audio-Buffer-Größe + SAI-Clock-Architektur (Anti-Kratzig)

**Status:** ACCEPTED — Buffer-Fix in r18.11 implementiert, SAI-PLL-Setup für Phase 4 fixiert
**Date:** 2026-06-11

## Kontext

User-Frage: „Wie lösen wir Buffer-Probleme beim DAC? Damit Lautsprecher nicht kratzig klingen und alle sounds richtig rüberkommen."

„Kratzig" hat in DAC-Setups typisch sechs Ursachen, in absteigender Wahrscheinlichkeit:

1. **Buffer-Underrun** — Engine schafft nächsten Audio-Block nicht in time → DMA wiederholt alten Buffer → Klick/Buzz
2. **PCM5102A Pin-Strapping falsch** (FLT/FMT/DEMP)
3. **Clock-Jitter** im I²S-BCK (vom HSE-Crystal über SYSCLK-PLL)
4. **Power-Rail-Kopplung** Class-D-PAM8403 ⇒ PCM5102A-AVDD
5. **Engine-Output-Clipping** an Int16-Grenze
6. **Speaker-Mechanik** überfordert (Sub-Bass < 60 Hz auf 40-mm-Treiber)

Engineering-Analyse pro Punkt vor Maßnahmen.

## Befund-Analyse

### Punkt 1 — Buffer-Underrun: **echtes Risiko** auf H7-Port

- aktueller Stand (r18.10): `AUDIO_BUFFER_FRAMES = 256` @ 44.1 kHz = **5.8 ms Window** pro Halb-Buffer
- Pico-2-Stand: lief stabil, weil PIO-PLL die Engine-CPU-Last natürlich begrenzte
- H7-Stand: 480 MHz M7 hat viel Headroom — ABER worst-case-Burst (Generative-Bed-Trigger + Reverb-Tail-Spike + LCD-DMA gleichzeitig) kann über 5 ms gehen
- Symptom: einzelne Klick/Knack-Geräusche, nicht permanent „kratzig"

### Punkt 2 — PCM5102A Pin-Strapping: **NICHT die Ursache** (verifiziert)

`generate_kicad_project.py` Zeilen 5029-5062 + audio.kicad_sch geprüft:

| Pin | Soll | Ist |
|---|---|---|
| 10 DEMP | GND (no de-emphasis) | ✅ GND |
| 11 FLT | GND (Normal-Latency = besseres SNR) | ✅ GND |
| 16 FMT | GND (I²S Format) | ✅ GND |
| 17 XSMT | controlled un-mute | ✅ MCP-GPA5 (PCM_XSMT-Net) |
| FSCK | Auto-Mode (BCK-only) | ✅ kein externer MCLK → PCM5102A-interne PLL self-tunes |

**Saubere Best-Practice-Strapping.** Hier ist nichts kaputt.

### Punkt 3 — Clock-Jitter: **strukturelles Risiko ohne Audio-PLL**

- HSE = 8 MHz ABLS-Crystal, GM = 2.97 (ADR-0008-Y1)
- Aktueller Plan (SPEC §5.9): PLL_M=4 → PLL_N=480 → PLL_P=2 → SYSCLK 480 MHz
- Wenn SAI von SYSCLK gespeist wird, koppelt CPU-Last als Phase-Noise auf BCK
- Bei 44.1 kHz × 256-fs = 11.2896 MHz Master-Clock erwartet PCM5102A weniger als ~50 ps RMS-Jitter für 16-Bit-Performance (24-Bit will ~10 ps)
- SYSCLK-abhängiger BCK liefert typisch 100-500 ps Jitter → DAC-PLL filtert das nur teilweise → hörbar als „rauh" oder „Hochton-Sand"

### Punkt 4 — Power-Rail-Kopplung: **Layout-Constraint**

- PAM8403 Class-D schaltet mit ~250-500 kHz, zieht 1-2 A-Spitzen bei Bass-Transienten
- PCM5102A-AVDD ist über FB1 (BLM18AG601) vom Digital-+3V3 getrennt (SPEC §3 dokumentiert)
- ABER: PAM8403 hängt direkt am +5V (kein eigenes Power-Filter)
- Wenn C_BULK weit vom PAM8403-PVDD-Pin sitzt → +5V-Rail moduliert mit Bass-Frequenz → koppelt via Common-GND auf PCM5102A-Output-Pfad

### Punkt 5 — Engine-Clipping: **bereits mitigiert** (Step-11-Soft-Limiter)

`engine.c` hat Soft-Clipper im Master-Stage (siehe `test_reverb_engine.c`: "master volume scaling vol1 peak=30748" zeigt: deutlich unter 32767-Vollausschlag). Nicht die Ursache.

### Punkt 6 — Speaker-Mechanik: **physisch nicht weiter lösbar**

40-mm-PUI-AS04008PS produzieren Sub-Bass < 60 Hz nicht zuverlässig — bei viel Volume mechanisches Klappern statt Tonwiedergabe. **Lösung ist im Sound-Design:** SubBass-Layer ausschließlich an J8 (Line-Out) routen, Speaker-Pfad als Mid-Range-Voice (Tiefpass bei ~70 Hz). Bereits in Sound-Constitution diskutiert.

## Entscheidungen

### r18.11 (sofort, Code-Änderung)

**AUDIO_BUFFER_FRAMES: 256 → 512** in `firmware-c-next/include/audio.h`.

- Latenz: 5.8 ms → 11.6 ms. Für Pad-/Drone-/Ambient-Sound deutlich unterhalb Perzeptionsschwelle (~20 ms ist die konventionelle Grenze für Sustain-Instruments; Cell-Tap-Latenz ist Input-Side, nicht Output-Side)
- Sicherheits-Marge: 2× gegenüber Worst-Case-Burst
- SRAM-Kosten: 2 × 512 × 4 Bytes = 4 kB statt 2 kB — vernachlässigbar (STM32H7 hat 1 MB)
- Tests: alle 12+ Host-Suiten grün nach Änderung

### Phase 4 (Firmware-Migration, NATIVE_PORT_PLAN Step 13.4)

**SAI-Clock-Speisung aus dedicated Audio-PLL.**

- PLL3-P konfigurieren auf 11.2896 MHz (= 44100 × 256-fs)
- SAI1-Clock-Source = PLL3-P (RCC_D2CCIP1R bit CKSAI1SEL = 01)
- SAI1-Divider intern: MCK-DIV = 1 (11.2896 MHz interner MCK), Bit-Clock-DIV = 4 (= 2.8224 MHz BCK), Frame-Length = 64 Bit (32 BCK pro Sample × 2 Channels) → LRCK = 44.100 kHz exakt
- PLL3-Speisung: HSE 8 MHz / DIVM = 4 → 2 MHz PFD → MULN = 96 → 192 MHz VCO → PLL3-P = /17 ≈ 11.2941 MHz (Abweichung 40 ppm = innerhalb DAC-PLL-Lock-Range). **Exakt:** PLL3-DIVN = 271, PLL3-P = 12 → 11.2916 MHz (besser, in Phase 4 final rechnen)
- Vorteil: SYSCLK-Last hat null Einfluss auf Audio-Clock; Jitter ~5-20 ps statt 100-500 ps

### Layout-Constraints (Phase 6, dokumentiert in SPEC §5.1)

1. AVDD/DVDD-Decoupling-Caps **< 5 mm vom PCM5102A**
2. Single-Star-GND-Punkt zwischen Audio- und Digital-Block (Mid-Layer-Polygon-Anker)
3. C_BULK (1000 µF) so platzieren, dass der Strompfad zum PAM8403-PVDD-Pin den kürzesten Polygon-Loop bildet — **wichtigster anti-kratzig-Hebel**
4. Optional: Ferrit-Bead (BLM18AG601-Klasse) je PAM8403-Speaker-Output zur Class-D-Trägerfrequenz-Filterung — fängt 250-500 kHz ab, reduziert EMI
5. PCM5102A-LDOO-Pin: 100 nF X7R 0603 direkt am Pin (interner LDO-Stability)

### Bewusst NICHT geändert

- HSE-Crystal bleibt (ABLS GM = 2.97, ADR-0008-Y1)
- PCM5102A-Strapping bleibt (ist optimal)
- Engine-Limiter bleibt (Step-11-Soft-Clip)
- Speaker bleibt (PUI AS04008PS, Sub-Bass über J8)

## Consequences

**Positive:**
- Buffer-Underrun-Risiko halbiert (2× Window, 4× Headroom-Wahrscheinlichkeit)
- Audio-Clock-Jitter um 10-50× reduziert via PLL3-Speisung
- Layout-Constraints fixiert, bevor Layout startet
- Echte Mechanismen adressiert statt „erhöh' die Lautstärke und probier"

**Negative:**
- +5.8 ms Audio-Latenz (vernachlässigbar für Ambient)
- +2 kB SRAM (vernachlässigbar)
- PLL3-Konfig erhöht Firmware-Komplexität um eine Routine

## Related

- ADR-0002 — STM32H743-Migration (warum überhaupt H7-Audio-PLL)
- SPEC §5.1 — SAI-Pin-Allocation
- SPEC §5.9 — HSE-Clock-Architektur
- NATIVE_PORT_PLAN Step 13.4 — Phase 4 Firmware-Migration
