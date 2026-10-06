# Sound checkpoint — 2026-10-06

## KERNURTEIL

Der Produktkandidat hat jetzt eine zusammenhängende, kleine Klangarchitektur:
drei tatsächlich unterschiedliche Grammatiken, dieselbe ehrliche Zulassung für
Manual/Generate, reale Hz/Besitzer/Tails, ein gemeinsamer Raum und optionale
Natur. Die größte offene Schwäche ist die Quellen-/Wahrnehmungsabnahme:
Algorithmus, Headroom und Buildgrün beweisen noch keinen beruhigenden Tonkörper.

## FUNDAMENTAL FALSCH

Der Referenzbuild ist kein neuer Produktstand. Seine Archive, impliziten Layer
und neun FX werden im Product-Profil nicht kompiliert. Eine ungeeignete Quelle
mit mehr Hall/Natur oder Storytelling zu rechtfertigen bleibt falsch.
Es gibt keine belegte Heilwirkung und keinen universellen Neurodivergenzclaim.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Bowed-Grain und 1,5f/2f-Sympathie, Horn-Onset-Air und Pluck-Artikulation
  sind trotz technischer Korrekturen noch hörseitig offen.
- HIGHLANDS muss trocken eine eigene ruhige Funktion zeigen oder entfallen.
- Optionales Nature muss einen Ort beitragen; kein Default-Rauschen erzwingen.
- Color/SHAPE/Room-Enden und feste Quellenlautheit brauchen Urteil am Ton,
  anschließend an tatsächlicher Ausgangslautstärke.
- Flash-Save blockiert Main/Generate-Planung; kein Journal erhält einen alten
  Save während unterbrochenem Erase. Cache-/Bankguard ersetzt kein Gerätetest.
- Physischer Drive/Altmodi sind vorläufige UX-Kompatibilität, keine fertige
  Bedienung. Die spätere Oberfläche darf keine funktionslosen Klangregler zeigen.

## LOCKED

Geprüfter technischer Vertrag:

- Drei globale Slots; Pluck zwei; Releases und One-shots belegen weiter.
- Originalbesitzer und angewandte Hz bis zum wirklichen DSP-Ende.
- Erst DSP-Ack schreibt Onset/MIDI/gehörtes Motiv, kein Phantom-Commit.
- Keine automatische Pad/Bass/Drone/Archiv- oder Rauschbegleitung.
- Eine interne Float-Roomengine, Nature separat und bootmäßig 0.
- Natürliches Pluck-Key-up, kein 20-ms-Ping durch kurzes Loslassen.
- Samplegenaues Pluck-Retirement statt callerabhängigem Blockende.
- Ganze Kette Clear/Mute linear ≤40 ms auf exakt Null, DSP hinter Null leeren;
  Unmute ruft keinen alten Raum zurück.
- Neue Pitchkontexte stornieren ungehörte Vorbereitung; bestätigte Hz bleiben.
- Ganze Source/Room/Master-Trajektorie 64 vs512: **max_delta 0** in allen Welten;
  ungültige Floats: bit-identisch.
- Kein Heap/Quellenaufbau/I/O im Render; drei feste Slots/16 Tails/eine Figur;
  konstante begrenzte Suche.

### Gemergte Checkpoints und geprüfte Stände

Ziel ausschließlich `claude/hall-sensor-bom-pcb-update-a8xj82`; kein Main-Release.

