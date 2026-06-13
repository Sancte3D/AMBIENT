# ADR-0013: Cell-Tasten — Low-Profile-Magnetic-Switches + lange Caps mit Stabilizern

**Status:** ACCEPTED (User-Direktive, 2026-06-12) — **ersetzt den FSR-Teil von ADR-0006**
**Date:** 2026-06-12

## Kontext (User-Anforderung)

> „wir verwenden gerade so touch cells oder so, ich wollte aber welche wie bei
> dem HiChord, aber so dass es auch längere Knöpfe sein können — wie bei einer
> Tastatur: Spacebar hat mittig den Knopf und links und rechts so Support."

Also: **echte mechanische Tasten** (HiChord-artiges Spielgefühl), **lange
Keycaps möglich** (Spacebar-Prinzip: 1 Switch mittig + Stabilizer links/rechts),
nicht die FSR-Touch-Pads aus ADR-0006.

ADR-0006 hatte FSR gewählt, um **Velocity** (Piano-Feel) zu bekommen. Die
beiden User-Anforderungen — mechanisches Tastengefühl UND Velocity — schließen
sich mit der richtigen Switch-Klasse nicht aus.

## Entscheidung

**Low-Profile-MAGNETIC-Switches (Hall-Effekt) + lineare Hall-Sensoren auf dem PCB.**

Klasse: **Gateron Low-Profile Magnetic (Magnetic Jade LP)** — dieselbe
Switch-Klasse wie in NuPhy-Air-HE-Boards. Eigenschaften (Hersteller-Daten):
analoge Weg-Erfassung 0.1–3.3 mm, kontaktlos (100 M Zyklen), linear, leise.

### Warum das beide Anforderungen gleichzeitig löst

| Anforderung | FSR (ADR-0006) | Plain Choc V2 | **LP Magnetic (dieses ADR)** |
|---|---|---|---|
| Mechanisches Tastengefühl (HiChord) | ❌ Touch-Pad | ✅ | ✅ ~3 mm echter Hub |
| Velocity (Piano-Feel) | ✅ Druck-basiert | ❌ binär | ✅ **besser: echte Weg/Zeit-Messung** |
| Lange Caps + Stabilizer | ⚠️ formbar | ✅ | ✅ LP-Stabilizer (Spacebar-Prinzip) |
| Schematic-Aufwand | ADC-Teiler | GPIO | **ADC unverändert** (nur Quelle wechselt) |

Velocity wird aus der Stem-Position über Zeit berechnet (dPos/dt beim
Durchgang durch die Trigger-Zone) — physikalisch korrekter als FSR-Druck.
Bonus: **Aftertouch und einstellbarer Trigger-Punkt** sind später reine
Firmware-Features (analoge Kurve liegt ja vor).

### Architektur

```
Frontpanel:   [ lange Cell-Cap (z.B. 2u) ]
                    │ mittig          ├─ LP-Stabilizer links/rechts
Plate:        [ Gateron LP Magnetic Switch ]   ← pin-los, Magnet im Stem
PCB:          [ linearer Hall-Sensor ]  → Serien-R 1k + 10 nF (RC, fc≈16 kHz)
                                        → CELLn_SENSE → STM32-ADC
```

- **Switch ist elektrisch pin-los** — er steckt in einer Plate; das PCB trägt
  nur den Hall-Sensor unter dem Stem. Sauberere Assembly als Hotswap-Sockets.
- **ADC-Interface unverändert**: PC0/PC1/PA4/PB0/PB1 (Pins 15/16/28/34/35),
  Netze `CELL1..5_SENSE` — identisch zu r18.9. Im Schematic wechselt nur der
  Quell-Block: FSR-Teiler → 1×3-Hall-Site + Serien-RC (Generator r18.14).

### Sensor-Kandidaten (PINOUT-VERIFY vor Phase 6 — AP7361-Lektion!)

1. **TI DRV5056A4** (SOT-23): ratiometrisch, 3.3-V-fähig, unipolar — primär.
2. **SS49E-Klasse** (TO-92S): billig, überall verfügbar — Prototyp-Pfad.

