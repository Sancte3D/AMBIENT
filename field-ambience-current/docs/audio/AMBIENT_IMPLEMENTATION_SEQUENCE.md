# AMBIENT — abgeschlossene Umsetzungspakete nach Konzeptfreigabe

2026-10-04. Plan aus [AMBIENT_PRODUCT_BRIEF.md](AMBIENT_PRODUCT_BRIEF.md).
Nutzerauftrag 2026-10-04 autorisiert Archivierung und Code-/Soundentwicklung.
Namen und klangliche Eignung bleiben überprüfbare Entwurfsannahmen; keine
Hardware- oder Klangfreigabe. Hördateien bleiben je Datei höchstens 30 s lang.

## Reihenfolge und Abhängigkeiten

| Paket | Konkreter Umfang | Ergebnis / erforderlicher Nachweis | Abhängigkeit |
|---|---|---|---|
| S0 | Quellen-, Tonhöhen- und Routingprotokoll des bestehenden Pfads | Sichtbar, welche realen Quellen inklusive Pad/Bass/Release einen Einsatz erzeugen | Konzeptfreigabe |
| S1 | Expliziter autonomer Quellenstart ohne implizite Begleitung | Ein Ereignis startet genau seine geplante Familie; manueller Bestand bleibt separat testbar | S0 |
| S2 | Gemeinsame Belegung, Quellenbesitz, weicher Stop | Volle Slots erzeugen weder harte Retrigger noch ungezählte Doppelstimmen; Pluck kann kontrolliert enden | S1 |
| S3 | COAST-Quelle trocken | Körper bleibt bei Mono, Register und Lautstärke erhalten; periodische Bewegung separat entschieden | S2 |
| S4 | WOODLAND-Quelle trocken | Gedämpfter Ton mit weichem erkennbaren Anfang; Body-Abtrag und Allokationsverhalten geprüft | S2 |
| S5 | HIGHLANDS-Quelle trocken | Eigene Artikulation ohne Pfeif-/Röhrencharakter; bei Scheitern Kandidat ersetzen | S2 |
| S6 | Drei Ereignisgrammatiken nacheinander | Eigene Zustände, gedächtnisbasierte Entwicklung, begrenzte Suche und reale Pausen | Quellen S3–S5 jeweils einzeln |
| S7 | Gemeinsame Harmonie und Registergrenzen integrieren | Kandidaten gegen reale Belegung und konservative Fahnen geprüft; Konflikte führen zu Ruhe | S6 |
| S8 | Reduzierter Raumpfad und World-Zielwerte | Identität bleibt erkennbar; Dry/Send/Wet und Mono nachvollziehbar | S7 |
| S9 | Worldwechsel / schnelle Zielwechsel / Generate-Ausstieg | Keine Engine-Stapel, falsche Releases, Pegelsprünge oder unbegrenzten Übergänge | S8 |
| S10 | Zugängliche musikalische Einflussbereiche | Defaults und erreichbare Grenzen sind nutzbar; Lautstärke getrennt; keine Effekt-Reparatur nötig | S9 |
| U1 | Bedienmapping, Zustände und visuelle Hierarchie | Ein vollständiger verständlicher Ablauf, im konkreten Control-Layout beurteilt | Sounddesign ausreichend bestätigt |
| U2 | Displayruhe, klangneutrales Wake und Licht | Human-idle korrekt; Stop/Volume sofort; Displayarbeit beeinträchtigt Audio nicht | U1 |
| H1 | Hardwareintegration und akustische Abstimmung | Passung, Ausgänge, Haptik, CPU/RAM/Stack und Verhalten am echten Gerät | U2 + realer Aufbau |

S0 ist Diagnose, kein Klangumbau. S3–S5 dürfen getrennt bearbeitet werden; daraus
folgt kein Bedarf an parallelen neuen Prozessorinstanzen. Jede Quelle wird erst
trocken ausgewählt. Raum ist eine spätere musikalische Integration.

## Notwendige Prüfungen statt Testzahlen als Qualitätsersatz

Tests werden an einem konkreten Failure Mode oder Vertrag ausgerichtet.
Keine Prüfung nur deshalb hinzufügen, weil eine Tabelle implementiert wurde.
Bestehende relevante Regressionen weiterverwenden; Testzahlen sind kein AAA-Beleg.

| Failure Mode | Gezielte Prüfsituation | Erfolgsbedingung |
|---|---|---|
| Versteckter Pad-Layer | Autonomes Einzelevent, Quellenhook und trockenes Routing | Nur beauftragte Quellen aktiv |
| Neue Noten überschreiben Release | Alle lokalen/globalen Slots belegt, weiterer Vorschlag | Ereignis entfällt/verschiebt sich oder genehmigte weiche manuelle Übergabe |
| Falscher Besitzer bei Wechsel | Press → Modus/World ändern → Release | ursprüngliche Quelle endet, keine neue wird versehentlich beendet |
| Falsche harmonische Erinnerung | Vorschlag abweisen, nächstes Motiv erzeugen | Abgewiesener Ton zählt nicht als tatsächlich gehört |
| Zu frühe Harmonieänderung | Farbton mit langer Fahne, benachbarter Farbton als Kandidat | Konfliktprüfung berücksichtigt noch relevante Fahne |
| Lange blockierende Tail-Schätzung | Raum abgeklungen, neue passende Ereignisse | Musikalische Wiederaufnahme; keine dauerhaft leer blockierte World |
| Regelmäßige innere Bewegung | Mehrere gehaltene Töne, Körpermodulation isolieren | Hörentscheidung auf getrennte Ursache stützen, nicht nur Einsatzabstände ändern |
| Allokations-/Pegel-Sprung | 1→2→3 Quellen und zurück, trocken und mit Raum | Begrenzter Pegel, kein Clipping; kein laufender AGC zum Kaschieren |
| Worldwechsel stapelt Engines | viele schnelle Zielwechsel bei vollen Slots | Letztes Ziel, festes Budget, keine anwachsende Übergangsqueue |
| Display weckt Musikzustand um | laufende World, Displayruhe, unterschiedliche Eingaben | Klangneutral für Navigation; Volume/Stop unmittelbar |
| Scene erzeugt lauten Neustart | Laden während Ruhe/Autoplay/Übergang | Master nicht unerwartet angehoben, kein ungewolltes Autoplay |
| Stereo kaschiert Tonverlust | derselbe Quellton und Raum in Stereo/Mono | Tonkörper bleibt; Auslöschung gesondert korrigieren |

