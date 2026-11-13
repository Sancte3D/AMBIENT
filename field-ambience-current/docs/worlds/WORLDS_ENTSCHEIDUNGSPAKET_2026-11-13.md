# AMBIENT – Worlds, Schritt 1: Entscheidungspaket

Stand 13.11.2026. Geprüft auf `main` 64c2620 plus Commit 8ac7741 (State Atlas und Auftrag).
Auftrag: `docs/WORLDS_BRIEF_2026-11-13.md`. **Keine Änderung am Klang**; `src/`, `include/` und
`cmake/` sind unverändert, `SOUND_CANDIDATE_INPUTS.json` gilt weiter.

Der vollständige Foliensatz (40 Folien, deutsch): https://claude.ai/artifact/8j3pzw31mxTHjhMqeY8Z8m
Diese Datei ist die dauerhafte Kurzfassung im Repo. Alles hier ist **Vorschlag bis Halt 1**.

## Was in diesem Paket liegt

| Datei | Inhalt |
|---|---|
| `docs/worlds/world_candidates.json` | 16 World-Kandidaten, 10 Tonvorräte, der heutige Stand zum Vergleich |
| `firmware-c-next/tools/check_world_identity.py` | prüft die Identitätsregel und die Harmonie-Zulassung der Tonvorräte; läuft in `run_tests.sh` |
| `firmware-c-next/tools/render_v3_world.c` | Host-Renderer für die sechs V3-Worlds mit Rechenzeit-Messung; läuft in `run_tests.sh` |
| `firmware-c-next/tools/world_sketches/sketch_worlds.py` | Hörskizzen CAVE, DUNES, JUNGLE (numpy, **kein Firmware-Klang**) |
| `docs/audio/world_sketches/*.mp3` | die drei Skizzen, je 30 s |
| `docs/audio/WORLDS_V3_HOST_MEASUREMENTS.json` | alle Messwerte dieses Pakets |
| `docs/ui/AMBIENT_STATE_ATLAS.html` | neuer Modus „KONZEPT · WORLDS“ zum Durchspielen |

## Gemessen

Linker-Map des vollen V3-Images (`FAM_TARGET=h743`, Product, Finish-Kandidat, V3, Release,
arm-none-eabi-gcc 13): Flash 217 384 B (212 KiB von 1920), RAM D1 353 724 B (345 KiB von 512),
RAM D2 236 704 B (231 KiB von 288), DTCM nur 16 KiB Stack, D3 und ITCM leer.
Größte Symbole: `sf_delay` 262 144 B, `sf` 110 088 B, `sf_mi_store` 65 536 B, `tank` 55 352 B,
`fb` 27 200 B, `wfx` 14 588 B.

Host-Render je World (Generate mit DRONE, 90 s, Seed 1234, Room 0,5, Nature 0, Flush-to-Zero an,
Xeon 2,1 GHz). **Kein Maß für das Gerät.**

| World | Rechenzeit je s Audio | Notenstarts je min | Spektraler Schwerpunkt | 85 % der Energie unter |
|---|---|---|---|---|
| COAST | 13,4 ms | 8,7 | 474 Hz | 732 Hz |
| WOODLAND | 8,2 ms | 12,7 | 293 Hz | 366 Hz |
| HIGHLANDS | 13,2 ms | 10,0 | 412 Hz | 495 Hz |
| FJORD | 19,4 ms | 8,0 | 778 Hz | 1098 Hz |
| MEADOW | 14,9 ms | 23,3 | 334 Hz | 366 Hz |
| GLACIER | 14,0 ms | 9,3 | 371 Hz | 484 Hz |

Nebenbefund: Ohne Flush-to-Zero kosten MEADOW und GLACIER am Host 70 und 54 ms je Sekunde
(Denormale in ausklingenden Resonatoren). Das Gerät muss FTZ sicher halten; Vorschlag: Selbsttest beim Start.

Identität heute (`check_world_identity.py --heute`): 13 Verstöße. 9 von 15 Paaren gleichen sich in
drei von vier Achsen; COAST, WOODLAND und HIGHLANDS haben keinen Signatur-Effekt; FJORD und GLACIER
teilen einen. Ursachen: ein Tonvorrat für alle (`core()`), ein Generate-Prinzip, `world = id % 3`
für Natur und Familie, ein gemeinsamer Raum der drei klassischen Worlds.

## Die 16 Kandidaten

**Entscheid des Besitzers vom 13.11.2026:** Diese Firmware heißt **Nature Landscapes** und enthält nur echte
Landschaften, in die man sich selbst hineindeuten kann. Denkräume und gebaute Orte kommen später als eigene
Firmware. Deshalb ersetzt: GARDEN → BLACKFOREST, EMBER → VOLCANO, CLOISTER → CANYON, ZENITH → STEPPE
(Klangbausteine bleiben, Ort, Natur und Name wechseln).

