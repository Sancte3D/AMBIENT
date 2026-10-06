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

## Concrete next work — after owned Pluck stop

Each row is a separate completed code/listening unit. Avoid simultaneous
source, scheduler and room rewrites: their effects must stay attributable.

| Order | Work | Acceptance evidence |
|---|---|---|
| 1 — implemented 2026-10-06 | A generated source-start transaction: check capacity and valid pitch, start DSP, then commit owner / harmony / hook / counters only on success | Failed admission changes no sounding-pitch state, event count or existing waveform; retry is delayed rather than retriggered every tick |
| 2 — implemented 2026-10-06 | New autonomous World path without automatic Bed, Eno loops, bass or generated reverse swell | Source-event trace starts only the planned family; harmony advances independently of accompaniment; Stop releases its owners; real rests exist before room tails |
| 3 | Common three-slot budget across actual source voices and release tails | Cross-world handover, maximum Shape release and repeated Generate cannot exceed the budget; when full, wait rather than hard-steal; account for both bass sub/deep if ever enabled, not just bass_active boolean |
| 4 | WOODLAND dry Pluck articulation: excitation, sustain, damping and ownership | Low-mid pitch stability across damping, no hiss-like onset; a short motif plus silence is recognizable without body/hall; <=30 s A/B |
| 5 | COAST dry Bowed: fundamental, detune, bow noise and motion | No stationary electrical buzz or octave dominance; slow overlap communicates motion without a fixed repeating sweep; <=30 s A/B |
| 6 | HIGHLANDS dry Horn: onset, register, body and modulation | No whistle/alarm/tube association across supported range; reject the candidate if it cannot distinguish itself calmly from COAST; <=30 s A/B |
| 7 | One shared room; modal body reviewed separately | Dry identity survives wet range and mono; no pitch-obscuring resonance, source loudness jump or unsafe feedback |
| 8 | Separate World grammars and calibrated macro ranges | Equal seed/register/dry gain still produces different phrasing; macros never turn sparse worlds into continuous beds or expose alarm-like leads |
| 9 | Manual World palette, explicit old-scene migration, unlink archive | Old scenes cannot silently choose new meanings; archived synth code is absent from product link; fresh H743 map proves actual resource reduction |

20 ms Pluck stop is a technical control ramp, not its musical decay design.
Natural self-decay remains; note-off semantics and soft-source transitions must
be evaluated with later WOODLAND phrasing. Removing an old role does not justify
adding three compensating effects. No new grammar may require SD streaming.

### 2026-10-06 progress

Generated starts now commit their pitch/event/phrase counters only on actual
source admission. Three World-family DSP slots, including releases and queued
onsets, govern this path. This is not yet a whole-product three-source limit:
legacy manual admissions remain outside that gate. Automatic Bed/Eno/bass
have now been removed from Generate (checkpoint below); whole-product occupancy
and source/room behavior during transitions remain the next validation. H743 CI now explicitly
builds Release and retains image plus linker map; on-device DWT remains open.

Release H743 verification completed for the 2026-10-06 admission code: linker
accepts all banks; Flash 12.98%, DTCM 91.13%, D1 79.62%, D2 87.52%. Exact bytes,
tested code commit and run are in WORLD_ROUTING_CHECKPOINT.md. No CPU or
physical sound-quality acceptance is implied. Further source/room work must
reuse pools or justify new placement against the remaining per-bank capacity.

### Autonomous accompaniment removed — 2026-10-06

Generate now schedules only the selected World source. Automatic source-8 pad,
source-5..7 loops, loop pre-swells, pad brightness walk and composer-driven
bass depth/gains are removed, rather than hidden behind disabled defaults.
The unused Eno gate API had no firmware/menu/scene callers and is removed;
World IDs and SCN6 wire layout retain their meanings.

The opening decision is immediate, subject to pitch safety, the existing onset
spacing and strict admission. Source attacks provide the onset. Ordinary events
retain density/rest decisions. The explicit harmonic-step API now changes
harmony only; renderers need scheduler ticks to hear World events. Low-level
player-priority return retains its softer delayed entry. Disabling World events
leaves no substitute tonal bed; atmosphere and shared FX remain independent.

Bass-follow is blocked throughout listening, including source releases and
entry/exit handovers. Its manual preference survives. Generate entry releases
manual drone; a World change releases the old scheduled owner explicitly,
even if two future Worlds share key/mode. Maximum Shape releases remain in the
three-slot gate; a saturated target change waits instead of stealing.

Next completed unit: close source/transition occupancy against the actual
entry/exit paths, then WOODLAND dry Pluck excitation and damping. Source sound
quality and speaker/headphone behavior are still unapproved. No new pools,
room instances, SD streaming or new per-sample calculations were added.

Release verification for this unit succeeded at code commit c76617cbecaadc572b82d594c3fe87fab3931507:
all five CI jobs green; Flash 254,028 B, DTCM 119,440 B, D1 417,376 B, D2
258,112 B. Actual reductions are 1,196 Flash bytes and 64 D1 bytes; inactive
legacy pools are not counted as freed. Exact run and open device gates are in
WORLD_ROUTING_CHECKPOINT.md.