Das Schematic führt pro Cell eine **1×3-2.54-mm-Site** (+3V3 / OUT / GND):
nimmt SS49E direkt oder ein SOT-23-Breakout. Finale SOT-23-Integration in
Phase 6 **nach** Datasheet-Pinout-Check (nicht vorher raten — r18.6-Lektion).

### Lange Caps / Stabilizer

- Cell-Caps ≥ 2u bekommen **Low-Profile-Stabilizer** (Gateron-LP-Klasse, wie
  NuPhy-Spacebars): Switch mittig, Stabilizer-Stems links/rechts — exakt das
  Spacebar-Prinzip aus der User-Anforderung.
- Cap-Geometrie (Länge, Profil, Material) = Industrial-Design-Sprint; die
  Mechanik-Schnittstelle (LP-Stem-Kreuz + Stabilizer-Raster) ist Standard im
  Keyboard-Ökosystem → Community-CAD verfügbar.

## Firmware-Stand (implementiert r18.15)

Das Velocity-Modell ist in Code und host-getestet, unabhängig von der noch
fehlenden Hardware:

- `include/cells.h` + `src/cells.c` — per-Cell-State-Machine (REST→ARMED→HELD),
  Velocity aus der Banddurchlaufzeit (BAND_LO→BAND_HI), Mapping auf
  Voice-Amplitude. Konstanten siehe SPEC §5.6a. Pure Logic, kein SDK.
- `engine_cell_sample(cell, pos, now_ms)` — Engine-Einstieg: PRESS spielt den
  Akkord-Grundton der Cell (harmonic brain) mit velocity-skalierter Amplitude,
  RELEASE stoppt ihn. Auf STM32 ruft die ADC-Schleife das; der RP2040-Bench
  kann Positionen aus den Digital-Buttons synthetisieren.
- `test/test_cells.c` (25 Checks) + ein End-to-End-Check im Engine-Test
  (schneller Druck messbar lauter als langsamer: Peak 8156 vs 4459).
- Web-Sim spiegelt die Kurve sichtbar (Klick-Höhe = Velocity, grüner Fill +
  0–127-Readout).

Offen bleibt nur das, was echte Hardware braucht: Velocity-Curve-Feintuning
am Muster, Z-Abstand Magnet↔Sensor, Trigger-Punkt-Geschmack.

## Konsequenzen

**Positiv:** Piano-Feel-Ziel aus ADR-0006 bleibt erreicht (sogar präziser);
HiChord-Tastengefühl wie gefordert; lange Tasten möglich; PCB-seitig
JLC-bestückbar (SOT-23); 100-M-Zyklen-Lebensdauer; Trigger-Punkt/Aftertouch
als Firmware-Feature offen.

**Negativ / offen:**
- Switches + Stabilizer sind Keyboard-Markt-Teile (Gateron direkt, kbdfans
  etc.), nicht LCSC → separater Beschaffungskanal (wie zuvor schon FSR).
- **Plate-Design nötig** (Switch-Cutouts 13.8×13.8-Klasse für LP — verifizieren)
  → neues Mechanik-Arbeitspaket, koppelt an Frontpanel-CAD.
- Hall-Sensor-Platzierung unter dem Stem braucht Z-Abstimmung (Magnet-Feld
  vs. PCB-Abstand) — Prototyp-Messung einplanen.
- ADR-0006-FSR-Beschaffung entfällt ersatzlos.

## Related

- ADR-0006 — Cell-Piano-Feel (Velocity-Ziel bleibt; Sensor-Technologie ersetzt)
- ADR-0012 — Encoder-Strategie (gleicher „Komponenten-Definition"-Sprint)
- ADR-0011 — Z-Budget (LP-Switch-Stack ~8 mm über PCB inkl. Cap — vor
  Frontpanel-CAD gegen 3D-Daten verifizieren)
- `mechanical/3d_models/MANIFEST.md` — Switch/Stabilizer-CAD-Quellen
