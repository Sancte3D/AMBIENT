# ADR-0022: QSPI-PSRAM Speichererweiterung (8 MB, memory-mapped)

**Status:** IMPLEMENTED (2026-07-07, r19.10) — im Generator emittiert (U9 +
Decoupling + 6 QSPI-Nets an PB2/PC11/PE7–10), PINMAP/BOM nachgezogen. ERC in
der KiCad-GUI durch Aron ist der letzte Schritt (kicad-cli 7.0.11 in dieser
Umgebung kann den generierten Schaltplan nicht laden — siehe „Verifikation").
**Date:** 2026-07-07

## Kontext
Der interne RAM ist die Wand für „maximalen Sound" (siehe
`docs/hardware/RESOURCE_BUDGET.md`): RAM_D1 96 %, RAM_D2 96 %. Interner
Rückgewinn liegt bei ~250 KB. Für **Sample-Playback, Convolution-Reverb mit
echten Impulsantworten, großes Granular und einen echten Multi-Tisch-
Blendwave** reicht das nicht — dafür braucht es Megabyte, nicht Kilobyte.

Der H743 hat einen **QUADSPI**-Controller mit **Memory-mapped Modus**: ein
externer QSPI-Chip wird in den Adressraum eingeblendet, die CPU liest ihn
**wie internen RAM** (D-Cache davor). Der Vendor-HAL ist vorhanden (aktuell
`HAL_QSPI_MODULE_ENABLED` auskommentiert, r18.86).

**Board-Timing:** Das ist eine PCB-Änderung → muss **vor** Arons Layout rein,
nachträglich = Respin.

## Entscheidung
**Ein QSPI-PSRAM (8 MB) auf QUADSPI Bank 2 hinzufügen.**

### Teil (verifiziert)
| Feld | Wert |
|---|---|
| MPN | **APS6404L-3SQN-SN** (AP Memory) |
| LCSC | **C3028887** (r19.15: was C5333729/3SQR-SN — went OUT OF STOCK at LCSC/JLC; 3SQN-SN = gleiche 3V-Familie 2,7–3,6 V, gleicher SOP-8-Pinout, LCSC 432 Stk. + JLC Economic+Standard, live geprüft 2026-07-07) |
| JLC-Typ | bestückbar auf **Economic**-Linie (kein Standard-Zwang), MSL 3 |
| Preis / Stock | $4,52@1 / $3,09@100 — 432 Stk. LCSC, „ships now" (live geprüft 2026-07-07) |
| Kapazität | 64 Mbit = **8 MB** |
| Package | **SOP-8 (150 mil)** |
| Spannung | 2,7–3,6 V (→ an +3V3) |
| Takt | bis 133 MHz (QSPI), Quad-IO |

### Pin-Zuordnung (verifiziert gegen DS12110, alle 6 Pins FREI)
QUADSPI **Bank 2** — die freien PE7–PE10 sind die BK2-IO-Pins, kontinuierlich:

| Signal | MCU-Pin | QUADSPI-Funktion | frei? |
|---|---|---|---|
| `QSPI_CLK` | **PB2** | QUADSPI_CLK (AF9) | ✅ |
| `QSPI_NCS` | **PC11** (Pin 79) | QUADSPI_BK2_NCS (AF9) | ✅ |
| `QSPI_IO0` | **PE7** | QUADSPI_BK2_IO0 (AF10) | ✅ |
| `QSPI_IO1` | **PE8** | QUADSPI_BK2_IO1 (AF10) | ✅ |
| `QSPI_IO2` | **PE9** | QUADSPI_BK2_IO2 (AF10) | ✅ |
| `QSPI_IO3` | **PE10** | QUADSPI_BK2_IO3 (AF10) | ✅ |

> DS12110-bestätigt: PC11 = QUADSPI_BK2_NCS (LQFP100 Pin 79; Pin 80 = PC12),
> PE7 = QUADSPI_BK2_IO0,
> QUADSPI_CLK auf PB2. Die genauen **AF-Nummern** sind Firmware-Sache (GPIO
> AFR), nicht Schaltplan — beim QSPI-Init final gegen DS12110 setzen.

### Footprint / Symbol
**Pinout VERIFIZIERT** gegen das AP-Memory-Datenblatt *APM SPI 3V PSRAM
Rev. 2.1 (25 Oct 2019)*, §3.1 „Package Types SOP/USON, Top view":

```
/CE        1 ── 8  VDD
SO/SIO[1]  2 ── 7  SIO[3]
SIO[2]     3 ── 6  SCLK
VSS        4 ── 5  SI/SIO[0]
```

→ Symbol im Generator: 1=CE · 2=SIO1 · 3=SIO2 · 4=VSS · 5=SIO0 · 6=SCLK ·
7=SIO3 · 8=VDD — **1:1-Match** (Pin 8 im Datenblatt heißt VDD, nicht VCC —
r19.10 im Symbol nachgezogen).

