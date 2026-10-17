# AMBIENT — Product Sound Candidate 0.10

Stand 2026-10-12. Gilt ausschließlich für `FAM_SOUND_PROFILE=product` /
`FAM_SOUND_PRODUCT`, Wireformat SCN7. Dies ist der reproduzierbare
**Softwarevertrag**, keine endgültige Klang- oder Gerätefreigabe.
`reference` bleibt der CMake-Default; historische Beschreibungen gelten dafür.
Der aktuelle Nachweis und die offenen Gates stehen in
[PRODUCT_CORE_CHECKPOINT.md](PRODUCT_CORE_CHECKPOINT.md) und
[COAST_GENERATOR_CHECKPOINT.md](COAST_GENERATOR_CHECKPOINT.md) sowie
[WOODLAND_GENERATOR_CHECKPOINT.md](WOODLAND_GENERATOR_CHECKPOINT.md) und
[HIGHLANDS_GENERATOR_CHECKPOINT.md](HIGHLANDS_GENERATOR_CHECKPOINT.md) und
[PHRASE_DEVELOPMENT_CHECKPOINT.md](PHRASE_DEVELOPMENT_CHECKPOINT.md) und
[SD19_ROOM_CALIBRATION_CHECKPOINT.md](SD19_ROOM_CALIBRATION_CHECKPOINT.md), die vollständige
Arbeitsliste in [AMBIENT_SOUND_DESIGN_TODO.md](AMBIENT_SOUND_DESIGN_TODO.md).

## Produktidee und bewusst kleiner Umfang

Drei Naturorte unterscheiden sich durch Tonkörper und musikalische Zeit:
COAST führt zwei äußere Stimmen zu einem gemeinsamen Ziel. WOODLAND verbindet
lange Saitentöne durch gemeinsame Akkordtöne und ersetzt jeweils eine Stimme;
HIGHLANDS verbindet weite Akkorde durch einen gehaltenen Ton und kehrt nach
einer offenen Pause zurück. Alle drei Rollenalgorithmen sind integriert;
ihre abschließende Hörwahl bleibt offen. Manual und
Generate verwenden dieselbe Familienartikulation, Belegungs- und Harmonieprüfung;
die internen Phrasen erhalten ihre jeweils ausdrücklich geprüften
Register. Gemeinsame Harmonik und ein Raum verbinden
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
| Offline-Renderer / Audits | REFERENCE/Nachweis, keine Firmwarefunktion | Reale C-Kette; kurze Quellenvergleiche und vollständige musikalische Phrasen getrennt ausgewiesen |

`tools/check_product_link.py` prüft die tatsächlichen ARM-Kompilierinputs,
verbotene DSP-Funktionssymbole, einen D2-Raum und den ausgerichteten D1-DMA-Puffer.
Der bloße Archivordner oder Amount=0 gilt ausdrücklich nicht als Entfernung.

## Tonhöhe, Eigentümer und Belegung

- Manual und öffentliche World-Note-API: 140..470 Hz, MIDI 50..69. Nur der
  interne COAST-Generator:105..850 Hz, Core-Noten MIDI45..80. Interner
  WOODLAND-Generator:105..470 Hz, Core-Noten MIDI45..68. Interner
  HIGHLANDS-Generator:105..650 Hz, Core-Noten MIDI45..75. Kein öffentlicher
  Bypass. Major-Core
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
  gehörte manuelle Saite klingt natürlich aus**; ihr ursprünglicher Besitzer und Slot
  bleiben bis zum tatsächlichen Ende belegt. Vorbereitung ohne ersten Sample
  wird beim Key-up storniert. Wiederanschlag eines noch belegten Owners kann
  abgewiesen werden; die spätere UX darf keinen Erfolg vortäuschen. WOODLAND-
  Generate verwendet explizite lange Halteabsichten mit dem eigenen weichen
  Saitenrelease; musikalischer Stop beendet sie ebenfalls damit.
- Nach tatsächlichem Quellenende: maximal 16 konservative Raum-Hz-Einträge,
  Ende spätestens 7,2 s später (1,5× maximal nominalem T60), oder nach
  250 ms gemessener nasser Ruhe unter 1e-5 ohne aktive Quellen. Drei Plätze
  werden für ausgehende Quellen reserviert, relevante Historie nicht verdrängt.
  Kein Anspruch auf universelle psychoakustische Unhörbarkeit dieser Schwelle.

## Drei deterministische, begrenzte Grammatiken

Nur bestätigte Töne werden gespeichert. Ein Vorschlag verändert den lebenden
Score/RNG nicht. Initialseed bei 0: 0xA6B13E7D; die drei separaten
Phrasenzustände beginnen mit demselben expliziten Seed. Nature verwendet
einen eigenen RNG/Seedkanal. Der alte reine `world_grammar_t`-Testhelfer
bleibt reproduzierbar, wird im Product-Generator nicht mehr dispatcht.
Dies reproduziert bei derselben zeitlichen Eingabefolge Entscheidungen und
PCM; es verspricht keine identische Aufnahme nach beliebigem Bedienverlauf.

| Welt | Timing vor Activity | Erinnerung / Variation | Grenze |
|---|---|---|---|
| COAST | Außenstimmen Hold 2,8..3,1 s; früheste geplante Schritte 5,8..6,2 s; zusätzlich nach wirklichem Außenquellenende 6*(1-Activity) s; Ziel Hold 5..5,5 s, danach zusätzliche Schlussruhe 1+9*(1-Activity) s ab Ende aller Quellen; Entwicklungsfaktor 1/1,08/1,12/0,96 | Gegenläufige Core-Wege um eine mittlere Tonika, Seed variiert obere Route, Dauern und Akzente; leiser gemeinsamer Akkordton bleibt während Partnerwartezeiten bis nach dem Ziel | drei feste Rollen, acht bestätigte Starts pro Phrase; Ziel einmal in Owner6, keine doppelte Unisonoquelle; weiterhin drei Slots inklusive Release |
| WOODLAND | sechs Starts; Hold je Rolle6..18,5 s; Basisabstände2/12/9/4/7 s vor Activity; Schlussruhe5..9 s nach letztem Hold | weiche Tonika/Terz → verwandter Akkord mit erhaltener Terz → höhere Antwort → gemeinsame D4-/Terz-Verbindung → Quinte oder verwandter Schluss; Seed variiert Schluss, Akzente und Dauern | zwei echte lang abklingende Saiten; Owner6/7 warten reale Releases ab, kein Wiederanschlagen des gehaltenen gemeinsamen Tons |
| HIGHLANDS | acht Starts; Partner-Holds6..7,75 s; Basisversatz0,8/0,8/8,4/1 s; Reprise1/1 s; offene Pause12..14 s vor Activity, mindestens8 s nach tatsächlichem Quellenende | weiter Akkord → verwandte Lage mit gleicher gehaltener Terz → offene Pause → weiter Akkord kehrt mit eigenen Dauern zurück; Seed variiert Akzente/Holds und Dur-Zwischenbass | drei feste Besitzer6/7/15 einschließlich Releases; gemeinsamer Ton ohne Wiederanschlag; Pause erst nach tatsächlichem Ende aller Besitzer |

