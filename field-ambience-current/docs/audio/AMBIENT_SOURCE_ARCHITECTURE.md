# AMBIENT — Quellen und Raum für drei Welten

> **Standhinweis 2026-10-06:** Die folgende Diagnose ist auf 2026-09-27
> datiert. Implizite autonome Begleitung, Pluck-Besitz/Stop und Generate-Zulassung
> wurden seither geändert. Aktueller Implementierungsnachweis:
> [WORLD_ROUTING_CHECKPOINT.md](WORLD_ROUTING_CHECKPOINT.md); verbindliche
> weitere Entscheidungen/Abnahme: [AMBIENT_SOUND_DESIGN_TODO.md](AMBIENT_SOUND_DESIGN_TODO.md).
> Alte manuelle Begleitung und vollständige Klangabnahme bleiben offen.

2026-09-27, Konzeptpaket 2. Ergänzt [AMBIENT_WORLD_SYSTEMS.md](AMBIENT_WORLD_SYSTEMS.md).
Codeprüfung am lokalen Commit 97d9440; Firmware seit dem vorherigen Konzeptpaket
unverändert. Codefakten, daraus abgeleitete Risiken und Entwurfsentscheidungen
werden getrennt. Keine Hörfreigabe, keine neue Aufnahme und kein DSP-Umbau.

## KERNURTEIL

Die vorhandenen Quellen liefern drei brauchbare technische Ausgangspunkte:
Bowed für COAST, Karplus-Strong Pluck für WOODLAND, die überarbeitete Hornquelle
für HIGHLANDS. Diese Auswahl ist eine begründete Startarchitektur, keine
Bestätigung ihres fertigen Klanges. Das größere Problem ist die Kopplung der
Quellen an gemeinsame Begleitung, Effektketten und Ereignisverwaltung. Eine
reine Änderung der World-Presets würde die neue Konzeption nicht realisieren.

## FUNDAMENTAL FALSCH

### 1. Ein musikalisches Ereignis erzeugt implizit zusätzliche Stimmen

Fakt: [engine.c](../../firmware-c-next/src/engine.c), engine_note_on, ruft im
Ambient-Pfad pad_note_on auf. Der Generator startet seine Melodien dort und
anschließend über melody_strike die World-Quelle. Zusätzlich existieren eigene
Begleitschichten. Das gilt nicht pauschal für den getrennten V2-Spielpfad.

Folge für den Entwurf: Ein HIGHLANDS-Fragment wäre nicht automatisch ein einzelner
Ton. Die Begleitung kann Pausen füllen; die Summe der realen Stimmen überschreitet
die bloß im neuen Konzept gezählten Ereignisse. Hörbare Maskierung bleibt zu prüfen.

Entscheidung: Ein autonomes Ereignis muss genau benennen, welche Quelle es
startet. Zusätzliche Stimmen werden eigens geplant und mitgezählt. Kein
impliziter Pad-Layer unter jedem Anschlag, kein verpflichtender Bassunterbau.

### 2. Ein freier musikalischer Slot ist noch kein freier Quellenslot

Fakt: [pluck.c](../../firmware-c-next/src/pluck.c) besitzt laut
[pluck.h](../../firmware-c-next/include/pluck.h) zwei Stimmen, keinen Note-off
und überschreibt bei der Allokation gegebenenfalls einen klingenden Slot.
Bowed und Horn besitzen jeweils drei aktive Slots und bereiten bei Belegung
eine kurze 353-Sample-Übergabe vor. Das ist kein gemeinsames Drei-Stimmen-Budget.

Entscheidung: Der neue Scheduler darf eine belegte Quelle nicht zum Retrigger
zwingen. Ein gemeinsames Register zählt aktive und ausklingende Quellen über
alle drei Familien. WOODLAND hat zusätzlich die lokale Grenze von zwei Plucks.
Abgewiesene Ereignisse werden nicht als gespielte Motive gespeichert.

## NOCH NICHT SELBSTVERSTÄNDLICH

### Befunde mit konkreter Konsequenz

