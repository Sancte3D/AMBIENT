# ADR-0007: Speaker-Cover — Dust-Mesh statt sichtbarer Speaker

**Status:** ACCEPTED (User-Entscheidung 2026-06-11, IMG_9713)
**Date:** 2026-06-11

## Context

Im Industrial-Design-Stand bis r18.7 waren die zwei Onboard-Speaker
(PUI AS04008PS, 8 Ω, 40 mm) als sichtbare Speaker-Grille gedacht — runde
Lochmuster im Frontpanel.

Im neuen Stand (IMG_9713) sind sie durch **schwarze ovale Dust-Mesh-
Membranen** auf den linken und rechten Seiten ersetzt. Visuell flat,
keine sichtbaren Löcher, keine Grille-Muster — ein anti-statisches schwarzes
Akustik-Mesh.

## Decision

Speaker-Cover wird zu einem **Schwarzakustik-Mesh (Anti-Dust + Schallpass)**
gewechselt.

Konkret (Material r18.17b nach Komponenten-Recherche festgelegt):

- **Cover-Material: Saati Acoustex 020–032** (präzisionsgewebtes
  Polyester-Monofilament, schwarz). Das ist die transparente Speaker-Cover-
  Klasse:

  | Grade | Akust. Impedanz | Poren | Offene Fläche | Dicke | Gewicht |
  |---|---|---|---|---|---|
  | Acoustex 020 | 20 Rayls | 68 µm | 38 % | 62 µm | 32 g/m² |
  | Acoustex 032 | 32 Rayls | 38 µm | 31 % | 48 µm | 25 g/m² |

  > **Korrektur:** Die frühere Angabe „~150–200 g/m²" war FALSCH — das ist eine
  > Saati-**Dämpfungs**-Klasse (080/115/155), die den Treiber dumpf machen würde.
  > Für eine Grille (Schallaustritt) gilt die transparente Klasse 020–032
  > (~25–32 g/m²). 3M-Akustik-Mesh wäre eine gleichwertige Alternative.
- **Selbstklebend (PSA):** Acoustex-Rohmesh ist **nicht** selbstklebend. Für
  die Serie wird es von einem Konverter (**Marian Inc.**, Standard-Saati-
  Konverter) gestanzt und mit einem **PSA-Klebering** (Kleber nur am Rand,
  Mitte atmet) laminiert → fertiges selbstklebendes Cover-Teil.
- **Prototyp-Pfad (erste 5–10 Einheiten):** selbstklebende „Phone-Speaker-
  Dustproof-Mesh"-Sticker (AliExpress/Amazon, schwarz, 20–200 Stück für
  wenige Euro), 36×24-Oval ausstanzen. Kein Hard-Tooling — deckt sich mit dem
  „kein Hard-Tooling für erste Einheiten"-Plan unten.
- **LCSC/Mouser/Digikey führen KEIN Akustik-Mesh** — Mesh-Beschaffung läuft
  über Saati→Marian (Serie) oder AliExpress-Sticker (Prototyp).
- Form: **Ovale Aussparung 36 × 24 mm pro Seite** (maßgeblich
  `mechanical/coordinates/mechanical_coordinates.md` §5; passt zum realen
  40×28.3-Treiber mit 2 mm Rim-Seat — die 50×30- und 36×70-Werte früherer
  Revisionen sind retired).
- Mesh wird zwischen Frontpanel und Speaker-Frame **eingeklebt oder
  thermofixiert** (Mesh-zu-Plastik-Bond, Standard-Lautsprecher-Tooling)
- Innenraum: **Closed-Box** (sealed, ADR-0011/SPEC §8 — kein Reflex bei
  F0=380 Hz), ~20–30 cm³ pro Kanal.

### Optionale Alternative (falls IP-Schutz gewünscht)

**GORE Acoustic Vent** (ePTFE-Membran, selbstklebend, IP-rated) statt
gewebtem Mesh — bessere Wasser-/Staub-Dichtheit bei gleichem Akustik-Verlust,
höhere Kosten. Nur nötig, wenn das Gerät eine IP-Klasse bekommen soll.

## Consequences

