# SD17 — Langfristige Entwicklung der tatsächlichen World-Generatoren

Stand2026-10-09, Product Candidate0.7, ein einzelner Softwarecheckpoint nach
PR150. Hör-/Quellen-/Geräteabnahme bleibt offen;24 geschlossene/30 offene
Gesamtaufgaben. `reference` bleibt Default, Journal OFF. Keine UI-Änderung.

## Musikalische Änderung

Alle drei integrierten Rollenalgorithmen behalten ihren Anfang. Danach führen
sie durch Ausgangsphrase, begrenzte Variation, verwandte Akkordlage und erinnerte
Ausgangsphrase. Nach der ersten Rückkehr variiert der Abstand zwischen zwei und
vier Zwischenphrasen. Der lebende Zufallskanal läuft weiter; Audioquellen, Raum
und Pitchhistory werden weder beim Phrasenende noch bei Rückkehr neu gestartet.

- COAST variiert die obere Gegenbewegung bei gleicher unterer Route und
  gemeinsamem gehaltenen Akkordton.
- WOODLAND wechselt den Schlusston der langen Zwei-Saiten-Phrase.
- HIGHLANDS nimmt die obere Quinte eine Oktave herunter. Der folgende Bass
  verwendet deren exakte gestimmte Lage erst nach realer Besitzerfreigabe,
  statt ein enges B3/A3-Paar gegen den tiefen Nachklang vorzuschlagen.

Die verwandte Lage verwendet die relative Minor-/Major-Pentatonik mit exakt
denselben Pitch Classes. D-Dur bekommt B-Moll-Voicings, D-Moll F-Dur-Voicings.
Globaler Key, Collection, Tuning und gehaltene Hz bleiben gleich. Damit bleiben
vorherige und neue Quellen und Tails in derselben erlaubten Sammlung. Normale tatsächliche
Intervallprüfung entscheidet weiterhin jeden einzelnen Einsatz.

Erst der letzte echte DSP-Start einer vollständigen Phrase lernt Opening-Seed
und Route. Abgebrochene Vorbereitung/Teilphrase erweitert das Gedächtnis nicht;
eine neue vollständige erste Phrase lernt ihren eigenen Anfang. Erinnerungen
rekonstruieren damalige Tonhöhen, Rollen, Akzente und Halte-/Abstandsabsichten.
Hold/Gap-Faktoren sind1 /1,08 /1,12 /0,96 für Home/Variation/Related/Return.
Activity skaliert dabei die nominalen Abstandsabsichten mit dem aktuellen
Wert. Reale Einsätze bleiben von Besitzern, Release und Tails abhängig.
Die HIGHLANDS-Quellenpausen folgen ihrem bisherigen Seed-/Activity-Wert.
Das ist eine reproduzierbare musikalische Regel, keine Garantie einer gelungenen
Langzeitform oder eines bestimmten menschlichen Wiedererkennens.

Gedächtnis:24B fester Main-State pro World, keine Queue, Allokation oder neue
Quelle. Erinnerungsrekonstruktion maximal sieben LCG-Schritte. Der alte reine
Einzelton-Grammatikhelfer bleibt historischer Testcode; neuer Nachweis führt
die realen Rollen-Generatoren aus. Gemeinsame Drei-Slot-Grenze, lokale Zwei-
Saiten-Grenze, private/öffentliche Register und normale SHAPE bleiben wirksam.

## Kurze Hörreihenfolge

Pro World ein29,5-s-Vergleich: **0–9,5s Ausgangsphrase;10–19,5s verwandte Lage;
20–29,5s Erinnerung**. Die zwei0,5-s-Lücken markieren Vergleichsschnitte.
Jedes Segment ist9,5s tatsächliches Engine-Audio;40-ms-Ränder dienen nur den
Ausschnittschnitten. Es ist keine vorgetäuschte durchgehende World-Überblendung.
Nature0, Room0,24, normaler Product-DSP und Makros, D-Dur/Equal, Seed1234.
Ein konstanter Hörgain je World für alle drei Segmente; keine lokale Normalisierung.

| WAV | Herkunft Related / erste Return-Öffnung im10-Minuten-Lauf |
|---|---|
| `AMBIENT_COAST_Development_compare_29s.wav` |117,78s /178,23s |
| `AMBIENT_WOODLAND_Development_compare_29s.wav` |95,54s /149,63s |
| `AMBIENT_HIGHLANDS_Development_compare_29s.wav` |147,23s /223,08s |

