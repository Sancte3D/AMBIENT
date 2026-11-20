# Auftrag: AMBIENT – die Worlds zum Kern des Produkts ausbauen

Du arbeitest im Repository Sancte3D/AMBIENT. Dein Startstand ist `main` (alles ist gemergt) plus ein Commit mit dem State Atlas und diesem Auftrag (Branch `claude/ueberpruefung-validierung-lztv0m`). AMBIENT ist ein haptischer Ambient-Synthesizer auf einem STM32H743: fünf Cells (Spieltasten), fünf Modifier-Tasten (SHIFT, HOLD, DRONE, GENERATE, CLEAR), vier Encoder mit Druckfunktion, ein 1,9″-Display (320 × 170) und zwei 40-mm-Lautsprecher. Einen Akku hat das Gerät nicht. Eine Platine gibt es noch nicht. Alles wird am Host gebaut, getestet und gerendert.

Lies zuerst `CLAUDE.md` und den obersten Eintrag in `PROJECT_STATUS.md`. Antworte mir auf Deutsch und in einfacher Sprache. Ich bin der Besitzer und gebe Klang nach Gehör frei, nicht nach Messwerten.

Dieser Auftrag liegt auch im Repo unter `field-ambience-current/docs/WORLDS_BRIEF_2026-11-13.md`. Lies ihn dort nach, wenn dir der Zusammenhang verloren geht.

## Worum es geht

Das Alleinstellungsmerkmal von AMBIENT sind die Worlds: Naturlandschaften und Denkräume, in die man sich hineinversetzen kann, „landscapes of desire“. Jede World soll ein eigener Ort mit eigenem Klang sein. Heute gibt es sechs. Das sind zu wenige, und im Kern sind sie sich zu ähnlich. Ich will:

1. deutlich mehr Worlds, jede sofort und klar von allen anderen unterscheidbar, und alle „mehr Ambient“;
2. Effekte, die perfekt klingen: die Klassiker, die jeder Synthesizer hat, und dazu Effekte, die es so nur bei uns gibt;
3. die Garantie, dass Speicher und Rechenzeit für alle Worlds reichen, obwohl wortwörtlich ganze Welten auf dem Gerät liegen;
4. ein festgelegtes Bedienkonzept: was die fünf Modifier-Tasten, die fünf Cells und die vier Encoder genau tun, wie das Menü aufgebaut ist, was das Display zeigt und wie Animationen eingebaut werden.

## Der Stand, geprüft am 13.11.2026 auf `main` (64c26200)

### Klang

- Die Firmware liegt in `field-ambience-current/firmware-c-next/`. Der aktuelle Klang liegt hinter `FAM_SOUND_PROFILE=product`, `FAM_SOUND_FINISH_CANDIDATE` und `FAM_SOUND_AMBIENT_V3` (Standard: aus). Mit Schalter aus muss die Ausgabe byte-identisch bleiben.
- Sechs Worlds: COAST (Streicherfläche), WOODLAND (weicher Mallet), HIGHLANDS (Okarina mit Atem), FJORD (Chor), MEADOW (Kalimba), GLACIER (Glocken). Diesen Stand habe ich an Host-Aufnahmen nach Gehör freigegeben, am Gerät noch nicht.
- Warum sie sich ähneln:
  - Alle sechs spielen dieselben zwei Fünfton-Skalen (`core()` in `src/world_grammar.c`).
  - Generate folgt überall demselben Prinzip: drei Loops, nur mit anderen Zykluszeiten.
  - Es gibt drei Stimmtypen (`bowed.c`, `pluck.c`, `horn.c`). FJORD, MEADOW und GLACIER sind Instrumente, die auf die drei alten Familien aufgesetzt sind (`world = id % 3`).
  - Eine World ist kein Datensatz. Sie ist über Schalter und mehrere Tabellen verteilt: `engine_product.c`, `worlds_product.c`, `WFX_PRESET` in `ambient_world_fx.h`, `SF_WORLD` in `sound_finish_fx.h`, `ambient_room.c`.
