# AMBIENT — verschiedene Ensemble-Algorithmen, 8. Oktober 2026

## Bestätigte musikalische Richtung

Der Nutzer hat die 27-s-Ensemble-Probe aus PR144 mit „ja besser!!!! weiter so“
positiv bewertet. Bestätigt ist diese Richtung: gehaltene gemeinsame Akkordtöne,
verschiedene Dauern und Registerwechsel. Das ist keine gesamte Produkt-/Geräte-
oder Familienabnahme. Als Nächstes braucht jede World eine eigene musikalische
Regel. Die Regeln werden einzeln umgesetzt und gehört.

| World | Musikalischer Algorithmus | Konkreter Stand |
|---|---|---|
| COAST | Zwei äußere Register nähern sich in Gegenbewegung einer gemeinsamen Mitte; ein Akkordton verbindet die Schritte | Seed-gesteuerter Host-Algorithmus und tatsächliche C-DSP-Hörprobe vorbereitet |
| WOODLAND | Eine kleine Akkordfigur stellt eine Idee vor; eine andere Stimme antwortet mit verwandter Kontur und verändertem Register; Variation und Ruhe | Kurzer Pluck-Entwurf vom Nutzer verworfen; neuer langer Saiten-Kandidat mit 45-s-Probe |
| HIGHLANDS | Weit verteilte gehaltene Stimmen verändern einzelne Akkordtöne mit eigenen Dauern; längere offene Pausen und Rückkehr | Seed-gesteuerter Akkord-/Pausen-Host-Algorithmus, reale Horn-Hörprobe mit 44 s |

Gemeinsamer Vertrag: begrenzter harmonischer Plan, klare Stimmrollen, echte
Hz-/Besitzerhistorie, bewusste Spannungen/Auflösungen. Die Klangfamilie allein
ist keine World-Identität. Unterschiedliche Regeln für Zusammenhang und Zeit
werden nicht mit denselben Zufallsereignissen und anderem Filter vorgetäuscht.

## COAST — Zusammenlaufen

Der Nutzer beschreibt zwei gleichzeitig gespielte äußere Oktaven: unten steigt
es, oben fällt es, bis die Linien in einer mittleren Lage zusammenfinden.
Die Probe setzt dies über musikalische Tonstufen um, mit D4 als Ziel:

- Unterer Weg: **D3 → A3 → B3 → D4**.
- Oberer Weg: **D5 → B4 → F#4 → D4**.
- Gemeinsamer Akkordton: leises F#3, von 0,7 bis 22,6 s gehalten.
- Beide äußeren Wege beginnen gleichzeitig. Ihre weiteren Einsätze und Dauern
  sind unabhängig; Seed bestimmt die gemeinsame Phrasenweite, obere Verzögerung,
  kleine Akzente und unterschiedliche Haltezeiten.
- Beim Ziel **18,49 s** (Seed1234) werden die beiden Tonabsichten zu **einer**
  gemeinsamen D4-Stimme zusammengeführt. Der Zielton bleibt bis 23,5 s stehen.
  Es werden keine zwei unkontrolliert gegeneinander phasenden Unisono-Oszillatoren
  erzeugt. Die gehaltenen Absichten der Rollen bleiben eine spätere Besitzfrage.

`review_converging_world.py` erzeugt den Score aus diesen Regeln und Seed,
setzt ihn in eine temporäre Kopie des vorhandenen echten C-Renderers ein und
kompiliert unveränderte Product-Bowed/Room/Shape/DSP-Quellen. Derselbe Tonkörper
wie im positiv bewerteten Entwurf, Room0,24, Nature0, Volume0,6. Kein neuer Layer.

Die ursprüngliche komponierte Ensemble-Probe bleibt als positiv bewertete
Referenz erhalten. Dieser neue COAST-Verlauf ist ein eigenständiger Algorithmus,
kein Ersatz-WAV unter derselben Identität.

## Prüfbefund

- Ausgewählter Seed1234: acht angenommene tatsächliche Quellenstarts;
  maximal drei reale Stimmen einschließlich Releases; alle Quellen enden
  natürlich vor 27 s. Kein Clear oder Schlussfade.
- Dry/Room jeweils bytegleiches tatsächliches PCM und Traces bei 64/512 Frames.
- PCM16 Stereo/44,1 kHz, 27 s; finite/Clipping-, Peak-, Mono- und DC-Prüfung.
  LISTEN −23 LUFS / −12,1 dBFS True Peak, ein konstanter Gain, RAW getrennt.
