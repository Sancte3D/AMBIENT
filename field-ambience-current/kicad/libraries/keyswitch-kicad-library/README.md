# Vendored: kiswitch keyswitch-kicad-library v2.4

Diese Footprints stammen aus der **kiswitch keyswitch-kicad-library**, Release
**v2.4**, und sind hier ins Projekt eingebunden (vendored), damit die
Choc-V2-Hotswap-Footprints ohne separaten PCM-Install auflösen.

- Quelle: https://github.com/kiswitch/keyswitch-kicad-library (Tag `v2.4`)
- Lizenz: CC-BY-SA bzw. MIT (siehe `LICENSE-CC-BY-SA` / `LICENSE-MIT`)

## Eingebundene Bibliotheken

| Ordner | Nickname (fp-lib-table) | Inhalt |
|---|---|---|
| `Switch_Keyboard_Hotswap_Kailh.pretty` | `Switch_Keyboard_Hotswap_Kailh` | Kailh Hotswap-Sockets inkl. `SW_Hotswap_Kailh_Choc_V1V2_1.00u` / `_2.00u` |
| `Mounting_Keyboard_Stabilizer.pretty` | `Mounting_Keyboard_Stabilizer` | Choc-/MX-PCB-Stabilizer (für die 2u-Cells im Layout) |

Registriert in `../../fp-lib-table` (projekteigene Footprint-Bibliothekstabelle).
KiCad merged diese mit der globalen Tabelle — Standard-Libs (Device, Package_SO,
Connector_Audio …) kommen weiterhin aus der KiCad-Installation.

Nur die für dieses Projekt benötigten `.pretty`-Ordner sind eingebunden, nicht
die komplette Library (kein MX/Alps/Hybrid, keine 3D-Modelle).