**Zweiter Entscheid vom 13.11.2026:** Die Namen sollen wie Orte der Erde klingen, die man bereist, ohne den
Klang auf eine Kultur oder landestypische Musik festzulegen. Umbenannt: COAST → ATLANTIC, GLACIER → ICEFIELDS,
REEF → ISLANDS, MOOR → MOORLAND, CANYON → CANYONS, STEPPE → GRASSLANDS. WOODLAND, HIGHLANDS, FJORD, MEADOW,
DUNES, JUNGLE bleiben. Kulturgebundene Beschreibungen sind neutral gefasst (JUNGLE: schwebende Klangstäbe und
fünf gleiche Stufen statt Gong-Paare und Slendro; DUNES: weiches Rohrblatt, ruhiger Puls). In der Firmware heißen
die sechs vorhandenen Worlds bis Schritt 3 weiter COAST … GLACIER; die Messwerte oben nennen diese Namen.

Regel: Klangquelle und Signatur-Effekt je nur einmal; in Tonvorrat, Zeitgefühl, Raum und Natur
gleichen sich zwei Worlds höchstens in zweien. Ergebnis des Prüfskripts: 0 Verstöße, größte Überdeckung 1 von 4.

| World | Status | Klangquelle | Tonvorrat | Zeitgefühl | Raum | Natur | Signatur-Effekt |
|---|---|---|---|---|---|---|---|
| ATLANTIC (Atlantikküste) | vorhanden | Streicherfläche | Dur gleichstufig | Gezeiten | weit und hell | Brandung | Brandungs-Atem |
| WOODLAND (Wald) | vorhanden | weicher Mallet | Moll gleichstufig | Ruf und Antwort | klein | Regen und Donner | Resonanzholz |
| HIGHLANDS (Hochland) | vorhanden | Okarina mit Atem | Sus gleichstufig | Einzelruf und Stille | offen und trocken | Höhenwind | Bergecho |
| FJORD (Fjord) | vorhanden | Chor | Man Gong gleichstufig | Stehender Akkord | gross und dunkel | Ferner Wasserfall | Vokal-Morph |
| MEADOW (Wiese) | vorhanden | Kalimba | Dur rein | Schwarm | offen und trocken | Grillen und Hummeln | Rückwärts-Band |
| ICEFIELDS (Eisfelder) | vorhanden | Glocken | Ritusen/Yo gleichstufig | Wechselläuten | weit und hell | Eisknacken und Schmelztropfen | Shimmer |
| CAVE (Grotte) | neu | Gestimmte Tropfen | Moll rein | Zufallstropfen | gross und dunkel | Tropfwasser | Tropfstein-Echo |
| DUNES (Dünen) | neu | Weiches Rohrblatt | Septimal dunkel | Ruhiger Puls | offen und trocken | Rieselnder Sand | Sandkorn-Wolke |
| JUNGLE (Regenwald) | neu | Schwebende Klangstäbe | Fünf gleiche Stufen | Verzahnte Muster | klein | Frösche und Zikaden | Schwebung |
| ISLANDS (Inseln) | neu | FM-Glas | Sus gleichstufig | Kanon | gedämpft | Lagunenwasser und Seevögel | Lagune |
| MOORLAND (Moorland) | neu | Windharfe | Septimal hell | Windgeführt | gedämpft | Moorblubbern und Wasservögel | Nebel |
| AURORA (Polarnacht) | neu | Glasharmonika | Man Gong gleichstufig | Auf- und Abbau | weit und hell | Schneestille und Knistern | Nordlicht-Vorhang |
| BLACKFOREST (Schwarzwald) | neu | Gezupfte Saite | Ritusen/Yo gleichstufig | Anlauf und Stille | klein | Waldbach und Specht | Waldstille |
| VOLCANO (Lavafeld) | neu | Handpan | Moll gleichstufig | Ostinato | offen und trocken | Knistern erkaltender Lava | Glutflackern |
| CANYONS (Schluchten) | neu | Harmonium | Dur rein | Choral | gross und dunkel | Fluss in der Tiefe | Hitzeflimmern |
| GRASSLANDS (Grasland) | neu | Sinus-Schwebung | Moll rein | Umlaufbahnen | offener Himmel | Gras im Nachtwind | Sternenlauf |

Für die sechs vorhandenen Worlds ist das der **Vorschlag**, nicht der heutige Klang. Jede Änderung an
ihnen kommt nur mit Freigabe nach Gehör (Schritt 2). Ort, Gefühl, DRONE, Bild und Akzentfarbe je World
stehen im Foliensatz (Folien 5–12) und die Akzentfarben in der JSON-Datei.

