# AMBIENT — Product Sound Candidate 0.3

Stand 2026-10-06. Gilt ausschließlich für `FAM_SOUND_PROFILE=product` /
`FAM_SOUND_PRODUCT`, Wireformat SCN7. Dies ist der reproduzierbare
**Softwarevertrag**, keine endgültige Klang- oder Gerätefreigabe.
`reference` bleibt der CMake-Default; historische Beschreibungen gelten dafür.
Der aktuelle Nachweis und die offenen Gates stehen in
[PRODUCT_CORE_CHECKPOINT.md](PRODUCT_CORE_CHECKPOINT.md), die vollständige
Arbeitsliste in [AMBIENT_SOUND_DESIGN_TODO.md](AMBIENT_SOUND_DESIGN_TODO.md).

## Produktidee und bewusst kleiner Umfang

Drei Naturorte unterscheiden sich durch Tonkörper und musikalische Zeit:
COAST übergibt Zusammenhang, WOODLAND erinnert und antwortet, HIGHLANDS lässt
zwischen Fragmenten Raum. Ein manueller Ton und Generate verwenden dieselbe
Quelle und Zulassung. Gemeinsame Harmonik und ein gemeinsamer Raum verbinden
sie. Nature ist eine getrennte, optionale Ortsandeutung, bootmäßig aus.

Die drei Quellen sind Kandidaten. HIGHLANDS wird entfernt, wenn der trockene
Vergleich keine eigene ruhige Artikulation zeigt; eine schwache dritte Welt
ist kein Produktgewinn. Historische Instrumentnamen im Code bezeichnen
Anregungsmodelle, keine authentischen Rekonstruktionen oder Kulturwelten.
Beruhigung ist das Gestaltungsziel, keine belegte medizinische Wirkung.

## Vollständiges Laufzeitinventar

| Weg | Entscheidung und Funktion | Erreichbarkeit / Nachweis |
|---|---|---|
| Bowed / Pluck / Horn | KEEP als COAST / WOODLAND / HIGHLANDS-Kandidaten | Einzige tonal rendernde Familien in `engine_product.c`; gleicher Admit-Pfad für Manual/Generate |
| SHAPE, Tuning, World Grammar, Brain/Pitch-Helfer | KEEP, Artikulation und neue Tonwahl | Fixe Stategrößen, kein eigener Audiolayer |
| `ambient_room.c` | KEEP, ein gemeinsamer Float-FDN | Eine interne D2-Tank-/Diffusionsbelegung; keine World-Hallkopien |
| `nature.c` | KEEP als optionale Kandidatenebene, default 0 | COAST leise Küstenbewegung, WOODLAND vereinzelte weich gefilterte Tropfen, HIGHLANDS Wind; nicht getunt, kein tonal belegter Slot |
| Bowed Grain / Horn Onset-Air | KEEP als leise Quellenartikulation | Keine frei wählbare Geräuschwelt; Hörgate gegen Buzz/Piep bleibt offen |
| Body / fixe Weltmaterialien | REMOVE aus Produkt | Nicht kompiliert, keine Additivfärbung oder World-Tankresets |
| Echo / Chorus / Tape Age / Blur / Shimmer / Reverse / Dream | REMOVE aus Produkt | Keine eigenständige Rolle nötig für den gewählten Quellen-/Raumkern; weder Engineadapter noch Scene aktiviert diese Wege |
| Pad / PADsynth / Ember / Glass / Choir / Guembri / Bass / Sub / Drone | REFERENCE, im Produkt REMOVE | Keine implizite Begleitung, kein bass-follow oder unowned One-shot; nicht kompiliert |
| V2, sechs `Synths_Archive`, Arp / Beat / Field / Beauty Guard | REFERENCE, im Produkt REMOVE | V2-Liste leer; Hauptprogramm registriert keinen Backend und initialisiert keine V2-Engine |
| Alte `ambience.c`, `texture.c`, Sea-Hum / Hiss / Brown-Rumble / Crackle | REFERENCE, im Produkt REMOVE | Keine alternative Nature-/Master-Rauschquelle kompiliert |
| `reverb.c`, `fx_master.c`, `ambient_effects.c` und Legacy-FX | REFERENCE, im Produkt REMOVE | Kein zweiter Hall, keine 245-KiB-Effektarena |
| Bloom / Rolepatch / Landscape / Gesture / Controls / Menu | Vorläufige Control-Kompatibilität, keine zusätzliche Quelle | Produkt-Controls verwenden den direkten Quellenpfad; reservierte alte Owner und Layer-APIs werden abgewiesen/no-op |
| Note/MIDI Hook | KEEP als Beobachter | Meldet erst bestätigten DSP-Start; kein eigener Audiostart, nur Control-Kontext |
| Volume / DC / Peak Protect / PCM / HAL | KEEP | Nach Room und Nature, für die gesamte Summe |
| SCN5 / SCN6 | Nur Migration | Kein Rückweg in Archiv-Synths, alte FX oder verborgene Layer |
| Offline-Renderer / Audits | REFERENCE/Nachweis, keine Firmwarefunktion | Reale produktive C-Kette, alle Hör-WAVs ≤27 s |