## Hörvergleich und Nachvollziehbarkeit

Jede kleine Hörentscheidung hat eine einzige Hauptfrage. A/B möglichst gleiche
Tonhöhe, Eingangspegel, zeitliche Ereignisse und passenden konstanten Export-Gain.
Keine einzelne Variante durch dynamische Normalisierung hübscher machen.
Pegelangleichung und Messwerte dokumentieren; absolute Kopfhörerlautstärke folgt
nicht aus einem digitalen LUFS-/Peak-Wert.

Für Identität: mehrere eigenständige Seed-Ausschnitte, gleiche Ausgangsharmonie,
verdeckte Namen und vergleichbare Lautheit. Ein leicht erkennbares Testintro
darf nicht die einzige erkennbare World-Eigenschaft sein. Verwechslungen mit
Zeitstelle und wahrgenommenem Merkmal notieren. Keine erfundene Zahl von
Testpersonen oder bereits erreichte Erkennungsquote melden.

Interne längere Ereignis-/Ressourcenläufe sind möglich; ausgegebene Audiodateien
bleiben unter dem Nutzerlimit. Langzeitverhalten darf aus einem schönen
20-Sekunden-Ausschnitt nicht als bestanden gelten.

## Ressourcenvertrag

- Neue Ereignisverwaltung arbeitet auf begrenzten Zuständen, ohne dynamische
  Allokation im Audio-/Control-Hotpath und ohne unbeschränkte Suche.
- Statische Pools zählen nicht als eingespartes RAM, wenn lediglich weniger
  Stimmen gleichzeitig aktiviert werden. Neue Tabellen, States und Puffer
  getrennt im aktuellen Linker-Map nachvollziehen.
- Konstant vorgegebene Quellenanzahl bedeutet nicht konstanten Rechenaufwand:
  Familie, Raum, Ausklänge und Übergabe verändern die Spitzenlast.
- CPU-Zeit auf dem Host ist keine MCU-Deadline-Messung. Bestehendes DWT-Ziel
  peak_load < 0,60 aus PRODUCT_REVIEW_EXECUTION bleibt ein Projektziel, kein
  aktueller bestandener Nachweis. Worst Case und Deadline-Misses am Gerät prüfen.
- Display, Eingaben und Speicherung dürfen den Audiozeitplan nicht blockieren.
  Potentiell teure Koeffizienten-/Pufferarbeit dort ausführen, wo sie tatsächlich
  in den vorhandenen Scheduling-Vertrag passt; keine pauschale ISR-Freigabe.
- Ausgangsmute, Low-Battery/Brownout und Steckerwechsel benötigen die tatsächliche
  Schaltung und Versorgung. Keine aus einer Prozentanzeige abgeleitete Schutzgarantie.

## Migration und Rückkehrmöglichkeit

Vor Runtime-Änderung die heutigen fünf World-IDs, sechs manuellen Engine-IDs,
Scene-Versionen und Defaults inventarisieren. Alte Scene-Inhalte werden nicht
durch stilles Wiederverwenden einer ID zu neuen musikalischen Welten.
Neue Konzepte erst als getrennte Kandidaten einführen; die endgültige Migration
mit expliziter Version/Mapping entscheiden. Ein Vergleichspfad darf temporär
im Entwicklungsbuild bleiben, ohne deshalb Teil der Produktoberfläche zu werden.

Jedes Paket erhält einen kleinen Commit mit Ursache, Änderung, erforderlicher
Prüfung und offenen Hör-/Gerätegrenzen. Ein abgelehnter Klangkandidat ist ein
gültiges Ergebnis. Erst die Ursache prüfen; keine zusätzlichen Layer oder
Effekte aufsetzen, um unpassende Quellen zu überdecken.

## Konkreter nächster Arbeitsschritt

Der Produktbrief und die neue manuelle Palette werden als Konzeptentscheidung
geprüft. Anschließend S0/S1: den echten autonomen Quellenpfad protokollieren und
den impliziten Pad-Start von ausdrücklich gewählten Quellen entkoppeln.
Diese Entkopplung hat einen klaren Zweck: erstmals jede World als ihr eigenes
System hören können. Erst danach Quellen-Sounddesign in kurzen Einheiten.

## Implementation checkpoint — 2026-10-04

User authorized sound/code work and archiving the manual catalog. Source files
are now under `src/v2/Synths_Archive`; compatibility links remain explicitly.
S1 melody dispatch no longer allocates an implicit pad and no longer starts
the World source twice. Bed/Eno sources and the global occupancy contract
remain separate pending work; S0/S1 as a whole are not declared complete.
See WORLD_ROUTING_CHECKPOINT.md.
