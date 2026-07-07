# ADR-0022: QSPI-PSRAM Speichererweiterung (8 MB, memory-mapped)

**Status:** PROPOSED (2026-07-07) — Entscheidung + verifizierte Integration
gelockt; Generator-Emit + ERC ist der Folge-Schritt.
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
| MPN | **APS6404L-3SQR-SN** (AP Memory) |
| LCSC | **C5333729** |
| JLC-Typ | bestückbar auf **Economic**-Linie (kein Standard-Zwang), MSL 3 |
| Preis / Stock | ~$2,94, auf Lager (LCSC, 2026-07) |
| Kapazität | 64 Mbit = **8 MB** |
| Package | **SOP-8 (150 mil)** |
| Spannung | 2,7–3,6 V (→ an +3V3) |
| Takt | bis 133 MHz (QSPI), Quad-IO |

### Pin-Zuordnung (verifiziert gegen DS12110, alle 6 Pins FREI)
QUADSPI **Bank 2** — die freien PE7–PE10 sind die BK2-IO-Pins, kontinuierlich:

| Signal | MCU-Pin | QUADSPI-Funktion | frei? |
|---|---|---|---|
| `QSPI_CLK` | **PB2** | QUADSPI_CLK (AF9) | ✅ |
| `QSPI_NCS` | **PC11** (Pin 80) | QUADSPI_BK2_NCS (AF9) | ✅ |
| `QSPI_IO0` | **PE7** | QUADSPI_BK2_IO0 (AF10) | ✅ |
| `QSPI_IO1` | **PE8** | QUADSPI_BK2_IO1 (AF10) | ✅ |
| `QSPI_IO2` | **PE9** | QUADSPI_BK2_IO2 (AF10) | ✅ |
| `QSPI_IO3` | **PE10** | QUADSPI_BK2_IO3 (AF10) | ✅ |

> DS12110-bestätigt: PC11 = QUADSPI_BK2_NCS (Pin 80), PE7 = QUADSPI_BK2_IO0,
> QUADSPI_CLK auf PB2. Die genauen **AF-Nummern** sind Firmware-Sache (GPIO
> AFR), nicht Schaltplan — beim QSPI-Init final gegen DS12110 setzen.

### Footprint / Symbol
Wie bei allen Custom-Parts im Repo (TS-1088, MST-12D18, Kailh):
**`easyeda2kicad --full --lcsc_id=C5333729`** ziehen → verifizierter Symbol +
Footprint + 3D-STEP direkt von LCSC/EasyEDA. Alternativ Standard
`Package_SO:SOIC-8_3.9x4.9mm_P1.27mm`.
`UNVERIFIED — NEEDS HUMAN CHECK`: exakten SOP-8-Pinout (Pin 1 CE#, JEDEC-
Serial-Memory-Standard: 1=CE#, 2=SIO1, 3=SIO2, 4=VSS, 5=SIO0, 6=SCLK,
7=SIO3, 8=VCC) + Pin-1-Orientierung beim Pull gegen das AP-Memory-Datenblatt
bestätigen, bevor Fab.

### Entkopplung
`100 nF` (0603) direkt an VCC/VSS + `10 µF` Bulk in der Nähe (PSRAM zieht bei
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

## Nächster Schritt
Generator-Emit (`generate_kicad_project.py`): PSRAM-Symbol (SOP-8) auf der
MCU-Seite (oder eigenem `memory`-Sheet) + Decoupling + die 6 QSPI-Labels an
PB2/PC11/PE7–10, dann `kicad-cli erc` = 0 neue Fehler. PINMAP + BOM +
`RESOURCE_BUDGET` nachziehen. **Erst dann** ist es auf dem Board, das Aron
routet.
