# AMBIENT — Produktreview, Umsetzung in abgeschlossenen Paketen

Stand: 2026-09-17. Ausgangspunkt: PR #129, Commit fa92c623.
Maßstab: eine nachvollziehbare musikalische Handlung verbindet Spiel,
Klangwelt und Rückmeldung. Technisch fehlerfrei ist eine Voraussetzung;
außergewöhnliche Klang- und Produktqualität erfordert zusätzlich Hör- und
Nutzungstests. Keine Freigabe aufgrund eines Namens oder Testzählers.

## KERNURTEIL

Die eigenständig lebende und zugleich spielbare Klangwelt ist die stärkste
Produktidee. Die bisherige Aufteilung in Instrument-, Generator-, Akkord-
und Ebenenbedienung widerspricht ihr. Wir bearbeiten zuerst diese Brüche,
bevor wir weitere Synths, Effekte oder Bedienebenen ausbauen.

## FUNDAMENTAL FALSCH

1. Generate ersetzte die Bedeutung der Spielflächen durch Composer-Befehle.
   In Character-Modi traf das auf einen gesperrten Generator. Paket 1 entfernt
   diese automatische Umdeutung; der Generator selbst bleibt in Character
   vorerst gesperrt. Das ist keine bereits umgesetzte Bed/Core-Koexistenz.
2. Character ersetzt weiterhin den Ambient-Mix. Die Sprache verspricht
   eine Färbung derselben Welt, die Architektur schaltet Instrumente um.
3. Eine Taste loszulassen durfte beim Generate-Wechsel verloren gehen.
   Auch der Empfänger eines Release wurde zuvor aus dem aktuellen statt
   dem ursprünglichen Spielmodus abgeleitet. Paket 1 korrigiert beides.

## NOCH NICHT SELBSTVERSTÄNDLICH

Voice und Character wählen verwandte Rollen über verschiedene Architekturen.
Note/Harmony/Land geben denselben Flächen unterschiedliche Bedeutungen.
Dream bearbeitet auch den direkten Klang mit Tape/Chorus/Blur. Keine dieser
Entscheidungen ist allein durch ihre Implementierung gerechtfertigt.

Ein zusätzlicher belegter Rahmen: Der aktuelle H743-Eingabepfad liest fünf
digitale Schalter und übergibt `CELL_TAP_AMP`. Die Spielflächen liefern dort
keine gemessene Anschlagsdynamik oder Drucktiefe. DSP-Velocity-Tests sind kein
Nachweis drucksensitiver Hardware. Der erste Entwurf muss mit Dauer, Pausen,
Wiederholung und den vorhandenen Encodern musikalisch funktionieren.

Aktuelle CAD-/PCB-Revision, Displaybilder und Haptik wurden in diesem Paket
nicht geprüft. Deshalb keine Freigabe von Abständen, Beschriftung, Druckgefühl,
Displayhierarchie oder mechanischer Machbarkeit.

## LOCKED

Als Produktprinzipien: gemeinsamer harmonischer Kontext; ein gemeinsamer
Effektraum mit individuellen Sends; musikalisch gewollte Pausen; eindeutige
Zuordnung von Anschlag und Loslassen. Die konkreten Klangfarben, Reglerbereiche
und physischen Ausführungen bleiben prüfpflichtig.

## REMOVE / MERGE / REDESIGN