Alle Rollenautomaten haben explizite Timing-Gültigkeit, auch bei einer
Deadline genau0 am Timerwrap. Activity-Faktor `1.35 - 0.70*a` skaliert
kommende Abstände, keine Holds, Gain oder Farbe. Belegung/Tail-Zulassung
kann Einsätze verlängern; Wiederholung wartet250 ms und verändert weder
RNG noch gehörte Historie. Kein Ersatzton durch nahe Alternativsuche.

Spec0.9 ergänzt ausschließlich bei COAST Wartezeiten nach tatsächlichem
Retirement. Partner warten bei Activity0/0,5/1 zusätzlich6/3/0 s,
die nächste Phrase nach Ende aller Quellen10/5,5/1 s, jeweils mit dem
Entwicklungsfaktor. Die nächste Wartezeit wird einmal mit der aktuellen
Activity geplant; eine bereits laufende Wartezeit und klingende Holds bleiben
bei Bedienänderungen erhalten. Pitch-/Room-History kann den folgenden Start
weiter verzögern. Die Schlusswartezeit beginnt nach dem bisherigen frühesten
Phrasentermin und Quellenende; sie ersetzt keine Releasezeit.
Default Activity0,5 und SCN7-Werte/Schema bleiben erhalten, derselbe gespeicherte
COAST-Activitywert bedeutet ab Spec0.9 diese neue zeitliche Kennlinie.
Die historischen Spec0.8-Hörpakete bleiben auf PR155 eingefroren; die aktuelle
Kennlinie wird separat gegen dessen unveränderten Core geprüft:
[SD33_COAST_ACTIVITY_CHECKPOINT.md](SD33_COAST_ACTIVITY_CHECKPOINT.md).

COAST besitzt einen eigenen begrenzten Phrasenzustand. Erste Außenstimmen
werden gemeinsam vorbereitet, Owner15 beginnt 700 ms nach deren bestätigtem
Start. Außenrollen6/7 werden erst nach ihrem wirklichen Quellenende wieder
belegt; Pitch-/Raumhistory bleibt unverändert wirksam. Der gemeinsame Zielton
wird nur einmal nach beiden Außenreleases gestartet. Erst sein DSP-Ack plant
auch das Ende des bereits gehaltenen Akkordtons. Velocity: unten0,55..0,60,
oben0,44..0,49, Verbindung0,32..0,37. Vorschläge ändern weder RNG noch
gehörte Phase; erst vollständig bestätigte Phasen schalten weiter. Vor dem
nächsten Durchlauf müssen alle drei Besitzer frei sein. Key/Collection/Tuning,
Seed/World, Stop, Autoplay-Gate, Präsenz, Mute und Clear verwerfen offene
COAST-Absichten; bestehende Hz werden nicht retuned. Neue Pitch-Kontexte lassen
gehörte Generate-Quellen natürlich ausklingen, manuelle gehaltene Hz bleiben.
COAST-Makrogrenzen und Quellen-DSP sind dabei unverändert. Deshalb erreicht die
echte D-Dur/Seed1234-Probe ihre gemeinsame Mitte erst bei43,8 s, später als der
direkte Host-Entwurf mit seinem kurzen SHAPE-Release.

WOODLAND besitzt einen eigenen Sechs-Schritt-Zustand. Es plant eine Rolle
pro Angebot; nur tatsächlicher DSP-Start setzt die Haltefrist und schaltet
Phase/RNG weiter. Ablehnung wartet250 ms ohne Tonersatz oder Historienverbrauch.
Gemeinsame klingende Akkordtöne bleiben im vorhandenen Owner stehen. Vor
Wiederkehr müssen beide Besitzer frei sein; der nächste Schluss kann zwischen
Quinte und verwandter Sext/Septim wechseln. Velocity unten0,58..0,62,
oben0,48..0,52. Activity skaliert kommende Zwischenräume, weder Tonlängen noch
Gain/Color. Der wirkliche Saitenausklang kann geplante Abstände verlängern.
Die sechs Schritte sind keine unbegrenzte gleichförmige Notenwarteschlange;
die langfristige Entwicklung folgt dem gemeinsamen begrenzten Phrasengedächtnis
unten. Quellen-/World-Hörwahl bleibt offen.

HIGHLANDS besitzt einen Acht-Schritt-Zustand. Seed1234/D-Dur öffnet mit
D3/A4/F#4, wechselt nur die Partner zu B3/D4 und behält F#4 im selben Owner15.
Erst der DSP-Start des zweiten neuen Partners plant das Ende des gemeinsamen
Tons mit dessen Hold. Alle Quellen enden über den normalen Horn-Release;
Horn-DSP und Attack-/Release-Makrogrenzen bleiben unverändert. Nach tatsächlicher
Retirement aller drei beginnt eine8..18,9-s-Quellenpause (12..14 s vor Activity).
Der Raum läuft natürlich weiter und behält seine Pitchhistory. Danach D3/A4/F#4
mit neuen Akzenten und ungleichen Holds. Minor benutzt die Terz3 und den
Zwischenbass auf der Quinte unterhalb der Tonika; Dur kann abhängig vom Seed
zwischen verwandter Sext und unterer Quinte wechseln. Velocities pro Start
0,55..0,59 /0,41..0,45 /0,45..0,49 /0,48..0,52 /0,50..0,54 /0,52..0,56 /
0,38..0,42 /0,44..0,48. Phase/RNG/Timer erst nach DSP-Ack; offene Starts und
generierte Holds werden bei Pitch-Kontextwechsel ebenfalls verworfen, manuelle
gehaltene Hz bleiben. Nach der Reprise erneut vollständige Retirement und Pause,
kein periodischer DSP-Reset. Default-Reprise43,24 s, letzter Einsatz45,26 s,
letztes tatsächliches Quellenende59,65 s in der65-s-Engineprobe. Die automatische
Rückkehr zählt als gehört; die langfristige Entwicklung folgt ebenfalls der
unten beschriebenen Erinnerung. Kein finaler musikalischer Loop-Lock.

