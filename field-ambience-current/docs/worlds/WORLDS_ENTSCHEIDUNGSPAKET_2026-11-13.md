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
| ICEFIELDS (Eisfelder) | vorhanden | Kristall-Synth | Ritusen/Yo gleichstufig | Wechselreihe | weit und hell | Eisknacken und Schmelztropfen | Shimmer |
| CAVE (Grotte) | neu | Gestimmte Tropfen | Moll rein | Zufallstropfen | gross und dunkel | Tropfwasser | Tropfstein-Echo |
| DUNES (Dünen) | neu | Weiches Rohrblatt | Septimal dunkel | Ruhiger Puls | offen und trocken | Rieselnder Sand | Sandkorn-Wolke |
| JUNGLE (Regenwald) | neu | Schwebende Synth-Töne | Fünf gleiche Stufen | Verzahnte Muster | klein | Frösche und Zikaden | Schwebung |
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

## „Mehr Ambient“ – prüfbare Regeln (überarbeitet am 16.11.2026)

Grundlage ist die vermessene Referenzaufnahme: `docs/audio/AMBIENT_REFERENCE_ANALYSIS.md`.

- **Synthesizer (Vorgabe des Besitzers, 16.11.):** Keine Glockenspektren: nur harmonische Teiltöne und weicher
  Anschlag. Rauschen nur als Hauch, mindestens 40 dB unter dem Bett; Tupfer sind tonal statt geräuschhaft.
  Betroffen: ICEFIELDS (bisher Glocken) bekommt einen Kristall-Synth; JUNGLE schwebende Synth-Töne statt Klangstäbe.
  Der heutige GLACIER-Glockenklang der Firmware ist damit nicht mehr Ziel.
- **Schichten:** Jede World hat Bett, Luft, Tupfer und Töne. Das Bett trägt den Ort.
- **Dichte:** höchstens 3 Stimmen; in Generate je nach World 1–14 Notenstarts je Minute bei Dichte 50 %;
  Tupfer höchstens 20 je Minute und mindestens 20 dB unter dem Bett.
- **Breite:** Links/Rechts-Korrelation des Betts höchstens 0,4; in Mono verliert kein Terzband mehr als 6 dB,
  und das Wogen bleibt in Mono erhalten.
- **Zwei Uhren:** Bett wogt mit Perioden von 0,5–2 s um mindestens 3 dB; Luft und Helligkeit ändern sich über 8–120 s.
- **Einschwingen:** gehaltene Quellen 10→90 % in ≥ 150 ms; angeschlagene Anstieg ≥ 3 ms, kein Knackser.
- **Ausklang:** ≥ 4 s bis −60 dB bei Raum 50 %; nichts wird abgeschnitten.
- **Gleichmaß:** Pegel über 10 Minuten Generate innerhalb ±1,5 dB; keine Wiederholung unter 60 s.
- **Frequenz:** Grundtöne des Betts ≥ 147 Hz (Lautsprecher); dunkel ist erlaubt, die frühere Schwerpunkt-Regel entfällt.
- **Pegel:** alle Worlds gleich laut ±1,5 LU.

## Lautsprecher-Pfad: Virtueller Bass (Vorschlag, 17.11.2026, vom Besitzer gewünscht)

Problem: Der Treiber CMS-402811-28SP hat laut Spezifikation seine Resonanz bei 450 Hz; das Bett liegt bei
150–300 Hz. Lauter machen geht nicht, weil ein 40-mm-Treiber dafür zu wenig Hub hat und verzerrt.

Prinzip (psychoakustischer Bass, „fehlender Grundton“): Das Ohr ergänzt einen Grundton, wenn seine Obertöne
klingen. Der Baustein nimmt den Anteil unter etwa 300 Hz, erzeugt daraus Obertöne und mischt sie im Bereich
450–1200 Hz bei, wo der Treiber gut abstrahlt. Der echte Tiefbass bleibt unverändert im Signal.

