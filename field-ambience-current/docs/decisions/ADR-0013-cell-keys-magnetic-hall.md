# ADR-0013: Cell-Tasten — Low-Profile-Magnetic-Switches + lange Caps mit Stabilizern

**Status:** ⛔ **SUPERSEDED (User-Direktive, 2026-06-30, r18.73)** — Cells sind
jetzt **digitale on/off-Switches am MCP23017-I²C-Expander** (HiChord-Batch-4+-Weg:
Switch → I²C-GPIO-Expander → MCU), **nicht** mehr Gateron-Magnetic + DRV5056A4-Hall.
War: ACCEPTED (2026-06-12), ersetzte den FSR-Teil von ADR-0006.
**Date:** 2026-06-12 (superseded 2026-06-30)

> **Supersede-Begründung (User, 2026-06-30):** HiChord macht es sehr
> wahrscheinlich nicht mit Gateron-Magnetic-Switches + Hall-Sensoren. Laut
> offizieller HiChord-Doku nutzt Batch 4+ **digitale I²C-Buttons** (keine
> Kalibrierung). Für reines **Triggern** der Cells reicht das digitale Modell:
> 5 Switches → I²C-GPIO-Expander (hier: das bereits verbaute MCP23017 U2,
> GPA0–GPA4) → MCU. Hall + Magnet wäre nur nötig, wenn die Cells **expressiv**
> (Drucktiefe/Anschlagsdynamik) sein sollen — das ist hier bewusst zurückgestellt
> („unnötiges Risiko"). **Umsetzung (r18.73):** entfernt 5× DRV5056A4 (J_CELL1–5),
> 5× R_CELL 1 kΩ, 5× C_CELL 10 nF; STM32-ADC-Pins PC0/PC1/PA4/PB0/PB1 freigegeben
> (Rev-B-Reserve); SW1–SW5 (HX B3F-4055-Y, C36498965 — gleiches Bauteil wie die
> Modifier SW6–SW10) auf GPA0–GPA4. **Kein neues Bauteil.** Stellt die
> ursprüngliche SPEC-v0.6-§7-„10 Switches"-Topologie wieder her. Hall bleibt
> unten als dokumentierte Option für eine spätere expressive Cell-Variante.
>
> Dieses ADR bleibt als Referenz für den **Hall-Pfad** erhalten (falls
> Velocity/Aftertouch später doch gewünscht ist). Der unten beschriebene Stand
> ist NICHT der aktuelle PCB-Stand.

> **Korrektur r18.74 (User-UX-Feedback, 2026-07-01):** r18.73 hatte SW1–SW5 auf
> **dasselbe Bauteil wie die Modifier SW6–SW10** gesetzt (HX B3F-4055-Y THT-
> Tactile). Der User wies zu Recht darauf hin, dass das die spielbaren Cells
> ununterscheidbar von simplen Modifier-Knöpfen macht und das "Tastatur/
> Keyboard"-Spielgefühl zerstört. **Fix (r18.74):** Cells bleiben elektrisch
> identisch digital (gleiche Netze/Pins/Pull-Up/IRQ), bekommen aber einen
> **echten Kailh-Choc-Keyswitch über Hot-Swap-Socket** zurück (~3 mm echter
> Hub) — exakt die **„Plain Choc V2"-Option**, die in der Vergleichstabelle
> unten (Kontext-Abschnitt) bereits als Alternative aufgeführt war, nur ohne
> Velocity.
>
> **Korrektur r18.75 (User-Nachfrage, 2026-07-01 — "wie wird das gelötet?"):**
> der Hot-Swap-Socket aus r18.74 hatte **keine saubere Hersteller-/LCSC-
> Teilenummer** und hätte eine nicht offensichtliche Klein-SMD-Handlöttechnik
> gebraucht — der User fragte zu Recht nach, wie das überhaupt gelötet werden
> soll. **Fix (r18.75):** der Kailh-Choc-V1-Switch (**CPG135001D01**, LCSC
> **C400229**, verifiziert real) wird jetzt **direkt auf die Platine gelötet**
> — 2 THT-Beinchen durch 2 Löcher, von hinten verlötet, exakt dieselbe Technik
> wie bei jedem anderen Button/Connector in diesem Design. Kein Socket mehr,
> keine Spezialtechnik — dafür ist der Switch jetzt fest verlötet (nicht mehr
> tauschbar). Footprint + 3D-STEP direkt von LCSC/EasyEDA für dieses exakte
> Teil gezogen (`easyeda2kicad --full --lcsc_id=C400229`), vendored als
> `field_ambience:SW_KailhChoc_CPG1350_THT_2P` — der Weg, den dieses Repo für
> alle Custom-Footprints vorschreibt, nicht aus einer Keyboard-Hobby-Library.
> Modifier SW6–SW10 bleiben das kleine HX-B3F-Tactile — bewusster UX-
> Unterschied (Cells = Keyboard-Feel, Modifier = simples Tactile). Details:
> `BOM_MASTER.md` §7, `MECHANICAL_REQUIREMENTS.md` §1.5,
> `PCB_FOOTPRINT_RISK_AUDIT.md`.

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

> **r18.21-Kostenrevision:** Stabilizer für den **Prototyp gestrichen**. Der
> User druckt die Cell-Caps selbst 3D — also werden sie **kurz (1u, ein Switch
> mittig)** gemacht statt lang (≥2u). Eine 1u-Cap braucht **keinen Stabilizer**
> (Stabilizer verhindern nur das Wackeln langer Tasten). Das spart $25–75 pro
> 5er-Run + vereinfacht das Plate-Design. **Wichtig:** Das HiChord-Spielgefühl
> kommt vom **Magnetic-Switch-Hub (0.1–3.3 mm analog)**, NICHT von der
> Cap-Länge — die Velocity-Erfassung bleibt voll erhalten. Lange Caps +
> Stabilizer bleiben als spätere Option dokumentiert (unten), falls das
> Industrial-Design es doch will.

- *(Spätere Option)* Cell-Caps ≥ 2u bekämen **Low-Profile-Stabilizer**
  (Gateron-LP-Klasse, wie NuPhy-Spacebars): Switch mittig, Stabilizer-Stems
  links/rechts — das Spacebar-Prinzip aus der ursprünglichen User-Anforderung.
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
