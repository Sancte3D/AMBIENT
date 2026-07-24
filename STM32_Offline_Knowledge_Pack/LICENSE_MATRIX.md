# Lizenzmatrix

## Kurzfassung für ein später verkauftes, geschlossenes Gerät

Am einfachsten bleibt die Firmware rechtlich sauber, wenn sie auf diesen
Beständen basiert:

- STM32 HAL, CMSIS Device, BSP und Basic Examples unter BSD-3-Clause;
- ARM CMSIS-DSP unter Apache-2.0;
- libDaisy, DaisyExamples und der MIT-Teil von DaisySP;
- eigene Dateien unter MIT-0.

Nicht in die proprietäre Firmware kopieren:

- `ST7789-STM32-GPL`;
- `DaisySP-LGPL`;
- AVR-Code aus Mutable Instruments;
- Code aus den nur referenzierten Repositories ohne Lizenz.

## Matrix

| Bestand | Lizenz | Exakter Lizenzort | Was bei Verteilung nötig ist | Legale Route ohne Copyleft/Sonderbindung |
| --- | --- | --- | --- | --- |
| STM32CubeH7 HAL | BSD-3-Clause | `upstream_sources/STM32CubeH7/Drivers/STM32H7xx_HAL_Driver/LICENSE.md` | Copyright, Bedingungen und Disclaimer erhalten; kein Endorsement | Direkt verwendbar, Hinweise mit Firmwaredokumentation ausliefern |
| STM32CubeH7 Basic Examples | BSD-3-Clause | z. B. `upstream_sources/STM32CubeH7/Projects/STM32H743I-EVAL/Examples/LICENSE.md` | Wie BSD-3-Clause | Als Startpunkt für eigene Board-Abstraktion verwenden |
| STM32CubeH7 Applications | SLA0044 | z. B. `upstream_sources/STM32CubeH7/Projects/STM32H743I-EVAL/Applications/LICENSE.md` | Hinweise erhalten; nur auf/in Verbindung mit ST-Prozessoren; nicht Open-Source-Bedingungen unterwerfen | Möglichst Basic Examples und eigene Implementierung statt Application-Code verwenden |
| ST USB Device/Host | SLA0044 | jeweiliges `LICENSE.md` unter `Middlewares/ST` | ST-only und weitere SLA0044-Bedingungen | Für eine rein permissive Codebasis nicht einbinden; alternativ permissiven USB-Stack wählen |
| STM32CubeH7 Gesamtpaket | Gemischt | `upstream_sources/STM32CubeH7/LICENSE.md` sowie `LICENSE_FILE_INDEX.txt` | Lizenz der konkreten Komponente beachten | Nur benötigte BSD/Apache/MIT-Komponenten übernehmen |
| CMSIS-DSP | Apache-2.0 | `upstream_sources/CMSIS-DSP/LICENSE` | Lizenz beilegen, Hinweise erhalten, Änderungen kennzeichnen | Direkt verwendbar |
| libDaisy | MIT | `upstream_sources/libDaisy/LICENSE` | Copyright- und Lizenztext erhalten | Direkt verwendbar; Submodule separat prüfen |
| libDaisy USB-Middleware | SLA0044 | `upstream_sources/libDaisy/Middlewares/ST/STM32_USB_Device_Library/LICENSE.md` | ST-only und SLA0044 beachten | USB-Middleware nicht bauen oder permissiv ersetzen |
| DaisyExamples | MIT | `upstream_sources/DaisyExamples/LICENSE` | Copyright- und Lizenztext erhalten | Direkt als Lern- und Portierungsbasis verwendbar |
| DaisySP Hauptbibliothek | MIT | `upstream_sources/DaisySP/LICENSE` | Copyright- und Lizenztext erhalten | Nur Dateien außerhalb `DaisySP-LGPL` verwenden |
| DaisySP-LGPL | LGPL-2.1 | `upstream_sources/DaisySP/DaisySP-LGPL/LICENSE` sowie die zweite Kopie in DaisyExamples | Bei Verteilung u. a. Bibliotheksquelltext und Relink-Möglichkeit sicherstellen; statisches Embedded-Linking ist besonders zu prüfen | Submodul vollständig aus dem Build entfernen; MIT-Module oder eigene Implementierung verwenden |
| Mutable STM32F-Code | MIT | Lizenzabschnitt in `upstream_sources/mutable-instruments-eurorack/README.md`; nähere Hinweise in Submodulen | MIT-Hinweise erhalten; Mutable-Instruments-Marke nicht als Produktname verwenden | Nur eindeutig als STM32F/MIT ausgewiesene Firmwareteile verwenden |
| Mutable AVR-Code | GPL-3.0 | derselbe README-Lizenzabschnitt und jeweilige Submodule | Bei abgeleiteter Verteilung GPL-Quellpflichten erfüllen | Für STM32H7 nicht übernehmen |
| Mutable Hardware | CC-BY-SA-3.0 | README-Lizenzabschnitt | Attribution und Share-Alike bei Adaptionen | Eigenes PCB aus Datenblättern und Anforderungen entwerfen |
| ST7789-STM32 | GPL-3.0 | `upstream_sources/ST7789-STM32-GPL/LICENSE` | Bei abgeleiteter/verlinkter Verteilung vollständigen korrespondierenden Quelltext unter GPL bereitstellen | Nicht kopieren; eigenen Treiber anhand des ST7789-Datenblatts und der HAL-API schreiben |
| Eigene Clean-Room-Dateien | MIT-0 | `clean_room/LICENSE` | Keine Namensnennungs- oder Copyleftpflicht | Direkt verwenden und verändern |
| Controllerstech STM32-HAL | Keine allgemeine Lizenzdatei; README gestattet Lernen/private Anpassung | Nur Upstream-README | Keine sichere kommerzielle oder Weiterverteilungsfreigabe ableitbar | Offizielle STM32Cube-Beispiele verwenden oder Autor um Lizenz bitten |
| YetAnother DSP-Repositories | Keine Lizenz gefunden | Upstream | Standardmäßig urheberrechtlich geschützt | Algorithmus aus Fachliteratur/Spezifikation unabhängig neu implementieren |
| stm32-monosynth | Keine Root-Lizenz gefunden; enthält weitere Abhängigkeiten | Upstream | Keine sichere Weiterverteilungsfreigabe ableitbar | Architektur selbst mit MIT/BSD/Apache-Bausteinen nachbauen |

## Was „legal umgehen“ tatsächlich bedeutet

Eine Lizenz darf nicht durch Entfernen von Headern, Umbenennen von Variablen,
automatisches Umschreiben oder kleine Änderungen umgangen werden. Rechtssichere
Wege sind:

1. Lizenz vollständig erfüllen.
2. Eine andere, kompatibel lizenzierte Implementierung wählen.
3. Beim Rechteinhaber eine kommerzielle Sonderlizenz erhalten.
4. Die Funktion unabhängig aus Datenblatt, Standard oder wissenschaftlicher
   Beschreibung neu implementieren.
5. Einen echten Clean-Room-Prozess einsetzen.

Das bloße Ausführen von Code durch eine KI macht die resultierende Datei nicht
automatisch unabhängig oder lizenzfrei.