- **Nur über die eingebauten Lautsprecher:** an, wenn kein Kopfhörer steckt (Jack-Erkennung vorhanden);
  beim Umstecken in 50 ms ein- und ausgeblendet, kein Knacken.
- **Tonhöhen-Buch:** Standard nur gerade Obertöne (2. und 4., Oktaven) durch Doppelweg-Gleichrichtung des
  Tiefbandes. Oktaven bringen keine neuen Tonklassen; die Harmonie-Zulassung bleibt wahr. Der 3. Oberton
  (Quinte) gäbe einen stärkeren Bass-Eindruck, kann aber Tonklassen außerhalb des Vorrats erzeugen und kommt
  nur nach Hörprobe.
- **Kosten (Schätzung):** vier Biquad-Filter und ein Gleichrichter je Kanal, kein Speicher über ein paar
  Zustände hinaus. Platz in der Kette: ganz am Ende, vor dem Ausgangsschutz.
- **Prüfung:** Spektralspitzen des Zusatzsignals nur auf Oktaven der Eingangstöne (±20 Cent); kein Sprung beim
  Umschalten Lautsprecher/Kopfhörer; Pegelzuwachs gegen den Ausgangsschutz geprüft; Hörvergleich mit und ohne
  über ein Lautsprecher-Modell (Hochpass 91 Hz, Abfall unter 450 Hz) in Schritt 2.
- **Herkunft:** eigener Code. Das Verfahren ist allgemein bekannt; markengeschützte Umsetzungen werden nicht
  übernommen.

## Aus dem HiChord übernommen (Besitzer-Entscheid 17.11.2026)

Quelle: HiChord-Handbuch Rev 3.0 (updater.hichord.shop/manual), vollständig gelesen. Übernommen werden Prinzipien,
nicht der Funktionsumfang: keine Jazz-Akkordtypen, keine Fremdtonart-Akkorde, keine Drums, keine Spielmodi-Vielfalt,
keine Effekte, die sich gegenseitig sperren.

1. **Automatische Stimmführung im Bett.** Wechselt der Akkord des Betts (Licht, Grundton, Choral von CANYONS,
   Weltwechsel mit gleichem Grundton), wird jede Bett-Stimme dem Zielton mit dem kürzesten Weg zugeordnet
   (kleinste Summe der Cent-Bewegungen; bei höchstens sechs Stimmen eine feste, begrenzte Suche) und gleitet in
   0,8–2 s dorthin. Keine Stimme springt; eine Stimme, die bleiben kann, bleibt. Rechnet im Hauptprogramm, der
   Audio-Interrupt rampt nur.
2. **Klare Grenze für „keine falschen Töne“.** Der Grundzustand jeder World ist immer konsonant. Reibung gibt es
   nur über einen eigenen, benannten Griff: **Reibung** (GENERATE halten + EN2), Standard 0. Sie erlaubt dichtere
   Lagen, Sekunden und Vorhalte, aber nur mit Tönen des Vorrats; Halbton und Tritonus sperrt die Harmonie-Zulassung
   weiterhin. Das Tonhöhen-Buch bleibt wahr.
3. **Rechenzeit-Wächter mit fester Reihenfolge.** Der vorhandene Profiler misst jede Render-Runde (DWT). Liegt die
   Zeit 16 Blöcke in Folge über 80 % des Budgets (11,6 ms je 512 Frames), stuft das Hauptprogramm eine Stufe ab,
   jeweils mit 200-ms-Rampe: (1) Shimmer- und Signatur-Grains, (2) Ensemble, (3) zweite Hall-Stufe,
   (4) obere Teiltöne des Betts, (5) Natur-Ereignisse. Rückkehr Stufe für Stufe erst nach 10 s unter 60 %. Das
   Display zeigt kurz „LEISTUNG BEGRENZT“. Kein hartes Abschalten, kein Sperren anderer Effekte; der Audio-Interrupt
   liest nur die freigegebene Stufe. Teil der Garantien: Er sichert das Gerät auch dann, wenn eine World teurer ist
   als gemessen.