- 300 Planfälle über drei Mittellagen und 100 Seeds prüfen Gegenrichtung,
  gemeinsames Ziel, eindeutige Zielquelle und Rückkehrabstand für reale Releases.
  Alternative Seeds ändern die Phrasierung; gleicher Seed wiederholt denselben
  Plan. Die zusätzlichen Seeds sind Planchecks, keine weitere Hör-/Geräteabnahme.
- C mit `-Wall -Wextra -Werror`, Python, Workflow und Diff geprüft;
  Source-/Header-/Tool-Hashes und der temporäre C-Score sind nachvollziehbar.

## Grenzen und nächste Arbeit

Das ist ein **Host-World-Algorithmus**, noch nicht in den autonomen Product-
Generator integriert. D5/B4 erweitern den bisherigen Produktregister-Kandidaten;
pro Rolle werden direkte Attackwerte und kurzer SHAPE-Release=0 verwendet.
Produktquellen, Header, Defaults und UI wurden nicht geändert. Der Raum darf
am Dateiende einen leisen Resttail enthalten; kein vollständiges Room-Ende
behauptet. Die COAST-Richtung wurde erneut positiv gehört; abschließende Geräteabnahme bleibt offen.

Die Integration braucht Chord-/Voicing-Zustand, erweiterte geprüfte Register,
Rollenbesitz beim gemeinsamen Ziel und einen planbaren Umgang mit tatsächlichen
Releases und Raumgedächtnis. Die bisherige pauschale Intervall-/Tail-Sperre darf
beabsichtigte Stimmführung nicht unbemerkt in Stillstand verwandeln. WOODLAND
und HIGHLANDS sind ebenfalls als getrennte Host-Algorithmen unten beschrieben.
Weiterhin 24 technisch geschlossene / 30 offene Gesamtgates.


## WOODLAND — verworfene kurze Ruf/Antwort-Probe (PR146)

`review_world_ensembles.py` erzeugt drei Figuren mit demselben erkennbaren
Rhythmus, ungleichen Zwischenräumen und eigenen Akzenten. Seed1234 in D:
D4–F#4–A4 (Ruf), D5–B4–F#4 (obere Antwort), B3–D4–F#4 (ruhigere Rückkehr).
Die Verbindung ist D-Dur zu h-Moll, mit F# als gemeinsamer Tonklasse. Keine
zusätzliche Fläche: zwei echte Karplus-Strong-Saiten klingen natürlich aus.
Seed verändert Transposition, Rhythmus und Attack; der jeweilige Antwortbeginn
liegt acht Sekunden nach dem Ruf. Echte Besitzerwechsel haben mindestens
4,9 Sekunden Abstand. Neun Starts, maximal zwei Quellen einschließlich Tails,
keine Note-Off-Kürzung der Saiten, natürliches Quellenende innerhalb von 30 s.
Direkter SHAPE-Release0,60; hier bedeutet Release natürliche Saitenabklingzeit.

## HIGHLANDS — weiter Akkord, gemeinsamer Ton, Pause, Rückkehr

D3/A4/F#4 öffnen mit versetzten Einsätzen einen weiten D-Dur-Akkord.
F#4 bleibt in derselben realen Stimme von 1,6 bis 18,6 Sekunden stehen;
D3 und A4 enden unabhängig. Erst nach ihren Quellenausklängen treten B3
und D4 hinzu: h-Moll mit demselben F#4. Danach enden alle Quellen natürlich.
Ab 28 Sekunden kehrt der weite D-Dur-Akkord mit eigenen Dauern zurück.
Acht Starts, maximal drei Quellen inklusive Releases, 44 Sekunden Hörprobe.
Seed verändert die gemeinsame Tonlage und den Zeitpunkt der inneren Übergabe;
kein Vibrato-/Filtertrick ersetzt den musikalischen Akkordwechsel.
Direkter kurzer SHAPE-Release0 wie im positiv gehörten Ensemble-Entwurf.

## Neue Prüfungen und Produktionsübergabe

Für beide neuen Worlds: 100 Plan-Seeds geprüft. Seeds1,42,1234 zusätzlich
vollständig als Dry/Room mit tatsächlichem C-DSP gerendert; auch hier 64/512
Frames bytegleich, Quellenzulassung strikt, Quellenausklang natürlich.
C-Warnings-as-errors, finite PCM, Format, Headroom, Mono/DC, feste LISTEN-
Pegel und Datei-/Quellenhashes geprüft. LISTEN liegt bei etwa −23 LUFS;
keine Kompression und keine zusätzlichen Effekte. Room0,24, Nature0,
Volume0,6, Quellenkalibrierung0,5. Der Raum darf am Dateiende noch leise klingen.
`WOODLAND_ENSEMBLE_METRICS.json` und `HIGHLANDS_ENSEMBLE_METRICS.json` dokumentieren
Seed1234; alternative Seeds liegen separat im CI-Artefakt.

