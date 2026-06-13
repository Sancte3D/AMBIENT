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

Konkret:

- Cover-Material: **Akustisches Polyester-Mesh** (z. B. Saati Acoustex,
  3M C2003F oder gleichwertig), schwarz, ~150-200 g/m² Flächengewicht,
  hohe Schallpässigkeit (-1 bis -2 dB Insertion-Loss bei 100 Hz - 8 kHz)
- Form: **Ovale Aussparung im Frontpanel** (50 × 30 mm pro Seite, r18.16-
  Mechanik-Koordinaten — die ursprüngliche 36×70-Angabe galt für den alten
  333×143-Outline und ist retired; maßgeblich ist
  `mechanical/coordinates/mechanical_coordinates.md` §5), nicht rund — passt
  zum Industrial Design
- Mesh wird zwischen Frontpanel und Speaker-Frame **eingeklebt oder
  thermofixiert** (Mesh-zu-Plastik-Bond, Standard-Lautsprecher-Tooling)
- Innerer Speaker-Mount unverändert: PUI AS04008PS bleibt, 40 mm runder
  Lautsprecher-Frame **dahinter** zentriert in der ovalen Aussparung
- Innenraum: Bass-Reflex oder Closed-Box je Enclosure-Volumen — gehört in
  Mechanical-CAD-Phase

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

| Phase | Was |
|---|---|
| Mechanical CAD | Frontpanel-Aussparung 36×70 mm ovale, Mesh-Sitz-Tasche (0.3 mm Versenkung) |
| Component Review | Mesh-Hersteller-Vergleich (Saati Acoustex, 3M Akustik-Tape, Pyramid Acoustic), Akustik-Specs verifizieren |
| Prototype | Stanz-Tool oder Cricut-Cut für erste 5-10 Einheiten — kein Hard-Tooling nötig |

## Was bewusst NICHT geändert wird

- **Lautsprecher selbst:** PUI AS04008PS bleibt (40 mm, 8 Ω, 4 W, mid-range
  voiced — passt zum Ambient-Sound-Charakter)
- **Audio-Path:** PCM5102A → PAM8403 → Speaker-Klemmen bleibt
- **Stereo-Trennung:** ein Speaker links, einer rechts — bleibt

## Related

- ADR-0006 — Cell-Piano-Feel (gleicher Mechanik-Update-Sprint)
- `../../mechanical/coordinates/mechanical_coordinates.md` — wird mit den ovalen Speaker-Aussparungen
  aktualisiert
