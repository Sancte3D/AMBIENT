# AMBIENT — drei eigenständige musikalische Welten

Stand: 2026-09-27. Konzeptentwurf v0.2, ausgearbeitet aus der Nutzerrichtung.
Reihenfolge: Konzept → Sounddesign → UX/UI/Display → echtes Gerät.
Dieses Dokument ändert weder Firmware noch Presets oder gespeicherte Scenes.
Arbeitsnamen und Zahlen sind Entwurfswerte, keine Hör- oder Gerätefreigabe.

## 1. Produktauftrag und Entscheidungen

AMBIENT erzeugt eigenständig ruhige, harmonisch zusammenhängende musikalische
Umgebungen. COAST, WOODLAND und HIGHLANDS sollen bereits durch ihr zeitliches
Verhalten unterscheidbar sein. Jede Welt muss zusätzlich eine eigene Klangfarbe
erhalten; gleiche Musik mit unterschiedlichen Instrumenten genügt nicht.

Für diesen Entwurf entschieden:

- Drei verschiedene Ereignissysteme unter gemeinsamer musikalischer Führung.
- Naturorte geben die vorstellbare Umgebung vor. Geschichte und Artefakte
  bleiben eine mögliche, kuratierte Identitätsebene. Keine erfundene historische
  Authentizität und keine verpflichtende Kultur pro Landschaft.
- Generatives Zuhören bleibt der bewusst gestartete Modus mit gesperrten
  Spielflächen. Manuelles Spielen bleibt ein eigener Nutzungskontext.
- Pro Welt ein tragendes Klangprinzip; Zusatzrollen müssen diesem dienen.
- Kein verpflichtender Bass, Dauerpad, Wind oder Rauschteppich in jeder Welt.
- Keine neue World nur wegen eines anderen Namens, Modus oder Hallanteils.
- Freie Überlagerung beliebig vieler Welten gehört nicht zum Entwurf v0.2.
  Harmonieren bedeutet zunächst innerhalb einer Welt und beim Weltwechsel.

Beruhigung ist die Gestaltungsabsicht, keine Wirkungsgarantie für jeden Menschen.
Lange Töne sind erlaubt. Zu vermeiden sind als Brummen, Summen, Piepen, Sirene
oder aufdringlicher Rhythmus wahrgenommene Ergebnisse, nicht pauschal Sustain.

## 2. Tatsächlicher Ausgangspunkt

Geprüft am lokalen Commit f8477aed3e97ddd0ecee31bf916366f17317c6b5.
Der bisherige Remote-Checkpoint ist 7017f903efd6f5441e1f7ed2fdef95d68b23a2d8.

- [worlds.c](../../firmware-c-next/src/worlds.c) definiert Alps, Open Sea,
  Fjords, Moss Fields und Desert über Voice, Harmonie, Effektwerte sowie eine
  Tabelle mit Tonlängen, Pausen und Dichte. Das ist kein Beleg für fünf
  grundsätzlich verschiedene Kompositionssysteme.
- [engine.c](../../firmware-c-next/src/engine.c), engine_generative_tick,
  liest worlds_phrase und führt die gemeinsame lange Melodiestimme mit
  Phrasenlänge 2–5, Wiederaufnahme früherer Motive, Kollisionsprüfung und
  gemeinsamen Begleitschichten. Diese vorhandenen Schutzmechanismen sind
  nützlich; das gemeinsame Ablaufmodell ist der Ansatzpunkt der Neugestaltung.
- Der [H743-Linkerplan](../../firmware-c-next/src/hal_h743/stm32h743_flash.ld)
  beschreibt getrennte Speicherbereiche und reservierten Scene-Flash.
  Aus den Bereichsgrößen folgt keine freie RAM- oder CPU-Reserve.

Die neuen drei Welten sind Kandidaten für eine reduzierte Produktarchitektur,
keine zusätzlichen drei Einträge und keine automatische Umbenennung der fünf
alten IDs. Migration wird erst bei der Implementierung gesondert entschieden.

## 3. Gemeinsamer musikalischer Vertrag

### Harmonie und Register

