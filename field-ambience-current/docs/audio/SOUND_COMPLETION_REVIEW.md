# AMBIENT — vollständiger Abgleich der 54 Soundaufgaben

Stand28.10.2026. Alle54 Punkte der kanonischen Projekt-To-do (Version50) gelesen,
mit aktuellem Code, Nachweisen und PR192-CI766 abgeglichen. Diese CI ist für
ddb3cef2bb9af29027531f80140f3705a10b75c1 vollständig30/30 erfolgreich.
Neue Änderungen werden separat im neuen PR geprüft; kein Parentpass als
aktueller ARM-Nachweis ausgegeben.

## In diesem Durchlauf konkret abgeschlossen

- Aktueller0.11-Vertrag ersetzt missverständliche0.10-Angaben, Originalvertrag
  bytegleich archiviert. Alle10 Effektziele direkt aus C ausgelesen,278 eigene
  Runtime-/Header-/HAL-/Startup-/Linker-/CMakeinputs versioniert und prüfbar.
- Fehlende Stack-/Audiofehlerbeobachtung als eigener expliziter Messbuild:
  geschützter Startup-MSP-Paint, SAI-/DMA-Fehlercodes, Savezähler und lesender
  Exporter. Speichergrenzen, echte HAL-Fehlerroute, Capture-/GDB-Rejekte und
  Bench-ABI nativ/sanitisiert geprüft. Kein Einfluss auf Soundziele.
  Zusätzlich realer SCN8-/H743-Adapter im Messmodus:1104 Stromabbrüche,
  Retry/Cancel und neun ganze12-s-Engine-Captures je nativ/sanitisiert;
  Savebeobachtungen, PCM und Planertraces37/512 stimmen vollständig.
- Neues Materialmodell2-Blindpaket,18 echte Cellfälle (drei Worlds, drei
  Register, trocken/FX), vollständige18-s-Ausgaben und Hz-/Ownertraces37/512
  gleich. Pro Hörfile112s, feste Abgleiche, Auflösung separat. Keine neue
  Klangänderung oder behauptete Blindabnahme.
- Entwurfs-Ausgangskette aus BOM_CURRENT/PCB_BOM geklärt:
  PCM5102A, PAM8406/Lautsprecher, TPA6132A2/J8 Phones/Line; J10=MIDI.
  Echte Bestückung/Lasten und physische Eigenschaften sind am Muster zu prüfen.

## Verbleibende konkrete Abnahmen

| Zusammengehöriger Schritt | Offene IDs | Tatsächlicher Bedarf |
|---|---|---|
| Klang-/Endwertwahl |24 Hör-/Freeze-Punkte | V2-Materialien, Grammatiken, Nature, Room und FX-Enden tatsächlich beurteilen; konkrete Zeitpunkte/Kritik, keine automatische Freigabe aus Messwerten |
| Speicher am H743 |SD41 | ECC, Stromunterbrechung/Retry/Rollover und Audio während Flashoperationen mit dem echten Board |
| Echtzeit/Stack/Ausgabe |SD47/48/49 | Volle IRQ-/Dispatchlatenz, echte Stackreserve, Nullfehler sowie Clock/DC/Noise/Pops/Lasten am verbauten Ausgang |
| Abschließender Lock |SD53, abhängig auch SD51 | Erst alle zutreffenden belegten Entscheidungen freigeben |

Encoderbelegung bleibt nach Michalis' Vorgabe zuletzt. Die ursprüngliche
Zählung25 geschlossen/29 offen bleibt: geschlossene alte Remove-/Technikkriterien
sind keine Hörabnahme der neu beauftragten Effektpalette. Neue Effekte haben
ihren expliziten0.11-Vertrag und offene Parameter-/Hörwahl. Keine neue World,
kein stiller Default, kein Merge/Retarget/Flashen und keine erfundene Geräteprobe.
Gerät und echte Hörbefunde sind keine durch weiteren Hostcode ersetzbaren Gates.

## Sämtliche ursprünglichen Aufgaben