### Entwicklung über mehrere vollständige Phrasen

Die ersten vollständigen Rollenphrasen sind gegenüber Candidate0.6 bytegleich.
Danach folgt **Ausgangsphrase → Variation → verwandte Akkordlage → Erinnerung**.
Nach der ersten Erinnerung liegen seedabhängig zwei bis vier vollständige
Variations-/Verwandtschaftsphrasen zwischen den Rückkehren. Der lebende RNG
läuft weiter, keine Source-/Room-/Pitchhistorie wird dafür zurückgesetzt.

Pro World existiert `phrase_development_t`:24B fester Main-State, vier32-Bit-
Werte plus fünf8-Bit-Werte/Alignment; keine Notenwarteschlange. Erst die letzte
bestätigte Rolle lernt die erste vollständige Phrase über Opening-Seed und
Route. Abgebrochene Teilphrasen lernen nichts; ihre neue Öffnung erhält den
aktuellen Seed. Nur ein kompletter gehörter Durchlauf zählt als Erinnerung.
Eine erinnerte Phrase rekonstruiert ihre damaligen Tonhöhen, Rollen, Akzente und
Halte-/Abstandsabsichten mit höchstens sieben LCG-Schritten. Tatsächliche
Abstände berücksichtigen die aktuelle Activity, auch bei Erinnerung. Reale
Einsätze warten weiterhin reale Besitzer/Tails ab; sie werden nicht als feste
Audiozeiten wiedergegeben. Pitch-Kontext/World/Seed-Neustart erzeugt einen neuen
Phrasenzustand; Mute/Präsenz/Stop lernen keine Teilphrasen. Erneutes Generate-On startet
mit einer neuen Ausgangsphrase; es stellt keinen Audiostate wieder her.

| Funktion | Tonführung | Zeitabsicht relativ zur Ausgangsphrase |
|---|---|---|
| Variation | COAST nimmt die andere obere Route; WOODLAND den anderen Schluss; HIGHLANDS die Quinte eine Oktave tiefer und anschließend deren exakte gestimmte Lage statt eines engen B3/A3-Nachklangpaars | Hold und geplante Abstände×1,08 |
| Verwandte Lage | Relative Minor-Pentatonik bei Major, relative Major-Pentatonik bei Minor; exakt dieselben erlaubten Pitch Classes, nur neue Tonika/Lage. D-Dur ergibt eine B-Moll-Lage. Kein globaler Key-/Tuningwechsel und kein Live-Retune | Hold und geplante Abstände×1,12 |
| Erinnerung | Tonfolge, Rollen und Akzente der vollständig gehörten ersten Phrase; gemeinsame reale Eigentümer bleiben nach den bestehenden World-Regeln stehen | erinnerte Hold-/Abstandsabsicht×0,96; keine identische Zeit-/Audio-Schleife |

Activity bleibt ausschließlich ein Abstand-/Pausenfaktor. Die Entwicklungs-
funktion skaliert die nominalen Holds separat; sie verändert keine SHAPE-/Color-
Makrogrenze. HIGHLANDS-Quellenpausen behalten ihren gewöhnlichen Activity-/Seed-
Wert und beginnen erst nach vollständiger Retirement. Alle privaten Register,
drei globalen/zwei lokalen Slots und Harmonie-/Release-Grenzen bleiben wirksam.
Die30-Minuten-Engine-Traces und neun10-Minuten-Langläufe prüfen tatsächliche
Fortschritte, vollständige Rückkehr, endliche Ausgabe und Ende nach Stop;
Wiedererkennbarkeit, musikalische Ruhe und Langzeitwirkung bleiben SD17-Hörgates.

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
| Color | Quelle, normiert 0 / 0,5 / 1 | Bowed-Cutofffaktor0,8..1,2; Horn0,85..1,15; lange WOODLAND-Saite: fester bisheriger Default-Loop-Damp≈0,015; Ausgangsfarbe x+(2Color−1)·(x−LP), LP-Cutoff2f (≤0,45SR), 0 dunkel/0,5 exakt neutral/1 hell; 80-ms-Zeitkonstante auch live | via Brightnessadapter in SCN7; WOODLAND-Endwertsemantik ab Spec0.10 geändert, Default0,5 exakt erhalten; World verändert Color nicht |
| Activity | Score, 0 / 0,5 / 1 | Faktor 1,35..0,65 für kommende Angebote; COAST zusätzlich Partnerwait6/3/0 s und Schlusswait10/5,5/1 s nach tatsächlichem Quellenende, mit Entwicklungsfaktor; laufende Waits/Holds bleiben stehen; weder Gain/Color/Key noch zusätzliche Quellen | SCN7 unverändert; COAST-Zeitsemantik ab Spec0.9 geändert; 0 ist langsame aktive Welt, kein Stop |
| Attack | neue Quelle, 0 / 0,5 / 1 | SHAPE x=0,35+0,30*a; Faktor 0,125×64^x = 0,5359 / 1 / 1,8661; note-on only | SCN7; keine rückwirkende Hüllkurvenänderung |
| Release | neue Quelle, 0 / 0,5 / 1 | x=0,35+0,30*r; Faktor 0,25×16^x = 0,6598 / 1 / 1,5157; note-on only | SCN7; 0 bleibt musikalischer Release |
| Room | gemeinsamer Raum, 0 / 0,5 / 1 | nominal T60=1,2+3,6*r² s; kalibrierter Rücklauf Wet=5*r, zusätzlich Enable; Amount linear 80 ms über vollen Bereich, Feedback 120-ms-Zeitkonstante | SCN7; 0 nach Rampe exakt Dry, kalter Pfad und Tank leer; World leert Tank nicht |
| Dry/Room | Raum, ID 0 / Room=1 / 1 | Enable linear 40 ms, danach echter kalter Dry-Pfad | SCN7; nur zwei Fälle, kein Archiv-FX-Rückweg |
| Nature | separater Ort, 0 / 0 / 1 | Amount 2-s-Zeitkonstante; World/Seed-Fade down 80 ms / up 2 s; unabhängig von Room und Score-RNG | SCN7; 0 kalt, kein Wetter-/Filterwork; positiv explizit auch ohne Phantomton hörbar |
| Generate | Laufabsicht, false / false / true | Eintritt alte Manualquellen weich ≤100 ms beenden; Ton erst nach Budget-/Harmoniezulassung; Stop natürliche Releases + Nature-Target 0 | nicht persistiert; keine Wiederherstellung alter Besitzer |
| Autoplay event gate | Control-Kompatibilität, 0 / 1 / 1 | sperrt neue World-Ereignisse, lässt natürliche Tails | nicht persistiert; kein weiterer UI-Modus versprochen |
| Seed / New Field | Score, uint32 / 0xA6B13E7D | 0 auf Default; neuer begrenzter Score, erzeugte Quellen weich beenden; eigener Nature-Seed | SCN7; allein startet Generate nicht |
| User presence | Control, false / false / true | explizite Präsenz plus 8-s-Nachpause unterdrückt Generate; Display-idle zählt nicht | nicht persistiert; keine physische Tastenentscheidung |
| Volume | Master, normiert 0 / Host 0,6; Gerät 0,3 / 1 | 120-ms-Zeitkonstante, nach Room/Nature; unabhängig von Klangfarbe | **nicht** SCN7; Recall erhält Wert; 0 reduziert Pegel, ist kein State-Clear |
| Mute | Ausgabe, false / false / true | ganze Kette linear ≤40 ms auf exakt 0; hinter Null DSP/Room leeren; Targets und Generate-Absicht erhalten | nicht SCN7; weder Scene/Nature noch neue Notes umgehen Mute; Unmute nur frische Ereignisse |
| Clear | Sitzung, Befehl | ganze Kette ≤40 ms auf 0; Sources/Nature/Room/DC/gehörte Tailhistory leeren, Generate off | keine Parameter-/Volume-Rücksetzung; DMA läuft weiter |