Ein gemeinsamer Kontext verwaltet Stimmung, tonales Zentrum, gerade erlaubte
Zusammenklänge, klingende Stimmen und konservativ berücksichtigte Raumfahnen.
Jede Welt schlägt Ereignisse vor; der gemeinsame Kontext entscheidet über deren
musikalische Zulässigkeit. Gleiche Tonleiter allein reicht dafür nicht.

Für den ersten späteren Vergleich: gleiches tonales Zentrum D und zunächst
dieselbe Stimmung, damit Tonartunterschiede keine World-Identität vortäuschen.
Beispielhafte weit gesetzte Voicings: D3–A3–E4, D3–A3–F4, D3–A3–C4.
Sie sind keine feste Akkordfolge und kein universelles Entspannungsrezept.
Beim Wechsel E4 → F4 wird E4 einschließlich relevanter Fahne berücksichtigt.
D und A können verbinden. Jede Welt darf Töne weglassen oder oktavieren,
solange Register- und Kollisionsregeln erhalten bleiben.

Unteres bis mittleres Register als Ausgangspunkt, keine automatische
Oktavwanderung nach oben. Bestehendes D3–A4 ist eine Ausgangsreferenz, kein
belegtes Wohlfühlband für jede Klangquelle. Körper entsteht aus tragfähigen
Obertönen, nicht aus einer verpflichtenden Suboktave. Andere Tonarten werden
später innerhalb geprüfter Register gesetzt, nicht blind beliebig transponiert.

### Belegung, Pegel und Bewegung

- Vorläufig höchstens drei gleichzeitig allozierte tonale Stimmen insgesamt;
  ausklingende Quellen zählen mit. Raumfahnen bleiben zusätzlich im harmonischen
  Gedächtnis. Das ist ein vorgeschlagenes Budget, keine Aussage zur bisherigen
  Polyphonie oder zum Prozessorbedarf einer Stimme.
- Maximal eine Stimme übernimmt gleichzeitig die tiefste Rolle. Keine
  automatische Verdopplung des Grundtons durch mehrere unabhängige Systeme.
- Ein fester, später kalibrierter Pegel pro Quellenrolle; keine laufende
  Lautheitsregelung, die Pausen hochzieht oder beim Wechsel pumpt.
- Neue Eintritte berücksichtigen bereits vorhandene Energie. Ein konservativer
  Pegel-/Belegungsansatz ersetzt keine spätere Peak-Prüfung oder Ausgangsstufe.
- Lautstärke, Tonhöhe, Helligkeit und Stereoposition bewegen sich nicht alle
  gleichzeitig. Tonhöhenmodulation ist zunächst kein Identitätsträger.
- Stereobreite darf in Mono nicht den Tonkörper verschwinden lassen. Positionen
  werden pro Ereignis gesetzt oder langsam entwickelt, nicht dauernd gewürfelt.
- Eine ausklingende Stimme wird nicht hart für einen neuen Ton abgeschnitten.
  Fehlt ein Slot, darf das neue Ereignis entfallen oder später neu geplant werden.

## 4. COAST — Stimmen übergeben den Zusammenhang

**Ort:** geschützte Bucht, weiter Horizont, ruhige Oberfläche.
**Hörbare Signatur:** weich einsetzende, überlappende Mehrstimmigkeit ohne
gemeinsames Anschwellen. Ein heller Moment darf entstehen, ohne zu glitzern.

| Rolle | Aufgabe | Entwurfsgrenze |
|---|---|---|
| Tragender Ton | Zeitweilige Orientierung im unteren/mittleren Register | Kein ununterbrochener Pedalton |
| Verbindender Ton | Gemeinsamen Ton beim Voicingwechsel halten | Individuelle Hüllkurve, kein globaler Wellen-LFO |
| Farbton | Einen Zusammenklang kurz öffnen | Nur bei Platz, kein obligatorischer dritter Layer |

Klangstart: obertonreiche, sanft aufgebaute Töne mit eigenem Körper. Kandidat
ist eine kontrollierte harmonische Synthese oder überarbeitete gestrichene
Quelle; starkes Unisono/Chorus ist keine Voraussetzung für Breite.