**Positive:**
- Visuell ruhiger, hochwertiger als sichtbare Speaker-Grilles — passt zum
  monochromen OP-1-Field-Sprachstil
- Dust-Mesh schützt Schwingspule und Membran vor Staub — Lebensdauer +
- Anti-Statik: kein „Funke"-Risiko bei trockener Luft auf Polyester-Mesh

**Negative:**
- Akustische Penalty: -1 bis -2 dB Insertion-Loss bei den meisten Mesh-Typen
  (vernachlässigbar; durch Volume-Calibration kompensierbar)
- Mehrkosten: ~$0.50-1.00 pro Cover (gestanzt + thermofixiert) — pro Einheit
- Tooling-Aufwand: Mesh-Stanzform + Heißbond-Tool, einmaliger Kostenpunkt

## Implementation Plan

| Phase | Was | Status |
|---|---|---|
| Mechanical CAD | Frontpanel-Aussparung 36×24 mm oval, Mesh-Sitz-Tasche (0.3 mm Versenkung) | ✅ in `mechanical_coordinates.md` §5 |
| Component Review | Mesh-Klasse + Lieferant + Speaker-Verifikation | ✅ r18.17b: Acoustex 020–032 + Marian-PSA + PUI-Korrekturen |
| Prototype | AliExpress-Klebe-Mesh-Sticker, 36×24-Oval ausstanzen — kein Hard-Tooling | ⏳ bei Proto-Bau |
| Production | Marian-PSA-Stanzteil (Saati Acoustex 020–032) | ⏳ vor Serie |
| Akustik-Charakterisierung | Insertion-Loss am realen Muster messen (Volume-Kalibrierung) | ⏳ Bring-Up |

## Lautsprecher — bleibt PUI, aber Spec-Korrekturen (r18.17b)

**Treiber bleibt PUI AS04008PS-4W-WR-R** — 2026 aktiv und gut lagernd
(DigiKey ~1600, Mouser ~500 Stück), niedrigstes F0 (380 Hz) seiner Klasse,
passt zum Ambient-Charakter. **Aber drei Datenblatt-Korrekturen:**

| Was | Falsch (bisher) | Korrekt |
|---|---|---|
| Maße | 40 × 40 × 9 mm | **40 × 28.3 × 11.5 mm** (rechteckiger Rahmen) |
| „-WR"-Suffix | (als „Wire" gelesen) | **Water-Resistant** (behandelter Konus) |
| Terminierung | (Kabel angenommen) | **Löt-Eyelets, KEINE Kabel** — Draht selbst anlöten |

**Assembly-Konsequenz:** Der Treiber ist **Hand-Assembly, kein PCB-/JLC-
Bestücken** (Eyelet-Through-Body, nicht Pick&Place). JLCPCB führt überhaupt
keinen 40-mm-8-Ω-2-W-Kompakt-Treiber in der Bestück-Bibliothek (nur große
Visaton-HiFi-Chassis) — der Speaker ist in jedem Fall ein separates
Hand-Lötteil an den PAM8403-Ausgang.

**Mechanik-Konsequenz** (11.5 mm Tiefe statt 9 mm): Gehäuse-Außenhöhe
19.6 → 21.6 mm (Above-PCB-Raum 10 → 12 mm), damit der von der Top-Platte
hängende Treiber nicht in die PCB-Ebene kollidiert. Details
`mechanical_coordinates.md` §2/§7.

**Dokumentierte Zweitquelle:** **Same Sky (CUI) CMS-402811-28SP** — gleicher
40 × 28.3 × 11.5-Footprint, 8 Ω, 2 W, Löt-Eyelet, F0 = 450 Hz (etwas weniger
Tiefgang als PUI → PUI bleibt erste Wahl). DigiKey/Mouser-lagernd.

## Was bewusst NICHT geändert wird

- **Audio-Path:** PCM5102A → PAM8403 → Speaker-Eyelets bleibt
- **Stereo-Trennung:** ein Speaker links, einer rechts — bleibt

## Related

- ADR-0006 — Cell-Piano-Feel (gleicher Mechanik-Update-Sprint)
- `../../mechanical/coordinates/mechanical_coordinates.md` — wird mit den ovalen Speaker-Aussparungen
  aktualisiert