| Paket | Entscheidung und Nutzen | Beleg / Abschlussbedingung | Stand |
|---|---|---|---|
| 1: Spielbarkeit | Generate lässt die gewählte Spielweise unverändert. Release gehört zum Anschlag. Modusausstieg räumt Noten-Latches auf, ohne globale Modifier zu löschen. | Original-HAL-Routing gegen vorher/nachher prüfen; Control-State-Tests; volle Host-Suite. | Implementiert; Prüfbericht unten. |
| 2: Gemeinsame Welt | Ein ausgewählter Core ersetzt die vorhandene spielbare Vordergrundrolle; das Bett bleibt unabhängig davon bestehen. | Quellen/Owner, Generator-Melodie, gehaltene Töne, Ausklänge und Sends explizit zuordnen. Keine zusätzliche komplette Engine. Speicherkarte und Spitzenlast einschließlich Übergang prüfen. | Offen; nächster Schritt. |
| 3: Auswahl reduzieren | Voice/Character zu einer verständlichen Auswahl zusammenführen; Note/Harmony/Land gegen eine einzige Standardspielweise vergleichen. | Nutzer kann Wirkung ohne Moduswissen vorhersagen. Jede verbleibende Alternative muss eine eigenständige musikalische Aufgabe lösen. Szenen-IDs/Migration separat behandeln. | Vorschlag, nicht umgesetzt. |
| 4: Klangrollen | Jeden Core und jede Ambient-Stimme trocken und in derselben Welt bewerten; Doppelungen streichen. | Register, Dynamik, Einschwingen, Ausklingen, Grundton/Obertöne, Mono; kurze Hörvergleiche. Dew, Glimmer und Ambient-Retuning bleiben in der Queue. | Offen. |
| 5: Raum und Bewegung | Erst gemeinsamer Raum, dann begründete Zusatzbearbeitung. Drone/Bass/Fundament auf Zusammenlegung prüfen. | Vergleich mit jeweils entfernter Effektstufe; Stabilität, Tonhöhenbeiträge, Verständlichkeit der Regler und Low-Mid-Summe. | Offen. |
| 6: Physisches Produkt | Erst auf Basis der reduzierten Interaktion Display, Licht, Beschriftung und Form bewerten. | Aktuelle CAD/PCB/Bilder, funktionierender Prototyp; Reichweite, Sichtbarkeit, Fehlbedienung und Ausgänge testen. | Evidenz noch erforderlich. |

Paket 2 muss vor dem Code folgende Konflikte lösen: Wer spielt die automatische
Melodie, wenn ein monophoner Core vom Menschen gehalten wird? Welche Quelle
wird ersetzt statt zusätzlich gemischt? Wie bleibt die bestehende Bass-/Tail-
Pitch-Memory korrekt? Wie verhalten sich Hold und Core-Wechsel? Was läuft im
teuersten Übergangsblock parallel? Ein Entfernen der Render-Guards allein wäre
keine fertige Integration.

### Paket 1 — fünf Perspektiven

| Perspektive | Entscheidung / noch fehlender Nachweis |
|---|---|
| Product Idea | Generate ergänzt den musikalischen Vorgang. Es übernimmt nicht mehr die Bedeutung der Tasten. |
| Interaction | Note/Harmony/Land funktionieren mit und ohne Generate nach derselben Zuordnung. Ein Wechsel beendet alte Eingaben sauber. Ihre spätere Reduktion bleibt offen. |
| Sound | Bisher verlorene Releases und blockierte neue Note-Eingaben werden beseitigt. Oszillatoren, Effekte, Gains und Hüllkurven sind unverändert. Kein neuer Klangqualitätsanspruch. |
| Industrial Design / UI | Die automatische STEER-Anzeige entfällt zusammen mit ihrem widersprüchlichen Verhalten. Keine zusätzliche Anzeige und kein zusätzlicher Taster. Visuelle/haptische Qualität noch nicht geprüft. |
| Engineering | Fünf Byte Router-Zustand, höchstens fünf Releases beim Aufräumen. Kein Heap, keine Audiopuffer, keine neue Arbeit pro Audiosample. Exakte Linker-/Flash-Auswirkung und H743-Laufzeit noch ungemessen. |

Technik: `cell_router.h` merkt pro Fläche den Empfänger des Anschlags und
löscht ihn vor dem Release-Callback. Wiederholtes Down schließt den vorherigen
Besitz ab; unzugeordnete Ups werden ignoriert. `controls_release_cells` beendet
Note-Quellen, Latches und Presence beim Verlassen dieses Modus und erhält die
Modifier. Clear leert zusätzlich offene Router-Eingaben. Die vorhandenen
physischen und Gesture-Ereignisse teilen weiterhin dieselben fünf Quellen;
eine unabhängige Besitzverwaltung für gleichzeitig gespielte und abgespielte
identische Zellen gehört noch zur Integrationsprüfung.

