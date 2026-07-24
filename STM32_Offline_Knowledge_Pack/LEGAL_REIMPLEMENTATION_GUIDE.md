# Legale Ersatzimplementierung

## Ziel

Dieser Ablauf minimiert das Risiko, dass GPL-, LGPL-, SLA- oder nicht
lizenzierter Code unbemerkt in proprietäre Firmware gelangt.

## Strenger Clean-Room-Ablauf

1. **Quarantäne:** Restriktiven Code in einem klar markierten Referenzordner
   halten. Nicht in das Produktrepository kopieren.
2. **Anforderungsquelle:** Verhalten aus Datenblatt, Standard, Messungen,
   Benutzeranforderungen oder frei nutzbarer Fachliteratur beschreiben.
3. **Neutrale Spezifikation:** Ein Dokument mit Inputs, Outputs,
   Zustandsautomaten, Timing, Fehlerfällen und Testvektoren schreiben. Keine
   Quelltextstruktur, Namen oder Kommentare des Originals übernehmen.
4. **Unabhängige Umsetzung:** Idealerweise implementiert eine Person, die den
   restriktiven Code nicht gesehen hat. Bei einer KI darf der restriktive Code
   nicht im Kontext liegen.
5. **Provenienzprotokoll:** Für jede neue Datei Quellen, Datum und Autor
   dokumentieren.
6. **Vergleich über Verhalten:** Nur Ein-/Ausgabe, Performance und
   Protokollkonformität vergleichen. Keine zeilenweise Ähnlichkeitsoptimierung.
7. **Lizenzscan:** Vor Veröffentlichung nach fremden Copyrightzeilen,
   SPDX-Kennungen und auffälligen Textübereinstimmungen suchen.
8. **Releaseprüfung:** Alle Third-Party-Notices erzeugen und den tatsächlich
   gelinkten Quellbaum prüfen.

## Konkrete Ersatzwege

### GPL-ST7789-Treiber

- Nicht aus dem GPL-Quelltext abschreiben.
- ST7789-Datenblatt und Displaymodul-Dokumentation als Quelle verwenden.
- Kleine eigene Schicht definieren:
  - `write_command(uint8_t)`
  - `write_data(const uint8_t *, size_t)`
  - `set_window(x0, y0, x1, y1)`
  - `begin_pixels()`
  - asynchroner SPI-DMA-Transfer mit Completion-Callback
- Panel-Offsets und Initsequenz am konkreten Display messen und dokumentieren.
- Eigene Tests für Rotation, Fenstergrenzen, Byteorder und DMA-Busy-State
  schreiben.

### LGPL-DaisySP-Module

Nicht aus `DaisySP-LGPL` verwenden, wenn keine LGPL-Releasepflichten gewünscht
sind. Mögliche MIT-Alternativen im Hauptordner:

- `Filters/ladder.*` statt `moogladder.*`;
- `Filters/svf.*` oder `Filters/onepole.h` statt LGPL-Filter;
- `Effects/wavefolder.*` statt `fold.*`;
- `PhysicalModeling/KarplusString.*` statt `pluck.*`;
- eigener Reverb aus `clean_room`.

### Nicht lizenzierte Audio-Tutorials

- I²S/SAI und DMA aus dem BSD-lizenzierten STM32H743 Basic Example ableiten.
- FIR/IIR/FFT über Apache-2.0-CMSIS-DSP implementieren.
- Reverb anhand eines allgemein beschriebenen parallelen
  Comb-/seriellen-Allpass-Netzes neu schreiben.
- Video nur als Lernhinweis ansehen, nicht als Quelltextquelle.

### SLA0044-Komponenten

- Wenn möglich BSD-3-Clause Basic Examples, HAL und BSP verwenden.
- ST-USB-, STemWin-, TouchGFX- oder andere SLA-Komponenten nur gezielt und
  ausschließlich unter ihren Bedingungen verwenden.
- SLA-Code nicht in einen GPL/MIT/Apache-lizenzierten abgeleiteten Bestandteil
  umdeklarieren.
- Für ein vollständig permissives Produkt entsprechende Komponente durch eine
  permissive Alternative oder eigene Implementierung ersetzen.

## Was nicht funktioniert

- Copyrightheader löschen.
- Code von einer KI „anders formulieren“ lassen.
- Variablennamen, Reihenfolge oder Formatierung ändern.
- Nur wenige Zeilen verändern.
- GPL-Code statisch einbauen und lediglich den eigenen Ordner geschlossen
  halten.
- Fehlende Lizenz als „Public Domain“ interpretieren.

Vor Serienverkauf sollte die konkrete Releasezusammenstellung von einer auf
Softwarelizenzen spezialisierten juristischen Person geprüft werden.