Quellenbasis: Bowed Attack0,30 s ×Faktor, Release-Zeitkonstante0,90 s ×Faktor;
Horn0,45 s /0,70 s ×Faktor. WOODLAND: Attack0,8 s ×Faktor, Sicherheitsgrenze
0,4..2,4 s, mit Produktmakro tatsächlich ca.0,429..1,493 s. Nominaler
Loop-T60 36 s ×Releasefaktor; Filter/Interpolation verkürzen den echten Decay.
Eigener musikalischer Stop2 s ×Releasefaktor, tatsächlich ca.1,32..3,03 s,
pro Note beim Beginn gespeichert. Kontextübergabe kann einen bestehenden
Ausklang auf100 ms verkürzen, nie wieder verlängern. Die Release-Zeitkonstante
ist **nicht** die volle hörbare Taildauer.
Pluck-Naturalende wird pro Sample bei getracktem Envelope <2,5e-4 bestimmt.
`engine_product.c` wählt nach Init und Clear fest dieselbe Ambient-Saiten-
Artikulation für Manual/Generate. Die kurze Quellenartikulation bleibt nur
Referenz und direkter historischer Prüfweg nach `pluck_init()`; kein Produktregler.

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

SD34-Pegelprüfung2026-10-13:Quellfaktoren0,50/0,22/0,50 undSend0,35 bleiben.
105 Solo-,162 Maximalsummen- und24 Generatefälle bei64/512 plus vollständige
Default-/Referenzkontrollen:596 WAV-Render/209,2min. Alle20 Manualtonhöhen
MIDI50..69 bei Velocity0,5:gleiche Tonhöhe zwischen Quellen≤2,329dB Körperunterschied;
Velocity0,50→0,55 ergibt0,828–0,831dB. Keine Kompensation des WOODLAND-Decays.
26 erlaubte geordnete Dreifamilienmischungen/Zwei-Saitenfälle haltenBudget3/2;
weitere Note wird abgewiesen. Größter TruePeak−5,1dBFS bei Master1;
dieselbe Summe bei definiertem Referenzmaster0,6:−9,5dBFS,≥6dB-SD42-Ziel erfüllt.
NaN/Limiter0,DC/Mono/stille Enden; Legacy/Manual/Worldverträge erhalten4s-PCM.
251 Coreinputs exakt PR160,Spec0.10/SCN7/Defaults unverändert.
Ein40-s-WAV mit echten unterschiedlichen Worldanfängen und gemeinsamem Gain+6,9dB.
Finale gehörte Lautheitsbalance/SD34 bleibt offen.
Details:[SD34_GAIN_CHECKPOINT.md](SD34_GAIN_CHECKPOINT.md).

SD35-Kombinationsprüfung2026-10-13:77 gezielte aktuelle Szenarien bei64/512
plus drei Defaultgegenproben,157 ganze Render/429,067min PCM. Acht Profile
für Activity/Attack/Release/Color/Room/Nature/Volume, zwei musikalische
Kontexte, volle manuelle Belegungen und schnelle Ziele.1049 tatsächliche
Starts mit frischen physischen Reservierungen, Budget3/2 und natürliche
Enden ohne Clear; manuelle lange Saiten bleiben35,68–45,04s nach Key-up.
PCM und ganze Events/Status/Controls blockgenau. Gepaarte volle Akkordwechsel
verursachen maximal3/32768 zusätzliche Erstsampleänderung;18 kombinierte
Activity/SHAPE-Edits erhalten vorbereitete/held/releasing Quellen6s PCM exakt.
NaN/Limiter0, Mono/DC bestehen. In dieser Auswahl−6,8dBTP bei Master1;
SD34 enthält weiterhin das breitere Maximum−5,1dBTP/Referenz−9,5dBTP.
Alle251 Coreinputs exakt grüner PR161, kein neuer Klangvertrag. SD35 bleibt
für musikalische Endwertwahl/Bandpeak-/Buzz-/Pumpen-Hörurteil offen.
Details:[SD35_PARAMETER_CHECKPOINT.md](SD35_PARAMETER_CHECKPOINT.md).