`tools/check_product_link.py` prüft die tatsächlichen ARM-Kompilierinputs,
verbotene DSP-Funktionssymbole, einen D2-Raum und den ausgerichteten D1-DMA-Puffer.
Der bloße Archivordner oder Amount=0 gilt ausdrücklich nicht als Entfernung.

## Tonhöhe, Eigentümer und Belegung

- Produktfrequenz 140..470 Hz; gewählte Noten MIDI 50..69. Major-Core
  `{0,2,4,7,9}`, Minor-Core `{0,3,5,7,10}`, relativ zur gewählten Tonart.
  Es gibt zwei Sammlungen, keine sechs dekorativen Modi.
- Equal oder 5-limit Just relativ zum aktuellen Key. MIDI-Root
  `50 + ((pitchClass-2+12)%12)`. Die tatsächliche Hz bleibt maßgeblich.
- Eingaben müssen höchstens 3 Cent vom **aktuellen** erlaubten Ton abweichen.
  Außerhalb wird abgewiesen; kein verborgenes Clamping/Transponieren.
- Neue lebende Unisoni unter 3 Cent werden vermieden. Andere Varianten
  desselben Tons unter 90 Cent, Halbtonklasse 100±45 Cent und Tritonusklasse
  600±45 Cent sind gesperrt. Unter 261,63 Hz mindestens 249,5 Cent Abstand,
  unter 196 Hz mindestens 299,5 Cent. Exakt gestimmte gemeinsame Raumtöne
  dürfen verbinden. Das ersetzt nicht die Hörprüfung tatsächlicher Teiltöne.
- **Drei globale Slots**, einschließlich Quellenrelease und Übergabe; Pluck
  zusätzlich maximal zwei. Kein held Steal und keine unbeschränkte Warteschlange.
  Manual-Owner 0..4 / 9..13, Generate 6 / 7 / 15; alte Rollen 5 / 8 gesperrt.
- Vorbereitete Einsätze reservieren einen Slot. Erst nach echtem DSP-Beginn
  werden Hook, gehörtes Motiv und Note-Counter bestätigt. Abbruch davor bleibt
  ohne Onset. Ledger/Hooks laufen in Main; Audiokontext besitzt DSP/Retirement.
- Key-up beendet Bowed/Horn mit ihrem natürlichen Release. Ein **bereits
  gehörter Pluck klingt natürlich aus**; sein ursprünglicher Besitzer und Slot
  bleiben bis zum tatsächlichen Ende belegt. Vorbereitung ohne ersten Sample
  wird beim Key-up storniert. Wiederanschlag eines noch belegten Owners kann
  abgewiesen werden; die spätere UX darf keinen Erfolg vortäuschen.
- Nach tatsächlichem Quellenende: maximal 16 konservative Raum-Hz-Einträge,
  Ende spätestens 7,2 s später (1,5× maximal nominalem T60), oder nach
  250 ms gemessener nasser Ruhe unter 1e-5 ohne aktive Quellen. Drei Plätze
  werden für ausgehende Quellen reserviert, relevante Historie nicht verdrängt.
  Kein Anspruch auf universelle psychoakustische Unhörbarkeit dieser Schwelle.

