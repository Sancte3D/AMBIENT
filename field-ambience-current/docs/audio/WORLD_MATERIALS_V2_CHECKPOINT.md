# AMBIENT — drei deutlich verschiedene Tonkörper, Materialmodell 2

27.10.2026. Michalis lehnt die bisherige World-Eigenständigkeit als denselben
synthetischen Ton in anderen Oktaven ab. Neuer ausdrücklicher Auftrag:
„sie muesen musikalsich extrem weit auseinander liegen … gleichzeitig …
ihren ambient sound character beibehalten“. Diese Fassung setzt deshalb die
Tonkörper selbst neu an. Eine vierte World wäre jetzt keine Lösung.

Basis ist PR189 / `df9a8a903d444d48133b980760a9686202455637`, Branch
`codex/ambient-distinct-worlds-2026-10-27`. Die frühere positive Einschätzung
war zu großzügig. Das negative Hörurteil gilt für V1; V2 ist ein neuer,
noch nicht persönlich abgenommener Hörkandidat. Kein Blindtest wird behauptet.

## Drei Instrumente statt Registervarianten

| World | Neuer tragender Tonkörper | Einsatz bei neutralem SHAPE | Zeit und Ambient-Charakter |
|---|---|---:|---|
| COAST | Kontinuierlich angeregte, weich gerundete Fläche mit deutlicher zweiter Harmonischer und kleinerer vierter. Diese Anteile bewegen sich gegensinnig, statt nur alle Obertöne gemeinsam leiser zu machen. | 1,8 s | Langsames Aufblühen, gehaltener Körper und zusammenhängende Bewegung; keine Detune-Schwebung. |
| WOODLAND | Einmal angeregter Modal-Körper mit ausgeprägter vierter/sechster Körperregion und wenig zweiter Harmonischer. Hohe Moden klingen abgestuft aus; die vierte bleibt länger hörbar. | 120 ms, weich gerundet; SHAPE 60–360 ms | Erkennbarer weicher Anschlag, danach langer freier Resonanzkörper. Grundton-T60 weiterhin nominell 36 s bei neutralem Release, 9–144 s über den vorhandenen Release-Bereich. Keine fortlaufende Nachanregung. |
| HIGHLANDS | Hohler Pfeifen-/Atemton aus 1./3./5./7./9. Harmonischer, mit tragender dritter und raschem oberen Rolloff. Leiser ungetunter Luftanteil bleibt an den bestehenden Notenbesitzer und seine Hüllkurve gebunden. | 700 ms | Sanfter Atemeinsatz und gehaltener hohler Körper; der Luftcharakter endet nicht mehr nach 90 ms vor dem Lautstärkeaufbau. |

Alle tonalen Komponenten sind ganzzahlige Vielfache derselben tatsächlich
zugelassenen Hz. Keine neue Grundtonlage, Suboktave, Inharmonizität, Pitch-LFO,
Zusatzstimme oder fremde Aufnahme erzeugt die Unterscheidung. COASTs Color
und WOODLANDs engerer Ausgangsshelf behalten ihre charakteristischen Anteile
auch im dunklen Bereich. HIGHLANDS' Filter lässt seine tragende dritte
Harmonische stehen. Helligkeit bleibt dadurch Variation innerhalb eines
Materials. Unterschiedliche Grammatiken, Aktivität und Register bestehen
zusätzlich, ersetzen aber nicht den Vergleich einzelner gleicher Töne.

WOODLANDs breiteres peakbegrenztes Anregungsprofil erhält eine Quellenpegel-
Kompensation von 0,22 auf 0,32 vor der bestehenden Ausgabe. Das ist Teil des
neuen Stimmenentwurfs, keine heimliche Normalisierung im laufenden Audiobus.
Der vorhandene Slot, Modal-/Delay-Union und Retirement bleiben bestehen.
Es gibt keine neuen Audio-/Samplepuffer und keine zusätzliche Quellbelegung.
Note-Off, Clear/Mute und Eigentümerregeln gelten auch für den Atemanteil.

## Aktivierung und Umfang

