# ADR-0024 — TPA6132A2 Kopfhoererverstaerker: J8 wird PHONES / LINE OUT (r19.19)

**Status:** Accepted — 2026-07-13
**Ausloeser:** User-Anforderung („ja das muss rein!!!"): die 3,5-mm-Buchse soll
Kopfhoerer UND externe Boxen koennen, mit dem bestehenden Auto-Mute-Verhalten.

## Problem

J8 hing passiv am PCM5102A (nur 22-Ω-Serien-R). Der DAC ist fuer Lasten
≥1 kΩ spezifiziert — Line-Eingaenge perfekt, aber 16–32-Ω-Kopfhoerer laufen
ausserhalb der Spec (leise, undefiniert). Das r19.18-Label „LINE OUT" war die
ehrliche Beschreibung dieses Zustands, nicht das gewuenschte Produktverhalten.

## Entscheidung

**TI TPA6132A2RTER** (DirectPath-Stereo-Kopfhoererverstaerker) zwischen DAC
und Buchse:

```
PCM5102A OUTL/R ──┬── C_in_L/R 1µF ── PAM8403 (Speaker, unveraendert)
                  └── C_HP_INL/R 1µF ── U11 TPA6132A2 (Gain −6 dB)
                                          │ EN = AMP_nSHDN (boot-safe low)
                                          └── R_LO_L/R 22Ω ── J8 T/R
```

- **DirectPath**: interne Ladungspumpe erzeugt die negative Rail → Ausgaenge
  ground-zentriert, **keine Auskoppel-Elkos**, aktive Pop-Unterdrueckung.
- **Gain −6 dB** (G0=G1=GND, DS Table 1): 2,1 Vrms DAC-Full-Scale → ~1,05 Vrms.
  Das ist gleichzeitig sauberer Consumer-Line-Pegel und (mit den 22-Ω-Serien-R,
  −4,6 dB an 32 Ω) sichere, trotzdem laute Kopfhoerer-Lautstaerke. Der Chip
  haelt die Maximalleistung versorgungsunabhaengig konstant (Acoustic-Shock-
  Design, DS §7.3.3).
- **EN = AMP_nSHDN**: gleiches Enable wie der Speaker-Amp — beide sind im
  Boot/Aus in Shutdown (R_SHDN_PD 10k). TPA-Shutdown-Iq 0,7–1,2 µA → die
  ungeschaltete +5V-Rail bleibt im Aus-Zustand drainfrei (passt zu ADR-0023).
- **Auto-Mute unveraendert**: Jack-Detect → MCP GPA6 → Firmware zieht nur
  AMP_nMUTE (Speaker stumm); U11 bleibt an → Kopfhoerer/Line live. Ausstecken
  → Speaker wieder an. Detect-Analyse r18.82 bleibt gueltig (TIP jetzt am
  TPA-Ausgang: DC 0 V, Shutdown-Zout ~20 Ω per DS).

## Beschaltung (alle Werte TI SLOS597B, Juli 2017)

| Element | Wert | DS-Quelle |
|---|---|---|
| C_HP_INL/R (Eingangskopplung) | 1 µF X5R (C15849) | §8.2.1.2.1 (fc < 15 Hz; INL+/INR+ direkt an GND fuer SE) |
| C_FLY_HP (CPP–CPN) | 1 µF X5R (C15849) | §8.2.1.2.2 (1–2,2 µF) |
| C_HPVSS (HPVSS→GND) | 1 µF X5R (C15849) | §8.2.1.2.2 (≥ Flying-Cap) |
| C_HP_VDD (VDD→GND) | 2,2 µF X5R (C1607) | §9 („within 5 mm") |
| C_HPVDD (HPVDD→GND) | 2,2 µF X5R (C1607) | §9 — **WARNING: HPVDD NIE an VDD** (interne Rail) |
| G0 / G1 | GND / GND | Table 1 → −6 dB |
| EN | AMP_nSHDN | VIH 1,3 V (3V3-GPIO ok); high = an |
| SGND (Pin 15) | GND, Layout: eigene Leitung zum J8-Sleeve | Pin-Functions-Tabelle |
| EP | GND | Pin-Functions-Tabelle (GND oder floatend) |

## Verifikation (r19.19)

- **TPA6132A2RTER C69901** live geprueft: LCSC 370 Stk., $1,35@1, JLC
  Economic+Standard. 2,2-µF-Cap **C1607** (Samsung CL10A225KP8NNNC): 16.200 Stk.
- **Pin-Map** aus TI SLOS597B Pin-Functions-Tabelle (RTE WQFN-16) extrahiert.
- **Footprint** `QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` gegen das JLC/EasyEDA-
  Landpattern fuer C69901 (Pads 0,28×0,8 @ 0,5 mm, EP 1,6×1,6, CCW) und die
  TI-RTE0016C-Zeichnung (EP-Metall 1,68±0,07) abgeglichen — kompatibel.
- **Netzliste**: 165 Netze, 649 Pins, 0 floating. AMP_nSHDN enthaelt jetzt
  U11.13(EN) + U4.12(/SHDN) + MCU PB14; HP_OUTL/R, HP_CPP/CPN, HP_HPVDD,
  HP_HPVSS jeweils exakt 2 Pins; J8-T/R haengen an den R_LO-Ausgaengen.
- check_pinmap gruen; JLC-BOM: 60 Teile-Typen, 200 Platzierungen.

## Konsequenzen

- Gehaeuse-Label: **PHONES / LINE OUT** (statt LINE OUT).
- +~$1,50 BOM (U11 + 2× C1607 + 4× 1µF), ~25 mm² Layout nahe J8.
- Kein Firmware-Change noetig: AMP_nSHDN/AMP_nMUTE-Logik deckt beide Amps ab.
- C69901-Stock (370) ist der niedrigste im BOM → Order-Day-Checkliste (B1).
- Layout-Pflichten: C_HP_VDD/C_HPVDD <5 mm an Pins 14/12; SGND-Leitung zum
  Jack-Sleeve; Ladungspumpen-Caps eng am Chip.