## Drei deterministische, begrenzte Grammatiken

Nur bestätigte Töne werden gespeichert. Ein Vorschlag verändert den lebenden
Score/RNG nicht. Initialseed bei 0: 0xA6B13E7D; World-Seed
`seed XOR (world*0x9E3779B9)`. Nature verwendet einen eigenen RNG/Seedkanal.
Dies reproduziert bei derselben zeitlichen Eingabefolge Entscheidungen und
PCM; es verspricht keine identische Aufnahme nach beliebigem Bedienverlauf.

| Welt | Timing vor Activity | Erinnerung / Variation | Grenze |
|---|---|---|---|
| COAST | Hold 6..14 s; Abstand 4..10 s; Episodenruhe 4..9 s | Fokus auf tatsächlich gehörtem Anfang, lokale Schritte ±2, Fokuswechsel um zwei Grade nach Episode | höchstens 2 gehaltene Stimmen; insgesamt weiterhin 3 inklusive Release |
| WOODLAND | 2..3 Töne; Abstand 2..6 s; Antwortgrenze zusätzlich 3..6 s; Ruhe 6..16 s | gehörte Seedfigur → Kontur-/Intervallantwort → letzte Stufe begrenzt variieren → Ruhe; letzte Figur gelegentlich später um −1/0/+1 Grad versetzt | zwei Plucks; reine natürliche Decays, kein erfundener Hold |
| HIGHLANDS | 2..4 Töne; Hold 1,5..5 s; Abstand Hold +0,9..2,4 s; Ruhe 8..20 s | kurzer Bogen mit Rückbezug, später gelegentlich gehörtes Fragment wieder aufnehmen | höchstens 1 gehaltene Stimme; Releases zählen weiter im gemeinsamen Budget |

Episodeninitialisierung 45..120 s, mit expliziter Gültigkeit auch bei einer
Deadline genau 0 am Timerwrap. Keine periodische Audio-/Quellenresetpflicht.
WOODLAND/HIGHLANDS erinnern höchstens eine Figur und deren tatsächlich gehörte,
auf Activity normierte Abstände (1,4..10 s). Bei 1:4 Wiederkehrentscheidung
bleibt die gesamte verschobene Figur im Register; kein nachträgliches Clamp,
das die Kontur zerdrückt. Nicht jedes kurze Exzerpt enthält eine Wiederkehr.

Activity-Abstandsfaktor `1.35 - 0.70*a`, minimale Lücke 1,4 s.
Holdzeiten werden davon nicht skaliert. Rollenvelocity 0,64..0,80.
Gen-Abweisung wartet 250 ms bei Belegung / 500 ms nach fehlender Tonzulassung.
COAST/HIGHLANDS suchen höchstens fünf nahe Alternativen; WOODLAND bewahrt
seine gehörte Figur. Gehört wird die erfolgreiche tatsächliche Alternative,
nicht der ursprüngliche Wunsch. Alle Grammatiken verwenden höchstens acht
Grade, auch wenn eine Transposition neun zulässige Registertöne ergibt.

## Versioniertes Parameterregister

Alle Floatwerte: NaN/Inf bewahren das letzte gültige Ziel; endliche
Bereichsverletzungen begrenzt. Alle Setter werden aus Main aufgerufen,
Audioberechnung liest feste/atomare Ziele. Physische Bindings sind vorläufig.
Scene-Werte sind Parameter, keine Besitzer-/Envelope-/Raumsnapshots.