### Tonvorrat und Harmonie-Zulassung

`compatible()` in `engine_product.c` sperrt Tonpaare im Abstand einer Halbton- oder Tritonus-Klasse
(±45 Cent) und verschiedene Stimmungen desselben Tons (3–90 Cent). Die Zulassung rechnet in Hz, nicht in
Skalenstufen. Ein eigener Tonvorrat je World ist deshalb ohne Änderung der Zulassung möglich, wenn kein
Paar des Vorrats gesperrt ist. Das gilt für zehn geprüfte Vorräte: die fünf Modi der Pentatonik
(gleichstufig), Dur und Moll in reiner Stimmung, zwei septimale Reihen und fünf gleiche Stufen. Vorräte mit Halbton
(In-Sen, Hirajoshi, Pelog) hätten 2–3 gesperrte Paare von 10; sie brauchen eine Ausnahme je World und sind
zurückgestellt. Folgen: `core()` wird eine Tabelle je World; der DRONE braucht je World eigene Intervalle
(Man Gong hat keine Quinte, die Reihe aus fünf gleichen Stufen eine von 720 Cent); die Resonanzsaiten folgen dem Vorrat.

## „Mehr Ambient“ – zwölf prüfbare Regeln

Höchstens 3 Stimmen · 6–14 Notenstarts je Minute bei Dichte 50 % · je zwei Minuten eine Pause ≥ 8 s ·
gehaltene Quellen 10→90 % in ≥ 150 ms · angeschlagene Quellen Anstieg ≥ 3 ms, kein Knackser ·
gehaltener Ton ändert in 10 s Pegel oder Helligkeit um ≥ 1 dB · nichts schneller als 8 Hz außer Hitzeflimmern,
Schwebung, Glutflackern · keine Wiederholung unter 60 s · Ausklang ≥ 4 s bis −60 dB bei Raum 50 % ·
nichts wird abgeschnitten · Schwerpunkt 250–1200 Hz, Grundtöne ≥ 147 Hz, je World eigene Lage ·
alle Worlds gleich laut ±1,5 LU.

## Architektur und Budgets (Vorschlag)

World = ein Deskriptor ≤ 512 B im Flash (statischer Assert): Name, Farbe, Bild-Werte, Klangquelle,
Tonvorrat, Zeit-Grammatik, Raum, Natur, Kette, Signatur. 16 Klangquellen aus 6 Bausteinen (Fläche, Modal,
Rohr, Stimme, Saite, FM/Sinus), 16 Zeitgefühle aus 5 Grammatiken, Natur aus 2 Bausteinen, eine Effektkette.
Eine World-Arena von 48 KB hält Quelle und Signatur der aktiven World; weil nur eine World klingt, kostet
eine weitere World 0 B RAM und ihre Rechenzeit addiert sich nicht.

| Bereich | Belegt | Größe | Budget |
|---|---|---|---|
| Flash | 212 KB | 1920 KB | 640 KB |
| RAM D1 | 345 KB | 512 KB | 448 KB |
| RAM D2 | 231 KB | 288 KB | 256 KB |
| DTCM | 16 KB | 128 KB | 96 KB |
| RAM D3 | 0 | 64 KB | Reserve |
| ITCM | 0 | 64 KB | 48 KB (heißer Code) |

Rechenzeit: am Host je World höchstens 1,6 × COAST (heute FJORD 1,45); am Gerät schlechtester Block
≤ 7 ms von 11,6 ms, sobald es eine Platine gibt. Prüfungen: Linker-Map in der CI, statische Asserts,
Lasttest je World mit FTZ, `lint_hotpath.sh`, Identitätsprüfung, Mess-Firmware am Gerät. Heiße Puffer nie im PSRAM.
Schätzung (keine Messung): alle 16 Worlds brauchen rund 130 KB mehr Flash.

Hebel, falls es eng wird: Delay in 16 Bit (131 KB frei oder 2,97 s statt 0,74 s je Kanal) · freie Regionen
DTCM/D3/ITCM (240 KB) · ein gemeinsamer Hall-Speicher (bis 100 KB, zu prüfen) · Hall in halber Abtastrate ·
Grain in 16 Bit · die Grenze von drei Stimmen bleibt.

## Bedienkonzept (Vorschlag)

Vier feste Encoder, Ebenen durch Halten. Modifier schalten beim Loslassen.