Ereignisgrammatik:
1. EINZELN: Ein Ton beginnt; es gibt noch keinen vollständigen Akkord.
2. VERBINDEN: Ein passender Ton ergänzt ihn, während der erste zurücktritt.
3. ÖFFNEN: Optionaler Farbton; nur ein Aspekt verändert sich deutlich.
4. AUSLICHTEN: Eine oder mehrere Stimmen enden. Ein Anschluss kann bestehen
   bleiben, vollständiges Ausklingen ist ebenfalls zulässig.

EINZELN darf direkt zu AUSLICHTEN führen. VERBINDEN darf erneut VERBINDEN
werden. ÖFFNEN ist optional; die Zustände bilden keinen festen Viererschritt.
Vorläufige Zeitbereiche: Ton inklusive Quellenrelease 8–24 s; neue
Eintrittsgelegenheiten nach 4–10 s. Überbelegung führt zum Auslassen.
Zeiten werden innerhalb einer Episode verwandt gewählt und langsam verändert;
keine identische Folge und kein gleichverteilter Zufall pro Audioblock.

Raum: diffuse, zurückhaltende Verbindung; Direktsignal bleibt tragend.
Kein auffälliges Echo, keine Pitch-Shimmer-Oktave im Basischarakter.
Hauptgefahr: homogene Synthfläche oder periodisches Pumpen.
Gegenprobe: ohne Hall muss die Übergabe zwischen Stimmen noch hörbar sein.

## 5. WOODLAND — Ereignisse besitzen Beziehungen

**Ort:** lichter Laubwald, nahe Details, gedämpfter Hintergrund.
**Hörbare Signatur:** runde Saitenansätze, kleine Antworten, wechselnde Abstände.

| Rolle | Aufgabe | Entwurfsgrenze |
|---|---|---|
| Naher Ton | Einen kleinen Gedanken beginnen | Kein Klick, kein dauerhaft heller Anschlag |
| Antwort | Einen Aspekt des Gedankens aufnehmen | Darf entfallen; kein ständiger Pingpong-Dialog |
| Körperresonanz | Gespielten Ton räumlich/materialhaft verbinden | Kein zusätzliches autonomes Dauergeräusch |

Klangstart: gedämpfte Saite, zurückhaltender Resonanzkörper, erkennbare
Grundtonlage. Ein Delay-Saitenmodell oder kleiner Anregungs-/Resonatoransatz
ist ein späterer Kandidat. Keine Festlegung auf ein exotisches Instrument.

Ereignisgrammatik:
1. KEIM: Ein Ton oder eine kleine Figur aus zwei bis drei Tönen erscheint.
2. ANTWORT: Ein späteres Ereignis übernimmt Kontur, Zeitabstand oder Endton.
3. VARIANTE: Genau ein Merkmal wechselt, etwa ausgelassener Ton oder andere Lage.
4. RUHE: Keine neue Figur; vorhandene Körper klingen aus.

KEIM kann unbeantwortet bleiben. Eine Episode umfasst zunächst ein bis drei
Figuren; danach folgt RUHE. Keine unbegrenzte Verkettung von Antworten.
Vorläufig: 2–6 s zwischen Einsätzen innerhalb einer Figur, 6–16 s zwischen
Figuren; Quellenklang 2–8 s. Normal ein bis zwei Stimmen, gemeinsame Obergrenze
drei inklusive Release. Eine zulässige Pause ist besser als ein erzwungener Ton.

Raum: nähere Quelle und zurückhaltende frühe Reflexionen. Lange diffuse
Raumenergie bleibt geringer als bei COAST. Resonanzen müssen mit dem
gespielten Ton zusammenpassen, ohne alle Töne nasal gleich einzufärben.
Hauptgefahr: Spieluhr, Arpeggiator oder zufälliges Plinkern.
Gegenprobe: eine Antwort muss anhand eines konkreten übernommenen Merkmals
im Ereignisprotokoll erklärbar sein; bloße zeitliche Nachbarschaft genügt nicht.

## 6. HIGHLANDS — Fragmente erhalten durch Pausen Gewicht

