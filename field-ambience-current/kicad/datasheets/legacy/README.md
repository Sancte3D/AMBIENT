# datasheets/legacy/ — veraltete / falsche Datenblätter

**Archiviert 2026-06-11 (r18.5).** Nicht für Verifikation verwenden:

| Datei | Problem |
|---|---|
| `PEC11R-4215F-S0024.pdf` | **F-2:** Bourns PEC11R ist das FALSCHE Teil — SPEC §4 fordert ALPSALPINE **EC11J1525402**. ALPS-Drawing ist noch zu beschaffen. |
| `SC1631_(Pico_2_module).pdf` | Pico-2-Modul — seit r18 (STM32H743-Migration) nicht mehr im Design. |

**Bekanntes offenes Problem (F-1):** `../TPS61089RNSR.pdf` ist die
RNSR-Variante; verbaut ist laut Schematic/BOM **TPS61089RNR**. Das
RNR-Datenblatt (ti.com TPS61089, Package RNR) ist zu beschaffen und die
Pin-Belegung im battery-Sheet dagegen zu prüfen, BEVOR Layout startet.