| Zustand | EN1 | EN2 | EN3 | EN4 |
|---|---|---|---|---|
| Grundebene drehen | World | Farbe | Raum | Lautstärke |
| Grundebene kurz drücken | Licht hell/dunkel | Signatur an/aus | Werte zeigen | Stumm |
| Grundebene lang drücken | Szenen (mit Cell) | frei | Setup | frei |
| SHIFT halten | Grundton | Signatur-Menge | Echo | Lautstärke |
| DRONE halten | DRONE-Farbe | DRONE-Pegel | Natur-Pegel | Lautstärke |
| GENERATE halten | Dichte | frei | frei | Lautstärke |
| HOLD halten | Attack | Release | frei | Lautstärke |

SHIFT + HOLD = Freeze · SHIFT + DRONE = Natur an/aus · SHIFT + GENERATE = neues Feld · SHIFT + CLEAR =
nur Stimmen · EN1 halten + Cell = Szene laden, mit SHIFT speichern · Cells spielen bei GENERATE mit.

Sinnprüfung der 44 Funktionen: 15 behalten, 16 ändern ihren Ort, 13 entfallen (7 Gesten-Loop, 22 Hall an/aus,
23 Plate, 28 Delay-Zeit, 29 Feedback, 30 Grain, 31 Grain-Größe, 32 Drive, 33 Crush, 34 Phaser, 35 Phaser-Tempo,
36 Cell-Modus, 40 Akku und Ausgang); fünf neue (Signatur-Menge, Signatur an/aus, Natur an/aus, Werte zeigen, Setup).
Die Tabelle je Funktion steht im State Atlas (Tab „44 Funktionen“, Spalten WORLDS) und im Foliensatz (Folien 31–34).

Empfehlungen zu den neun Fragen: 1 ja, EN4 immer · 2 Ebenen durch Halten · 3 drei Werte: World, Farbe, Raum ·
4 drei Makros: Raum, Echo, Signatur · 5 EN1 halten + Cell · 6 vorerst nein · 7 ja, als „Licht“ (gleiche fünf Töne,
anderer Grundton) · 8 das Bild zuerst · 9 Helligkeit statt Farbe; LED-Farbe der Modifier: UNVERIFIED — NEEDS HUMAN CHECK.

Animationen: aus Formeln, Bildbereich 320 × 100 (gerechnet 17 ms Übertragung bei 30 MHz), 12–15 Bilder/s,
≤ 4 ms Zeichnen im Hauptprogramm, ≤ 1,5 KB Code und 256 B Zustand je World, keine Bilddaten.

## Kein Akku – betroffene Stellen (nur aufgelistet)

Firmware: `src/battery.c`, `include/battery.h`, `src/menu.c` (Akku-Symbol), `src/hal_h743/main_h743.c`
(Status „BAT %“, 1-Hz-Abfrage, USB-Erkennung), `src/hal_h743/adc_h743.c` (BAT_SENSE), `src/hal_h743/diag_h743.c`,
`test/test_menu_battery.c`, `test/test_display_bench.c`, `test/test_ui_layouts.c`, `tools/ui_wheel.*`,
`tools/ui_layouts.c`, `tools/render_oled.c`, `tools/display_sim.html`.
Hardware-Unterlagen (**nicht geändert**): `field_ambience_pcb_SPEC_v0.7.md`, `BOM_CURRENT.md`, `PCB_BOM.md`,
`kicad/generate_kicad_project.py`, `docs/hardware/` (BRING_UP, PINMAP, SCHEMATIC_WALKTHROUGH, KICAD_BLUEPRINT,
PCB_HANDOFF, ERC_DRC_CHECKLIST, MECHANICAL_REQUIREMENTS), `NATIVE_PORT_PLAN.md`, `MEINE_TODO.md`.

## Annahmen (nicht gemessen)

Rechenzeit auf dem H743 · alle Kosten neuer Bausteine · Wirkung über die 40-mm-Lautsprecher ·
Rechenzeit-Klassen der Signatur-Effekte · Wirkung der Hebel 3 und 4 · die Hörskizzen sind gemessen
(Pegel, Spektrum), aber von niemandem gehört.

## Halt 1 – Entscheidungen des Besitzers

1. Welche Worlds? Empfehlung: zwölf zuerst (die sechs vorhandenen, CAVE, DUNES, JUNGLE, MOORLAND, BLACKFOREST, GRASSLANDS), vier später (ISLANDS, AURORA, VOLCANO, CANYONS).
2. Dürfen die sechs vorhandenen eigenen Tonvorrat, Zeitgefühl und Natur bekommen? Empfehlung: ja, einzeln per Hörskizze.
3. Die neun Fragen (oben).
4. Die Streichliste der 13 Funktionen. Empfehlung: annehmen.
5. Cells spielen bei GENERATE mit? Empfehlung: ja.
6. Budgets als verbindlich? Empfehlung: ja.
7. V3 als Standard am Ende von Schritt 3, in eigenem Pull Request? Empfehlung: ja.