| Bereich | Codefakt | Konsequenz für das nächste Sounddesign |
|---|---|---|
| Bowed | Zwei Saw-Oszillatoren mit fester Verstimmung; Körperbewegung 0,13 Hz; alle neuen Stimmen beginnen mit bodyPh=0 | Unregelmäßige Noten allein beseitigen keine periodische innere Klangbewegung. Vergleich gegen ruhenden Körper; phasenstarre Neuanfänge vermeiden |
| Bowed | Resonatoren bei 1,5× und 2× Tonfrequenz, Q 9/8 | Klangprägende Nebenresonanzen auf tonale Dominanz prüfen; nicht als unabhängige, harmonisch geprüfte Noten behandeln |
| Horn | Grundtonverstärkung, nichtresonanter Körperfilter, kein alter fester 950-Hz-Formant/Sub; weiterhin 0,9-Hz-Filterbewegung und 90-ms-Anblastextur | Ausgangspunkt ist bereits reduziert. Restbewegung/Anblastextur separat gegen Weglassen vergleichen |
| Pluck | Anregung durch gefiltertes Rauschen; Loop-Dämpfung beeinflusst spektralen Verlauf; zwei 1024-float-Puffer | Gedämpfte Saite ist vorhanden. Anschlagsfarbe, Grundtonstabilität und tatsächlich hörbares Ausklingen separat abstimmen |
| Pluck → Body | engine.c schickt Plucks durch body_process, dann zum Direkt- und Raumbus | Ein weicher Quellton ist nicht gleich der tatsächlich ausgegebenen Klangfarbe |
| Body | Feste, teils inharmonische Resonanzen pro World, Standardanteil 0,38 additiv; Worldwechsel setzt Resonatorzustände zurück | Ohne Body anfangen; gezielte Materialfärbung nur nach Vergleich behalten. Wechsel mit Restenergie gesondert behandeln |
| Shape | Globaler Attack-/Release-Faktor; Bowed/Horn lesen ihn beim Start, Pluck verwendet Release für den Loop-Gain | Ein globaler Parameter kann nicht ungeprüft alle World-Artikulationen sinnvoll steuern |
| Automatische Dynamik | melody_strike setzt Mindestpegel etwa 0,38 für Bowed und 0,34 für Horn | Eine leise vorgeschlagene Antwort kann am Quelleneingang angehoben werden. Explizite Rollenpegel statt unbemerkter Untergrenzen |

Die Frequenzen und Koeffizienten sind Codebefunde, keine Messung ihrer
Wahrnehmbarkeit. Feste Körperresonanzen sind nicht grundsätzlich falsch:
Materialklang darf sie besitzen. Für unsere Quellen dürfen sie Tonidentität
und geforderte Ruhe jedoch nicht dominieren. Kein vorsorgliches Komplettverbot.

Die T60-Konstante im Pluck-Code ist ein Entwurfsparameter des Loop-Gains.
Zusätzliche Dämpfung beeinflusst den tatsächlichen spektralen Abfall; daraus
folgt nicht, dass jeder Ton hörbar exakt 3,2 Sekunden ausklingt.

## LOCKED

Beibehalten als Architekturprinzip, nicht pauschal als freigegebener Klang:

- Gemeinsame Stimmung, konservative Berücksichtigung klingender Töne und
  begrenzte Kandidatensuche. Bei Konflikten pausieren statt erzwingen.
- Getrennte Direkt- und Send-Busse mit gemeinsamem Ausgang. Damit lässt sich
  räumlicher Zusammenhang herstellen, ohne die trockene Identität aufzugeben.
- Endliche Quellenslots und statische Audiopuffer als Ressourcenprinzip.
  Neue musikalische Grammatiken benötigen keine eigenen langen Audiopuffer.
- Bestehende Korrekturen an Grundtonerhalt und ruhigerem Hornkörper nicht
  durch alte Presets oder Kommentare wieder rückgängig machen.
- Autonomer Hörmodus und manuelles Spielen bleiben getrennt; keine automatische
  Umwidmung der sechs manuellen Synths zu sechs parallelen World-Stimmen.

## REMOVE / MERGE / REDESIGN