- Es klingen höchstens drei Stimmen gleichzeitig. Dazu kommen der DRONE-Teppich, eine Naturkulisse (Brandung, Regen mit Donner, Wind) und die Effektkette.
- Effekte heute: Delay, Grain, Freeze, Drive, Crush, Phaser, Plate und Room. In den drei Instrument-Worlds zusätzlich Tape, ein atmender Filter, Resonanzsaiten, Ensemble und Hall (Mutable Instruments Rings, MIT) sowie Oktav- und Rückwärts-Grains.
- Die Harmonie-Zulassung führt Buch über die klingenden Tonhöhen. Ein Effekt, der neue Tonhöhenklassen erzeugt, macht dieses Buch falsch. Oktaven und Rückwärts sind unkritisch.
- Ein naheliegender Hebel: Fünf Cells spielen fünf Töne. Jede World könnte ihren eigenen Fünfton-Vorrat und ihre eigene Stimmung bekommen. Prüfe, was das für die Harmonie-Zulassung bedeutet.
- Eine frühere Prüfung der Welt-Ähnlichkeit (Stand vor V3) steht in `field-ambience-current/docs/audio/WORLD_IDENTITY_AUDIT.md`.

### Speicher und Rechenzeit

Gemessen aus der Linker-Map des vollständigen V3-Images:

| Bereich | Belegt | Größe |
|---|---|---|
| Flash | 212 KB | 1920 KB |
| RAM D1 | 345 KB | 512 KB |
| RAM D2 | 231 KB | 288 KB |
| DTCM | nur der 16-KB-Stack | 128 KB |
| RAM D3 | unbenutzt | 64 KB |
| ITCM | unbenutzt | 64 KB |

- Rund 85 % des belegten RAM sind Effekt-Speicher: `sf_delay` 262 KB (Float), `sf` 110 KB (Plate und Grain), `sf_mi_store` 64 KB, `tank` 55 KB. Alle Stimmen zusammen brauchen unter 20 KB, das Display-Bild 27 KB.
- Daraus folgt eine Vermutung, die du prüfen sollst: Neue Worlds kosten kaum RAM, solange sie sich die Effekt-Speicher teilen, und Flash ist reichlich da. Der Engpass ist wahrscheinlich die Rechenzeit.
- Die Rechenzeit ist auf dem H743 noch nicht gemessen. Am Host brauchen die Worlds 9–17 ms CPU je Sekunde Audio; was das auf dem Gerät bedeutet, weiß noch niemand. Dort stehen 11,6 ms je Block von 512 Frames zur Verfügung. Eine Mess-Firmware existiert (CELL1 beim Einschalten halten).
- Ein QSPI-PSRAM mit 8 MB ist im Schaltplan vorgesehen. Die Engine nutzt es nicht, und laut Echtzeitregel dürfen heiße Puffer dort nie liegen.

### Bedienung und Display

- Es gibt einen Foliensatz mit allen 44 Funktionen, einem Encoder-Vorschlag und neun offenen Entscheidungen: https://claude.ai/artifact/39aa27PZwNjHCUBd8J1iMR. Die neun Fragen sind noch nicht entschieden.
- Der **State Atlas** (`field-ambience-current/docs/ui/AMBIENT_STATE_ATLAS.html`) zeigt denselben Stand als durchspielbares Modell: Zustands-Timeline, Simulator der Frontplatte, Seiten, alle 44 Funktionen und die neun Fragen. Er ist eine eigenständige HTML-Datei. Die Daten stehen als Tabellen im Skript (`SEED_FUNCTIONS`, `SEED_PAGES`, `SEED_DECISIONS`, `SEED_SCENARIOS`).
- Nichts davon ist fest. Bedienung und UX dürfen geändert und ergänzt werden. Prüfe bei jeder Funktion und jedem Zustand, ob sie für dieses Instrument sinnvoll ist. Zwei Beispiele von mir:
  - Das Gerät hat Lautsprecher, aber keinen Akku. Akku-Anzeige und Akku-Status sind damit überflüssig.
  - Die Aufnahme von Cell-Anschlägen (Gesten-Loop) bringt aus meiner Sicht wenig.