Bewerten: klingt der mittlere Abschnitt wie ein verwandter neuer Akkord, bleibt
die letzte Phrase wiedererkennbar, bewahrt jede World ihren eigenen Verlauf?
Die Montage prüft kurze Motive. Für die vollständige mehrminütige Form muss der
reproduzierbare lange Lauf gehört werden; ein29,5-s-Vergleich schließt SD17 nicht.

## Software-/Audio-Nachweis

`test_phrase_development.c`:2304 Kombinationen aus drei Worlds,12 Keys, zwei
Collections und32 Seeds, je16 vollständige Phrasen. Alle vier Funktionen,
Rückkehrabstände2–4, erlaubte Register/Core, reine Proposals, Duplicate-Acks,
abgebrochene gehörte Öffnung, erst vollständiges Lernen, weiterlaufender RNG
und exakte erinnerte Pitch-/Velocity-/Hold-/Gap-Absicht geprüft.

Neun echte10-Minuten-Engine-Läufe:90 Minuten tatsächliches PCM, je World
Seed1234/D-Dur/Equal/default, Seed42/D-Moll/Just/default und Seed91267/A-Dur/
Equal/Activity0/Attack1/Release1. Jeweils alle vier tatsächlich gehörten Stufen
und mindestens eine vollständige Gedächtnisrückkehr; kein dauerhafter Halt,
maximal3/2/3 Quellen, keine NaN/Limiter, Samplepeak unter−6dBFS, natürliche
Retirement nach Stop. Das ist gezielte Grenzabdeckung, keine Exhaustivabnahme.
Volle Host-Suite prüft zusätzlich bisherige Keys/Collections/Stimmungen,
Timerwrap/Abbrüche, Manual/Generate, Scenes, Raum/Nature, alle sechs World-
Richtungen, lange Stresstests und Referencepfade.

Lokale Host-Prüfliste vollständig über zwei Läufe: der erste Runner wurde im
Reference-Synth-Host-Abschnitt ohne Abschlussmeldung unterbrochen. Dieser
Abschnitt und alle folgenden Originalprüfungen wurden separat erneut ausgeführt.
Die neuen Product-/Phrasen-/Room-/Stressprüfungen hatten bereits bestanden;
CI führt weiterhin den unveränderten vollständigen Runner-Einstieg aus.

`review_phrase_development.py`:je World600s normaler realer Product-Generator,
Nature0,10-ms-Main-Raster;64/512-Renderpartitionen ergeben identische komplette
WAVs und DSP-Ack-Traces.30 Minuten Audio je Partition. Alle Return-Tonhöhen,
Rollen und Velocities entsprechen den tatsächlich gestarteten Home-Noten.
Stop590s, alle Quellen vor600s frei. Finite PCM, Headroom, Mono/DC und feste
Hörpegel geprüft. Exakte Eingabe-/Audio-/Tracehashes, Pegel, Ausschnittzeiten und
gehörte Phrasenöffnungen stehen in[PHRASE_DEVELOPMENT_METRICS.json](PHRASE_DEVELOPMENT_METRICS.json).
Die70-/50-/65-s-RAW/LISTEN-Anfangsproben und Traces bleiben bytegleich zuPR150.
CI erhält den neuen Langlauf und lädt ausschließlich kurze WAVs/Metrik/Traces.

## H743

GCC13.2.1/Newlib4.4, Product Release/Journal OFF. PR150 mit identischem wieder-
hergestelltem Toolchain gebaut. Flash194316→195936B(+1620), D165600→65664B(+64),
D260808B unverändert; DTCM-Stackreservation16384B unverändert. Eine interne
D2-Roomarena55352B+5296B,4096B ausgerichteter D1-AudioDMA; keine Archiv-DSPs
gelinkt. Link-/Bank-/DMA-/Compilerframe-Audit besteht. Größter einzelner Core-
Compilerframe weiterhin192B;182 Coreframes. Kein CPU-/ISR-/Stack-High-water-
Nachweis am Gerät. Exakte ELF-/Compiler-/Bankdaten:
[PHRASE_DEVELOPMENT_ARM_METRICS.json](PHRASE_DEVELOPMENT_ARM_METRICS.json).

## Fortsetzung

SD17 ist softwareseitig integriert und geprüft, bleibt als Gesamt-Hörgate offen.
Nächste einzelne Vorbereitung: SD19, gleicher gehaltener Ensemble-Ausschnitt je
World mit Dry und wenigen festen gemeinsamen Raumwerten. Keine neue Effektwelt
oder Bedienoberfläche vor Quellen-/World-/Raumhörwahl. Storage/ECC, wirkliche
H743-Spitzen/Stack, Ausgabe und UX bleiben ihre eigenen offenen Geräteaufgaben.