| Bestandteil | Entscheidung für den neuen autonomen Entwurf | Begründung |
|---|---|---|
| Bowed | COAST-Kandidat, Körperbewegung und Nebenresonanzen reduzieren/präzisieren | Bereits mehrstimmige gehaltene Quelle mit eigenem Release |
| Pluck | WOODLAND-Kandidat, Belegungs-/Release-Vertrag ergänzen | Physisch anders entstehender Ton mit natürlichem Abfall |
| Horn | HIGHLANDS-Kandidat mit klarer Verwerfungsbedingung | Andere Artikulation möglich, geringer Quellumfang; Pfeif-/Röhrenrisiko offen |
| Choir | Nicht als Kernquelle der drei neuen Welten | Zusätzliche Fläche, Vokalformant und Vibrato überschneiden sich mit Risiken; keine derzeit notwendige eigene Rolle |
| Guembri | Nicht als Kernquelle | Metallisches Schnarren und schnelle Helligkeitsbewegung widersprechen der gewünschten Ausgangsrichtung |
| Pad/Bass/Drone | Kein automatisch aktiver Unterbau | Rollen bei Bedarf bewusst besetzen, sonst Pause ermöglichen |
| Modal Body | Im ersten WOODLAND-Entwurf umgehen | Saitenidentität zuerst nachweisen; Körperfärbung danach begründet hinzufügen |
| Sechs V2-Synths | Manuellen Bestand erhalten; vorerst nicht für autonome Polyphonie portieren | Host arbeitet mit aktiver/vorheriger Engine; z. B. Bamboo-Kern hat globalen monophonen Zustand |
| Dream-Kette | Nicht gemeinsamer Pflichtpfad der neuen Worlds | Mehrere Klangveränderungen erschweren klare Unterschiede und Ursachenanalyse |

Diese Entscheidungen entfernen aktuell keinen Code, keine alten Presets und
keine manuellen Funktionen. Insbesondere folgt aus dem Ausschluss von Choir
keine Löschung einer gespeicherten Scene oder öffentlichen Quell-ID.

## BESTE VERSION

### Drei Klangfamilien, eine musikalische Verwaltung, ein Raum

COAST: Bowed-artige Quelle allein trägt den Ton; langsamere individuelle
Einsätze, ein bis drei tonale Stimmen einschließlich Release. Kein zweites Pad
für die vermeintlich fehlende Fülle. Wächst der Klang nur durch Chorus/Hall,
muss die Quelle zuerst verbessert werden.

WOODLAND: nackter Pluck als Ausgangspunkt, normalerweise ein bis zwei Stimmen.
Anregung wird weich, ohne ihre hörbare Kontur zu verlieren. Materialfärbung
ist eine dosierte Eigenschaft dieser Quelle, kein obligatorischer nachgeschalteter
World-Filter. Die Antwort unterscheidet sich in Lage/Artikulation, benötigt
aber nicht zwingend eine zweite Synthesefamilie.

HIGHLANDS: Horn-artiger Ton mit warmer Obertonstruktur, normalerweise eine
Stimme, gelegentlich zwei. Der gehaltene Teil darf nicht wie ein Sinus-Piepser
oder eine stationäre Orgel wirken. Zuerst spektralen Verlauf und Artikulation
formen; Hall und Hintergrundstimmen dürfen einen unpassenden Kern nicht kaschieren.
Scheitert dieser Kandidat, einen kleinen harmonischen Tonkörper mit separat
geformten Obertonhüllkurven prüfen. Das wäre ein Ersatz, keine vierte Zusatzengine.

### Gemeinsamer Quellenvertrag

Für jede aktive Quelle braucht die Verwaltung mindestens: Ereignis-ID,
Quellenfamilie, tatsächliche Tonhöhe, Rolle, Pegel, Startzeit, Releasezustand
und Ende/konservative Fahne. Diese Angaben sind konzeptionelle Felder, noch
kein behauptetes vorhandenes API oder Speicherlayout.

- start darf fehlende Kapazität melden, ohne eine klingende Stimme zu opfern.
- release beendet die Anregung/Hüllkurve weich; Pluck benötigt einen expliziten
  sanften Dämpf-/Stop-Pfad für Ausstieg und Notfälle.
- active meldet reale Quellenbelegung einschließlich Übergabe und Release.
- render schreibt Direkt- und Raumsignal mit nachvollziehbaren Pegeln.
- Ereignis-ID bleibt über Worldwechsel gültig; alte Releases werden nicht der
  neu ausgewählten Quelle zugeordnet.
- Nur tatsächlich klingende Quellen belegen musikalische Slots. Raumfahnen
  behalten harmonische Relevanz auch nach Freigabe eines Quellenslots.

### Minimaler Effektweg

Quelle → dosierte eigene Klangformung → Direktbus plus Send → gemeinsamer
Raum → gemeinsame Ausgangsregelung. Der Raum ist dieselbe technische Instanz,
aber mit unterschiedlichen Zielwerten für die Welt. Vorhandenen Prozessor
gezielt nutzen statt drei neue Reverbs anzulegen.