4. **„OG“-Varianten statt stiller Klangänderungen.** Jede der sechs freigegebenen Worlds, deren Klang sich ändert,
   behält ihre heutige Fassung als wählbare Variante „OG“ (Setup → Klangfassung, je World: NEU oder OG), bis du sie
   nach Gehör streichst. Ein Test sichert, dass OG byte-identisch zum heutigen Stand bleibt. Kosten: der alte Code
   bleibt im Flash (gemessen heute 212 KB von 1920 KB).
5. **Benannte Hüllkurven.** HOLD halten + EN1 wählt eine benannte Form: **SWELL** (langsames Anschwellen), **LONG**,
   **SOFT**, **PLUCK** (angezupft, natürlicher Ausklang), **SHORT**. Jede World hat ihre Standardform. Ersetzt Attack
   und Release als rohe Werte. (Ein Feinwert „Länge“ war vorgesehen; gestrichen am 17.11., siehe Bedienkonzept.)
6. **Basston wählbar.** Im Setup je World: **AUS**, **GRUNDTON**, **QUINTE**, mit Standard je World. Über die
   eingebauten Lautsprecher wirkt er über den virtuellen Bass. (Kein eigener Griff, siehe Bedienkonzept.)

## Architektur und Budgets (Vorschlag)

World = ein Deskriptor ≤ 512 B im Flash (statischer Assert): Name, Farbe, Bild-Werte, Klangquelle,
Tonvorrat, Zeit-Grammatik, Raum, Natur, Kette, Signatur. 16 Klangquellen aus 6 Bausteinen (Fläche, Modal,
Rohr, Stimme, Saite, FM/Sinus), 16 Zeitgefühle aus 5 Grammatiken, Natur aus 2 Bausteinen, eine Effektkette.
Neu seit 16.11.: Das **Bett** ist ein eigener, siebter Baustein (Bank aus Tabellen-Oszillatoren auf dem Tonvorrat,
links/rechts mit eigenen Phasen); es ersetzt den heutigen DRONE als Fundament jeder World.
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
Letzte Stufe zur Laufzeit: der Rechenzeit-Wächter (siehe „Aus dem HiChord übernommen“, Punkt 3).
Schätzung (keine Messung): alle 16 Worlds brauchen rund 130 KB mehr Flash.

Hebel, falls es eng wird: Delay in 16 Bit (131 KB frei oder 2,97 s statt 0,74 s je Kanal) · freie Regionen
DTCM/D3/ITCM (240 KB) · ein gemeinsamer Hall-Speicher (bis 100 KB, zu prüfen) · Hall in halber Abtastrate ·
Grain in 16 Bit · die Grenze von drei Stimmen bleibt.

## Bedienkonzept (vereinfacht am 17.11.2026)

Vorgabe des Besitzers (17.11.): Modifier und Bedienung dürfen nicht so komplex werden, dass sie unverständlich
sind. Deshalb gelten nur drei Regeln, ohne Ausnahme:

1. **Taste tippen = an oder aus.** (HOLD, DRONE, GENERATE, CLEAR; SHIFT wirkt nur gehalten.)
2. **Taste halten + drehen = die Werte genau dieser Taste.** Kein Drücken von Encodern während eine Taste gehalten wird.
3. **EN4 ist immer Lautstärke.** Was selten gebraucht wird, steht im Setup (EN3 lang).

Einzige Kombination mit SHIFT auf einer Taste: SHIFT + HOLD = Freeze. SHIFT + Cell = Oktave höher.
Ein Wert auf 0 ist „aus“; eigene Schalter dafür gibt es nicht.