Kompatibilität: keine Änderung an Menü-/Scene-IDs oder Speicherformat.
Generate+Cell löst absichtlich keine Composer-Intents mehr aus. Die interne
`engine_generative_nudge`-API bleibt für explizite Aufrufer/Tests erhalten.
Shift+Generate erzeugt weiterhin ein neues Feld. Keine zusätzliche versteckte
Geste für die entfernte automatische Steer-Bedienung.

## BESTE VERSION

Eine Welt ist eine kuratierte musikalische Umgebung. Die Flächen bleiben
spielbar, während Generate die Umgebung weiterentwickelt. Die ausgewählte
Stimme gehört räumlich, dynamisch und harmonisch dazu. Die Bedienung erzeugt
eine verständliche Wirkung, ohne dass der Nutzer DSP-Modi verwalten muss.
Eine gute erste Berührung funktioniert mit den tatsächlichen digitalen
Tastern. Mehr Drucksensitivität wird nicht behauptet oder vorausgesetzt.

Die stärkste Version kann weniger Welten, Stimmen oder Effekte enthalten als
der heutige Code. Kein Bestandsschutz für die Anzahl sechs oder für einen
bereits belegten Menüpunkt. Klangvielfalt muss hörbar nützlich sein.

## TEST AM GERÄT

1. In Note eine Taste drücken, Generate an/aus, loslassen. Der Ton muss gemäß
   seiner Hüllkurve enden; kein festhängender Presence-Zustand. Umgekehrte
   Reihenfolge und alle fünf Tasten wiederholen.
2. Mit aktivem Generate jede Spielweise bedienen. Keine automatische
   STEER-Umdeutung. In Character müssen Tasten wieder Noten auslösen; das
   noch fehlende automatische Bett ausdrücklich als offene Lücke behandeln.
3. Note mit Hold und Shift belegen, Spielmodus wechseln, zurückkehren.
   Der erste neue Druck muss einen neuen Ton erzeugen, keinen alten Latch
   ausschalten. Währenddessen gehaltene Tasten anschließend loslassen.
4. Clear und Moduswechsel während einer Gesture-Wiedergabe; Überlagerung
   derselben physischen/aufgenommenen Zelle separat prüfen. Scene-Browser
   während eines gehaltenen Tons als noch offene Routing-Grenze testen.
5. Erst nach Paket 2: gleiche kurze musikalische Handlung durch Worlds und
   Characters; Übergänge, Mono und echte Ausgänge hören. H743-Profiler:
   `peak_load < 0.60`, keine Deadline-Misses, Stack und Linker-Map prüfen.

### Reproduzierbare Prüfung von Paket 1

`python3 test/test_cell_routing.py` kompiliert die tatsächlichen Routing- und
Moduswechselfunktionen aus `main_h743.c` gegen Callback-Probes. Es wird kein
zweites Routingmodell nachgebaut. Mit dem alten HAL-Quelltext als Argument
scheitert derselbe Test am verlorenen Release nach dem Generate-Wechsel.
Der neue Stand prüft zusätzlich alle neun Kombinationen alter/neuer Spielweise,
Modusaufräumen, wiederholte Presses sowie unzugeordnete/ungültige Releases.
`test_controls.c` prüft die Latch-Bereinigung mit echten Control-/Engine-Modulen.

Ergebnis: vollständiges `bash test/run_tests.sh` erfolgreich, einschließlich
16.478 Device-Path- und 478.860 Effektprüfungen; Hot-Path-Lint ohne verbotene
Aufrufe. Das bescheinigt diesen technischen Stand, keine klangliche Freigabe.

Diese Host-Prüfungen sind keine vollständige Ausführung des STM32-Main-Loops.
Ein ARM-Compiler wurde in dieser Umgebung nicht gefunden; ein neuer H743-
Cross-Build und ein aktueller Speicher-/Timingnachweis liegen nicht vor.
Für diese Eingabelogik liefert eine weitere WAV-Datei keinen zusätzlichen
Nachweis. Die nächste Hörprobe muss die tatsächliche Bed/Core-Integration
prüfen und bleibt auf höchstens 30 Sekunden begrenzt.