**Footprint:** SOP-8L(150) = 3,9 mm Body / 1,27 mm Pitch = der KiCad-Standard
`Package_SO:SOIC-8_3.9x4.9mm_P1.27mm` (Standard-SOIC-Pad-Nummerierung 1–8,
deckt sich mit der Datenblatt-Top-View). Dimensionally korrekt.
**Belt-and-suspenders vor Fab** (Repo-Methode wie TS-1088/MST-12D18/Kailh):
`easyeda2kicad --full --lcsc_id=C3028887` → exaktes LCSC-Land-Pattern + 3D-STEP.

### Entkopplung
`100 nF` (0603) direkt an VDD/VSS + `10 µF` Bulk in der Nähe (PSRAM zieht bei
133-MHz-Burst kurzzeitig Strom). Beide an +3V3.

### Nets (AI_READY-konform, keine Slashes/Spaces, konsistent)
`QSPI_CLK`, `QSPI_NCS`, `QSPI_IO0`, `QSPI_IO1`, `QSPI_IO2`, `QSPI_IO3`.

## Was es ermöglicht
- **Sample-Playback** (Field-Recordings, One-Shots)
- **Convolution-Reverb** mit echten Raum-IRs (statt/neben dem Dattorro-Tank)
- **Großes Granular** (Clouds-Style, Sekunden-Puffer)
- **Echter Multi-Tisch-Blendwave** (die 64-KB-RAM-Grenze aus r19.5 fällt weg)

## Grenzen (ehrlich)
- **Read-mostly:** QSPI ist seriell → langsamer als interner SRAM. Ideal für
  große statische/lesende Daten hinter dem Cache. **Heiße Pro-Sample-Puffer
  (Reverb-Feedback jede Sample) bleiben intern** — PSRAM ist additiv, kein
  Ersatz für den schnellen RAM.
- **Cache-Kohärenz + MPU:** memory-mapped Region cacheable konfigurieren;
  DMA in die Region vermeiden. Standard-H7-Pfad, aber **auf echter Hardware
  validieren** (Host-Tests fangen das nicht).
- **BOM/Kosten:** +~$2,94 + 6 Pins + etwas Routing (QSPI kurz + grob
  längengematcht bei ~66–133 MHz).

## Firmware-Plan (später, nach Board)
1. `HAL_QSPI_MODULE_ENABLED` einschalten, QSPI-Init (BK2, IO-AF10).
2. PSRAM in Memory-mapped Modus (Read + Write) bringen.
3. MPU-Region für den QSPI-Adressraum (cacheable, normal memory).
4. Große Puffer (Sample/IR/Wavetable) dorthin, heiße Puffer intern lassen.

## Umgesetzt (r19.10)
Im Generator (`generate_kicad_project.py`, Sheet `stm32h743`) emittiert:
- **Symbol** `Memory_RAM:APS6404L` (SOP-8, 8 Pins, JEDEC-Serial-Pinout
  1=CE#·2=SIO1·3=SIO2·4=VSS·5=SIO0·6=SCLK·7=SIO3·8=VCC).
- **U9** APS6404L-3SQN-SN, FP `Package_SO:SOIC-8_3.9x4.9mm_P1.27mm`,
  LCSC C3028887, mit `FP_NOTE` = `UNVERIFIED — NEEDS HUMAN CHECK` (FP via
  `easyeda2kicad --full --lcsc_id=C3028887` ziehen + Pin-1 gegen AP-Memory-DB).
- **6 QSPI-Nets** an den MCU-Pins PB2(36)/PC11(79)/PE7–10(37–40) →
  `QSPI_CLK`/`QSPI_NCS`/`QSPI_IO0–3` (per Local-Label, beidseitig).
- **Decoupling** `C_QSPI` 100 nF (C14663) + `C_QSPI2` 10 µF (C15850), beide an
  +3V3/GND; U9 VCC→+3V3, VSS→GND.
- **PINMAP** (Pins nicht mehr frei), **BOM** (U9 + 2 Cs) nachgezogen.

## Verifikation
`kicad-cli` 7.0.11 in dieser Umgebung kann den generierten Schaltplan **nicht
laden** (`Failed to load schematic file` — auch am unveränderten Baseline, per
`git stash` bestätigt; Parser-Inkompatibilität, nicht durch diese Änderung
verursacht) und hat kein `sch erc`-Subkommando. Daher **Text-Level-Check** des
emittierten `stm32h743.kicad_sch` statt CLI-ERC:
- jedes der 6 QSPI-Netze erscheint genau **2×** (MCU-Stub + U9-Pin),
- jeder U9-Signalpin ist per Wire an das passende Label geführt
  (CE→NCS, SCLK→CLK, SIO0–3→IO0–3), VCC→+3V3, VSS→GND,
- U9/C_QSPI/C_QSPI2 + Symbole emittieren mit voller Metadata.

**Offen für Aron:** finaler **ERC in der KiCad-GUI** (v7+) + verifizierten
Footprint ziehen, bevor Fab. **Erst dann** ist es sicher auf dem Board.