| ID | Aufgabe | Aktueller Gate |
|---|---|---|
| SD00 | Erreichbare Soundpfade abschließend klassifizieren | Softwarekriterium geschlossen |
| SD01 | Ressourcenbaseline als fortlaufenden Vertrag führen | Softwarekriterium geschlossen |
| SD02 | COAST-Tonkörper trocken auswählen | Hör-/Endwertentscheidung offen |
| SD03 | COAST-Bewegung von der Quelle aus ordnen | Hör-/Endwertentscheidung offen |
| SD04 | HIGHLANDS-Tonkörper behalten oder verwerfen | Hör-/Endwertentscheidung offen |
| SD05 | HIGHLANDS-Artikulation und Atembewegung begrenzen | Hör-/Endwertentscheidung offen |
| SD06 | WOODLAND-Klang und musikalischen Decay fertig auswählen | Hör-/Endwertentscheidung offen |
| SD07 | Gemeinsamen Quellen-/Inputvertrag schließen | Softwarekriterium geschlossen |
| SD08 | Besitzer und sämtliche One-shot-Tails vollständig verfolgen | Softwarekriterium geschlossen |
| SD09 | Globale Belegung auch manuell verbindlich machen | Softwarekriterium geschlossen |
| SD10 | Manuelle Palette auf World-Quellen vereinheitlichen | Softwarekriterium geschlossen |
| SD11 | Gemeinsame Harmonik und Quellenregister final konstruieren | Hör-/Endwertentscheidung offen |
| SD12 | Harmoniegedächtnis mit realen Releases/Effekttails verbinden | Hör-/Endwertentscheidung offen |
| SD13 | World-Datenmodell und begrenzte Ereigniszustände bauen | Softwarekriterium geschlossen |
| SD14 | COAST-Grammatik implementieren | Hör-/Endwertentscheidung offen |
| SD15 | WOODLAND-Grammatik implementieren | Hör-/Endwertentscheidung offen |
| SD16 | HIGHLANDS-Grammatik implementieren | Hör-/Endwertentscheidung offen |
| SD17 | Langfristige Entwicklung mit Wiedererkennbarkeit bauen | Hör-/Endwertentscheidung offen |
| SD18 | Body mit/ohne entscheiden und Übergänge korrigieren | Softwarekriterium geschlossen |
| SD19 | Einen gemeinsamen Raum als Produktbasis wählen | Hör-/Endwertentscheidung offen |
| SD20 | Dry/Send/Room und Natur voneinander entkoppeln | Softwarekriterium geschlossen |
| SD21 | Echo als räumliche Antwort prüfen oder entfernen | Softwarekriterium geschlossen |
| SD22 | Chorus/Detune gegen Quellenbewegung abwägen | Softwarekriterium geschlossen |
| SD23 | Age auf sinnvolle Klangalterung reduzieren | Softwarekriterium geschlossen |
| SD24 | Blur auf Grundton und musikalischen Nutzen prüfen | Softwarekriterium geschlossen |
| SD25 | Shimmer behalten oder vollständig aus Produkt entfernen | Softwarekriterium geschlossen |
| SD26 | Reverse Swell strikt auf Produktnutzen begrenzen | Softwarekriterium geschlossen |
| SD27 | Finale Effektpalette reduzieren und Moduswechsel prüfen | Softwarekriterium geschlossen |
| SD28 | Naturebene rechtfertigen und separat steuern | Hör-/Endwertentscheidung offen |
| SD29 | Wind nach vorhandenem Irregularitätsumbau hören | Hör-/Endwertentscheidung offen |
| SD30 | Regen und Wellen auf Ereignisse statt Looptrim prüfen | Hör-/Endwertentscheidung offen |
| SD31 | Alle übrigen Geräuschwege schließen | Hör-/Endwertentscheidung offen |
| SD32 | Vollständiges versioniertes Parameterregister schreiben | Softwarekriterium geschlossen |
| SD33 | Activity/Color/Room als musikalische Bereiche konstruieren | Hör-/Endwertentscheidung offen |
| SD34 | Quellen-/Rollenpegel fest kalibrieren | Hör-/Endwertentscheidung offen |
| SD35 | Kritische Parameterkombinationen prüfen | Hör-/Endwertentscheidung offen |
| SD36 | Katalog und Scenes ausdrücklich migrieren | Softwarekriterium geschlossen |
| SD37 | Sämtliche Worldwechsel musikalisch schließen | Hör-/Endwertentscheidung offen |
| SD38 | Autonomen Betrieb als Audiobefehl fertig definieren | Softwarekriterium geschlossen |
| SD39 | Stillwerden/Clear/Mute für die gesamte Kette bauen | Softwarekriterium geschlossen |
| SD40 | Live-Key/Mode/Tuning und Besitzerwechsel prüfen | Softwarekriterium geschlossen |
| SD41 | Boot/Recall/Seed ohne ungewollte Ausgabe schließen | Gerätenachweis offen |
| SD42 | Ganze Ausgabekette gegen harte/hohle Ergebnisse absichern | Softwarekriterium geschlossen |
| SD43 | Hör-/Renderverfahren für die ganze Kette festlegen | Softwarekriterium geschlossen |
| SD44 | World-Unterschied ohne Labels prüfen | Hör-/Endwertentscheidung offen |
| SD45 | Lange reale Audioläufe und Recovery prüfen | Hör-/Endwertentscheidung offen |
| SD46 | Entfernte Produktpfade tatsächlich auslinken | Softwarekriterium geschlossen |
| SD47 | Hotpath und Note-on-Arbeit auf Spitzenlast prüfen | Gerätenachweis offen |
| SD48 | Echte H743-Zeit-/Stackabnahme durchführen | Gerätenachweis offen |
| SD49 | Reale Ausgangskette und Hardwarezustände abnehmen | Gerätenachweis offen |
| SD50 | Ruhige Wahrnehmung mit echten Hörern abnehmen | Hör-/Endwertentscheidung offen |
| SD51 | Sound-Spezifikation mit exakten Endwerten einfrieren | Hör-/Endwertentscheidung offen |
| SD52 | Bestehende UI auf unveränderte Audioverträge anbinden | Softwarekriterium geschlossen |
| SD53 | Sound-Definition-of-Done abschließen | Gesamtfreigabe offen |

Maschinenlesbarer Abgleich: [SOUND_COMPLETION_STATUS.json](SOUND_COMPLETION_STATUS.json).
Aktuelle Software: [Produktvertrag](PRODUCT_SOUND_SPEC.md),
[Input-/Parameterregister](SOUND_CANDIDATE_INPUTS.json),
[Messanleitung](../../firmware-c-next/tools/DEVICE_CAPTURE_README.md).