- 15 Funktionen haben noch kein Bedienelement: alle Effektwerte, Freeze, Plate, DRONE-Farbe und -Pegel, Dur/Moll und der Cell-Modus. Encoder EN1 ist im Produkt-Build frei.
- Display-Entwürfe stehen in `field-ambience-current/docs/decisions/ADR-0026` bis `ADR-0028` (Rad, Szenen, Encoder-Farben) und in `field-ambience-current/docs/ui/MOTION_RULES.md`. Sie sind vorgeschlagen, nicht beschlossen, und beziehen sich auf einen älteren Parametersatz.
- Die Anzeige auf dem H743 arbeitet heute mit 4-Bit-Graustufen und einer Akzentfarbe je World. Ein volles Bild braucht rund 29 ms über SPI.
- Im Repo liegen Skills für dieses Projekt (`.claude/skills/ambient-*`). Nutze sie für UX, Motion, Art Direction, Prototyping und Verifikation.

## Was ich von dir will

Arbeite in drei Schritten und halte an den zwei markierten Stellen an. Der Grund: Die Auswahl der Worlds ist meine Entscheidung, und Klang gebe ich nur nach Gehör frei.

### Schritt 1 – Entscheidungspaket (noch keine Änderung am Klang)

Erstelle einen deutschen Foliensatz mit diesen Teilen:

a) **World-Bibel.** Mindestens 14 Kandidaten, die sechs vorhandenen eingeschlossen. Je World:
   - Ort und Moment, und wofür man dorthin geht (das Gefühl);
   - Klangquelle, Tonvorrat und Stimmung;
   - Zeitgefühl: wie Generate dort spielt;
   - Raum, Naturkulisse und DRONE;
   - der eine Effekt, den es nur dort gibt;
   - Bild und Animation auf dem Display, Akzentfarbe;
   - ein Satz dazu, was sie von jeder anderen unterscheidet.

b) **Identitätsmatrix.** Alle Worlds gegen die Achsen Klangquelle, Tonvorrat, Zeitgefühl, Raum, Natur und Signatur-Effekt. Klangquelle und Signatur-Effekt gibt es je nur einmal. In den übrigen Achsen dürfen sich zwei Worlds höchstens in zweien gleichen. Sag ehrlich, welche der sechs vorhandenen Worlds diese Regel heute verletzen und wie du sie verbessern würdest.

c) **„Mehr Ambient“ als prüfbare Regeln:** Dichte, Einschwingzeiten, Bewegung, Ausklang.

d) **Effektkatalog.** Die Klassiker als gemeinsame Bausteine und die Signatur-Effekte. Je Effekt: Herkunft (eigener Code oder Open Source mit erlaubter Lizenz), Kosten in RAM und Rechenzeit, Risiko für das Tonhöhen-Buch, und wie wir prüfen, dass er sauber klingt (keine Knackser, kein Pegelsprung beim Einschalten, mono-verträglich).

e) **Architektur „World = Datensatz“.** Ein Deskriptor je World in einer Tabelle, ein kleiner Satz gemeinsamer Klangbausteine, eine gemeinsame Effektkette. Zeig, was eine weitere World dann an Flash, RAM und Rechenzeit kostet.

f) **Garantien.** Feste Budgets für Flash, jede RAM-Region und die Rechenzeit je World, und wie sie automatisch geprüft werden: Prüfung der Linker-Map in der CI, statische Asserts, ein Lasttest je World am Host, die Messung am Gerät, sobald es eines gibt. Nenne die größten Hebel, falls es eng wird, zum Beispiel 16-Bit-Delay-Speicher oder die freien RAM-Regionen.