| PR | Merge / getesteter Code | Nachweis |
|---|---|---|
| [130](https://github.com/Sancte3D/AMBIENT/pull/130) | 89c352ca9faa361ed3589a1527740dc52504c52c | COAST Grundtonswing 3,102→0,048750 dB, 24 echte PCM-Probes; finite Setter |
| [131](https://github.com/Sancte3D/AMBIENT/pull/131) | 7b9bed37731d0b4758dd63479a9448153e71ce16 | 11520 deterministische Grammatiktransaktionen, wirkliche Antwortgrade, Wrap |
| [132](https://github.com/Sancte3D/AMBIENT/pull/132) | aebdc4414bd4f7c09bd39e042641946813977ff3 | tatsächlicher gemeinsamer Kern; 12 min PCM, Source/Hz/Room ownership |
| [133](https://github.com/Sancte3D/AMBIENT/pull/133) | e8d37d52b392bb4384aea0584cca26a37bad9ed8 | reduzierter realer H743-Link, SCN7/SCN5/6-Migration und Produktcontrolintegration |
| [134](https://github.com/Sancte3D/AMBIENT/pull/134) | abfe9c3eb3fa666fd9272c8f31ce42500a66946e | Room-Null/kalt, orthogonale Mono-Readouts, Nature/Seed/Scoretrennung, 27-s-Hörpacks |
| [135](https://github.com/Sancte3D/AMBIENT/pull/135) | 1f4d435afbc3a890e826cac214f06de84044e5eb, in PR136 enthalten | echte Intervallerinnerung, spätere Figurenrückkehr, schnelle Kontextübergabe, zusätzliche 36 min PCM |
| [136](https://github.com/Sancte3D/AMBIENT/pull/136) | letzter voller Zwischenstand b5ddf4049fb627fe88ca8fe1d579f710e2bd1948 | CI 37511549818, 6/6 grün; Mute, natürliche Plucks, Sample-Ende, Pitchkontexte, True Peak/Blind/Endpoints/Frames |

Finale Cache-/volle-Pool-Übergangsergänzungen erhalten erneute CI vor Merge.
PR135 darf durch den vollständig geprüften Gesamtstand PR136 mit übernommen
werden; ein hängender älterer Bench-Job ist keine Audiofreigabe und wird nicht
als erfolgreich ausgegeben. Hör-/Gerätestatus bleiben in jeder Version offen.

### Ressourcen — tatsächlich gelinkter H743, keine Host-Schätzung

Release, ARM-GCC, Cortex-M7 hard float. Bankwerte des angegebenen jeweiligen
CI-/Codezustands; DTCM 16 KiB ist **Stackreservierung**, kein gemessener Stack.

| Stand | FLASH B | DTCM B | D1 B | D2 B |
|---|---:|---:|---:|---:|
| PR133 Product | 184740 | 16384 | 65440 | 60808 |
| PR134 Product | 188852 | 16384 | 65472 | 60808 |
| PR135 Product | 189876 | 16384 | 65600 | 60808 |
| PR136 b5dd / CI37511549818 Product | 190684 | 16384 | 65600 | 60808 |
| derselbe geprüfte Reference | 256628 | 119440 | 417440 | 258112 |
| Linkerbudget | 1966080 | 131072 | 524288 | 294912 |

Produkt-Audio braucht keine SD, Samplebank oder externe PSRAM. Keine zugesagte
SD-Nachrüstbarkeit. Ein zusätzlicher Bank-1-Loadimageguard (≤1048576 B) schützt
die Bank-2-Scene-Schreibarchitektur; das größere Flash-Linkerbudget allein
wäre dafür nicht ausreichend.

Compilerframes im b5dd-Stand: engine_render 88 B, Room192 B, Nature136 B,
GenerateTick176 B, SceneSave104 B; maximaler einzelner Coreframe192 B.
Pluck-Wrapper 0 B bedeutet nicht null Stackbedarf seiner Callees.
Keine Aussage über Callchain, IRQ/FPU-Stacking, libc oder tatsächlichen High-water.
DWT muss Cache/DMA-/HAL-Arbeit zusätzlich zum bestehenden Rendererfenster erfassen.

### Audio-Messung und Nachweisgrenze

- Jeder volle Hostlauf rendert mindestens **48 min tatsächliches PCM**
  (12+36), zuzüglich Room-/Nature-/Transition-/Recovery-Probes.
- 12 lange Grenzfälle, jeweils 180 s: 14..40 bestätigte Onsets; längste
  gemessene Onsetlücke im bisherigen Grenzstand 35,759 s; kein Stuck/NaN/Limiter.
  Das sind ausgewählte Fälle, kein Beweis für sämtliche Seeds/Benutzerverläufe.
- 859 gehörte Figurenrückkehren in Pure-Score-Tests, begrenzte Intervallmemory;
  späteres Wiedererkennen braucht zusätzlich eine Hörabnahme.
- Wet-Mono-Energie beim Roomimpuls vorher ~0,34, danach 0,75126 bei Room0,5 /
  0,74651 bei Room1. Der Readoutumbau erhöht auch Wetimpulsenergie; kein
  behaupteter gleicher subjektiver Hallpegel.
- 24 reale Summen-/Targetstep-Probes bei Volume1/Velocity1/Room1:
  **worst true peak −9,84 dBFS**, schlechteste Monoenergie **0,93006**,
  Mean-DC innerhalb 0,0002. Keine notwendigen Limiter-Eingriffe.
- Hörpacks: RAW-Firmwarepegel + separate konstante Vergleichsgain,
  ≤+12 dB, Ziel −26 LUFS / TP-Decke−6. Nature bleibt bewusst viel leiser.
  Jede WAV **27 s**, keine lange WAV aus Langzeittests.

Quelle: actual-product Tests `test_product_sound.c`, `test_product_scene.c`,
`test_product_stress.c`, `test_room_nature.c`, `test_world_grammar.c`;
`review_product_audio.py` und `check_product_link.py`.
Onsettrace bezeichnet Block-Acknowledgement, nicht sampleexakten Anfang.

## REMOVE / MERGE / REDESIGN

Aus Produktlink entfernt: Archive/V2, zusätzliche Pad-/Bass-/Drone-/Choir-/
Guembri-/Ember-/Glasspfade, Body, Alttexturen, zweite Raumengine,
Echo/Chorus/Age/Blur/Shimmer/Reverse/Dream. Kein neuer Effekt als Ersatz.
Alle noch zugänglichen produktiven/retired Parameter und Callrollen sind in
[PRODUCT_SOUND_SPEC.md](PRODUCT_SOUND_SPEC.md) einzeln zugeordnet.

SCN7 ersetzt fünf alte Worlds dokumentiert und versioniert:
Alps→HIGHLANDS, Open Sea/Fjords→COAST, Moss Fields/Desert→WOODLAND.
Migrationprovenienz bleibt bis Save erhalten; kein Anspruch auf alten Klang.
CRC/Fehlsave-RAM-Rollback/Volume-Erhalt/stiller Boot sind real getestet.
Power-loss-Erhaltung und unterbrechungsfreier Live-Save bleiben offen.

## BESTE VERSION

COAST, WOODLAND und gegebenenfalls HIGHLANDS fühlen sich wie unterschiedliche
musikalische Zeit in derselben ruhigen Haltung an. Tonwahl, Geräuschanteil,
Pegel und Raum werden nicht durch Effektdichte ersetzt. Ein kurzer manueller
Pluck ist ein klingender Ton; Generate hat hörbare Erinnerung statt Timerwürfeln.
Stillwerden ist ein verlässlicher Audiobefehl. Die Quellen werden trocken
gewählt, dann gemeinsam gehört; schwache Teile fallen weg.

### Nächste konkreten Gates, keine neue Featurequeue

1. Dry/Register (D3/D4/A4), Color0/0,5/1 und SHAPE0/0,5/1 hören:
   Zeitstelle für Alarm/Piep/Tube/Buzz/Grundtonverlust benennen.
   HIGHLANDS behalten oder entfernen; danach Quellen-/Rollenpegel final wählen.
2. Blind A/B/C ohne Room/Nature: Artikulation, Beziehungen und Pausen beschreiben;
   danach dieselben Worlds mit gemeinsamem Room vergleichen.
3. Nature allein und zur Musik: Ortsnutzen, mechanische Wiederkehr, homogene
   Rauschbänder, Ticken oder Maskierung. Entbehrliches Nature entfernen.
4. Langzeitform an ausgewählten 27-s-Fenstern vergleichen; längere reale
   Nutzung nicht durch kurze Anfangsdateien ersetzen.
5. UX auf dieselben Audiooperationen legen; Drive-/Altmodusreste entfernen,
   Generate-Lock, 15-min-Displayruhe/LED und klangneutrales Wake gestalten.
6. Geräte-/Ausgangs-/Storage-/Langhörgates schließen; erst dann Sound-Freeze.

## TEST AM GERÄT

- DWT peak_load <0,60, null Deadline-Misses, gemessener Main-/IRQ-Stack,
  kalter Cache, volle Releases, Note-on, Clear, Display/LED/MIDI und Save.
- Tatsächliche DAC/Amp-/Ausgangslasten, Mono-Körper, Pegel/Headroom/DC/
  Eigennoise, Mute-/Boot-/Wake-/Power-Pops und reale Clockkonfiguration.
- Flash-Read nach Save (M7-Cache), Save-Latenz/Generate-Timing, Fehler/
  Unterspannung/Power-loss/ECC; kein unterbrochener Save als Erfolg.
- Ruhige längere Hörsitzungen bei echter niedriger und typischer Lautstärke;
  konkrete Störstellen lösen oder entfernen. Keine allgemeine Heil-/Verträglichkeitszusage.

**Abschlussstand:** 24 reine Software-/Remove-Aufgaben geschlossen, 30 Aufgaben
mit klaren offenen Gates. Softwarekonzept und Implementierung sind konkret
reviewbar; kein einziger KEEP-Tonkörper erhält eine erfundene Hörfreigabe.