Der bestehende ausdrückliche Hörschalter `FAM_SOUND_FINISH_CANDIDATE=ON`
verwendet jetzt Materialmodell 2. Ohne diesen Schalter bleiben der bisherige
Product-Pfad und dessen separat hörbarer WOODLAND-Modell-1-Kandidat erhalten.
`reference` bleibt CMake-Default. Alte V1-Hördateien und ihr Git-Stand werden
nicht überschrieben. FX-Modell 1 ist ein anderes Modul und bleibt unverändert.
Kein neues Scene-Format, keine vierte World und keine weitere Bedienebene.
Die noch offenen FX-Regler-/Speicheranbindungen und Geräteabnahmen bleiben offen.

## Kontrollierter Vergleich und Grenzen des Urteils

60 tatsächliche manuelle Enginefälle: MIDI 50/57/62 jeweils Color 0/0,5/1,
zusätzlich MIDI 69 bei Color 0,5; alle drei Worlds trocken und mit den echten
Standard-FX. D-Dur/ET, Velocity 0,7, Master 0,75, SHAPE neutral, Seed 1234,
Nature 0; Note-On 1 s, physisch gehaltene Entfaltung, Note-Off 11 s, Aufnahme
18 s. Ganze PCM-Ausgaben, bestätigte Hz-/Owner-Ereignisse und Belegungstraces
stimmen jeweils zwischen 37/512-Frame-Caps auf demselben Main-Raster exakt
überein. Zusätzlich der tatsächliche 96-s-Generator in beiden Partitionen.
Insgesamt 51.861.600 Stereo-Frames verglichen. Renderer prüfen tatsächliche
Starts/Offs, Quellgrenzen und null NaN-/Ausgangsschutzereignisse.

Vorherige Kennzahlen stammen aus dem bereits gesicherten Audit. Alle dessen
Eingabedateien wurden gegen den alten Git-Stand geprüft. Beide alten ganzen
55-s-Hör-WAVs wurden hier aus dem alten C-Code bytegenau reproduziert;
das ist ein beobachteter lokaler Nachweis, keine universelle Bytegarantie
über unterschiedliche Compiler/libm-Versionen.

Mittlere Kosinusähnlichkeit ausschließlich der Oberton-Amplituden bei den
drei gemeinsamen Tönen, Color 0,5. Ein kleinerer Wert beschreibt andere
gemessene Verteilungen, keine prozentuale Unterscheidbarkeit oder Qualität:

| Paar | V1 trocken | V2 trocken | V2 Standard-FX |
|---|---:|---:|---:|
| COAST / WOODLAND | 0,953 | 0,472 | 0,498 |
| COAST / HIGHLANDS | 0,985 | 0,145 | 0,167 |
| WOODLAND / HIGHLANDS | 0,894 | 0,234 | 0,251 |

Am gemeinsamen trockenen D3 liegen nun 64,00 % / 72,51 % / 58,01 % der
untersuchten harmonischen Energie im Grundton, gegenüber V1 94,67 % /
89,35 % / 93,55 %. Das ist ausdrücklich nicht der Anteil an aller Audioenergie.
Die neuen tragenden Schwerpunkte sind H2 bei COAST, H4 bei WOODLAND und H3
bei HIGHLANDS. Einschwingen und Materialverlust unterscheiden sich ebenfalls.
Das belegt einen erheblichen Entwurfswechsel. Es belegt nicht von selbst,
dass Michalis die Unterscheidung als extrem genug oder den Klang als angenehm
und ausreichend ambient beurteilt. Höhere Obertonenergie kann auch als
aufdringlich wahrgenommen werden; diese Hörentscheidung wird nicht vorweggenommen.

## Technische Prüfungen

- Vollständige Soundtests native und ASan/UBSan: beide Raumvarianten,
  Freeze/Dry/Modellwechsel, maximale Effektmakros, alte Besitzer-/Loop-Ties,
  Clear/Mute und vollständige 45-s-PCM-Paare; alle bestehen.
- 48 laute World-/Color-/SHAPE-/Nature-/Major-Minor-/ET-JI-Grenzfälle und
  18 Minuten tatsächlich generative Musik in zwölf Kontexten, jeweils native
  und sanitisiert, ohne NaN, Ausgangsschutz oder überschrittene Quellgrenzen.