| Welt | Direktheit | Raumverhalten | Im ersten Entwurf deaktiviert |
|---|---|---|---|
| COAST | Ton bleibt tragend | Diffuser, breiter, längerer Ausklang | Chorus, Blur, Shimmer, hörbarer Echo-Puls |
| WOODLAND | Am nächsten und klarsten | Kürzer, sparsame Verbindung | Modal Body zunächst, Dream-Inserts, rhythmisches Delay |
| HIGHLANDS | Klarer Ton aus etwas Distanz | Rückzug vor nächstem Fragment | Lange Dauerfahne, Pitch-Shimmer, Auto-Pan, wiederkehrendes Echo |

Fakt: [ambient_effects.c](../../firmware-c-next/src/ambient_effects.c) führt
in DREAM_CHAIN Tape, Chorus, Blur, Echo und Reverb mit Shimmer-Parameter aus.
Die Reverb-Variante ohne diese Kette ist als vorhandener Ausgangspunkt zu prüfen.
Null gesetzte Mischanteile garantieren keine CPU-Ersparnis; Bypass/Allokation
müssen gesondert betrachtet werden. Neue Zahlenwerte für Raum erst nach
Quellenvergleich bestimmen, nicht aus Landschaftsnamen ableiten.

### Übergänge und Ressourcen

Ein geteiltes Drei-Stimmen-Budget umfasst alte und neue Quellen. Die vorhandenen
Familienpools können zunächst bestehen bleiben; nur insgesamt drei aktive
Quellen zuzulassen spart nicht automatisch deren statisch reservierten RAM.
Die zwei bestehenden Pluck-Delaypuffer beanspruchen zusammen 8192 Bytes reine
float-Daten bei 4-Byte-float, ohne Verwaltungszustand. Weitere Kopien sind für
den Entwurf nicht erforderlich. Aktuelle freie Speicherbereiche und Spitzenlast
sind damit nicht bestimmt.

Beim Worldwechsel Quelle zuerst übergeben, vorhandene Raumenergie weiterführen.
Nur im tatsächlich gewählten Effektalgorithmus stabil glättbare Parameter live
ändern. Kein blindes Verstellen laufender Delay-Längen, kein Reset voller
Resonatoren. Wenn eine Änderung so nicht möglich ist, Send auslaufen lassen
und den Raum später kontrolliert umstellen. Keine zweite vollständige Effektkette
als ungeprüfte Standardlösung.

## TEST AM GERÄT

Erst nach Konzeptabschluss und Sounddesign. Zunächst spätere Software-/Hörpakete:

1. Quellenisolierung: jeder World-Einsatz aktiviert genau die geplanten Quellen;
   kein verstecktes Pad, Bass oder Naturgeräusch. Aktive Slots inklusive Release zählen.
2. COAST: Bowed trocken mit/ohne periodische Körperbewegung und Nebenresonanzen;
   Grundton, Mono und mehrere Tonlagen vergleichen, je Export höchstens 30 s.
3. WOODLAND: nackter Pluck gegen bisherigen Body-Pfad; Anschläge und Ausklänge,
   volle Slots, sanfter Stop und schnelle Zielwechsel separat prüfen.
4. HIGHLANDS: aktueller Ton gegen reduzierte Bewegung/Anblastextur; mit gleichen
   Tonhöhen und Raum gegen COAST. Bei mangelnder Eigenständigkeit Quelle ersetzen.
5. Raum erst danach: Quelle trocken/mit Raum, dann paarweise Worldwechsel.
   Kein Pegelvorteil darf die bevorzugte Variante bestimmen.
6. Reales Gerät: Hardwareausgänge, Mono, Speaker/Kopfhörer, Spitzenlast bei
   Übergängen, Speicherbelegung und erreichbares Stop prüfen.

Aktuell abgeschlossen: Codebezug, Quellenauswahl, Ausschlüsse und notwendige
Schnittstellen sind festgelegt. Nicht abgeschlossen: Klangabnahme, endgültige
Effektwerte, Firmwareumbau und Hardwaremessung.

Der vollständige Ablauf ist inzwischen in
[AMBIENT_PRODUCT_BRIEF.md](AMBIENT_PRODUCT_BRIEF.md) zusammengeführt.
Der Brief empfiehlt zusätzlich dieselbe World-Palette beim manuellen Spielen;
das ist keine stillschweigende Freigabe oder Löschung des bisherigen Synth-Katalogs.
Nach gemeinsamer Konzeptfreigabe folgt
[AMBIENT_IMPLEMENTATION_SEQUENCE.md](AMBIENT_IMPLEMENTATION_SEQUENCE.md),
zunächst Quellen-Diagnose und explizites Routing. Keine erneute offene Suche
nach zusätzlichen Welten nötig.