g) **Bedienkonzept.**
   - Eine Sinnprüfung aller 44 Funktionen: behalten, ändern oder streichen, je mit einem Satz Begründung. Was wegfällt, macht Platz für das, was die Worlds brauchen.
   - Die genaue Belegung der fünf Modifier-Tasten, der fünf Cells und der vier Encoder als Zustandstabelle.
   - Menüaufbau und Bildschirm-Inventar.
   - Der Wechsel zwischen vielen Worlds. Wenn die Worlds der Kern sind, verdient der Weltwechsel einen direkten Weg.
   - Animationen je World: gezeichnet aus Formeln statt aus Bilddaten, mit Zeit- und Speicherbudget.
   - Zu jeder der neun offenen Fragen aus dem Foliensatz eine Empfehlung.
   - Das neue Konzept zusätzlich als aktualisierter State Atlas, damit ich es durchspielen kann.

Wenn der Aufwand es zulässt, lege für drei möglichst verschiedene neue Worlds je eine Hörskizze von 30 Sekunden bei.

**Halt 1:** Ich wähle die Worlds aus und entscheide die offenen Fragen.

### Schritt 2 – Hörskizzen

- Für jede gewählte World ein Host-Render von 60–90 Sekunden als MP3: gespielte Cells, Generate mit DRONE, und der Signatur-Effekt für sich.
- Ein Blindtest-Paket: kurze Ausschnitte ohne Namen. Ziel ist, dass man jede World nach fünf Sekunden erkennt.
- Dieselben Ausschnitte durch ein Lautsprecher-Modell, weil die Identität auch auf dem Gerät tragen muss: Hochpass bei 91 Hz und kleine 40-mm-Treiber. Die Schallkammer färbt nach grober Schätzung um 2 kHz; das ist nicht gemessen.
- Im Repo gibt es noch keinen Renderer für die V3-Worlds. Baue zuerst einen unter `tools/`.

**Halt 2:** Ich höre und gebe frei oder schicke zurück.

### Schritt 3 – Härten

Bring das Freigegebene in die Firmware: Deskriptor-Tabelle, Bausteine, Effekte, Bedienung und Display. Dazu Tests für jede World (Unterscheidbarkeit als Messwert, Budgets, Echtzeitregeln), eine grüne CI und ein Pull Request gegen `main`.

## Regeln

- `CLAUDE.md` ist bindend. Das gilt vor allem für die Echtzeitregel für alles, was von `engine_render()` erreichbar ist: kein Heap, nichts Blockierendes, jeden Parameter rampen, heiße Puffer nie ins PSRAM. `test/lint_hotpath.sh` prüft das.
- Open-Source-DSP ist erlaubt, standardmäßig nur mit freizügiger Lizenz (MIT, BSD, Apache). Der Lizenzkopf bleibt erhalten, und die Quelle kommt in `field-ambience-current/firmware-c-next/THIRD_PARTY_NOTICES.md`.
- Der Klang der sechs vorhandenen Worlds ändert sich nur mit meiner Freigabe nach Gehör. Schalter aus bleibt byte-identisch. Schlag vor, wann V3 zum Standard werden soll.
- Keine Änderung an Schaltplan oder Platine.
- Das Gerät hat keinen Akku. Firmware und Hardware-Unterlagen gehen stellenweise noch von einem aus. Behandle das in der Bedienung als entschieden, liste die betroffenen Stellen auf und ändere die Hardware-Unterlagen nicht.
- Rate nicht. Was du nicht prüfen kannst, markierst du als Annahme. Zahlen kommen aus Messungen (Linker-Map, Tests, Render), nicht aus Schätzungen.
- Nach jeder Änderung an Klang, UI oder Build muss `./test/run_tests.sh` (in `firmware-c-next/`) grün sein. Ändern sich Klang-Quelldateien, erzeuge `field-ambience-current/docs/audio/SOUND_CANDIDATE_INPUTS.json` mit `firmware-c-next/tools/freeze_sound_candidate.py` neu.
- Arbeite auf dem Branch, den dir die Sitzung vorgibt, und öffne einen Pull Request gegen `main`. Nie direkt auf `main` pushen.

## Wie du berichtest

An jedem Halt zuerst das Ergebnis in wenigen Sätzen, dann die Entscheidungen, die ich treffen muss, je mit deiner Empfehlung. Sag klar, was gemessen ist und was Annahme.