- Bestehender tatsächlicher WOODLAND-Test zusätzlich im vollständigen
  Kandidaten: nominales 36-s-Grundton-T60 über Release 0,25/1/4 erhalten,
  größter gemessener Pitchfehler 0,000120 Cent und relativer T60-Fehler 0,024445.
  Stimme nach 20 s aktiv, natürlicher Test-Ruhestand bei 1.334.842 Frames
  (rund 30,27 s), unveränderter Nachbar, Abbruch und State-Wiederverwendung.
- Nature selbst gegenüber PR189 unverändert: derselbe Nature-only-PCM,
  unabhängige Note-Traces, Additivität, Raumunabhängigkeit und genaue 37/512-
  Ausgabe. 66 tatsächliche Enginecaptures, 67.737.600 verglichene Stereo-
  Frames. In den gemessenen gehaltenen Cell-Fällen Nature im Mittel mindestens
  31,44 dB leiser, lautestes geprüftes 1-s-Fenster mindestens 20,50 dB leiser.
  Keine universelle Pegelgarantie für beliebig leise ausklingende Cells.
- Echte H743-Inputfixture native/sanitisiert: 126 ganze 12-s-Wake-Vergleiche
  sowie Scene-Releases/HOLD/REC, SHIFT-Snapshot und physische Entfaltung
  bestehen mit den neuen Stimmen. Der Reference-/bisherige Product-PCM bleibt
  in den bestehenden Baselinevergleichen unverändert.

Der neue GitHub-ARM-Build gehört zu diesem Branch und muss separat am neuen
HEAD gelesen werden. Vorherige grüne CI760 ist der Parent, nicht dieser
Klangentwurf. Physische H743-Laufzeit, IRQ-/gesamte Stacklast sowie DAC,
Kopfhörer und Lautsprecher bleiben ungemessen. Journal OFF; kein Merge,
Retarget oder physisches Flashen.

## Neue Hörfassung

- `AMBIENT_Worlds_V2_Trocken.mp3/.wav`, 55 s: COAST 0:00, WOODLAND 0:18,5,
  HIGHLANDS 0:37. Identisches D3 und Eingaben, je World ein fester Gain aus
  dem 3–8-s-RMS auf −26 dBFS: +5,89 / +8,50 / +5,65 dB. Die natürliche
  Attack-/Abklinggestalt bleibt bestehen, keine AGC oder Kompression.
- `AMBIENT_Worlds_V2_Musik.mp3/.wav`, 96 s: wirklicher Generator mit
  Standard-FX, COAST 0:00, WOODLAND 0:32, HIGHLANDS 1:04. Nature aus.
  Ein gemeinsamer fester Hörgain +6,02 dB, keine individuelle World-Anhebung.

Beide MP3s sind vollständige kleine Hörkopien derselben jeweiligen WAV-PCM.
Alle Abschnitte erhalten ausschließlich 40-ms-redaktionelle Schnittkanten;
18/32 s sind Ausschnitte, nicht als vollständiger natürlicher Tail ausgegeben.
Vor dem Schnitt werden die vollständigen Vergleiche durchgeführt. Vollständiges
Decoding bestätigt 2.425.500 bzw. 4.233.600 Stereo-Frames. Kennzahlen, tatsächliche
Ereignisse und Eingabe-/Dateihashes: [WORLD_MATERIALS_V2_METRICS.json](WORLD_MATERIALS_V2_METRICS.json).

```sh
python3 -m pip install numpy
python3 tools/review_world_materials.py build-world-materials-v2
python3 tools/review_sound_finish.py build-world-materials-v2/sound
python3 tools/review_nature_accents.py build-world-materials-v2/nature
python3 tools/review_ui_idle.py build-world-materials-v2/ui --fixture-only --finish-candidate --sanitize
cmake -S . -B build-h743-world-materials -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_SOUND_FINISH_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-world-materials
python3 tools/check_product_link.py build-h743-world-materials
```

ffmpeg und die bestehenden C-/ARM-/CMake-Tools sind erforderlich. Die neuen
Offline-Materialvergleiche benötigen numpy; die vorhandene CI führt weiterhin
die erweiterten Sound-/Nature-/Inputtests und den H743-Hörbuild aus.