**Ort:** weite Graslandschaft, milde Luft, großer Abstand zwischen Ereignissen.
**Hörbare Signatur:** weiche artikulierte Tonkörper, wenig Gleichzeitigkeit,
kurze unaufdringliche Fragmente. Kein Hornruf als geografisches Pflichtmotiv.

| Rolle | Aufgabe | Entwurfsgrenze |
|---|---|---|
| Getragener Ton | Ein kurzes Fragment ermöglichen | Keine exponierte Lead-Melodie |
| Entfernte Antwort | Gelegentlich einen Ton aufnehmen | Leiser, erst bei freiem musikalischem Platz |

Klangstart: artikulierter, obertonreicher Ton mit leichter Bläseranmutung,
ohne aufdringliche feste Formanten, Suboktave oder gleichmäßiges Vibrato.
Die bestehende Hornquelle ist ein zu prüfender Kandidat, keine gesetzte Lösung.
Diese Welt trägt das größte bisher ungelöste Timbre-Risiko: Röhren-/Pfeifton.

Ereignisgrammatik:
1. TON: Ein Ton erscheint und entwickelt sich geringfügig in der Klangfarbe.
2. FRAGMENT: Optional ein oder zwei weitere Töne, überwiegend kleine Schritte.
3. FREIGEBEN: Quellen enden; der Raum klingt zurück.
4. PAUSE: Die nächste Entscheidung ist bewusst noch nicht fällig.

TON kann direkt zu FREIGEBEN führen. Eine Antwort ist optional und verbraucht
dasselbe Stimmen-/Dichtebudget. Vorläufig: 2–7 s pro Ton, 1,5–4 s Abstand
zwischen Tönen eines Fragments, danach 6–16 s ohne neues Fragment. Normal
eine Stimme, höchstens zwei; kein permanenter Hintergrundpad.

Raum: hörbarer Direktkörper, zurücktretender Nachklang, keine Kathedralenfahne.
Weite entsteht durch zeitlichen und räumlichen Abstand. Kein starkes Auto-Pan.
Hauptgefahr: Signalruf, traurige Solomelodie oder COAST mit weniger Stimmen.
Gegenprobe: nicht nur gleiche Quelle mit anderem Hall; Artikulation und spektraler
Verlauf müssen bei angeglichenem Raum eindeutig anders wirken.

## 7. Gedächtnis, Variation und begrenzter Zufall

Ein Seed bestimmt eine reproduzierbare Ausprägung, zusammen mit Algorithmus-
version und Nutzereingaben. Er ist kein Versprechen sampleidentischer Ausgabe
über unterschiedliche Plattformen oder spätere Firmwareversionen hinweg.

Gespeichert werden vorgeschlagen: World-ID, Ereigniszustand, nächste Frist,
Voicing, letzte acht Ereignisse und zwei kurze Motiv-/Fragmentbeschreibungen.
Es werden keine laufenden Audioaufnahmen als musikalisches Gedächtnis benötigt.
Speichergröße folgt erst aus Datentypen und Implementierung.

Eine Episode hält ihre Identität etwa 45–120 s als erster Entwurfsbereich.
Ihr Ende erzwingt weder neuen Akkord noch Dramaturgie. Nur ein Merkmal wird
zur nächsten Episode deutlich verschoben: Dichte, Motiv, Artikulation oder
Harmonie. Auch unverändertes Fortsetzen bleibt möglich. Keine Pflichtsteigerung.

Die letzten Ereignisse verhindern mechanische exakte Wiederholung, verbieten
aber keine ruhigen wiederkehrenden Töne. Zwanghafte Neuheit wäre ebenfalls
unruhig. Bei unzulässigem Vorschlag wird zunächst Zeit/Lage angepasst,
dann eine kompatible Alternative gesucht. Nach begrenzter Suche wird pausiert;
niemals für musikalische Vielfalt die Kollisionsregeln umgehen.

Konzeptioneller Ablauf pro fälliger Entscheidung:
1. Releases und tatsächliche/konservativ geschätzte Belegung aktualisieren.
2. World-Grammatik schlägt Rolle, Kontur und Zeitfenster vor.
3. Gemeinsamer Kontext prüft Register, Zusammenklang, Slot- und Pegelbudget.
4. Zulässiges Ereignis sanft starten, sonst neue Gelegenheit planen.
5. Nur tatsächlich gestartete Ereignisse als musikalische Erinnerung verbuchen.