| Zustand | EN1 | EN2 | EN3 | EN4 |
|---|---|---|---|---|
| Grundebene drehen | World | Farbe | Raum | Lautstärke |
| Grundebene kurz drücken | Licht hell/dunkel | — | — | Stumm |
| Grundebene lang drücken | Szenen (mit Cell) | frei | Setup | frei |
| SHIFT halten | Grundton | Signatur (0 = aus) | Echo | Lautstärke |
| DRONE halten | Wogen (Bewegung des Betts) | Bett-Pegel | Natur (0 = aus) | Lautstärke |
| GENERATE halten | Dichte | Reibung | — | Lautstärke |
| HOLD halten | Hüllkurve (SWELL · LONG · SOFT · PLUCK · SHORT) | — | — | Lautstärke |

Vorgabe des Besitzers (16.11.): Die Stärke des Betts muss immer regelbar sein. DRONE tippen blendet das Bett
ein und aus; DRONE halten + EN2 regelt seinen Pegel, + EN1 sein Wogen von fast stehend bis deutlich bewegt.
Die DRONE-Farbe entfällt als Regler (sie gehört zur World).

EN1 halten + Cell = Szene laden, mit SHIFT speichern · Cells spielen bei GENERATE mit.
Setup (EN3 lang): Basston je World · Klangfassung NEU/OG je World · Stimmung gleichstufig · Helligkeit ·
Ruhezeit · MIDI · Version.

Damit: 13 Drehwerte auf 4 Encodern in 5 Ebenen, 1 Druck (Licht), 1 SHIFT-Kombination (Freeze), Szenen, Setup.

Sinnprüfung der 44 Funktionen (Streichliste vom Besitzer angenommen am 17.11.): 13 behalten, 15 ändern ihren Ort,
16 entfallen (6 Neues Feld, 7 Gesten-Loop, 10 Stimmen leeren, 19 Release (steckt in der Hüllkurve), 22 Hall an/aus,
23 Plate, 28 Delay-Zeit, 29 Feedback, 30 Grain, 31 Grain-Größe, 32 Drive, 33 Crush, 34 Phaser, 35 Phaser-Tempo,
36 Cell-Modus, 40 Akku und Ausgang). Neu: Signatur-Menge, Reibung, Setup mit Basston und Klangfassung,
Rechenzeit-Wächter (automatisch). Nicht mehr vorgesehen: Signatur an/aus, Natur an/aus, Werte zeigen.
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

## To-do aus der Natur-Recherche (vom Besitzer am 17.11.2026 als To-do gesetzt)

Quelle: `docs/research/NATUR_SOUNDSCAPE_RECHERCHE_2026-11-17.md` (ChatGPT Deep Research; Quellen dort nicht geprüft).
Leitsatz, den wir übernehmen: **Natur liefert keine Tonart, sondern Verhalten** (Ereignisdichte, Abstände, Zeitskalen,
Spektrum, Raum). Tonvorrat und Klangquelle bleiben ausdrücklich Interpretation – AMBIENT ist ein Synthesizer.

