# AMBIENT — verschiedene Ensemble-Algorithmen, 8. Oktober 2026

## Bestätigte musikalische Richtung

Der Nutzer hat die 27-s-Ensemble-Probe aus PR144 mit „ja besser!!!! weiter so“
positiv bewertet. Bestätigt ist diese Richtung: gehaltene gemeinsame Akkordtöne,
verschiedene Dauern und Registerwechsel. Das ist keine gesamte Produkt-/Geräte-
oder Familienabnahme. Als Nächstes braucht jede World eine eigene musikalische
Regel. Die Regeln werden einzeln umgesetzt und gehört.

| World | Musikalischer Algorithmus | Konkreter Stand |
|---|---|---|
| COAST | Zwei äußere Register nähern sich in Gegenbewegung einer gemeinsamen Mitte; ein Akkordton verbindet die Schritte | Jetzt im tatsächlichen Product-Generator integriert;48 Engine-Phrasen, Host-Suite, H743 und70-s-Firmwareprobe geprüft |
| WOODLAND | Zwei lange Saiten ersetzen einzelne Akkordtöne, während ein gemeinsamer Ton stehen bleibt; eigene Dauern, Antwort und Ruhe | Lange Quelle und Generator integriert;48 echte Phrasen und50-s-Engineprobe geprüft; neue Hörwahl offen |
| HIGHLANDS | Weit verteilte gehaltene Stimmen verändern einzelne Akkordtöne mit eigenen Dauern; längere offene Pausen und Rückkehr | Seed-gesteuerter Akkord-/Pausen-Host-Algorithmus, reale Horn-Hörprobe mit 44 s |

Gemeinsamer Vertrag: begrenzter harmonischer Plan, klare Stimmrollen, echte
Hz-/Besitzerhistorie, bewusste Spannungen/Auflösungen. Die Klangfamilie allein
ist keine World-Identität. Unterschiedliche Regeln für Zusammenhang und Zeit
werden nicht mit denselben Zufallsereignissen und anderem Filter vorgetäuscht.

## COAST — Zusammenlaufen

### Aktuell: tatsächlicher Product-Generator

`engine_product.c` verwendet jetzt einen begrenzten COAST-Phrasenzustand aus
`world_grammar.c`. Acht bestätigte Starts je Durchlauf: äußeres Paar, leiser
gemeinsamer Akkordton, zwei gegenläufige Paare, ein gemeinsamer Mittelton.
Owners6/7/15 und tatsächliche Quellenreleases bleiben im globalen Drei-Slot-
Budget. Der gehaltene Akkordton wird beim Ziel nicht neu gestartet; sein Ende
wird erst nach dem tatsächlichen DSP-Beginn des Zieltons geplant. Kein doppelt
phasendes Zielunisono. Seed variiert obere Route, Haltezeiten und Akzente;
alle vorgeschlagenen Töne bleiben im gewählten Major-/Minor-Core.

Das erweiterte MIDI45..80-/105..850-Hz-Register ist nur intern für diese
COAST-Phrase freigegeben. Manual/öffentliche World-Note-API und die beiden
anderen Generatoren behalten ihre bisherigen Grenzen. Quellen-DSP,
Attack-/Release-Makrobereiche und7,2-s-Tailhistory sind unverändert.
Wirkliche Quellenenden und harmonische Fahnen können geplante Einsätze
verzögern: die vollständige D-Dur/Equal/Seed1234-Probe trifft sich bei43,8 s.
`review_coast_generator.py` rendert70 s über die echte Engine mit10-ms-
Main-Steuerung, Dry/Room bei64/512 Frames jeweils bytegleich. Alle Quellen
enden natürlich; kein Clear, Schlussfade, SHAPE-Bypass oder direkter Quellenstart.
LISTEN−23LUFS/−10,3dBFS True Peak, fester Gain. Die ursprüngliche positive
Host-Probe bleibt Referenz; diese neue Firmwareprobe braucht ein eigenes
Hörurteil. Vollständige Prüfungen und Ressourcen:
[COAST_GENERATOR_CHECKPOINT.md](COAST_GENERATOR_CHECKPOINT.md).

### Positiv gehörte Host-Referenz aus PR145

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

## Prüfbefund der ursprünglichen27-s-Host-Referenz

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

## Grenzen der Host-Referenz und nächste Arbeit

PR145 war ein **Host-World-Algorithmus**. Seine direkte Hörprobe verwendet ein
weiteres Register und eigene kürzere Hüllkurven. D5/B4 erweiterten den damaligen Produktregister-Kandidaten;
pro Rolle werden direkte Attackwerte und kurzer SHAPE-Release=0 verwendet.
Produktquellen, Header, Defaults und UI wurden nicht geändert. Der Raum darf
am Dateiende einen leisen Resttail enthalten; kein vollständiges Room-Ende
behauptet. Die COAST-Richtung wurde erneut positiv gehört; abschließende Geräteabnahme bleibt offen.