## 8. Generate, Varianten und Worldwechsel

Generate bleibt nach dem bestehenden Nutzerauftrag der Eintritt/Austritt des
autonomen Hörmodus. Spielflächen sowie Hold/Drone bleiben darin gesperrt;
manuelle Auswahl wird gemerkt. Automatisch erzeugte Ereignisse zählen nicht
als menschliche Aktivität für die spätere Displayruhe nach 15 Minuten.
Konkrete LED-Animation und Bedienbelegung werden erst in der UX-Phase entschieden.

Korrektur der bisherigen Gesprächsentwürfe: Generate wird nicht stillschweigend
zusätzlich zu einem New-Seed-Knopf umdefiniert. Eine neue Variante ist vorerst
eine interne Fähigkeit; ihre Auslösung wird später in die Bedienung eingeordnet.
Wiedereintritt soll den Zusammenhang bewahren statt jedes Mal neu zu würfeln.

Worldwechsel:
1. Neue World als Ziel setzen, keine weiteren alten Ereignisse planen.
2. Alte Releases und Raumfahnen bleiben im gemeinsamen Kontext aktiv.
3. Eine kompatible neue Stimme darf freie Kapazität übernehmen; gemeinsames
   tonales Zentrum zunächst halten, andere Harmonie erst später entwickeln.
4. Quelle/Artikulation zuerst wechseln; Raumparameter erst danach und nur mit
   einer technisch stabilen Übergabe. Delay-Längen nicht ungeprüft live verziehen.
5. Ohne freien Slot warten, statt eine zweite vollständige Engine zu starten.

Bei rascher Mehrfachauswahl gilt das zuletzt gewählte Ziel. Eine laufende Fahne
wird deshalb nicht neu gestartet; keine Warteschlange vollständiger Worldwechsel.
Eine wirklich unverträgliche Harmonie wartet auf ausreichend abgeklungene Töne.
Es wird kein Pitch-Glide über einen gehaltenen Akkord zur Abkürzung verlangt.
Stop/Lautstärke müssen jederzeit erreichbar bleiben; technische Fehler dürfen
keinen unbegrenzten Übergang verursachen. Ein späterer Timeout muss weich
beenden, statt dauerhaft hängende Stimmen zu akzeptieren.

## 9. Effekte und technische Reduktion

Vorgeschlagen ist ein gemeinsamer Raumprozessor mit World-spezifischer
Abstimmung und dosierten Sends. Die Identität muss ohne diesen Raum bestehen.
Keine neuen drei parallelen Hallnetze, keine verpflichtende Shimmer-, Chorus-,
Blur-, Echo- und Tape-Kette. Ein zusätzlicher Effekt benötigt eine hörbare Aufgabe.
Eigenrauschen, künstliches Brummen und periodischer Wind bleiben ausgeschlossen.
Naturtexturen sind zunächst aus dem Identitätsvergleich entfernt; ihre spätere
Rolle bleibt offen und darf Tonklarheit nicht verdecken.

Realisierbarkeitsannahme: gemeinsame Harmonie-/Ereignisverwaltung und begrenzte
Quellenzahl erlauben einen überschaubaren Entwurf. Neue Quellen können sehr
verschiedene Kosten pro Stimme haben. Bestehende Puffer, Zustände, Speicherbänke,
Renderzeiten und maximale Übergangslast müssen vor ihrer Implementierungsfreigabe
am tatsächlichen Build geprüft werden. Keine RAM-/CPU-Zusage aus diesem Konzept.

## 10. Durchgespielte Fehlerfälle und Entscheidungen