Alle drei musikalischen Algorithmen sind damit als reproduzierbare Host-
Kandidaten ausgebaut. Ihre gemeinsame Produktionsintegration ist weiter offen:

1. `engine_product.c:admit` begrenzt aktuell auf 140..470 Hz und den engen
   pentatonischen Vorrat. COAST/WOODLAND benötigen geprüfte obere Register.
2. `world_grammar_propose` liefert nur einzelne Ereignisse ohne Akkord-/Voicing-
   Vertrag; HIGHLANDS erlaubt bisher nur eine gehaltene Stimme. Der neue
   Akkordplan muss ausdrücklich mehrere Rollen und gemeinsame Töne verwalten.
3. Reale Releases und das 7,2-s-Raumgedächtnis dürfen Rollen nicht unplanbar
   blockieren. Gemeinsame Ziele müssen einmal klingen, Besitzer dürfen erst
   nach tatsächlichem Quellenausklang wiederverwendet werden.
4. Direct-SHAPE-Werte müssen in geprüfte Produkt-Makrogrenzen überführt werden;
   Dry/Room-Proben ersetzen keine echten Generator-/Übergangs-/Gerätechecks.

Keine Produktionsquelle, Header, Default oder UI geändert. Keine neuen
Gesamtgates geschlossen; die abschließende WOODLAND-/HIGHLANDS-Hörabnahme fehlt.


## WOODLAND — lange Saiten nach Nutzerkorrektur

Nutzerurteil am 8. Oktober: Die 30-s-Probe ist zu kurz artikuliert und zu
gezupft. Ambient braucht lange bzw. gestreckte Töne. Die kurze WOODLAND-
Ruf/Antwort-Probe ist damit als musikalische Richtung verworfen; HIGHLANDS
und COAST werden durch diese Korrektur nicht verändert.

`review_woodland_long.py` erstellt einen isolierten Saiten-Kandidaten und eine
45-s-Hörprobe: D3/F#3 öffnen langsam, F#3 verbindet den Wechsel zu B3,
D4 bildet mit B3 h-Moll; anschließend F#3/D4 und F#3/A3. Nur sechs Einsätze,
versetzte Haltezeiten von 6 bis 18 Sekunden, zwei tatsächliche Saiten mit
Überlagerung. Es ist ein langer natürlich abklingender Saitenton, kein endloser
konstant gehaltener Oszillator und kein Zeitstrecken einer Audiodatei.

Drei klar begrenzte Änderungen in einer temporären Kopie von `pluck.c`:
nominaler T60 von 3,2 auf 36 Sekunden, Attackbasis von 8 auf 800 ms
(begrenzter Kandidatenbereich 0,4..2,4 s), weicher eigener Stop von 20 ms
auf 2 Sekunden. Loop-Dämpfung0,015 statt0,42; direkter SHAPE-Release0,50.
Delay-Länge, Stimmenpool, Phasen-/Tonhöhenverfahren und Raum bleiben bestehen.
Keine Wiederanregung, Kompression, neue Fläche oder neue FX. Diese Änderungen
sind ausdrücklich kein neuer Product-Default; ursprüngliche Quelldateien
werden weder überschrieben noch als unverändert klingender Render behauptet.

Prüfung: Ein isolierter D3-Test weist eine deutlich weichere erste100-ms-Phase
und einen noch tragenden Klangkörper bei8..9s nach. Der spätere RMS muss
mindestens20% des RMS bei2..3s betragen; dies ist eine Zeit-/Energieprüfung,
keine Hörabnahme. Der volle45-s-Score hat sechs strikt angenommene Starts,
maximal zwei reale Quellen inklusive der langen Releases und Quellenende
vor Dateiende. Kein Stealing oder Clear. Dry/Room-PCM und Traces bei64/512
Frames bytegleich; finite/Format/Peak/Mono/DC, C-Warnings-as-errors, Python,
Workflow und Quellen-/Tool-Hashes geprüft. Fester Gain auf−23LUFS,
True Peak−12,9dBFS. Room0,24, Nature0, Volume0,6; Raumtail darf noch leise stehen.

Zuerst `AMBIENT_WOODLAND_Long_room_listen_45s.wav` hören; Dry ist dieselbe Folge.
`WOODLAND_LONG_METRICS.json` dokumentiert Original-/Kandidatenhashes,
Parameter, Score, isolierte RMS-Verhältnisse und vollständige Renderbefunde.
Die ursprüngliche WOODLAND-Quelle und Generatorintegration bleiben offen:
kurze Pluck-Figuren werden nicht mehr als bestätigte Richtung vorausgesetzt.