| Parameter / Zweck | Besitzer, Einheit, min / default / max | Kurve, Glättung und Livewirkung | Persistenz / Priorität / Null |
|---|---|---|---|
| World | Produkt, ID 0 / COAST / 2 | neue Familien-/Zeitwahl; bestehende manuelle Hz bleiben | SCN7; Worldwahl erhält alle gemeinsamen Userwerte |
| Key | Harmonie, Pitch Class 0 / D=2 / 11 | mod12, begrenzter Registerroot; nur neue Einsätze | SCN7; idempotent, kein Retune klingender Quellen |
| Collection | Harmonie, 0 Major / 0 / 1 Minor | zwei obige Cores; nur neue Einsätze | SCN7-Collection; keine alte Mode-ID still umdeuten |
| Tuning | Harmonie, 0 Equal / 0 / 1 Just | neue Hz; alte angewandte Hz/Fahnen geschützt | SCN7; keine Pitchrampe |
| Color | Quelle, normiert 0 / 0,5 / 1 | Bowed-Cutofffaktor 0,8..1,2; Horn 0,85..1,15; Pluck Damp 0,65..0,30, FIR-Seitengewicht Damp×0,25/0,9; 80-ms-Zeitkonstante, auch live | via Brightnessadapter in SCN7; World verändert Color nicht |
| Activity | Score, 0 / 0,5 / 1 | Faktor 1,35..0,65, nur kommende Angebote; weder Gain/Color/Key noch zusätzliche Quellen | SCN7; 0 ist langsame aktive Welt, kein Stop |
| Attack | neue Quelle, 0 / 0,5 / 1 | SHAPE x=0,35+0,30*a; Faktor 0,125×64^x = 0,5359 / 1 / 1,8661; note-on only | SCN7; keine rückwirkende Hüllkurvenänderung |
| Release | neue Quelle, 0 / 0,5 / 1 | x=0,35+0,30*r; Faktor 0,25×16^x = 0,6598 / 1 / 1,5157; note-on only | SCN7; 0 bleibt musikalischer Release |
| Room | gemeinsamer Raum, 0 / 0,5 / 1 | nominal T60=1,2+3,6*r² s; Wet=(0,12+0,50*r)*r; Amount linear 80 ms über vollen Bereich, Feedback 120-ms-Zeitkonstante | SCN7; 0 nach Rampe exakt Dry, kalter Pfad und Tank leer; World leert Tank nicht |
| Dry/Room | Raum, ID 0 / Room=1 / 1 | Enable linear 40 ms, danach echter kalter Dry-Pfad | SCN7; nur zwei Fälle, kein Archiv-FX-Rückweg |
| Nature | separater Ort, 0 / 0 / 1 | Amount 2-s-Zeitkonstante; World/Seed-Fade down 80 ms / up 2 s; unabhängig von Room und Score-RNG | SCN7; 0 kalt, kein Wetter-/Filterwork; positiv explizit auch ohne Phantomton hörbar |
| Generate | Laufabsicht, false / false / true | Eintritt alte Manualquellen weich ≤100 ms beenden; Ton erst nach Budget-/Harmoniezulassung; Stop natürliche Releases + Nature-Target 0 | nicht persistiert; keine Wiederherstellung alter Besitzer |
| Autoplay event gate | Control-Kompatibilität, 0 / 1 / 1 | sperrt neue World-Ereignisse, lässt natürliche Tails | nicht persistiert; kein weiterer UI-Modus versprochen |
| Seed / New Field | Score, uint32 / 0xA6B13E7D | 0 auf Default; neuer begrenzter Score, erzeugte Quellen weich beenden; eigener Nature-Seed | SCN7; allein startet Generate nicht |
| User presence | Control, false / false / true | explizite Präsenz plus 8-s-Nachpause unterdrückt Generate; Display-idle zählt nicht | nicht persistiert; keine physische Tastenentscheidung |
| Volume | Master, normiert 0 / Host 0,6; Gerät 0,3 / 1 | 120-ms-Zeitkonstante, nach Room/Nature; unabhängig von Klangfarbe | **nicht** SCN7; Recall erhält Wert; 0 reduziert Pegel, ist kein State-Clear |
| Mute | Ausgabe, false / false / true | ganze Kette linear ≤40 ms auf exakt 0; hinter Null DSP/Room leeren; Targets und Generate-Absicht erhalten | nicht SCN7; weder Scene/Nature noch neue Notes umgehen Mute; Unmute nur frische Ereignisse |
| Clear | Sitzung, Befehl | ganze Kette ≤40 ms auf 0; Sources/Nature/Room/DC/gehörte Tailhistory leeren, Generate off | keine Parameter-/Volume-Rücksetzung; DMA läuft weiter |