COAST und langes WOODLAND erfüllen jetzt ihren Rollen-/Register-Vertrag im
realen Generator; Releases und Raumgedächtnis bleiben ausdrücklich wirksam.
HIGHLANDS bleibt ein getrennter Host-Algorithmus. Die historischen Quellenproben
unten behalten ihre jeweils dokumentierte direkte Artikulation.
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

Die kurzen WOODLAND-/HIGHLANDS-Ensembles unten dokumentieren PR146.
Kurzes WOODLAND ist verworfen und durch die lange integrierte Richtung ersetzt.
Die folgenden Integrationsgrenzen gelten jetzt noch für HIGHLANDS:

1. Manual und öffentliche World-Note-API begrenzen auf140..470 Hz und den engen
   Core-Vorrat. Private COAST-/WOODLAND-Erweiterungen gelten nicht für HIGHLANDS;
   dessen Zielregister muss eigenständig gewählt und geprüft werden.
2. `world_grammar_propose` liefert nur einzelne Ereignisse ohne Akkord-/Voicing-
   Vertrag; HIGHLANDS erlaubt bisher nur eine gehaltene Stimme. Der neue
   Akkordplan muss ausdrücklich mehrere Rollen und gemeinsame Töne verwalten.
3. Reale Releases und das 7,2-s-Raumgedächtnis dürfen Rollen nicht unplanbar
   blockieren. Gemeinsame Ziele müssen einmal klingen, Besitzer dürfen erst
   nach tatsächlichem Quellenausklang wiederverwendet werden.
4. Direct-SHAPE-Werte müssen in geprüfte Produkt-Makrogrenzen überführt werden;
   Dry/Room-Proben ersetzen keine echten Generator-/Übergangs-/Gerätechecks.

Die Host-Proben ändern keine Produktionsquelle oder Defaults. Die spätere
COAST-Integration ändert Engine/Grammatik/Header, aber keine Quellen-DSP-/Makro-
Defaults oder UI. Keine neuen Gesamtgates geschlossen; die abschließende
WOODLAND-/HIGHLANDS-Hörabnahme fehlt.


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
Die45-s-Probe war ein isolierter Quellenentwurf aus PR147; sie wird nicht als
bestätigte Hörwahl vorausgesetzt. Die echte Integration ist nun unten beschrieben.

## WOODLAND — tatsächliche lange Quelle und Akkordrollen

Die Product-Engine wählt jetzt nach Init und Clear dieselbe lange
Karplus-Strong-Artikulation für Manual/Generate: nominaler T6036 s, Attackbasis
0,8 s und musikalischer Stop2 s, jeweils innerhalb der normalen Produktmakros.
Color begrenzt die Schleifendämpfung auf0,025..0,005, default0,015.
Die zwei vorhandenen Delay-Linien bleiben; keine weitere Fläche oder
Wiederanregung. Manual-Key-up lässt den tatsächlichen Saitenton natürlich aus.
Generate plant lange Halteabsichten und ihren eigenen weichen Release.
Kontextwechsel können Releases auf100 ms verkürzen; Clear/Mute leeren die
gesamte Kette nach ihrem40-ms-Mastergate. Eine kürzere Rampe wird nie verlängert.

Der neue Main-Phrasenzustand plant sechs bestätigte Starts. Seed1234/D-Dur:
D3/F#3 → B3/F#3 → B3/D4 → F#3/D4 → F#3/A3. Gemeinsame Akkordtöne behalten
ihre realen Besitzer; pro Schritt wird nur eine Saite ersetzt. Halteabsichten
6..18,5 s, geplante ungleiche Zwischenräume2/12/9/4/7 s. Activity verändert
kommende Zwischenräume, nicht Haltezeit/Pegel/Color; tatsächliche Releases
und Pitchhistory können Schritte verlängern. Seed variiert kleine Akzente,
Haltezeiten und den Schluss zwischen Quinte und verwandtem Ton.

Nur echte DSP-Startbestätigung schaltet Phase/RNG weiter. Owner6/7 werden erst
nach tatsächlichem Ende wiederbelegt; Ruhe/Wiederkehr wartet beide Quellen ab.
Interner Generatorbereich MIDI45..68/105..470 Hz, gewählter Major-/Minor-Core;
Manual/öffentliche Note-API bleibt im bisherigen Register. HIGHLANDS- und
COAST-Algorithmen werden dadurch nicht vereinheitlicht.

Neue50-s-Dry/Room-Probe aus der tatsächlichen Engine, normale Makros,
keine direkten Quellenstarts oder temporäre DSP-Kopie: sechs Starts, max zwei
reale Quellen einschließlich Releases und natürliche Freigabe vor Dateiende.
64/512-Frame-PCM und Traces bei gleicher10-ms-Main-Steuerung bytegleich.
LISTEN−23LUFS/−12,2dBFS True Peak, ein konstanter Gain. Die neue Probe braucht
noch ein Hörurteil.48 vollständige Engine-Phrasen einschließlich realer gemeinsamer
Töne und aktuelle H743-Ressourcen:
[WOODLAND_GENERATOR_CHECKPOINT.md](WOODLAND_GENERATOR_CHECKPOINT.md).
