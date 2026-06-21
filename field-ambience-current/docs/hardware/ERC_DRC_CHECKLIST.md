# ERC/DRC-Checkliste — Field Ambience

**Stand: r18.6.** Diese Checkliste sichert ab, dass der Schaltplan beim ersten Öffnen in KiCad 9 sauber durchläuft und das spätere PCB nach DRC fehlerfrei ist. Die Validierung im Repo (Python-Strukturchecks, Klammern-Balance, Pin-Anbindung, Crossref) ist eine Vorstufe — sie ersetzt **nicht** die KiCad-GUI-eigene ERC/DRC.

## ERC (Electrical Rule Check) — auszuführen in KiCad 9

Voraussetzungen vor dem ERC-Lauf:

1. **Projekt in KiCad 9 öffnen** (`field_ambience.kicad_pro`)
2. **Symbol Libraries → Refresh**: Symbol-Library-Cache aktualisieren
3. **Annotation prüfen**: Alle Symbole haben Reference (U1, R1, C1, ...) — der Generator vergibt sie deterministisch, sollten keine Fragezeichen entstehen
4. **Tools → Update Symbols from Library**: alle aus der KiCad-Standard-Lib gezogenen Symbole (R, C, L, LED, GND-Symbole) auf aktuelle Lib-Version bringen
5. **Inspect → Electrical Rules Checker → Run**

### Erwartete *bewusste* Warnungen (kein Fehler)

| Warnung | Stelle | Warum bewusst |
|---|---|---|
| Input Power pin not driven | `+5V_OUT`-Label-Brücken zwischen Power-Tree und Battery-Sheet | beide Sheets speisen aus derselben Quelle — KiCad sieht Net von zwei Seiten gleichzeitig getrieben |
| Pin not connected (PB3 SWO) | STM32-Sheet, PB3 | SWO ist optional, wird nur als Label geführt — explizit als no-connect zu markieren wäre falsch (es **kann** in Phase 5 verbunden werden) |
| Single label "MIDI_TX" | STM32-Sheet, PD5 | Netz wartet auf ADR-0004-Entscheidung (Buchse + R) |
| Pin not connected (X-NC_GPB7, NC_GPB8, NC_PCA_LED11/13/14/15) | mcp-Sheet | Reserve-Pins, bewusst nur Label gegen Dangling — `no_connect` wäre semantisch falsch (in Rev-B nutzbar) |

### Warnungen, die NICHT erlaubt sind

- "Hierarchical labels not connected to sheet pin" → Crossref-Fehler, sofort fixen
- "Conflict between hierarchical label and net" → Naming-Fehler, sofort fixen
- "Power input pin connected to power output pin without flag" → fehlendes Power-Symbol
- "Pins of two different power nets connected" → Spannungs-Misch (kritisch — kann Hardware zerstören)
- "No matching symbol/footprint" → fehlende Library

### Manufacturing-Gate (Schematic-Seite)

**Vor Phase-6-Layout-Start:** ERC im KiCad-9-GUI muss laufen mit
- **0 errors**
- **nur die oben aufgelisteten bewussten Warnungen** — jede andere Warnung muss adressiert oder als ADR begründet sein

## Footprint-Validierung — vor DRC

1. **Symbol → Footprint-Mapping**: jedes Symbol muss ein Footprint-Feld haben (Skript-validiert)
2. **Footprints in fp-lib-table verfügbar**:
   - `Switch_Keyboard_Hotswap_Kailh` (kiswitch v2.4, im Repo)
   - `Mounting_Keyboard_Stabilizer` (kiswitch v2.4, im Repo)
   - `field_ambience` (Repo-eigene Custom-Footprints, aktuell: SW_HX_12x12x7.3_SMD-4P)
   - alle übrigen: KiCad-Standard-Libraries (über das globale fp-lib-table-Setting des Anwenders)
3. **FP_VERIFY-Properties prüfen**: Symbole tragen explizite Hinweise, welche Footprints noch nicht gegen das offizielle Datenblatt verifiziert sind. Liste in `PCB_LAYOUT_STATUS.md`.

## DRC (Design Rule Check) — wenn `.kicad_pcb` existiert

Aktuell **nicht durchführbar — Layout existiert nicht** (PCB_LAYOUT_STATUS.md).

Wenn Layout startet:

1. **Board-Setup**: Stack-Up Sig/GND/+5V/Sig (SPEC §9), JLC-4-Lagen-Standard
2. **Design Rules** auf JLCPCB-Capabilities setzen (Track ≥ 0.127 mm, Spacing ≥ 0.127 mm, Via ≥ 0.3 mm bei 0.15 mm Hole, BGA-Pitch falls relevant)
3. **DRC → Run**

### Erwartete bewusste Warnungen (kein Fehler)

- *unbedrahtete Footprints* (vor finalem Routing) → klar
- *Footprint not in library* — sollte bei korrekt geführter fp-lib-table nicht auftreten

### Warnungen, die NICHT erlaubt sind

- *Track clearance / pad clearance / via clearance violations* → Routing-Fehler
- *Hole-to-hole / hole-to-pad clearance violations* → Mechanik-Fehler
- *Unconnected nets* → Vergessenes Routing
- *Footprint pad doesn't match net* → kann Bauteil zerstören
- *Silkscreen overlap with pad* → Bestückbarkeits-Risiko
- *Courtyard violations* → Pick-&-Place-Risiko

### Manufacturing-Gate (Layout-Seite)

**Vor Gerber-Export:** DRC mit JLC-4-Lagen-Profil muss laufen mit
- **0 errors**
- **0 warnings** für Clearance/Mechanik/Routing
- alle anderen Warnungen explizit als „acknowledged" markiert

## Automatisierbar (geplant, aktuell nicht verfügbar)

`kicad-cli sch erc` / `kicad-cli pcb drc` würden diese Checks headless in der CI fahren. Aktuell nicht in der Repo-CI verdrahtet — wenn die KiCad-CLI verfügbar wird:

```bash
kicad-cli sch erc field_ambience.kicad_sch --severity-error --exit-code-violations
kicad-cli pcb drc field_ambience.kicad_pcb  --severity-error --exit-code-violations
```

Bis dahin: GUI-Lauf bleibt der maßgebliche Schritt vor jedem Layout-Commit.