Quellenbasis: Bowed Attack 0,30 s ×Faktor, Release-Zeitkonstante 0,90 s ×Faktor;
Horn 0,45 s / 0,70 s ×Faktor; Pluck Attack 8 ms ×Faktor (4..32-ms-Sicherheitsgrenze,
im Produkt ca. 4,29..14,93 ms), nominaler Loop-T60 3,2 s ×Releasefaktor, Filter
kürzt ihn. Die Release-Zeitkonstante ist **nicht** die volle hörbare Taildauer.
Pluck-Naturalende wird pro Sample bei getracktem Envelope <2,5e-4 bestimmt.

Bowed ist fest Farbe 0: kein detunter Begleitsägeoszillator, kein gemeinsamer
0,13-Hz-Body-LFO; leiser Grain und 1,5f/2f-Sympathie bleiben Hörkandidaten.
Horn: keine Suboktave/fester 950-Hz-Formant, Produkt-Drift 0 statt 0,9-Hz-LFO;
90-ms-Onset-Air bleibt leise. Pan ist familiengebunden und kein Produktregler.

### Öffentliche Adapter vollständig zuordnen

| API | Produktbedeutung |
|---|---|
| `engine_try_note_on` | normierte Velocity 0..1, tatsächliche Hz, Manual-Owner, bool Erfolg |
| `engine_note_on` / `engine_cell_sample` | Legacy-Cell-Amplitude / CELL_AMP_MAX, dann derselbe Admit; kein Pad |
| `engine_try_world_note_on` | derselbe Admit mit Generate-/Owner-/Suppression-Gate |
| `engine_note_off` | ursprünglicher Besitzer, natürlicher Familienausklang; Pluck-One-shot-Regel |
| `engine_set_space` / `engine_set_reverb_size` | Room-Alias, kein zweiter Hall |
| `engine_set_atmosphere` | Nature-Alias, kein Source-Send |
| `engine_set_motion` | Activity-Alias, kein Filter-LFO |
| `engine_set_mood` | Color-Alias |
| `engine_set_brightness` | Legacy-Offseteinheit −600 / 0 / +800, **keine Hz-Cutoff-Frequenz**; Color 0,5+b/1200 bei b<0, sonst 0,5+b/1600 |
| `engine_set_key` | MIDI mod12 auf Key-PC, kein Bass-/Droneglide |
| `engine_generative_new_field` | Seed-Alias, keine versteckte neue Source |
| `engine_generative_advance` | Beobachtung eines Episodenindex, keine Chordmutation und kein Ton |
| `engine_set_note_hook`, Frequenz-/Note-/Source-/Counter-Getter | Beobachtung; konservative reservierte/ausklingende Belegung, keine neue Quelle |
| `engine_boot_mute` | vor DMA Volume current/target exakt 0, kein Laufzeit-Mute-Ersatz |
| `engine_set_synth_backend/synth/synth_param/voice/pad_voice/vibe` | retired no-op; Synthgetter 0 |
| `engine_set_drone`, `engine_bass_follow/set/off/glide/depth`, `engine_motif_strike/sparkle_strike` | retired no-op; Bassactive false |
| `engine_set_drive/reverb_drive/wet_amp/send/reverb_damp/resonance/sweep/envmod/texture/age/echo/blur/shimmer` | retired no-op; Resonancegetter 0 |
| `engine_generative_nudge` | retired no-op; keine Phantom-Intentfunktion |
| `bowed_set_colour`, freie Source-/FX-Internals, native Slotwerte | kein erreichbarer Produktregler; Engine/Scene/HAL verwenden sie nicht als Klangwahl |

Physischer Drive-Encoder und alter Bypass sind im Kandidaten funktionslos.
Sie dürfen im endgültigen UI nicht als Klangregler erscheinen. Die spätere
UX entscheidet Anzahl/Namen/Bindings; dieser Vertrag erzeugt keinen Auftrag
für weitere Encoder, Menüseiten oder Shift-Funktionen.

## Gain und Ausgabekette