Room: 8 feste Delaylinien 1117/1277/1429/1601/1789/1999/2203/2423 Samples,
vier Allpässe 239/389/277/419, Householder-Feedback, 0,25 Dämpfung.
Input-HP ca. 100 Hz (kein Feedback-HP), feste Stereo-Seitengewichtung 0,30,
orthogonale balancierte 8-Tap-Readouts ×0,1775352. Keine live veränderbaren
Delaylängen und damit keine durch Room-Zeit verursachte Pitchfahrt.
Float-Tank 55.352 B + Diffusion 5.296 B = 60.648 B zuzüglich kleiner States.
D2-Gesamtbelegung ist davon getrennt im ARM-Linknachweis ausgewiesen.

SD19-Ausgangsstand PR152/Spec0.7: Wet=(0,12+0,50*r)*r ergab bei Room0,5
nur0,66–1,04% RMS-Differenz zum identischen trockenen Ensemble-Ausschnitt.
Spec0.8 kalibriert ausschließlich den Ausgangsrücklauf auf5*r. Bei gleichem
Room0,5 entstehen8,86–13,99%, bei Maximum22,72–38,87%; dies sind gemessene
PCM-RMS-Verhältnisse, keine Wet-Energieprozente oder finalen Hörurteile.
Send, feste Delays, Diffusion, Dämpfung, Breite und Feedback bleiben gleich.
Die normierte gespeicherte Roomposition und der nominale Decay bleiben SCN7;
der Rücklauf an derselben Position ist im Kandidaten jetzt stärker. Das ist
eine ausdrücklich versionierte Kalibrieränderung, keine finale Raumfreigabe.
Die frühere0.7-Audiofassung ist über PR152 reproduzierbar. Neue echte, fest
lautheitsangepasste Held/Release-Vergleiche und Host/H743-Nachweis:
[SD19_ROOM_CALIBRATION_CHECKPOINT.md](SD19_ROOM_CALIBRATION_CHECKPOINT.md).

SD28-Hörvorbereitung 2026-10-11: drei echte Mit/Ohne-Vergleiche bei Nature0/0,35
und Room0,5, jeweils gehaltenes Ensemble plus späterer Übergang/Quellenpause.
Dieser optionale Kandidat enthält Wind in allen drei Worlds, COAST zusätzlich
Wellen und WOODLAND Tropfen; Default Nature bleibt0. Kein Familien-/Ortsnutzen
oder Ruhegewinn ist dadurch freigegeben. Dieselben Noten, Besitzer und Phrasen
bei zwei Seeds und Nature0/0,35/1, echtes Stop-Ende und Room-Unabhängigkeit sind
geprüft. Gleiches Gain für alle vier Abschnitte; kein Anheben der Natur/Pause.
Klang, Schema und Firmwareinputs bleiben exakt PR153/Spec0.8. Einzelne Worlds
dürfen nach der Hörwahl vollständig ohne Natur bleiben. Details:
[SD28_NATURE_CHECKPOINT.md](SD28_NATURE_CHECKPOINT.md).

SD33-Color-Hörvorbereitung 2026-10-11: drei38,5-s-Ensemblevergleiche bei
0/0,5/1, mit gleichen vollständigen musikalischen Ereignissen/Statusdaten
über beide Seeds und Livewechsel. DefaultPCM entspricht dem normalen Renderer
exakt; Firmwareinputs bleiben PR156/Spec0.9. WOODLANDs vorhandener Dampbereich
ändert diese gehaltene Passage nur um0,203/0,204% RMS zum Default; sein Color-
Bereich benötigt eine eigene Kalibrierung bei erhaltenem langen Ton.
RMS/FFT belegen keinen musikalischen Nutzen; kein Color-/SD33-Hörhaken.
Details: [SD33_COLOR_CHECKPOINT.md](SD33_COLOR_CHECKPOINT.md).

SD33-WOODLAND-Color-Kalibrierung2026-10-12, Spec0.10: nur die lange Product-Saite
erhält eine Ausgangsfarbe. Loop, Tonhöhe, Hold-/Releaseabsicht und tatsächliches
Quellenende bleiben erhalten; Default0,5 ist exakt der bisherige Klang.
Endwerte unterscheiden jetzt tatsächlich die Obertöne: FFT-Center175,67/181,27/
188,86Hz statt181,08/181,27/181,46Hz.63 volle Render/117,6min,24 Float-Solos,
gleiche vollständige musikalische Historie/Statusdaten, Neutral/andere Worlds
bytegleich, Hostrunner und echte Baseline/Kandidat-H743-Linkaudits bestehen.
SCN7/Brightnessadapter bleiben; gespeicherte WOODLAND-Endwerte bekommen die
neue Klangfarbe. Filterbedingte Pegelunterschiede bleiben bewusst gemessen,
keine AGC oder individuelle Hörclip-Normalisierung. Hören/SD33 bleiben offen.
Details: [SD33_WOODLAND_COLOR_CHECKPOINT.md](SD33_WOODLAND_COLOR_CHECKPOINT.md).

SD33-Attack-Hörvorbereitung2026-10-12: drei38,5-s-Ensemblevergleiche enthalten
den tatsächlichen Anfang0–12,5s bei0/0,5/1 und Release0,5. Capture bei neuer
Tonvorbereitung ist mit PCM belegt: bestehende vorbereitete/held Stimmen werden
durch Drehen nicht rückwirkend verändert. Nominale Sourceattacks:
COAST0,161/0,300/0,560s, WOODLAND0,429/0,800/1,493s, HIGHLANDS0,241/0,450/0,840s.
48 volle Render/89,6min,64/512, gleiche ganze musikalische Ereignisse/Statusdaten,
gültige Live-/Future-/Clamp-/Invalidinputverträge und natürliche Enden bestehen.
Beginn bleibt ungefadet, ein Gain je World. Quellen/Core/Spec0.10 exakt PR158;
kein neuer Klang-/Geräte-/SD33-Hörhaken. Release ist separat geprüft (unten).
Details: [SD33_ATTACK_CHECKPOINT.md](SD33_ATTACK_CHECKPOINT.md).