| Fall | Entscheidung im Entwurf | Noch zu beweisen |
|---|---|---|
| Drei Quellen klingen, neue World gewählt | Eintritt wartet oder übernimmt frei gewordenen Slot | Kontinuität ohne lange gefühlte Verzögerung |
| COAST enthält E4-Fahne, neue Farbe verlangt F4 | Andere zulässige Lage/Ton oder warten | Tail-Modell konservativ genug, nicht blockierend |
| WOODLAND beantwortet dieselbe Figur mehrfach | Nach begrenzter Episode Ruhe, später ein Merkmal ändern | Wiedererkennung ohne Ohrwurm/Mechanik |
| HIGHLANDS bleibt in einem Zweitonruf hängen | Kontur aussetzen/verkürzen; Pausen allein genügen nicht | Kein Alarmcharakter bei mehreren Seeds |
| Hall maskiert alle Quellenunterschiede | Send/Decay reduzieren; Identität trocken prüfen | Direktheit bei leiser Wiedergabe |
| Langer Ton wird als Brummen erlebt | Quelle/Voicing/Artikulation prüfen, kein pauschales Sustain-Verbot | Individuelle Hörbeurteilung |
| Kein Ton ist zulässig | Pause; keine Noten erzwingen | Begrenzte Suche und musikalische Wiederaufnahme |
| Mehrere Worldwechsel schnell hintereinander | Letztes Ziel, gemeinsames Slotbudget, kein Engine-Stapeln | Worst-Case-RAM/CPU und Klickfreiheit |

Diese Fälle sind konzeptionell durchgearbeitet, nicht als Softwaretests ausgeführt.

## 11. Abnahme und nächste abgeschlossene Pakete

Konzeptstand: drei unterscheidbare Grammatiken, gemeinsame Regeln, Übergänge
und Umfang sind beschrieben. Hörbare Unterscheidbarkeit und Beruhigung bleiben
Hypothesen. HIGHLANDS-Timbre und die Anzahl der endgültigen Worlds bleiben offen.
Dies ist keine automatische Freigabe sämtlicher späterer Implementierungsdetails.

Nach Abschluss der Konzeptphase:
1. Pro World ein trockener Quellenentwurf, jeweils maximal 30 s; gleiche
   Tonhöhen und vergleichbare Lautheit. Zuerst Körper und Artikulation beurteilen.
2. Die drei Ereignisgrammatiken in kleinen getrennten Paketen umsetzen.
   Nachweisen: zulässige Ereignisse, begrenzte Suche, Stimmenbudget, Reproduzierbarkeit.
3. Erst dann Raum hinzufügen; anschließend Übergänge inklusive Fehlerfälle.
4. UX/UI/Display aus dem dann konkreten Verhalten gestalten.
5. Echtes Gerät: Ausgangsklang, Lautstärkebereiche, Mono/Kopfhörer/Speaker,
   Haptik, Speicher und Spitzenlast prüfen. Kein Softwarewert ersetzt diese Phase.

Spätere Identitätsprüfung: verdeckte World-Namen, mindestens mehrere Seeds,
je Ausschnitt 20–30 s. Nach kurzer Einführung muss Zuordnung bei gleichen
Tonhöhen und vergleichbarer Lautheit nachvollziehbar möglich sein. Verwechslung
nennen, nicht durch unterschiedliche Pegel oder Naturaufnahmen kaschieren.
Langzeitverhalten kann intern länger untersucht werden; ausgegebene Hördateien
bleiben ohne ausdrücklichen Nutzerwunsch höchstens 30 s lang.

Quellen-/Effektarchitektur inzwischen ausgearbeitet:
[AMBIENT_SOURCE_ARCHITECTURE.md](AMBIENT_SOURCE_ARCHITECTURE.md).
Bowed, Pluck und Horn sind die drei Startkandidaten; gemeinsame Quellenverwaltung
und ein reduzierter Raumweg ersetzen implizite Begleitung als Entwurfsprinzip.
Vollständiger Nutzungsablauf inzwischen in
[AMBIENT_PRODUCT_BRIEF.md](AMBIENT_PRODUCT_BRIEF.md), kleine Umsetzungspakete in
[AMBIENT_IMPLEMENTATION_SEQUENCE.md](AMBIENT_IMPLEMENTATION_SEQUENCE.md).
Neue manuelle Palette ist eine Empfehlung im Brief, keine rückwirkende
Nutzerfreigabe. Keine weitere neue World vor Abschluss dieser Auswahl.