Manual/Generate: normierte Velocity × **0,50 Bowed/Horn / 0,22 Pluck**.
Feste Faktoren, keine automatische Master-Gainkompensation oder AGC.
Gemeinsamer Source-Send 0,35; danach ein Room; Nature direkt danach.
Master-DC-Tracker 0,00228 pro Sample (ca. 16 Hz), Volume und 40-ms-Gate,
Peakschutz ab |x|>0,75, PCM16. Schutz-/NaN-Counter sind im normalen
geprüften Lastfall 0; ein Limiter ersetzt keine gesunde Gainstruktur.

Room: 8 feste Delaylinien 1117/1277/1429/1601/1789/1999/2203/2423 Samples,
vier Allpässe 239/389/277/419, Householder-Feedback, 0,25 Dämpfung.
Input-HP ca. 100 Hz (kein Feedback-HP), feste Stereo-Seitengewichtung 0,30,
orthogonale balancierte 8-Tap-Readouts ×0,1775352. Keine live veränderbaren
Delaylängen und damit keine durch Room-Zeit verursachte Pitchfahrt.
Float-Tank 55.352 B + Diffusion 5.296 B = 60.648 B zuzüglich kleiner States.
D2-Gesamtbelegung ist davon getrennt im ARM-Linknachweis ausgewiesen.

## Kontextwechsel und Persistenz

World-/Seedwechsel beendet nur erzeugte Quellen ≤100 ms (Pluck 20 ms),
storniert vorbereitete alte Angebote, erhält manuelle klingende Frequenzen
und den gemeinsamen Raum. Nur das jüngste Ziel wird geplant. Auf Entry in
Generate gilt dieselbe begrenzte Übergabe für alte Manualquellen.

Key/Collection/Tuning betreffen neue Töne. Bereits bestätigte Besitzer/Hz
bleiben stehen; ungehörte alte Vorbereitungen werden storniert. Gradgedächtnis
wird im neuen Kontext neu begonnen statt rückwirkend umgedeutet. Derselbe
Setterwert erneut ist kein Reset. Alte Tails können neue Zulassung verzögern.

SCN7: 5 Slots, exakt 368 B inkl. CRC32; Slot 72 B, Collection im bisherigen
SCN6-Paddingbyte 65; Brightness 66, Seed 68. Store-CRC umfasst Header/Slots
inkl. Padding. Collectionbit 0x80 kennzeichnet Ersatzmigration bis Save.

| Altkatalog | SCN7-Zuordnung |
|---|---|
| Alps (0) | HIGHLANDS (2), Major |
| Open Sea (1) | COAST (0), Major |
| Fjords (2) | COAST (0), Minor |
| Moss Fields (3) | WOODLAND (1), Minor |
| Desert (4) | WOODLAND (1), Minor |

Ersatzzuordnungen sind keine akustische Erhaltung alter Scenes. Native/Layer/
alten FX/Lock-Felder 0; altes FX0 → Dry, sonst Room; Nature0, Activity50.
Key/Tuning/SHAPE/Room/Color/Seed werden begrenzt übertragen. Genaue SCN5-
(32-B-Slot) und SCN6-Layouts erhalten echte Fixtures. Kaputte CRC/fehlende
Persistenz: leere stille Scene-Liste, keine Wiedergabe. Save-Fehler stellt
RAM-Slot/CRC/Active/Zeitanzeige wieder her. Volume und Generate-Besitzer
werden nicht gespeichert; Recall startet Generate nicht aus einem Off-Zustand.

Flash: reservierter Bank-2-Sektor ab 0x081E0000, maximal 512-B-Staging.
Magic-Flashword zuletzt; M7-Cache vor Read gezielt invalidieren.
**Kein Journal:** Stromverlust während Erase kann den vorherigen Save verlieren.
Eine gültige vorherige Flashversion wird nicht atomar erhalten. Zudem blockiert
Erase/Program den Main-Loop, also auch Generate-Planung; das ist kein
zertifizierter Live-Save. Bank-1-Imageguard schützt gegen Firmware-Spill in
die Schreibbank. Save-Latenz, ECC/Power-loss und Storage-UX bleiben Gates.

## Ressourcen und zeitlicher Vertrag

