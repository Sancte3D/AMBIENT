# ADR-0023 — BQ24074 Power-Path ersetzt MCP73831 + Dioden-OR (r19.18)

**Status:** Accepted — 2026-07-13
**Ausloeser:** Externes statisches Hardware-Audit (2026-07, "AMBIENT v19 hardware
audit", Verdikt: DO NOT FABRICATE). Alle pruefbaren P0-Befunde wurden gegen die
selbst-extrahierte Netzliste verifiziert und bestaetigt.

## Problem (Audit P0-1 … P0-7, alle netzlisten-bestaetigt)

Die alte Lade-/Versorgungs-Architektur (MCP73831-Linear-Lader + SS34-Dioden-OR
D3B/D3 auf die +5V-Rail) hatte fabrikationsblockierende Fehler:

| # | Befund | Folge |
|---|---|---|
| P0-1 | Lader-VDD hing an **VBUS_USBC (VOR der Sicherung F1)** | Lader ungeschuetzt bei USB-Fehlern |
| P0-2 | LED_CHRG-Anode am **globalen +5V** (auch Boost-gespeist) | LED-Strom rueckwaerts in den STAT-Pin aus dem eigenen Boost, Anzeige sinnlos ohne USB |
| P0-3 | Boost **U8.EN an BAT_PLUS** = immer an | Akku-Drain im "Aus" (Boost-Iq + Rail-Lasten dauerhaft) |
| P0-4 | **Kein Zellschutz** im BAT+-Pfad | Hard-Short = unbegrenzter Zellstrom |
| P0-5 | Dioden-OR: zwei weiche 5V-Quellen parallel | Undefinierte Uebergaenge, Rueckspeise-Risiken |
| P0-6 | **~580 µF Bulk direkt am USB-Pfad** | Hot-Plug-Inrush weit ueber USB-Spec |
| P0-7 | Kein Eingangsstrom-Management (kein CC-Handling) | Schwache Quellen brechen ein |

## Entscheidung

**TI BQ24074RGTR** (Power-Path-Charger mit DPPM) als zentrales Power-Element:

```
USB-C → F1 (3A/6A) → VBUS_FUSED → U7 BQ24074 IN      [D2 SMAJ5.0A klemmt VBUS_FUSED]
                                    ├─ OUT = VSYS (4,4V @USB / VBAT @Akku)
                                    │      → U8 TPS61089 Boost → D3 → +5V-Rail (EINZIGE Quelle)
                                    └─ BAT ← F2 PTC 2,6A/5A ← J9 LiPo 2000mAh
SW_PWR: COM=PWR_ON (100k-PD), Throw-A=VSYS  →  PWR_ON schaltet U8.EN UND U_PWR.ON
```

- **D3B entfaellt**, das Dioden-OR ist weg; die +5V-Rail hat genau eine Quelle
  (Boost). D3 bleibt als Trennung Regelknoten↔Rail-Bulk (Kompensations-Analyse
  r18.80 bleibt unveraendert gueltig).
- **SW_PWR-Pull-Quelle = VSYS** (nicht mehr +5V-Rail): VSYS ist immer versorgt
  — sonst Henne-Ei (Rail existiert erst NACH dem Einschalten des Boost).
- **LED_CHRG**: VBUS_FUSED → LED → 1k → CHG (open-drain). Leuchtet nur bei
  USB-praesent UND ladend.
- **Bulk hinter dem Boost**: USB-Hot-Plug laedt nur noch C_CHG_IN (4,7 µF);
  Inrush zusaetzlich durch BQ-ILIM begrenzt (P0-6 geloest).

## Dimensionierung (alle Werte TI SLUS810N, Okt 2021)

| Element | Wert | Rechnung / DS-Quelle |
|---|---|---|
| R_ISET (Pin 16→GND) | 1 kΩ (C21190) | ICHG = KISET/R = 890/1000 = **0,89 A typ** (0,80–0,98) ≈ 0,45C — DS Gl. 2, Bereich 590 Ω–8,9 kΩ |
| R_ILIM_IN (Pin 12→GND) | 1,2 kΩ (C114605) | IIN-MAX = KILIM/R = 1610/1200 = **1,34 A typ** (1,25–1,43) — DS Gl. 1, Bereich 1,1–8 kΩ |
| EN2 / EN1 | VSYS / GND | ILIM-Widerstand-Modus (Table 7-2); EN-VIH 1,4–6 V — VSYS 3,0–4,4 V immer HIGH. Bewusst nicht an VBUS_FUSED (TVS-Klemme kann Abs-Max 7 V reissen) |
| ITERM (15) | **NC** | Default 10 % ≈ 89 mA (DS Table 7-1) |
| TMR (14) | **NC** | Default-Timer: tMAXCHG 18000 s = 5 h typ; Ladung ≈ 2,3 h CC + CV ≈ 3,2 h < 5 h |
| TS (1) | 10 kΩ fest → GND (C25804) | DS: exakte Vorgabe fuer Anwendungen ohne Pack-NTC |
| CE_N (4) | GND | Laden aktiv (intern 285k-PD, darf nicht floaten) |
| IN-Bypass | 4,7 µF (C46653) | DS: 1–10 µF |
| OUT/BAT-Bypass | je 22 µF (C45783) + 100 nF | DS: 4,7–47 µF |
| F2 | SMD1812P260TF/16 (C438899) | 2,6 A hold / 5 A trip / 16 V / 15 mΩ — Dauerentnahme ~2 A liegt unter hold, Zell-Short trippt |

**Aus-Zustand:** U8-Shutdown-Iq < 3 µA (SLVSD38C §8.3.2). Der TPS61089 hat
**kein Output-Disconnect** — die Rail liegt im Aus ueber Body-Diode + D3 auf
~VSYS−0,7 V. Alle Rail-Lasten sind dann hochohmig: PAM8403 via R_SHDN_PD in
Shutdown, LED-Anoden gegen unbestromten PCA9685 high-Z, 3V3-Domaene hinter
U_PWR getrennt. Rest-Drain im Aus: µA-Bereich (vorher: Boost regelte dauerhaft).

**P0-7 (CC-Management), bewusste Teilloesung:** kein USB-C-CC-Controller.
IIN 1,34 A setzt ein 5V/≥1,5A-Netzteil voraus (SPEC nennt 5V/3A). An schwachen
Quellen greift DPPM: Systemlast hat Prioritaet, Ladestrom wird gedrosselt,
notfalls supplementiert der Akku — genau das Design-Verhalten dieses Chips
("protection against poor USB sources", DS §1). Volle USB-C-Konformitaet
(CC-basierte Stromwahl) waere ein eigener Chip — fuer ein Geraet mit
beiliegendem Netzteil akzeptiert ausserhalb des Scopes.

## Verifikation (r19.18)

- **BQ24074RGTR C54313**: live geprueft — LCSC 1.074 Stk., $2,24@1/$1,19@1k,
  JLC Economic+Standard. Pin-Map aus TI SLUS810N Table 7-1 (Spalte '74).
- **Footprint**: KiCad `QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` gegen JLC/EasyEDA-
  Landpattern fuer C54313 geometrisch abgeglichen (Pitch 0,5, Pads 0,28×0,85,
  EP 1,70×1,70, Pin-1 links oben CCW — identisch). TI-Outline RGT0016C: EP
  1,68±0,07. Fuer die B-Variante (RGT0016B = '74) publiziert TI nur die
  Generic-View → Massabgleich am physischen Teil vor Fab = billiger Check.
- **F2 C438899**: live geprueft — 1.730 Stk., JLC Economic+Standard.
- **R_ILIM_IN C114605**: live geprueft — 407.400 Stk. (1,1k/1,2k UNI-ROYAL
  waren bei LCSC out-of-stock → YAGEO-Alternative).
- **Netzliste** (geometrischer Extractor, alle 7 Sheets): 157 Netze, 620
  Pin-Verbindungen, **0 floating Pins**. VBUS_FUSED/VSYS/PWR_ON/BAT_PLUS
  enthalten exakt die Soll-Mitglieder.
- `scripts/check_pinmap.py`: gruen (Generator-NETS ↔ PINMAP deckungsgleich).

## Konsequenzen

- JST-PH-Kontaktrating 2 A/Pin: Dauerentnahme durch F2 auf ≤2,6 A begrenzt,
  Transienten reiten die VSYS-/Boost-Caps. Restrisiko dokumentiert (SPEC §4).
- Zellen-Vorgabe verschaerft: **nur Pouches mit Schutz-PCB** (F2 ist Backup,
  kein Ersatz fuer Zellschutz: kein Overcharge-/Overdischarge-Schutz im PTC).
- J4 (SWD): DNP-Debug-Pads, Ex-MPN "TC2030-IDC" entfernt (war das
  Tag-Connect-Kabel, nicht der Header) — JLC-BOM enthaelt kein "TBD" mehr.
- J8 explizit **LINE OUT** (kein Kopfhoerer-Treiber) — Gehaeuse-Label.
- mechanical_coordinates.md: Batterie-Zeile auf 2000 mAh korrigiert
  (5000-mAh-Angabe war Pi-Aera-Leiche).