SD33-Release-Hörvorbereitung2026-10-12: drei38,5-s-Vergleiche enthalten die
gleichen tatsächlichen Anfangstöne und NoteOffs bei0/0,5/1 und Attack0,5.
Ausgabe2–14,5s des32-s-Renders,Generate-Stop4s;ein Gain je World,
40-ms-Ausschnittkanten und0,5-s-Vergleichslücken. Ganze Ausklänge bis zur Stille
sind geprüft; keine künstliche Stop-Hüllkurve. Quellenfreiheit nach Stop:
COAST5,82/8,82/13,37s,WOODLAND1,32/2,00/3,04s,HIGHLANDS4,76/7,22/10,91s.
66 ganze WAV-Render/99,2min bestätigen64/512,Default-PCM und tatsächliche
physische Reservierung aller250 Starts nach Freigabe des alten Quellenplatzes.
18 Änderungen nach Vorbereitung/während Hold/Release lassen ganze6s-PCM exakt;
erst neue Töne übernehmen den Faktor0,65975392/1/1,51571643. Größere Releasewerte
dürfen Partnerübergaben und Keeper-Holds verlängern,ohne gehaltene Töne neu
anzuschlagen. Lange WOODLAND-Schwingung und natürliche Manual-Tails bleiben.
251 Firmwareinputs exakt PR159,Spec0.10 unverändert; SD33/Hören bleiben offen.
Details: [SD33_RELEASE_CHECKPOINT.md](SD33_RELEASE_CHECKPOINT.md).

## Kontextwechsel und Persistenz

World-/Seedwechsel beendet nur erzeugte Quellen ≤100 ms einschließlich
der langen WOODLAND-Saite. Die ältere20-ms-Angabe gilt für den kurzen
Referenz-Pluck und wird hier als Dokumentationskorrektur ersetzt. Der
vorhandene Product-Kontextstop bleibt100 ms; kürzere laufende Stops
werden nicht verlängert.
Der Wechsel storniert vorbereitete alte Angebote, erhält manuelle klingende Frequenzen
und den gemeinsamen Raum. Nur das jüngste Ziel wird geplant. Auf Entry in
Generate gilt dieselbe begrenzte Übergabe für alte Manualquellen.

SD37-Nachweis2026-10-13:54 aktuelle Fälle in allen sechs Richtungen bei64/512,
108 ganze Render/220,8min PCM.670 DSP-Starts an678 frischen physischen
Reservierungen;acht unbegonnene alte Starts bleiben ohne Audio/Hook/History.
Volle Belegung/Pending/Release/Batch/Rapid/Nature/Timerwrap und27 gepaarte4-s-
PCM-Fälle bestätigen Quellenbesitz, weiterklingenden gemeinsamen Raum und nur
die jüngste Ziel-Familie. Manuelle Hz/Artikulation bleiben bei Worldwahl erhalten.
Erzeugte100-ms-Enden werden bei schnellen weiteren Wahlen nicht verlängert.
Ganze Audio-/Event-/physische Besitzer-/Zielverläufe blockgleich; Quiet-History-
Diagnose bei78 leeren Beobachtungen um einen10-ms-Tick verschieden, keine
Audio-/Ereignisabweichung. SD12-Hörkalibrierung bleibt separat offen.
Firmware251 Inputs exakt PR162;dieser Abschnitt korrigiert nur die veraltete
20-ms-Beschreibung. Finale musikalische Worldübergänge und Hardware bleiben offen.
Details:[SD37_WORLD_TRANSITION_CHECKPOINT.md](SD37_WORLD_TRANSITION_CHECKPOINT.md).

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

Aktueller SD41-Nachweis2026-10-13 mit unverändertem langen Klangkern:
sechs zuvor laute Bootzustände,380 gerichtete Native-Fallbacks,zehn Legacy-
und20 native Off-Recalls einschließlich aller fünf Slotindizes und Volume/Mute.
Zwölf Manual-/drei Savefehler-/neun Seed-Triplets erhalten ganze8s PCM und
Hooks bei512/64; neun Timerwrap-Paare ebenfalls ganz exakt. Native-Recall
stellt keinen gehaltenen/erzeugten Besitzer wieder her. Ausdrücklich positives
Nature im Scene-Recall bleibt eine hörbare optionale Schicht ohne Phantomton;
Boot und gemutete/Volume0-Ausgabe bleiben still. Laufender Generate-Seed-Recall
erhält seine bereits aktive Absicht, Room und alte Hz-History, beendet alte
erzeugte Quellen100ms und hebt Autoplay-Gate0 nicht auf. Neue Seeds reproduzieren
bei frischem Init den geprüften8-s-Anfang; laufender Room macht einen Live-
Neustart nicht zu demselben frischen PCM. RAM-Backend, keine Flash-/HAL-/IRQ-
Simulation, keine neue Gerätefreigabe.
[SD41_BOOT_RECALL_CHECKPOINT.md](SD41_BOOT_RECALL_CHECKPOINT.md).

Flash: reservierter Bank-2-Sektor ab 0x081E0000, maximal 512-B-Staging.
Magic-Flashword zuletzt; M7-Cache vor Read gezielt invalidieren.
**Default ohne Journal:** Stromverlust während Erase kann den vorherigen Save verlieren.
Eine gültige vorherige Flashversion wird nicht atomar erhalten. Zudem blockiert
Erase/Program den Main-Loop, also auch Generate-Planung; das ist kein
zertifizierter Live-Save. Bank-1-Imageguard schützt gegen Firmware-Spill in
die Schreibbank. Save-Latenz, ECC/Power-loss und Storage-UX bleiben Gates.

Seit 2026-10-07 gibt es einen expliziten, standardmäßig deaktivierten
`FAM_SCENE_JOURNAL_CANDIDATE`: zwei reservierte Bank-2-Sektoren, unverändertes
Scene-Blob in CRC-geprüften Transaktionen, Commit zuletzt und Legacy-Read bis
zum ersten gültigen Journal. Abbruch-/Korruptionstests belegen den
Softwarealgorithmus im NOR-Modell. Der H743-Adapter hat noch keine sichere
ECC-Fault-Behandlung und Save bleibt synchron; daher keine neue Gerätegarantie.
[SCENE_JOURNAL_CHECKPOINT.md](SCENE_JOURNAL_CHECKPOINT.md) beschreibt Vertrag,
Tests und ausstehende Aktivierung.