| # | Aufgabe | Wann | Stand |
|---|---|---|---|
| R1 | Natur-Schicht aus Messwerten statt aus Gefühl: je World 3–5 gemeinfreie Aufnahmen (NPS Natural Sounds, Freesound nur CC0; Lizenz je Datei notieren) am Host analysieren – Ereignisse pro Minute, Verteilung der Abstände, langsame Modulation, Spektralbalance – und die Zahlen in den Deskriptor schreiben. Werkzeug unter `tools/`. | Schritt 2 | offen |
| R2 | Mehr Zeitskalen ohne neuen Regler: heute zwei Uhren (0,5–2 s, 8–120 s); dazu Minuten (Dichte und Raum driften) und 10–30 min („Wetter“-Zustand). Automatisch, deterministisch, gerampt. | Schritt 2/3 | offen |
| R3 | Stille als Wert: DUNES und GRASSLANDS dürfen lange Pausen haben; Abstände als Verteilung, nicht fest. | Schritt 2 | offen |
| R4 | Blindtest zweistufig: erst nur Klang ohne Namen (Verwechslungsmatrix, je World 2–3 unabhängige Läufe, gleiche Lautheit), danach mit Namen und Bild. Erfolg = unterscheidbare Profile und stabile Wiedererkennung über Läufe, nicht „80 % erkennen Wald“. | Schritt 2 | offen |
| R5 | Sprachregeln für Handbuch und Marketing: keine Heil- oder Stressversprechen, keine „Frequenz der Landschaft“, kein 432 Hz, kein „1/f = Natur“. Erlaubt: „für ruhiges, eintauchendes Hören“. | vor Veröffentlichung | offen |
| R6 | Display: Landschaft als reduzierte, langsam bewegte Struktur statt Dauerfoto – deckt sich mit den Formel-Animationen; beim Weltwechsel kurz deutlicher, dann zurücknehmen. | Schritt 3 | passt schon |
| R7 | Bedienung gegen die fünf Makros der Recherche prüfen (Raum, Dichte, Bewegung, Helligkeit, Natur/Musik) – unser vereinfachtes Konzept hat sie bereits: EN3 Raum, Dichte, Wogen, Farbe, Natur-Pegel. | erledigt | passt |
| R8 | Echte Aufnahmen im Gerät: **vorerst nicht.** Gemessen: Die Platine hat außer dem internen Flash (1920 KB, davon 212 KB belegt) nur PSRAM (APS6404L, flüchtig, für heiße Puffer verboten), keinen externen Flash, keine SD-Karte. Das reicht für wenige Sekunden Audio, die als Schleife erkennbar wären. Später prüfen: kurze, stark komprimierte Körner als Material für Granular. | später | Hardware-Frage |
| R9 | Optional TAGESZEIT (Morgen, Tag, Abend, Nacht) als langsamer Zustand, der das Verhalten ändert – nur als Automatik oder im Setup, kein neuer Griff (Regel: Bedienung bleibt einfach). | nach Schritt 3 | Idee |

Nicht übernommen: die Reduktion auf sechs Kategorien (COAST, RIVER, FOREST, WETLAND, GRASSLAND, DRYLAND). Unsere
Empfehlung für die ersten zwölf deckt alle sechs schon ab (ATLANTIC, FJORD/CANYONS, WOODLAND/BLACKFOREST/JUNGLE,
MOORLAND, GRASSLANDS, DUNES); HIGHLANDS entspricht ALPINE. Die Warnung zu GLACIER/TUNDRA stützt die offene Frage zu
ICEFIELDS und AURORA. Studien mit 48–72 Personen sind Umfang einer Abschlussarbeit, nicht von Schritt 2.

## Halt 1 – Entscheidungen des Besitzers

Stand 17.11.2026: Streichliste angenommen; Bedienung vereinfacht (drei Regeln); Hörskizzen „okay, nicht krass, aber ok“ –
für Schritt 2 heißt das: mehr Charakter je World, bei gleichen Regeln (keine hässlichen Glocken, kein starkes Rauschen).
Offen: Auswahl der Worlds, Fragen 1–3 und 5–9, Punkte 2 und 5–7.

1. Welche Worlds? Empfehlung: zwölf zuerst (die sechs vorhandenen, CAVE, DUNES, JUNGLE, MOORLAND, BLACKFOREST, GRASSLANDS), vier später (ISLANDS, AURORA, VOLCANO, CANYONS).
2. Dürfen die sechs vorhandenen eigenen Tonvorrat, Zeitgefühl und Natur bekommen? Empfehlung: ja, einzeln per Hörskizze.
3. Die neun Fragen (oben).
4. Die Streichliste. **Entschieden 17.11.: angenommen, erweitert auf 16 (siehe Bedienkonzept).**
5. Cells spielen bei GENERATE mit? Empfehlung: ja.
6. Budgets als verbindlich? Empfehlung: ja.
7. V3 als Standard am Ende von Schritt 3, in eigenem Pull Request? Empfehlung: ja.