44100 Hz, Stereo PCM16, H743-Renderblock 512 Frames (=11,61 ms); beliebige
positive Renderlänge wird in höchstens 512 zerlegt. Rampenangaben (40/80/100 ms)
sind nominelle Audioprozessor-Dauern; Befehlsübernahme/Retirement-Ack kommt an
Blockgrenzen hinzu (bei 512 bis 11,61 ms), nicht als latenzfreier Main-Aufruf. Source/Room-Rampen und
natürliches Pluck-Ende sind samplebasiert. 64-vs-512-PCM und ungültige
Eingaben werden gegen den tatsächlichen Kern geprüft.

Kein Heap, kein I/O und keine Quellenanregung im Audio-Render. Drei feste
Slots, 16 Tails, eine Figur, fünf Alternative-Pitches, keine unbounded Suche.
Pluck-Vorbereitung: fixer 1024-Float-Loop, höchstens acht harmonische Moden;
im Produkt N≤315, keine große Stacktable. Sourcefilterkoeffizienten maximal
alle 32 Samples pro aktiver Bowed/Horn-Quelle; `tanf` bleibt ein echter
Worst-case-CPU-Kandidat. Room pro Sample 8 Feedbacklinien +4 Allpässe;
kaltes Room/Nature überspringt DSP, Zero-Übergang leert einmalig den Tank.
Clear-Init/Memset im Audiokontext, kalte Cachepfade und DMA-Clean gehören
zur späteren Spitzenlastmessung.

ARM `-fstack-usage` dokumentiert individuelle Compilerframes in
`PRODUCT_BUILD_AUDIT.json`. Kein Beleg für ganze Callchains, Bibliotheks-
Callees, Interruptstapel oder realen High-water. Der vorhandene DWT-Profiler
misst das Rendererfenster; Cache-Clean/weitere IRQ-Arbeit muss für die
Geräteabnahme zusätzlich berücksichtigt werden.

Interne Audio-Speicher reichen im tatsächlichen Link; keine SD-Karte,
Samplebank oder erforderliche externe PSRAM für diesen Klangpfad.
Das beweist keine SD-Nachrüstbarkeit; PCB-/Pin-/Busfragen bleiben davon getrennt.
Projektziel am Gerät: peak_load <0,60, null Deadline-Misses, gemessene
Stackreserve mit Display/LED/MIDI/Storage. Kein CPU-Versprechen aus Hostzeit.

## Hörnachweis und Freeze-Regel

`tools/review_product_audio.py`: reale Quellen/Kette, RAW-Firmwarepegel plus
separate **eine konstante Gainänderung je Vergleichsdatei**, Ziel −26 LUFS,
max +12 dB, True-Peak-Decke −6 dBFS. Keine Dynamikautomatik.
Jede Hör-WAV exakt 27 s; lange PCM-Läufe bleiben interne Messung.
Commit, Sourcehashes, Parameter, Seed, Frame-acknowledgement-Trace, rohe/matched
LUFS/True-Peak/DC/Mono sind gespeichert. Hookframe ist ein DSP-Acknowledgement,
kein behaupteter samplegenauer Attackbeginn.

Dry: D3/D4/A4; Color: D4 bei 0/0,5/1; SHAPE: D4 mit Attack+Release gemeinsam
0/0,5/1; World: echtes Anfangsstück; Nature: optionale Ebene allein;
Blind: unbekannte Reihenfolge, gleiche D-Major/Equal, Dry/Nature0.
24 achtsekündige Summenprobes messen True Peak/DC/Mono an behaltenen
Parametergrenzen, WAVs werden danach verworfen. Antwortschlüssel getrennt.

Sound-Freeze erst nach trockener Familienauswahl, sinnvoller Nature-Entscheidung,
Blindunterscheidung, längerer angenehmer Wahrnehmung und echter H743-/Ausgangs-
Abnahme. Keine technischen Messwerte schließen Alarm-/Buzz-/Tube-Einwände
oder garantieren Beruhigung. UX folgt diesem Softwarevertrag; Displayruhe und
Wake dürfen weder Audio-DMA noch musikalische Zeit anhalten.