Seit2026-10-14 ist ein separater Main-Journal-Schreiber vorbereitet, noch ohne
Scene-/H743-Anschluss und ohne Änderung des synchronen Live-Save-Defaults.
Begin übernimmt höchstens512 B und führt kein I/O/Recovery aus. Ein Step startet
oder pollt höchstens einen I/O oder liest32 B; keine Warte-/Scanloops. Payload
zuerst, unveränderter JSC1-Commit zuletzt; Cache erst nach bestätigter Header-
und vollständiger Payload-Rückleseprüfung. Ein Abbruch hält laufende Operation
und geliehenen Puffer bis zum terminalen Poll fest. Nach möglicher Mutation
erzwingt ein Fehler neue Recovery; Begin versteckt keinen synchronen Vollscan.
5252 Modellfehlerfälle,79 Abbruchzustände,511 Saves und drei echte Product-
PCM-Triplets bei512/64 bestehen. Die72s getrennte Ausgabe und sämtliche Hooks
sind ganz bytegleich; Busy-Latenzen sind modelliert, keine Gerätezeitmessung.
Alle251 damaligen Firmwareinputs bleiben PR164. Die unten beschriebene separate
Recovery ergänzt diese Vorbereitung. Bestätigtes Scene-Save sowie sicherer
Flash-/ECC-Anschluss und Geräteabnahme sind offen;
Journal bleibt OFF. [SD41_ASYNC_JOURNAL_CHECKPOINT.md](SD41_ASYNC_JOURNAL_CHECKPOINT.md).

Ebenfalls seit2026-10-14 ist eine separate schrittweise Recovery vorbereitet,
ohne Scene-/H743-/CMake-Anschluss. Begin macht kein I/O und sperrt die
Cacheverfügbarkeit. Je Step höchstens ein Read von32 B und CRC über32 B;
keine Record-/Sektorschleife. Journal/Medium gehören während BUSY exklusiv
diesem Scanner. Bestätigte Metadaten und Basisstaging bleiben während des Scans
erhalten; erst der vollständige erfolgreiche Scan veröffentlicht Gewinner oder
gültiges leeres Journal mit Pflicht zum frischen Erase im anderen Sektor.
CORRUPT überspringt den Record, ERROR/Cancel lassen den Cache unverfügbar;
kein Teilfund oder versteckter Scan beim Writer-Begin. Ein sicherer rascher
Readcallback bleibt Adapterpflicht, keine gemessene Flash-/IRQ-/ECC-Garantie.
7288 Gegenproben zum alten Scanner,4983 Cut-/Retryfälle und35 Cancelzustände
bestehen. Neun12-s-Product-PCM-Triplets für Recovery, partiellen Writefehler und
Readfehler mit erneutem Scan/Save sind bei64/512 ganz exakt;21 Render/252s mit
drei geteilten Baselines, keine vollständigen musikalischen Langphrasen.
Alle253 bisherigen Inputs bleiben PR165, einschließlich des unveränderten
vorbereiteten Writers;251 Laufzeitinputs unverändert. Bestehende synchrone
Scene-Read-/Savewege bleiben. Nächster Softwareteil ist die ausstehende und
bestätigte Scene-Save-Transaktion; Journal OFF und Geräteabnahme bleiben.
[SD41_JOURNAL_RECOVERY_CHECKPOINT.md](SD41_JOURNAL_RECOVERY_CHECKPOINT.md).


Seit2026-10-14 ist die Scene-Save-API separat mit Product und
`FAM_SCENE_SAVE_ASYNC_PREPARED` vorbereitet: echter368-B-SCN7-Snapshot beim
Begin ohne I/O; späteres Drehen verändert ihn nicht. „Speichern läuft“ bedeutet
angenommen, „gespeichert“ erst nach kompletter Writer-Rückleseprüfung mit
Veröffentlichung bestätigter Slots/aktiver Scene. Während BUSY kein Sync-Save/
Recall/Init; fehlgeschlagene oder abgebrochene Aufträge bestätigen keinen neuen
RAM-Stand. Ein vollständiger physischer Commit kann trotzdem recoverbar sein.
Cancel drainiert geliehenen I/O; unsicherer Cache verlangt vor Retry schrittweise
Recovery. Neuer Versuch übernimmt neue aktuelle Werte. Je Main-Step höchstens
ein Start/Poll/Read≤32B; feste Snapshot-/Start-/Publikationsarbeit≤512B ist keine
gesamte32-B-CPU- oder gemessene Geräte-WCET-Grenze.
337 native Fälle/283.584 Checks,alle51 Append-/53 Fresh-Writer-I/O-Fehler,
20 Recovery-Readfehler und206 Cancelpositionen bestehen.15 komplette12-s-Product-
PCM-Triplets in allen Worlds bei64/512 exakt;33 Render/396s mit geteilten Baselines.
Bestehende Scene-/Legacy-/Boot-/Recalltests bestehen auch mit vorbereitetem Flag.
254/255 alte Inputs exakt; nur scenes.c erhält ausgeschaltete Hooks, unaktivierte
Reference-/Product-Übersetzungseinheiten präprozessiert vollständig gleich.
Kein UI-/H743-/CMake-/Defaultanschluss; bestehendes Live-Save synchron und Journal
OFF. Der folgende Stand ergänzt den isolierten H743-Adapter; physische Prüfung
bleibt offen. [SD41_SCENE_SAVE_CHECKPOINT.md](SD41_SCENE_SAVE_CHECKPOINT.md).

Seit2026-10-15 ist der H743-Start/Poll/Read-Adapter separat vorbereitet. Er
benutzt echte ST-HAL-Interruptoperationen für Bank2/Sektoren6–7; das geliehene
Programword liegt als feste32-B-Kopie bis zum terminalen Poll. EOP- und
Fehlercallback im selben IRQ ergeben ERROR. Terminal bedeutet Hardware plus
HAL quieszent und Bank2 wieder gesperrt, niemals bloß ein Callbackflag. Ein
ungültiger Kontext oder weiterhin laufender I/O hält Poll auf BUSY.
Read≤32B invalidiert nur die betroffenen Flashcachelines, kopiert höchstens
neun ausgerichtete32-bit-Worte und publiziert erst bei vollständig gültigem
Read. Präziser BusFault wird nur bei aktivem Guard, exaktem LDR-PC/BFAR und
passender Bank2-Doppel-ECC-Adresse zur festen Rückkehrstelle geleitet.
MSP/PSP und beide FPU-Frameformen benutzen R0–xPSR am Stackanfang (PM0253
Fig11); keine willkürliche PC-Inkrementierung, keine allgemeinen Faults ignoriert.
70740 native Checks mit4480 Reads und328 Framefällen sowie tatsächliche
Recovery/Write/Cancel über den Adapter bestehen. C11/O2/Werror, ASan/UBSan,
Cortex-M7-Kompilierung mit echtem HAL-/Scene-Teillink und binäre Guardprüfung
bestehen. Alle258 bisherigen Firmwareinputs sind exakt PR167; keine Klangänderung.
Die Komponente ist noch nicht an CMake/Main/Vektoren oder UI angeschlossen.
Ein isolierter Geräte-Testbuild muss die dokumentierten Bank-/Vektor-/Besitz-
Voraussetzungen erfüllen; erst reale ECC-Injektion, Power-cut und IRQ-/Latenz-
Messungen können die Geräteverträge abnehmen. Journal bleibt OFF und Live-Save
synchron. [SD41_H743_SCENE_IO_CHECKPOINT.md](SD41_H743_SCENE_IO_CHECKPOINT.md).

## Ressourcen und zeitlicher Vertrag

44100 Hz, Stereo PCM16, H743-Renderblock 512 Frames (=11,61 ms); beliebige
positive Renderlänge wird in höchstens 512 zerlegt. Rampenangaben (40/80/100 ms)
sind nominelle Audioprozessor-Dauern; Befehlsübernahme/Retirement-Ack kommt an
Blockgrenzen hinzu (bei 512 bis 11,61 ms), nicht als latenzfreier Main-Aufruf. Source/Room-Rampen und
natürliches Pluck-Ende sind samplebasiert. 64-vs-512-PCM und ungültige
Eingaben werden gegen den tatsächlichen Kern geprüft.

Kein Heap, kein I/O und keine Quellenanregung im Audio-Render. Drei feste
Slots,16 Tails, drei feste Phrasenzustände und begrenzte Core-Registertabellen;
höchstens zwei Vorbereitungen pro COAST-Tick, sonst eine, keine unbounded Suche.
Pluck-Vorbereitung: fixer 1024-Float-Loop, höchstens acht harmonische Moden;
im Produkt N≤315, keine große Stacktable. Sourcefilterkoeffizienten maximal
alle 32 Samples pro aktiver Bowed/Horn-Quelle; `tanf` bleibt ein echter
Worst-case-CPU-Kandidat. Room pro Sample 8 Feedbacklinien +4 Allpässe;
kaltes Room/Nature überspringt DSP, Zero-Übergang leert einmalig den Tank.
Clear-Init/Memset im Audiokontext, kalte Cachepfade und DMA-Clean gehören
zur späteren Spitzenlastmessung.

Roomziele werden von genau einem Main-Schreiber als vollständige Generation
veröffentlicht. Der Audio-Leser versucht höchstens einmal pro Renderabschnitt,
mit maximal elf atomaren Wortloads und ohne Retry/Spin. Bei laufender oder
geänderter Publikation behält er seinen letzten vollständigen Zielstand.
Die acht unveränderten `expf`-Berechnungen bleiben vor der Publikation in Main;
Audio-Zielcache36B plus Version4B. Gezielte Preemptionfälle und ganze normale
PCM sind geprüft; tatsächliche IRQ-/Cachelatenz bleibt SD48. Details:
[SD47_ROOM_HANDOFF_CHECKPOINT.md](SD47_ROOM_HANDOFF_CHECKPOINT.md).

Vor einer neuen Quellenvorbereitung entfernt Main die alten Release-/Quick-
Bits des nach tatsächlichem Retirement freien Besitzers. Ein Audio-IRQ kann
die alte Quelle vor deren später Main-Stopp-Publikation beenden; dieser alte
Befehl darf keinen neu belegten Ton löschen. Zwei atomare Wort-RMWs in Main,
kein neuer State und keine zusätzliche Audiooperation. Andere Besitzer und
gültige aktuelle Stopps bleiben erhalten. Ein Main-Schreiber, keine Engine-
Steuerungsreentranz im Note-Hook; Cortex-M-Exclusive-Retries sind keine feste
Main-WCET. Gezielte IRQ-Fälle, ganze normale PCM und echter ARM-Gegenlink:
[SD47_SOURCE_HANDOFF_CHECKPOINT.md](SD47_SOURCE_HANDOFF_CHECKPOINT.md).

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
Quellen-/Grenzvergleiche27 s. Vollständige integrierte musikalische Phrasen
COAST70 s, WOODLAND50 s und HIGHLANDS65 s, ohne Clear/Schlussfade; getrennte
RAW-Messung und eine feste LISTEN-Gainänderung mit Ziel−23LUFS. Längere
Langzeitläufe bleiben interne Messung mit kurzen ausgewählten Exzerpten.
Commit, Sourcehashes, Parameter, Seed, Frame-acknowledgement-Trace, rohe/matched
LUFS/True-Peak/DC/Mono sind gespeichert. Hookframe ist ein DSP-Acknowledgement,
kein behaupteter samplegenauer Attackbeginn.

Dry: D3/D4/A4; Color: D4 bei 0/0,5/1; SHAPE: D4 mit Attack+Release gemeinsam
0/0,5/1; World: echtes Anfangsstück; Nature: optionale Ebene allein;
Blind: unbekannte Reihenfolge, gleiche D-Major/Equal, Dry/Nature0.
42 achtsekündige Summen-/Registerprobes messen True Peak/DC/Mono an behaltenen
Parametergrenzen: 24 volle Endwertkombinationen im weiten Register plus 18
gezielte niedrige/hohe/dichte Registerfälle (je min/max). Kein kartesischer
Sweep; WAVs werden danach verworfen. Antwortschlüssel getrennt.

Sound-Freeze erst nach trockener Familienauswahl, sinnvoller Nature-Entscheidung,
Blindunterscheidung, längerer angenehmer Wahrnehmung und echter H743-/Ausgangs-
Abnahme. Keine technischen Messwerte schließen Alarm-/Buzz-/Tube-Einwände
oder garantieren Beruhigung. UX folgt diesem Softwarevertrag; Displayruhe und
Wake dürfen weder Audio-DMA noch musikalische Zeit anhalten.
