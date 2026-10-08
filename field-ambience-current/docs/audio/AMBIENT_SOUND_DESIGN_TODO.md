# AMBIENT — verbindliche Sounddesign-To-do

## KERNURTEIL

AMBIENT besitzt jetzt einen tatsächlich reduzierten Produktkandidaten: drei
verschiedene Grammatiken, gemeinsame Quellen-/Besitz-/Harmonieprüfung,
echte Hz-/Besitzer-/Tailhistorie und einen gemeinsamen Raum. Die größte offene
Schwäche ist die ungehörte Familienauswahl: insbesondere HIGHLANDS, Bowed-Grain
und optionale Natur sind noch kein belegter ruhiger Produktklang.

Stand **2026-10-08**. Diese Liste umfasst alle SD00–SD53. Aktueller Vertrag:
[PRODUCT_SOUND_SPEC.md](PRODUCT_SOUND_SPEC.md); verifizierte Checkpoints:
[PRODUCT_CORE_CHECKPOINT.md](PRODUCT_CORE_CHECKPOINT.md). UI, Display,
physische Encoder/Tasten und die 15-Minuten-Ruhe folgen nach dem Soundstand.
`FAM_SOUND_PROFILE=product` ist der neue Kandidat, `reference` bleibt Default.
Ältere Render-/Next-/Locked-Einträge sind keine Produktfreigabe.

## Hörfeedback 2026-10-08 — musikalische Priorität

Der Nutzer bewertet die bisherigen trockenen Einzelquellenbeispiele als RAW,
langweilig, zu ähnlich und ohne musikalische Besonderheiten. Gewünscht ist ein
Ensemble aus gehaltenen Akkordtönen mit gemeinsamen Tönen, Oktavwechseln,
verschiedenen Dauern sowie hörbarer Spannung und Auflösung.
**Zuerst SD11/SD14: Akkordbogen und Stimmführung.** Ein komponierter 27-s-
Entwurf mit unseren echten C-Quellen ist vorbereitet; er ist noch kein autonomer
Generator und bestätigt keine isolierte Familienwahl aus SD02–SD04. Siehe
[MUSICAL_ENSEMBLE_DIRECTION.md](MUSICAL_ENSEMBLE_DIRECTION.md).
Weitere reine Quellen-/SHAPE-Pakete folgen nach diesem musikalischen Fundament.
**Nutzerurteil:** Die Ensemble-Probe wurde positiv gehört („ja besser!!!!“).
Gewünscht sind jetzt verschiedene musikalische Algorithmen pro World. COAST
hat einen seed-gesteuerten Gegenbewegungs-Entwurf: D3/D5 treffen sich in D4,
verbunden durch einen gehaltenen Akkordton und erneut positiv gehört.
HIGHLANDS-Akkord/Pause/Rückkehr (44 s) ist ebenfalls als Host-Algorithmus
vorbereitet. Die kurze WOODLAND-Ruf/Antwort-Probe wurde als zu gezupft verworfen.
Ein neuer langer Saiten-Kandidat (45 s, weicher Einsatz, lange Überlagerung)
ist gerendert und zeitlich/technisch geprüft; seine Hörwahl bleibt offen. Siehe [WORLD_ENSEMBLE_ALGORITHMS.md](WORLD_ENSEMBLE_ALGORITHMS.md).
COAST ist jetzt im echten Product-Generator integriert: zwei gegenläufige
Außenstimmen, gehaltener gemeinsamer Akkordton und ein einzelner Zielton.
48 vollständige Engine-Phrasen über alle Keys, Collections und Stimmungen,
Host-Suite und H743-Build bestehen. Die70-s-Firmwareprobe erreicht mit den
normalen Hüllkurven die Mitte bei43,8 s; sie braucht noch ein eigenes Hörurteil.
Siehe [COAST_GENERATOR_CHECKPOINT.md](COAST_GENERATOR_CHECKPOINT.md).
Auch WOODLAND ist jetzt als lange Saite mit eigener Akkordrollen-Regel im
Product-Generator integriert. Zwei vorhandene Saiten, sechs weiche Einsätze,
ungleiche Halteabsichten und echte gemeinsame Akkordtöne. Die50-s-Probe
verwendet die normale Engine;48 vollständige Phrasen und H743 sind geprüft.
Neue Quellen-/Firmware-Hörwahl bleibt offen. Siehe
[WOODLAND_GENERATOR_CHECKPOINT.md](WOODLAND_GENERATOR_CHECKPOINT.md).
HIGHLANDS-Generatorintegration und abschließende World-Abnahme bleiben offen.
Weiterhin eine konkrete Arbeitseinheit nach der anderen.

## FUNDAMENTAL FALSCH

Die Referenzarchitektur mit fünf Presets, zusätzlichem Pad/Bass und neun FX
als fertige neue Produktidee auszugeben wäre falsch. Diese Produktpfade sind
im reduzierten Kandidaten tatsächlich ausgeschlossen und Scenes migriert.
Eine trockene alarmartige/summende/hohle Quelle mit Nature oder Hall zu
kaschieren bleibt ausgeschlossen. Harmonie-/Pegeltests belegen keine
Beruhigung und keine allgemeine Verträglichkeit.

## NOCH NICHT SELBSTVERSTÄNDLICH

Quellenregister, Color/SHAPE, Lautheitsbeziehung und Nutzen von Nature müssen
gehört werden. Der dritte Tonkörper wird bei fehlender eigener ruhiger Rolle
entfernt. Live-Save blockiert noch Main/Generate-Planung; der Default erhält
bei Power-loss während Sector-Erase keinen alten Save. Ein separat geprüfter
[Journal-Kandidat](SCENE_JOURNAL_CHECKPOINT.md) erhält den vorherigen Datensatz
im Flash-Modell, bleibt aber bis zu ECC-/Geräteabnahme deaktiviert.
Storage-/Geräteabnahme ist offen.
Die vorläufige physische Drive-Zuweisung ist funktionslos und muss vor einer
fertigen Produkt-UI verschwinden; die Audioarchitektur bekommt dafür kein FX.

## LOCKED

Als geprüfter technischer/produktlogischer Vertrag, nicht als gehörte Klangwahl:

- Erst tatsächlicher DSP-Start zählt in Hooks, Motiv und gehörter Historie.
- Drei gemeinsame Slots für Manual/Generate; Pluck zwei; Releases zählen.
- Originalbesitzer und angewandte Hz bleiben bis zum tatsächlichen Ende.
- Keine implizite Pad/Bass/Drone-/Rauschbegleitung; Nature default aus.
- Ein gemeinsamer interner Float-Raum; getrennte optionale Naturebene.
- Kein harter held Steal und kein neues Hall-/Pitchgedächtnis pro World.
- Kurze Quellenvergleiche bleiben kompakt; vollständige lange Ensemble-Phrasen
  dürfen45/50/70 s zeigen. Rohe Pegel und fester Hörgain sind getrennt dokumentiert.
- Konzept → Sound → UX/UI/Display → reale Geräteabnahme bleibt die Reihenfolge.

Die Tabellen A–E und Aufgabenbeschreibungen unten enthalten den ursprünglichen
Entwurfs-/Referenzausgangspunkt. Ihre **aktuelle** Umsetzung/Entscheidung steht
in der folgenden Statusmatrix und vollständig in der versionierten
Produktspezifikation. Historische Kandidatenbereiche sind keine zweiten Defaults.

## REMOVE / MERGE / REDESIGN

| Bestandteil | Entscheidung für die Zielarchitektur | Woran die Entscheidung hängt |
|---|---|---|
| Automatisches Bed, Bass, Eno-Loops, Drone | Kein Default; versteckte manuelle Begleitung entfernen | Gleiches Klangprinzip manuell und autonom; keine leeren Pausen mit Unterbau füllen |
| Sechs V2-Synths in `Synths_Archive` | Referenz/Kompatibilität; nach Scene-Migration aus Produktlink nehmen | Keine gespeicherte Bedeutung still ändern; tatsächliche Link-/RAM-Wirkung messen |
| Choir, Guembri, Ember, Glass und alternative Pad-Stimmen | Keine zusätzliche World allein aufgrund ihrer Existenz | SD00 entscheidet Erreichbarkeit; erhaltene Produktfunktion braucht eine eigene Rolle und volle Abnahme |
| Body | Zuerst mit/ohne vergleichen; bei fehlendem Nutzen entfernen | Eigenständiger Klangkörper ohne feste röhrenartige Peaks, Mono-/Übergangsschäden |
| Raum | Gemeinsamer reduzierter Raumpfad als Basis | Körper, Abstand und Tails wirken bei allen Welten zusammen |
| Delay | Nur behalten, wenn eine leise räumliche Antwort entsteht | Kein Taktgeber, keine automatisch neue kontrastierende Stimme |
| Chorus / Age / Blur | Rollen gegeneinander prüfen; redundante Bewegung zusammenführen oder entfernen | Quellen enthalten teils schon Verstimmung/Bewegung; zusätzlicher Effekt braucht hörbaren Mehrwert |
| Shimmer / Reverse Swell | Kein Default, kein autonomer Pflichtvorlauf; Produktnutzen zuerst beweisen | Kein hoher Alarmanteil, Phantomereignis oder harmonisch unverwalteter Ton |
| Dream Chain / neun Effektseiten | Keine obligatorische Kombination oder Anzahl | Kleinste verständliche Palette wählen; entfernte Funktionen wirklich aus Laufzeitpfaden nehmen |
| Wind / Regen / Wellen / weitere Texturen | Natur optional und getrennt vom Raum; zunächst aus für World-Abnahme | Ortswirkung statt Dauermaskierung; Aufwand muss hörbaren Nutzen rechtfertigen |
| Brightness, Mood, Resonance, Sweep, Envmod, Motion, Vibe | Bedeutungen zusammenführen oder begrenzen | Ein musikalischer Einfluss braucht eine klare, durchgehend brauchbare Wirkung |
| Bestehende fünf World-IDs | Versioniert migrieren, nicht umbenennen und umdeuten | Alte Scene lädt definiert oder erhält eine ausdrücklich dokumentierte Ersatzzuordnung |

## BESTE VERSION

AMBIENT erzeugt drei eigenständige Arten musikalischer Zeit aus einer gemeinsamen
Klanghaltung: **Zusammenhang weitergeben, einander antworten, Raum zwischen
Fragmenten lassen.** Jede Welt besitzt einen tragenden Tonkörper, begrenztes
Gedächtnis und eigene Regeln für Einsatz, Wiederholung, Variation und Ruhe.
Keine Welt ist eine zusätzliche Spur. Manuelles Spielen nutzt dieselbe Familie
und dieselben Klanggrenzen; der autonome Betrieb übernimmt die Zeitgestaltung.
Der gemeinsame Raum verbindet beide, ohne die Quellen unkenntlich zu machen.

### A. Drei hörbar verschiedene Systeme

Die Werte in dieser Tabelle sind **Startbereiche für die Konstruktion und den
Hörvergleich**, keine Eigenschaften des aktuellen Builds oder fertige Presets.
Sie werden in SD51 anhand der Abnahme durch exakte Werte ersetzt.

| Welt | Tonkörper und Identität | Ereignisgrammatik | Anfangsgrenzen und Verwerfungsgrund |
|---|---|---|---|
| COAST | Bowed-Kandidat: warmer, tragfähiger Ton; kein Flötenpfeifen, Bienen-/Orgelteppich oder synchrones Pumpen | Tragender Ton → verbindender gemeinsamer Ton → optionaler Farbton → individuelle Ablösung. Stimmen übergeben den Zusammenhang; nicht alle zugleich anschwellen | Gehaltene Töne zunächst 6–14 s; neue Einsätze ungefähr 4–10 s auseinander, individuell geplant. Meist 1–2 Quellen, dritte selten. Scheitert, wenn Verstimmung, gemeinsame Modulation oder Resonanzen die Identität dominieren |
| WOODLAND | Pluck-Kandidat: sanfter erkennbarer Anfang, hörbarer Grundton, abnehmende Obertonenergie; weder Klick noch Rassel | Kleine Figur → hörbare Antwort aus deren Merkmalen → begrenzte Variation → Ruhe. Motiverinnerung wirkt auf Kontur, Rhythmus und Register | Zunächst 2–3 Ereignisse pro Figur, 2–6 s innerhalb einer Figur, 6–16 s zwischen Figuren; lokale Pluck-Poolgrenze 2 beachten. Scheitert als bloßer Zufallsarpeggiator oder uniforme Glockenfolge |
| HIGHLANDS | Horn-Kandidat: runder Ton mit anderer Artikulation als COAST, ohne Rohr-/Signal-/Sirenencharakter | Kurzes Fragment → Nachklang → längere offene Pause → verwandtes oder neu kontrastiertes Fragment. Ruhe ist tragende Rolle | Zunächst 2–4 Töne pro Fragment, Tonlängen 1,5–5 s, 8–20 s Pause danach; meist 1 Quelle, 2 nur begründet. Keine regelmäßig alternierende Zweitonfolge. Quelle verwerfen, wenn Pfeifen/Hohlklang bleibt oder COAST nur dünner klingt |

Die Bereiche sind keine zufällig gewürfelten Timer. Eine Figur bekommt eine
zeitliche Absicht; eine Variation bewahrt mindestens ein erkennbares Merkmal.
Activity verkürzt später nicht blind jeden Abstand. Ruhe und lange Übergaben
bleiben auch am oberen brauchbaren Parameterende erhalten.

**HIGHLANDS-Abbruchregel:** Erst Ursache trocken isolieren, dann höchstens eine
begründete neue Horn-Fassung vergleichen. Bleibt sie unpassend, Horn als
Produktquelle verwerfen. Ein einfacherer harmonischer Tonkörper ist ein neuer,
gesondert abzunehmender Kandidat, kein zusätzlicher Layer. Solange kein eigener
ruhiger Klangkörper überzeugt, liefern zwei starke Welten die bessere Version
als eine künstlich erzwungene dritte.

### B. Gemeinsamer Klangvertrag

1. **Tonhöhe:** Geplante, im DSP tatsächlich angewandte und als klingend
   eingetragene Frequenz stimmen überein. Jeder Quellenbereich ist bekannt.
   Unzulässige Noten werden vor dem Commit abgelehnt; kein heimliches Clamp
   eines 20-Hz-Vorschlags zu einer realen 60-Hz-Note mit falscher Historie.
2. **Register:** Für den vergleichbaren Einstieg D als tonales Zentrum,
   gleiche Stimmung und das bisherige D3–A4 als Referenz. Danach wird pro Quelle
   ein hörbar tragfähiger Bereich festgelegt. Tiefe allein genügt nicht;
   Grundton/Obertöne müssen auch an der tatsächlichen Ausgangskette tragen.
3. **Harmonie:** Gemeinsamer Core, sparsame modale Farbe, weite tiefere Lagen,
   keine unkontrollierten tiefen Sekunden oder Tritoni. Farbwechsel prüfen
   laufende Quelltöne und relevante Effektnachklänge. Ruhe ist ein gültiges
   Ergebnis; ein dauerhaft leer blockiertes System ist ein Fehler.
4. **Belegung:** Ziel sind höchstens drei allozierte tonale Quellen über
   Familien, Weltwechsel und Releases hinweg; lokale kleinere Pools gelten
   zusätzlich. Eine Quelle belegt die tiefste tragende Rolle, keine unabhängige
   Grundtonverdopplung. Effekttails belegen Harmoniegedächtnis, keine neue Stimme.
5. **Pegel:** Feste Kalibrierung nach Quelle/Rolle/Register; keine laufende
   Normalisierung oder AGC. Generative Akzente bleiben klein. Aufnahmegain und
   tatsächliche Lautstärke sind getrennte Größen.
6. **Artikulation:** Erkennbarer weicher Anfang, natürliche Entwicklung,
   weicher kontrollierbarer Schluss. Attack darf einen Pluck nicht in einen
   anonymen Pad verwandeln; lange Releases dürfen keine versteckten Dauerstimmen
   erzeugen. Release und Raumfahne bleiben getrennte Zustände.
7. **Bewegung:** Zeitentwicklung stammt primär aus musikalischen Beziehungen.
   Mikrovariation ist begrenzt, pro Stimme und seed-reproduzierbar. Tonhöhe,
   Pegel, Farbe und Pan bewegen sich nicht alle gleichzeitig. Keine gemeinsame
   periodische Welle als Ersatz für Entwicklung.
8. **Raum:** Eine gemeinsame Raumtopologie, musikalisch kalibrierte Sends,
   erkennbare direkte Tonkörper, brauchbare Mono-Summe. Neue World schaltet keine
   zweite Arena ein und löscht nicht blind den vollen Nachhall.
9. **Stille:** Leerer Eingang erzeugt kein Effekt-Eigenbrummen/-rauschen.
   Ein musikalischer Stop beendet neue Ereignisse und lässt definierte Tails;
   Stillwerden beendet die gesamte Ausgabe weich und begrenzt. Master/Mute
   wirken nach dem Raum. Displayzustand ist kein Audiozustand.

### C. Vollständiges Inventar und Entscheidungspflicht

Jeder tatsächlich erreichbare Weg erhält in SD00 genau eines von **KEEP**
(Produktpfad, vollständig prüfen), **REMOVE** (Migration + alle Einstiegspunkte
entfernen) oder **REFERENCE** (nicht aus Produktbedienung/Scenes erreichbar).
Ein Name, Amount=0 oder ein verschobener Ordner beweist keine Entfernung.
Historische Codepfade werden nicht aufwendig retuned, wenn sie nicht ausliefern.

| Bereich | Tatsächlicher Ausgangspunkt / zu erfassende Wege | Ziel / zuständige Aufgaben |
|---|---|---|
| Primärquellen | `bowed.c`, `pluck.c`, `horn.c` | Drei Kandidaten trocken und in Mono; SD02–SD07 |
| Kompatibilitätsquellen | `choir.c`, `guembri.c`, `pad.c`, Pad-Stimmen/PADsynth, Ember/Glass, Bass/Sub/Deep/HarmonicBass, Drone; Oneshots und automatische Hooks | Erreichbarkeit statt Vermutung; SD00, SD08–SD10, SD36, SD46 |
| V2-Archiv | Sechs Familien in `src/v2/Synths_Archive`, verbundene native Parameter sowie Arp/Beat/Field-Wege | Weiter gelinkt; IDs/Scenes prüfen, danach nicht gewählte Produktpfade auslinken; SD00, SD36, SD46 |
| Tonführung | `harmony.c`, Pitch-Memory, `pitch_modes`, Equal/Just, Key/Mode/Vibe, Voice/Backend, Autoplay/Generate/Seed | Tatsächliche Töne statt nur Pitch Classes verwalten; SD07–SD17, SD40 |
| Tonkörper | `body.c`, `shape.c`, Gainadapter, Quellen-Farb-/Damp-Parameter | Kein pauschaler Weltmaterial-Filter; SD02–SD07, SD18, SD32–SD35 |
| Aktiver Masterraum | `fx_master.c` / `ambient_effects.h`: Bypass, Dark Reverb, Ping Pong Delay, Chorus Detune, Tape Age, Reverse Swell, Shimmer Reverb, Blur, Dream Chain | Alle neun erreichbaren Fälle beurteilen; kleinste gerechtfertigte Palette; SD19–SD27 |
| Weitere Raumwege | `reverb.c` und vorhandene Sends/Returnadapter; Legacy Echo/Tape/Blur/Shimmer-Dateien | H743 schließt die vier alten FX-Dateien bereits aus. Tatsächlich gelinkte/aufgerufene Wege erfassen; keine zweite Effektarena rekonstruieren; SD00, SD19–SD20, SD46 |
| Natur | `ambience.c`: Wind für alle bisherigen Welten bei Amount>0; Regen, Wellen/Sea-Hum, Fjord-/Desert-Varianten | Optional, kein default identity layer; Wind ist bereits unregelmäßig. Verbleibende klangliche Homogenität hören; SD28–SD31 |
| Texturen | `texture.c`: braunes Tiefrauschen, Breath, Warm/Air und erreichbare weitere Noise-/Crackle-Wege | Rumpeln, Bandpeaks, Maskierung und Idle-Ausgabe prüfen oder Pfad entfernen; SD00, SD31 |
| Endstufe | Dry/Send → Master-FX → Volume/Mute/DC/Peak/PCM und reale HAL-Ausgabe | Ganze Kette, nicht nur isolierte Synths; SD34, SD42–SD43, SD48–SD50 |
| Erinnerung | SCN6 mit SCN5-Migration, World-/Synth-IDs, Parameterdefaults, Scene-Recall/Seed | Kein unerwartetes Autoplay, keine heimliche Umdeutung; SD36, SD41 |

### D. Parametervertrag vor Tastenbelegung

SD32 führt ein versioniertes Register **jedes erreichbaren Soundparameters**.
Pflichtfelder: Zweck; Besitzer; Einheit; Minimum/Default/Maximum; hörbare Kurve;
Smoothing; Wirkung auf laufende oder nur neue Stimmen; Speicherverhalten;
Priorität gegenüber World-Preset; abhängige Parameter; Verhalten bei 0, Stille,
ungültigem Float und Szenenwechsel. Gültige Bereichsverletzungen werden begrenzt;
NaN/Inf werden verworfen und behalten den letzten gültigen Zielwert. Interne
Setter werden damit nicht automatisch zu Produktreglern.

| Vorhandene Parameterfamilie | Zielentscheidung, bevor sie erreichbar bleibt | Aufgabe |
|---|---|---|
| Key, Key-PC, sechs Modes, Equal/Just, Vibe, Mood | Welche musikalische Rolle bleibt? Keine doppelte Farbsteuerung. Alle freigegebenen Kombinationen innerhalb der Quellenregister; retune betrifft Besitzer/Fahnen definiert | SD11–SD12, SD32, SD40 |
| Source/Voice, Pad-Voice, Synth/Backend, native Param-Slots | In Produktwelten gebunden; Archivparameter nur mit bewusster Kompatibilitätsentscheidung erreichbar | SD00, SD10, SD32, SD36 |
| Attack, Release / SHAPE | Pro Familie erkennbare Artikulation über den gesamten Bereich; bestehende neutrale Defaults beibehalten, bis Hörvergleich bessere Werte liefert | SD02–SD07, SD32, SD35 |
| Pluck Damp; Bowed Colour und innere Bewegung; Horn Körper/Breath | Quellengebundene musikalische Grenzen, zuverlässiger Tonkörper, keine ungewollte Pitch-/Pegelbewegung | SD02–SD07, SD34 |
| Brightness, Resonance, Drive, Sweep, Envmod | Ein gemeinsamer Farbauftrag mit familiengeeigneter Umsetzung; wirkungslose/redundante Setter entfernen. Drive nie Voraussetzung für Körper | SD18, SD32–SD35 |
| Reverb Size/Damp/Drive, Wet-Amp, Source Send | Einen aktiven Raumvertrag wählen; Legacy- und Mastermappings nicht gleichzeitig widersprüchlich betreiben | SD19–SD20, SD32 |
| Space, Atmosphere, Texture | Raum und Natur trennen. Aktuell verändert Atmosphere sowohl Naturlevel als auch FX-Send | SD20, SD28, SD32–SD33 |
| Echo, Delay Seconds, Feedback | Antwort ohne Pulszwang; Zeitänderung ohne unkontrollierten Pitch-Sweep oder Pegelanstieg | SD21, SD32, SD35 |
| Motion, Age, Blur | Rollen/Modulationsbereiche trennen oder reduzieren; kein Summen, Flutter-Alarm, Grundtonverlust oder Doppeleffekt | SD22–SD24, SD27, SD32 |
| Shimmer, Reverse Swell, FX-Mode | Nur nach Nutzennachweis; Effekttöne zählen im Harmoniegedächtnis. Offline-Reverse ist kein Echtzeit-Feature | SD25–SD27, SD32 |
| FX Width, Tone, Level | Auch indirekte Presetwerte prüfen: Mono-Körper, kein schriller Maximalwert, kein versteckter World-Lautstärkeregler | SD19–SD20, SD32, SD34, SD42 |
| Bass Depth, Drone, Texture Amount, Autoplay Melody | Kein verdecktes Wiederaktivieren entfernter Begleitung durch Scene, Makro, Tuning oder Hook | SD00, SD10, SD32, SD36, SD41 |
| Generate Program/Seed, Dichte/Activity | Seed reproduziert musikalische Entscheidungen; Activity steuert Figuren/Übergaben, nicht beliebige Notenflut. Abgewiesener Start ist nicht gehört | SD13–SD17, SD33, SD38 |
| Master Volume, Mute, Clear/Stillwerden | Unabhängig von Quelle, World und Raum; keine ausgenommene Fahne oder Hintergrundquelle | SD39, SD41–SD42 |

Arbeitsmodell für spätere musikalische Makros: **Activity, Color, Room** plus
getrennte Lautstärke. Das legt weder Anzahl noch Position von Encodern/Tasten
fest. Eine vierte Klangdimension wird nur ergänzt, wenn sie einen unabhängigen,
sofort hörbaren Nutzen besitzt. Die elf Felder von `AmbientFxParameters` sind
damit kein Auftrag für elf Bedienelemente.

### E. Ressourcenvertrag

Gemessener Linkstand des oben genannten DSP-Commits, H743 Release,
Cortex-M7 Hard Float, `-O3 -DNDEBUG`:

| Bank | Belegt | Linkerbudget | Frei | Folgerung |
|---|---:|---:|---:|---|
| Flash | 255.316 B | 1.966.080 B | 1.710.764 B | Scene-Sektor bereits aus diesem Firmwarebudget ausgeklammert |
| DTCM | 119.440 B | 131.072 B | 11.632 B | Kritische Reserve; keine zusätzlichen großen automatischen/hot States hineinlegen |
| D1 | 417.408 B | 524.288 B | 106.880 B | Quellen/Übergänge anhand echter Sektionen budgetieren |
| D2 | 258.112 B | 294.912 B | 36.800 B | Aktive Master-FX-Arena bleibt hier; keine zweite große Arena |
| D3 / ITCM | je 0 B | je 65.536 B | je 65.536 B | Kein Beleg für freie DMA-taugliche oder austauschbare Speicherfläche |

Master-FX: 8 FDN-Linien, interne Arena bis 245.760 B, derzeit 44,1 kHz,
maximal 750 ms Delay und 220 ms Blur-Puffer. Drei Welten benötigen drei kleine
Grammatikzustände, keine drei Kopien davon. Jede DSP-Einheit weist Delta und
Bankplatzierung aus. Archivierung alleine hat keine Pools eingespart.

Der Produktplan arbeitet **ohne SD-Streaming und ohne erforderliche Samplebank**.
Eine SD-Nachrüstung kann erst aus PCB/Pins/Bus/Strom/Layout beurteilt werden;
sie ist keine Voraussetzung oder zugesagte Option. Eventuelle serielle externe
Speicher sind kein Ersatz für diese heißen Audio-Delay-Linien.

Projektziel am Gerät: DWT-`peak_load < 0.60`, null Deadline-Misses, nachgewiesene
Stackreserve im Worst Case. Das ist **noch nicht gemessen**. Linkerplatz sagt
nichts über Note-on-Spitzen, gleichzeitig retirierende alte Engines, Stack,
Interrupts oder die reale Ausgabekette.

### F. Historischer Referenznachweis — nicht erneut blind abarbeiten

Diese Tabelle beschreibt den Ausgangspunkt vor PR130–PR136; der aktuelle
Produktstand ist die Statusmatrix unten, nicht der alte manuelle/FX-Pfad.

| Ergebnis | Status und genaue Grenze |
|---|---|
| Generativer Start als Transaktion | Implementiert/Host geprüft; ungültige oder volle Starts verändern keine gehörte Historie. Allgemeine Parameter-/Actual-Pitch-Verträge bleiben SD07 |
| Keine automatische autonome Begleitung | Implementiert; Harmoniestufen starten keine Pad/Bass/Eno/Reverse-Pflichtstimmen. Alte manuelle Starts bleiben SD10 |
| Drei-Slot-Zulassung bei Generate und Handover | Implementiert; zählt auch bestehende Pad/Ember-, zwei Bass- und Drone-Zustände sowie Releases. Globale manuelle Belegung bleibt SD09 |
| Muted native/alte Ambient-Releases | Retirieren statt einzufrieren und später wiederzukehren. Zusätzliche Übergangs-CPU bleibt Geräteprüfung |
| WOODLAND-Pluck | Harmonische zentrierte Anregung, monotone symmetrische Dämpfung, feste Read-Delay-Phase, weicher 4–32-ms-Onset, Ausgabe-DC-Entfernung. Zwei lokale Stimmen und Besitz/Stop erhalten; kein fertiger World-Sound |
| Pluck-Nachweis | 35 statische + 6 Live-Pitch-Cases, worst static 0,034 Cent; zusätzliche Host-Regression und 26-s-Dry-A/B. Klang-/Decay-/Body-/Raumabnahme weiter offen |
| Frühere Bowed/Horn/FX-Korrekturen | Gemeinsames 5,1-Hz-Bowed-Vibrato entfernt, Grain reduziert; Horn-Suboktave/fester 950-Hz-Formant entfernt; Age-Eigenrauschen entfernt. Diese Ursachen nicht erneut als aktuelle Implementierung behaupten |
| H743/Host | Fünf CI-Jobs grün für den Referenzcode, obige Linkwerte. Kein CPU-, Lautsprecher- oder Nutzerurteil |

Belege: [WORLD_ROUTING_CHECKPOINT.md](WORLD_ROUTING_CHECKPOINT.md),
[WOODLAND_DRY_REVIEW.md](WOODLAND_DRY_REVIEW.md),
[WOODLAND_DRY_METRICS.json](WOODLAND_DRY_METRICS.json),
[BOWED_MOTION_REVIEW.md](BOWED_MOTION_REVIEW.md),
[ALPS_BODY_REVIEW.md](ALPS_BODY_REVIEW.md).

### G. Arbeitsmodus und Abnahmestufen

- **Entwurf:** Zweck, Signalweg, Grenzen, Default, Failure Modes und Entscheidung
  sind benannt. Ein Parameterbereich kann als ausdrücklich vorläufiger Kandidat
  festgelegt sein; die exakten Endwerte bleiben nicht unmarkiert offen.
- **Software geprüft:** Implementierung, relevante Hostprüfungen und bei DSP
  Änderungen H743-Release/Map stimmen mit dem Vertrag überein.
- **Gehört:** Kurzer reproduzierbarer Vergleich besteht die konkrete Hörfrage.
  Objektive Tonhöhen-/Pegelwerte sind kein Ersatz dafür. Offenes Nutzerurteil
  bleibt als offen protokolliert, statt durch eine Testzahl ersetzt zu werden.
- **Am Gerät locked:** Tatsächlicher Ausgang, CPU/Stack, Bedienintegration und
  Hörabnahme bestehen. Erst dann darf der Produktpfad als Sound-fertig gelten.

Pro Einheit: eine Frage beantworten, kleinste erforderliche Änderung, relevante
Prüfungen, gegebenenfalls ein <=30-s-Hörvergleich, Befund mit Commit/Seed/Pfad und
offener Restfrage sichern. Nicht Quellen-, Raum-, Grammatik- und UI-Umbau in
einem Hörvergleich vermischen. Keine Tests, die nur Tabellen abschreiben.
Ein entferntes Feature schließt seine Aufgabe erst, wenn alle Eintrittspunkte
und die Migration geprüft sind; ein ungehörter Klang wird nicht abgehakt.

### Umsetzung am 2026-10-06 nach Erstellung der Roadmap

PR130–PR134 sind in den Entwicklungsbranch gemerged: trockener COAST-/Inputfix,
reiner Score, echter reduzierter Kern, H743-Product-Profil / SCN7 und
Room-/Naturekorrekturen. Referenz bleibt Default, die drei Produktfamilien
bleiben Hörkandidaten.

PR135 liefert gehört bestätigte Motiv-/Intervallerinnerung und begrenzte
Kontextübergaben. Sein Code ist Bestandteil von PR136. PR136 schließt
natürliche Pluck-Key-up-Tails, samplegenaues Retirement, echten Mute,
vorbereitete Pitch-Kontextstornos, Endpoint-/Blindpakete und Summen-/Frameaudits.
Final gemerged über PR136: 476f4db068345c88469e141be2fa2f6dac81f6a3.
Geprüfter Head38f7d51b3f6e2dbe2c30026dc8cf26b8430dc364, CI37515501334,
sechs Jobs bestanden. Cacheguard, volle Übergänge und dichte Registerfälle
sind ebenfalls geprüft; PR135 ist als mitübernommener Draft geschlossen.

Zwölf +36 Minuten tatsächliches PCM pro vollem Hostlauf, konservative Hz-Tails,
64-/512-PCM exakt gleich; keine NaN/Inf oder Limiter-Eingriffe. 42 Summen-/
Registerprobes: mindestens 9,66 dB gemessene True-Peak-Reserve, schlechteste
Monoenergie 0,91874.
Hör-WAVs je 27 s, Raw und fixe Vergleichskopie; keine Lang-WAVs.

[PRODUCT_CORE_CHECKPOINT.md](PRODUCT_CORE_CHECKPOINT.md) dokumentiert SHA,
CI/Banken und Restgates. Die folgenden Checkboxes und Statusmatrix sind
**aktuell**, nicht die historischen pauschalen Open-/Next-Einträge.

### Aktuelle Statusmatrix — sämtliche 54 Aufgaben

Abgleich mit der wiedergefundenen PR129-To-do am 2026-10-07: deren pauschale
54 offenen Checkboxen sind überholt. PR130–PR136 liefern den unten belegten
Stand. Als nächster Softwareteil von SD41 liegt ein Zwei-Sektor-Journal mit
Abbruch-/Korruptionstests und Legacy-Erhalt vor; Aktivierung, ECC-sichere Reads
und nichtblockierendes Save sind noch offen. Siehe
[SCENE_JOURNAL_CHECKPOINT.md](SCENE_JOURNAL_CHECKPOINT.md).
Getesteter Code-Head `9768966cc6a3881ea9ea56cb9e8c2eff2b959a8a`,
[CI37602680211](https://github.com/Sancte3D/AMBIENT/actions/runs/37602680211):
6/6 Jobs bestanden, inklusive Default/Product/Journal-H743 und vollständiger
Host-Suite. Der Kandidat liegt im
[gemergten PR138](https://github.com/Sancte3D/AMBIENT/pull/138); die Hardware-Gates
und die 24/30-Zählung ändern sich dadurch nicht.

Fortsetzung einzeln ab dem ersten offenen Punkt: **SD02** hat jetzt getrennte
27-s-Gegenproben für 1,5f, 2f und Grain, beide Quellenkonstruktionen sowie
einen bytegleichen vollständigen Kontrollrender. Kein Produkt-DSP geändert,
keine Quellenwahl vorweggenommen. Messung und genaue Hörreihenfolge:
[SD02_COAST_COMPONENT_CHECKPOINT.md](SD02_COAST_COMPONENT_CHECKPOINT.md).
SD02 bleibt bis zur Hörentscheidung offen. Ein einzelner Durchlauf
hat **SD03 am bestehenden Kandidaten** geprüft: echte gemeinsame Stimmen
gegen separat gerenderte Summe, unabhängiges Grain, natürliche Releases und
bytegleiches Product-PCM bei 64/512 Frames. Die endgültige SD03-Klangabnahme
bleibt von SD02 abhängig; kein weiterer Sound-DSP-Umbau.
[SD03_COAST_INDEPENDENCE_CHECKPOINT.md](SD03_COAST_INDEPENDENCE_CHECKPOINT.md).
PR139 ist durch CI37608566520 (6/6 Jobs) vollständig software-/ARM-geprüft.
PR141 ist inzwischen durch CI37611452027 vollständig geprüft. **SD04** hat
jetzt einen trockenen, fest pegelgematchten COAST/HIGHLANDS-Vergleich über
D3/D4/A4, bytegleiche Ereignistraces und eine 27-s-Montage. Keine Klangwahl
oder neue Horn-Fassung vorweggenommen. Siehe
[SD04_HIGHLANDS_COAST_CHECKPOINT.md](SD04_HIGHLANDS_COAST_CHECKPOINT.md).
PR143 ist durch CI37783241714 vollständig geprüft. Das neue Nutzerfeedback
priorisiert jetzt den musikalischen Ensemble-Entwurf und seine Generatorübertragung.

`geschlossen` bedeutet: das genannte technische/Entfernungs-Kriterium ist
geprüft. Bei `Hörgate` ist der Code gebaut und softwareseitig auditiert, die
zugehörige Klangwahl bleibt offen. Geräte-/UX-Aufgaben werden nicht als erledigt
markiert. Geprüfte SHA/CI/Bankwerte und Hörartefakte stehen im Checkpoint.

| Aufgabe | Gesamtstand | Konkreter Nachweis / nächster tatsächlicher Gate |
|---|---|---|
| SD00 | geschlossen | Produkt-Compile-/Symbolinventar, Spec; jeder Eingang zugeordnet |
| SD01 | geschlossen | ARM-Banken/Compilerframes, keine zweite Arena; Gerätezeit separat SD48 |
| SD02 | Hörgate, isoliert vorbereitet | D3/D4/A4: voller Kontrollrender bytegleich; 1,5f/2f/Grain einzeln entfernt, 2×27-s-Montage; Quellenwahl offen |
| SD03 | Hörgate, unabhängig geprüft | Summenrest ≤1,50e-8, Grain-Korrelation <0,006, Sustain-Swing <0,012 dB, natürliche Releases, 64/512 PCM exakt; SD02/Hörwahl offen |
| SD04 | Hörgate / Keep-or-drop, vorbereitet | COAST/HIGHLANDS D3/D4/A4, identische Ereignisse, fixe −26-LUFS-Kopien und 27-s-A/B; Quellenwahl offen, zwei starke Worlds zulässig |
| SD05 | Hörgate | Horn-LFO/Sub/Formant aus; Onset-Air und SHAPE-Endpunkte prüfen |
| SD06 | Lange Quelle integriert; Hörgate | Product-Saite36-s-Nominaldecay,0,8-s-Attackbasis, eigener weicher Release; Default-Onset/Body0,031 und8-s/Body0,298, natürliche Manual-Tails; Quellenhörwahl offen |
| SD07 | geschlossen | Finite Input, tatsächliche Hz/Velocity, Bereiche, rejected Starts ohne Source/Hook |
| SD08 | geschlossen | Originalowner, DSP-Ack, Storno, tatsächliche One-shot-/Release-Enden |
| SD09 | geschlossen | Drei globale Slots / zwei Plucks auch Manual, kein held Steal |
| SD10 | geschlossen | Gemeinsame Familie/Admit ohne Pad/Bass/Archive; SCN7-Migration geprüft |
| SD11 | Hörgate | Zwei Cores,12 Keys, Equal/Just und niedrige Intervallgrenzen; COAST45..80 und WOODLAND45..68 jeweils in48 echten Phrasen geprüft; Teilton-/Registerhörwahl offen |
| SD12 | Hörgate | Reale Releases, 16 Hz-Tails, max 7,2 s / 250-ms-Quiet; Hörrelevanz kalibrieren |
| SD13 | geschlossen | COAST/WOODLAND: eigene begrenzte Rollen-/Phrasenzustände mit DSP-Ack; HIGHLANDS: bisheriger Transaktionsautomat mit höchstens acht Gradindices |
| SD14 | Generator integriert; Hör-/Gerätegate offen | Gegenbewegung, gemeinsamer Akkordton, einmaliger Mittelton; echte Besitzer/Releases/Tails,48 Engine-Phrasen, volle Host-Suite und H743 geprüft; neue70-s-Probe noch hören |
| SD15 | Lange Quelle und Generator integriert; Hör-/Gerätegate offen | Sechs lange Akkordrollen-Starts mit real gehaltenen gemeinsamen Tönen, zwei Saiten inklusive Release,48 echte Phrasen und50-s-Engineprobe; kein Wiederanschlag gemeinsamer Töne; neue Hörwahl offen |
| SD16 | Algorithmus/Hörprobe vorbereitet | HIGHLANDS-Akkordbogen mit gehaltenem F#4, realem Horn, Pausen/Rückkehr; Generator-/Geräte-/Hörabnahme offen |
| SD17 | Hörgate | 859 Pure-Score-Wiederkehren, tatsächliche Intervallerinnerung; Langzeitwirkung hören |
| SD18 | geschlossen durch Remove | Body nicht kompiliert; kein Materialreset oder additive Röhrenfärbung |
| SD19 | Hörgate | Ein FDN; Impuls/Decay/Mono geprüft und Mono-Auslöschung korrigiert |
| SD20 | geschlossen | Source-Send 0,35, ein Room, unabhängige Nature, gemeinsame Userwerte erhalten |
| SD21 | geschlossen durch Remove | Eigenständiger Echo-Pfad/Parameter/Scene-FX ausgeschlossen |
| SD22 | geschlossen durch Remove | Chorus/zusätzlicher Detune ausgeschlossen |
| SD23 | geschlossen durch Remove | Tape Age und Eigenhiss/-hum ausgeschlossen |
| SD24 | geschlossen durch Remove | Blur/Arena/Altparameter ausgeschlossen |
| SD25 | geschlossen durch Remove | Shimmer/Oktavregeneration ausgeschlossen |
| SD26 | geschlossen durch Remove | Reverse-/Generate-Vorswell ausgeschlossen |
| SD27 | geschlossen | Nur Dry/Room, exakte Null/kalt, kein Tankrevival; Scenes reaktivieren nichts |
| SD28 | Hörgate / optional weglassen | Nature getrennt/default 0; echter Score bleibt mit/ohne identisch |
| SD29 | Hörgate | Unregelmäßige Wetter-/Gustuhren und Seedstarts; Homogenität nicht numerisch freigeben |
| SD30 | Hörgate | Nur neue leise Wellen/Tropfen, kein Sea-Hum oder Sampleloop; Nutzen hören |
| SD31 | Hörgate verbleibender Wege | Alttexturen auslinkt; übrig nur deklarierte Nature/Quellenartikulation |
| SD32 | geschlossen | Vollständiges normiertes Parameterregister samt Aliases/retired API und Priorität |
| SD33 | Hörgate | Begrenzte Activity/Color/Room/SHAPE und kurze Endpointvergleiche |
| SD34 | Hörgate | Feste .50/.22-Quellfaktoren, Raw/Listen getrennt; kein AGC; Lautheit final hören |
| SD35 | Hörgate | 24 gezielte Endwertszenarien + lange Activity/Tuning/Key/Releasefälle; keine Exhaustivbehauptung |
| SD36 | geschlossen | SCN5/6→SCN7, 368 B, CRC, Provenienz, fehlgeschlagener Save-RAM-Rollback |
| SD37 | Hörgate | Alle sechs Richtungen, Pending/volle Pools/Releases/Room/Rapid Targets softwareseitig geprüft |
| SD38 | geschlossen | Entry ≤100 ms, Stop natürliche Releases, begrenztes Retry, echte Onsets |
| SD39 | geschlossen | Clear/Mute ≤40 ms exakt Null, kein alter Tank bei Unmute, Targets bewahrt |
| SD40 | geschlossen | Held Hz unverändert; neue Key/Mode/Tuning, ungehörte Vorbereitung storniert, Setter idempotent |
| SD41 | Storage-/Gerätegate | Boot/Recall/Seed/Cache geprüft; Journal-Modell erhält vorige Daten bei Abbruch; Kandidat OFF, ECC/H743/Live-Save offen |
| SD42 | geschlossen softwareseitig | 42 Summen-/Registerprobes: True Peak ≥6 dB Reserve, DC/Mono/NaN/Clip; physische Ausgabe SD49 |
| SD43 | geschlossen | Echte C-Kette, SHA/Parameter/Trace/rohe Pegel/fester Gain; jeder Hör-WAV 27 s |
| SD44 | Hörgate | Blindpaket A/B/C: gleiche Key/Tuning, ohne Room/Nature; Antwortschlüssel separat |
| SD45 | Hörgate Langzeit | Mindestens 48 min tatsächliches PCM plus Recovery/Wrap; keine NaN/Limiter/Stuck; Form hören |
| SD46 | geschlossen | Tatsächlicher ARM-Compile-/Funktionssymbol-/DMA-/Bank-Audit, Archive nicht nur umbenannt |
| SD47 | Geräte-/Storagegate | Bounded Hotpath/Prep und Compilerframes dokumentiert; Journal außerhalb Audio-IRQ, Main-Save-Stall und echte Spitzen offen |
| SD48 | echtes Gerät | DWT <0,60, null Misses und realer Stack-High-water mit UI/MIDI/Storage |
| SD49 | echtes Gerät | Gebauter Ausgang, Pegel/DC/Noise/Pops/Lasten/Clock/Power/Cache/Flash prüfen |
| SD50 | echte Hörer + Gerät | Alarm/Tube/Buzz/Beep/Ermüdung konkret lösen, keine Heilbehauptung |
| SD51 | Freeze-Gate | Software Candidate 0.3 dokumentiert; endgültige Endwerte erst nach Quellen-/Hör-/Geräteabnahme |
| SD52 | spätere UX | Tasten/Encoder/Display/LED/15-min-Ruhe, Wake ohne Klangereignis |
| SD53 | Produkt-DoD offen | Alle zutreffenden Gates bestehen; erst dann Sounddesign fertig nennen |

## Verbindliche Arbeitsliste

**54 Aufgaben, SD00–SD53: 24 vollständig geschlossene Software-/Entfernungsaufgaben,
30 Aufgaben mit offener Quellen-, Hör-, Geräte-, Storage- oder UX-Abnahme.** Bereits erledigte Teilbefunde stehen oben und
in den entsprechenden Aufgaben. Jede Checkbox bleibt offen, bis die zugehörige
Abnahme besteht oder der gesamte betroffene Produktpfad nachweislich entfernt
wurde. Abhängigkeiten beschreiben die Reihenfolge, keine zusätzliche Architektur.
Bei Tailwerten und Migration gilt eine ausdrückliche Zweistufigkeit: zuerst
den benötigten Vertrag bzw. KEEP/REMOVE-Entscheid festlegen, dann nach Aufbau
des Zielpfads seine endgültigen Werte bzw. Migration bestätigen. SD19 benötigt
den Quellenvertrag aus SD12, nicht dessen noch ungemessene finale Raumtailwerte.
SD36 benötigt die Effektentscheidung aus SD27; die Entfernung wird anschließend
gegen die migrierten Scenes geprüft. Keine zirkulären Freigabebedingungen.

### 1. Inventar, trockene Quellen, ehrliche Stimmenführung

- [x] **SD00 — Erreichbare Soundpfade abschließend klassifizieren.**
  Für obiges Inventar sämtliche Einstiegspunkte aus Manual, Generate, Scene,
  Makro/Hook und V2 erfassen; KEEP/REMOVE/REFERENCE mit Begründung zuweisen.
  **Abnahme:** Kein gelinkter, bedienbarer oder aus Scene ladbarer Klangweg ohne
  Zuordnung. Nicht ausgewählte Stimmen erhalten keine neue Sounddesignserie.
  Voraussetzung: aktueller Routingcheckpoint; vor SD10/SD27/SD31/SD46.
- [x] **SD01 — Ressourcenbaseline als fortlaufenden Vertrag führen.**
  Obige gemessene Bankwerte und Compilerkonfiguration übernehmen; jede weitere
  Einheit mit State-/Flash-/Stack-/Bankdelta und Quelle des Nachweises erfassen.
  **Abnahme:** Tatsächlicher H743-Link statt Host-`sizeof`; keine nicht budgetierte
  zweite Arena, große Stackvorbereitung oder Samplebank. Gerätezeit offen lassen.
- [ ] **SD02 — COAST-Tonkörper trocken auswählen.**
  Bowed-Grundton, Begleitsäge mit Faktor 1,0041, sympatische Resonanzen bei
  1,5f/2f und spektrale Farbe getrennt vergleichen. Die 1,5f-Komponente auf
  harmonische/raue Wahrnehmung prüfen; keine pauschale Resonanzverstärkung.
  **Abnahme:** Tragfähiger eigener Ton über das gewählte Register, gleicher
  Pegel in beiden Farben, Mono erhalten; weder Pfeifen noch Orgel/Buzz/Tube.
  **Fortsetzung 2026-10-07:** Zehn getrennte Komponenten-/Registerproben und
  zwei 27-s-Hörmontagen, volle Kontrollquelle bytegleich, rohe Pegel und
  konstante Hörgains dokumentiert. Siehe SD02-Checkpoint; nicht als gehört abgehakt.
  Nächste konkrete Quelle; vor SD14/SD18.
- [ ] **SD03 — COAST-Bewegung von der Quelle aus ordnen.**
  Wiederkehrende 0,13-Hz-Körperphase, identischen Startwert und Grain getrennt
  prüfen. Das frühere 5,1-Hz-Gemeinschaftsvibrato ist bereits entfernt.
  **Abnahme:** Mehrere Stimmen beginnen/entwickeln sich eigenständig; keine
  mechanische gemeinsame Welle, störende Schwebung oder Rauschdecke. Gegen
  bewegungslose Referenz prüfen, nicht einfach zusätzliche Zufälligkeit einbauen.
  **Fortsetzung 2026-10-07:** Drei echte Stimmen einschließlich sämtlicher
  Releases gegen Einzelstimmensumme geprüft, aktuelles Grain gegen stationäre
  no-Grain-Referenz, zwei volle 27-s-Passagen und kurze Montage. Product-PCM
  bei 64/512 Frames bytegleich. SD03-Checkpoint; SD02-Auswahl/Hörabnahme offen.
  Voraussetzung: SD02.
- [ ] **SD04 — HIGHLANDS-Tonkörper behalten oder verwerfen.**
  Horn trocken bei gematchtem Pegel gegen COAST: Grundton, Obertöne, Register,
  Filterkörper. Keine Suboktave/festen 950-Hz-Formanten wieder hinzufügen.
  **Abnahme:** Eigenständige warme Artikulation ohne Signal-/Röhrencharakter.
  Bei Scheitern gilt die Abbruchregel oben; keine Umbenennung als Ersatz.
  **Fortsetzung 2026-10-08:** Echter trockener Product-Renderer, bytegleiche
  Ereignistraces, D3/D4/A4 in RAW und fest pegelgematchten LISTEN-Kopien sowie
  sechs kurze A/B-Ausschnitte in 27 s. SD04-Checkpoint; KEEP/REMOVE bleibt offen.
  Clear-/Ausschnittgrenzen sind dokumentiert, keine natürliche Release-Freigabe.
  Vor SD16; nicht mit Raum kaschieren.
- [ ] **SD05 — HIGHLANDS-Artikulation und Atembewegung begrenzen.**
  Verbleibende ungefähr 0,9-Hz-Filterbewegung und kurze Breath-Textur isolieren;
  Beginn/Ende bei minimaler und maximaler SHAPE prüfen.
  **Abnahme:** Kein hörbares Wah, Sirenenanstieg oder angesetztes Flötenpiepen;
  selbst kurze Fragmente sind rund. Dauer und Kontur tragen, statt Effektbewegung.
  Voraussetzung: SD04 oder freigegebener Ersatzkandidat.
- [ ] **SD06 — WOODLAND-Klang und musikalischen Decay fertig auswählen.**
  Den korrigierten Pluck hören: trockenes Register, Anregungsvariation, sämtliche
  Damp-/SHAPE-Grenzen, leise Wiederholungen und zwei lokale Releases. Technische
  Pitch-/Repeat-Pegelkorrektur ist erledigt; nicht denselben Fix erneut schreiben.
  **Abnahme:** Erkennbarer sanfter Tonbeginn und nützlicher Ausklang ohne Rassel,
  Klick, dominantes Oberton-Piepen oder anonymen Swell. Referenz und rohe Pegel
  dokumentieren. Vor SD15/SD18.
- [x] **SD07 — Gemeinsamen Quellen-/Inputvertrag schließen.**
  Actual Pitch, erlaubte Frequenz, Velocity/Gain, Attack/Release und Status pro
  Familie definieren. Pluck-Minimum 60 Hz gegenüber World-API 20 Hz angleichen
  oder vor Zulassung ausdrücklich ablehnen. Float-Parameter finite-validieren;
  `shape.c` verwirft inzwischen NaN/Inf; der gemeinsame Vertrag wird gegen echtes PCM geprüft.
  **Abnahme:** Ungültiger Input erzeugt weder Stimme noch Commit/Hook/RNG-Effekt;
  gültige Note wird mit ihrer tatsächlichen Frequenz eingetragen. Vor SD08–SD12.
- [x] **SD08 — Besitzer und sämtliche One-shot-Tails vollständig verfolgen.**
  Auch unowned Guembri-/Oneshot- und manuelle Hooks erfassen, solange erreichbar.
  Press → World/Modus wechseln → Release muss ursprüngliche Quelle treffen.
  **Abnahme:** Keine Phantomnote, hängenbleibende Quelle oder verlorene relevante
  Fahne; Hooks melden nur erfolgreiche reale Starts. Entfernte Pfade brauchen
  keine neue Stimmenarchitektur. Voraussetzung: SD00/SD07.
- [x] **SD09 — Globale Belegung auch manuell verbindlich machen.**
  Drei-Slot-Ziel über alle verbleibenden Quellen, Releases und Übergänge; lokale
  Pluck-Grenze zwei zusätzlich. Generate-Gate ist bereits vorhanden. Für Manual
  höchstens eine definierte weiche Übergabe bereits losgelassener Quellen;
  keine held Note hart stehlen, keine unbegrenzte Warteschlange.
  **Abnahme:** Full-pool/Press/Release/Wechsel kann Budget nicht umgehen. Alle
  held Slots voll bedeutet ehrlich abweisen; keine fünfte Quelle allein wegen
  fünf Eingabeflächen versprechen. Voraussetzung: SD07–SD08.
- [x] **SD10 — Manuelle Palette auf World-Quellen vereinheitlichen.**
  Manuell dieselbe ausgewählte Familie und Harmonik nutzen; implizites Pad,
  Bass-follow und konkurrierende Autoplay-Hooks aus diesem Produktweg entfernen.
  **Abnahme:** Ein manueller Einsatz startet genau den angekündigten Tonkörper;
  Loslassen/Stop folgt derselben Besitzerlogik. Kompatibilität erst mit SD36
  entfernen; keine vorweggenommene Tastenbelegung. Voraussetzung: SD00/SD08–SD09.

### 2. Harmonie und drei echte Kompositionssysteme

- [ ] **SD11 — Gemeinsame Harmonik und Quellenregister final konstruieren.**
  Core, modale Farben, gemeinsame Töne, minimale tiefe Abstände, Rollen und
  Voice Leading definieren. Alle sechs bestehenden Modes und Equal/Just prüfen;
  Modes ohne eigenen ruhigen Nutzen entfernen statt als Anzahl verteidigen.
  **Abnahme:** Tatsächliche Frequenzen/Grundtöne und relevante Teiltonkonflikte
  berücksichtigen; keine Registerflucht, tiefe Reibung oder bloßer Parallelakkord.
  Key-Transposition bleibt im akzeptierten Quellenbereich. Voraussetzung: SD02–SD07.
- [ ] **SD12 — Harmoniegedächtnis mit realen Releases/Effekttails verbinden.**
  Quellrelease, Raum, Echo/Blur und gegebenenfalls Shimmer-Oktave konservativ
  erfassen; Timeout/Schwellwert aus hörbarer Energie begründen.
  **Abnahme:** Farbwechsel wartet bei relevanter Kollision, gemeinsamer Ton darf
  verbinden, alte leise Fahne sperrt nicht dauerhaft jeden neuen Ton. Ein
  abgewiesener Vorschlag verbraucht keine gehörte Historie. Voraussetzung: SD08/SD11;
  endgültige Tailwerte nach SD19–SD27 nachführen.
- [x] **SD13 — World-Datenmodell und begrenzte Ereigniszustände bauen.**
  Je Welt kleine Zustandsmaschine, Motivspeicher, eigener Planungszustand/Seed,
  Quellenrollen und Defaults; gemeinsame Zulassung als Transaktion beibehalten.
  **Abnahme:** Vorschlag, Zulassung, Beginn, Ende und Ruhe getrennt; begrenzte
  Suche/retry; unterschiedliche Grammatik statt weiterer Preset-Flags. Keine
  World-ID still wiederverwenden. Voraussetzung: SD09/SD11–SD12.
- [ ] **SD14 — COAST-Grammatik implementieren.**
  Individuelle Stimmenübergaben mit gemeinsamem Ton; optionaler Farbton nur bei
  Platz, keinesfalls obligatorischer dritter Layer. Tonbeziehungen verändern,
  statt dieselbe Fläche in festen Abständen neu zu starten.
  **Abnahme:** Dry-Eventtrace zeigt eigenständige Hüllkurven/überlappende Übergaben,
  sinnvolle Pausen und begrenzte Rollen. Auch ohne Pan/Hall hörbare Kontinuität.
  **Neue Priorität 2026-10-08:** Ein hörbarer Akkord-/Voicing-Bogen mit
  gemeinsamen gehaltenen Tönen, gezieltem Registerwechsel und eigenen Dauern
  wurde als Richtung positiv gehört. Die COAST-Gegenbewegung ist jetzt im
  autonomen Product-Generator mit drei Rollen und acht tatsächlichen Starts
  integriert. Der Mittelton wird einmal gestartet; Slots und unveränderte
  Raumhistory warten echte Releases ab. Alle12 Keys, beide Collections und
  Stimmungen, Host-Suite und H743-Build geprüft. Neue70-s-Firmwareprobe und
  endgültige Quellen-/Gerätewahl bleiben offen; deshalb kein Gesamt-Haken.
  Siehe COAST-Generator-Checkpoint und Ensemble-Direction.
  Voraussetzung: SD02–SD03/SD13; technische Basis ist keine musikalische Abnahme.
- [ ] **SD15 — WOODLAND-Grammatik implementieren.**
  Seed-Figur mit Kontur/Abstandsmerkmalen; Antwort bewahrt ausgewählte Merkmale,
  Variation verändert nur begrenzte Eigenschaften; dann wirkliche Ruhe.
  **Abnahme:** Antworten sind aus gehörten Figuren ableitbar, fehlgeschlagene
  Starts nicht als Motiv gespeichert; kein regelmäßiger Zufallsarp oder
  endloses Einzelton-Ping. Zwei lokale Slots berücksichtigen. Voraussetzung: SD06/SD13.
  **Aktuell nach Nutzerkorrektur:** Die kurze gezupfte Richtung ist verworfen.
  Lange Saite und eigene Sechs-Schritt-Akkordrolle sind jetzt integriert:
  weicher Anfang, gehaltene gemeinsame Töne, individuelle Halteabsichten,
  eigener weicher Release und Wiederkehr erst nach echter Freigabe der Besitzer.
  Default-Dry/Room-Probe50 s,48 echte Engine-Phrasen über Keys/Collections/
  Stimmungen und H743 geprüft. Neue Quellen-/World-Hörwahl und Geräteabnahme
  bleiben offen; Gesamt-Haken folgt daraus noch nicht.
- [ ] **SD16 — HIGHLANDS-Grammatik implementieren.**
  Kurze zusammenhängende Fragmente, variierte Kontur, danach lange offene Pause;
  kein festes hin/her zweier Töne oder dauerhaft gerufener Grundton.
  **Abnahme:** Eigenes Zeitempfinden ohne Naturebene, Pitch-Sweep oder extremen
  Hall; Ruhe wirkt beabsichtigt. Nur implementieren, wenn SD04–SD05 bestehen;
  sonst produktseitig zurückstellen. Voraussetzung: SD13.
- [ ] **SD17 — Langfristige Entwicklung mit Wiedererkennbarkeit bauen.**
  Ereignisse zu Episoden ordnen; Ausgangskandidat 45–120 s, ohne periodischen
  Pflichtreset. Wiederkehr, lokale Variation und Ruhe in musikalischer Reihenfolge;
  Rückkehr erinnert, ohne identischen Loop vorzuspielen.
  **Abnahme:** Mehrere Seeds erzeugen weder erkennbare kurze Endlosschleife noch
  orientierungslosen Wandel, unbegrenzte Dichte oder permanenten Halt. Gedächtnis
  und Suchaufwand bleiben begrenzt. Voraussetzung: SD14–SD16 bzw. ausgewählte Welten.

### 3. Tonkörper, gemeinsamer Raum und sämtliche Effekte

- [x] **SD18 — Body mit/ohne entscheiden und Übergänge korrigieren.**
  Feste 4–6-Moden-Weltmaterialien, additive dry+wet-Wirkung, Amount 0,38,
  Links/Rechts-Versatz und State-Reset beim Worldwechsel isolieren.
  **Abnahme:** Klarer hörbarer Nutzen bei gleicher Lautheit, kein Tube-/Metallpeak,
  Mono-Ausfall oder Tailabriss. Fehlt der Nutzen: Body entfernen. Bei Behalten
  neue Quellenfarben begründen statt alte fünf Materialien blind übernehmen.
  Voraussetzung: SD02–SD06; vor finaler Raum-/Gainabnahme.
- [ ] **SD19 — Einen gemeinsamen Raum als Produktbasis wählen.**
  FDN-Körper, frühe/diffuse Anteile, Dämpfung, Decay, Breite und Tail-Harmonie
  für alle Quellen kalibrieren. Keine zweite Arena; keine unkontrollierte
  Delaylängenänderung, die live Tonhöhe bewegt.
  **Abnahme:** Quelle bleibt ortbar und tragfähig, Raum verbindet alle Welten;
  Mono, Impuls, tiefer/mittlerer Sustain und volle drei Quellen stabil. Init-Fail
  bleibt definierter Dry-Fallback. Voraussetzung: SD18/SD12.
- [x] **SD20 — Dry/Send/Room und Natur voneinander entkoppeln.**
  Alle Source-Sends, Wet-Amp/alte Reverbadapter, Atmosphere/Space und Tone-Pushes
  inventarisieren; einen eindeutigen Parameterbesitzer festlegen.
  **Abnahme:** Raumändern aktiviert kein Wind; Worldload überschreibt keine
  fremden Userwerte in undefinierter Reihenfolge. Drykörper und Master wirken
  über sämtliche Pfade. Livechanges zipperfrei und ohne tail reset. Voraussetzung: SD19.
- [x] **SD21 — Echo als räumliche Antwort prüfen oder entfernen.**
  Echolevel, Feedback, Delay Seconds, Ping-Pong, Zeitänderung und Nachklang prüfen.
  **Abnahme:** Kein rhythmischer Maschinenpuls, unharmonischer alter Ton oder
  zunehmende Rückkopplung; Zeitänderung erzeugt keine Sirene. Sustain und Stille
  bei maximal erlaubten Werten prüfen. Nur beibehalten, wenn Raum allein diesen
  Nutzen nicht erfüllt. Voraussetzung: SD12/SD19–SD20.
- [x] **SD22 — Chorus/Detune gegen Quellenbewegung abwägen.**
  Breite/Modulation, vorhandene Bowed-Verstimmung und Mono-Summe vergleichen.
  **Abnahme:** Direkter Grundton bleibt, kein hohler Swirl oder hörbarer Vibrato-
  Ersatz. Frühere Gainkorrektur nicht als vollständige Klangabnahme werten.
  Wenn kein unabhängiger Nutzen: Produktweg entfernen. Voraussetzung: SD03/SD19.
- [x] **SD23 — Age auf sinnvolle Klangalterung reduzieren.**
  Wow, Flutter, Bandverlust und Sättigung separat sowie gemeinsam prüfen.
  50-Hz-Hum/Bandrauschen sind im aktiven Master bereits entfernt.
  **Abnahme:** Tonzentrum und Dynamik bleiben ruhig, kein Eiern/Piepen/Fizz;
  Idle bleibt still. Redundanz zu Color/Motion/Blur führt zu Merge/Remove.
  Voraussetzung: SD19; keine neue Eigenrauschquelle.
- [x] **SD24 — Blur auf Grundton und musikalischen Nutzen prüfen.**
  Grainüberlappung, langes Eingangssignal, Artikulation, Quellewechsel und
  maximaler Amount; Output-/Send-/Tailerinnerung erfassen.
  **Abnahme:** Kein ausgedünnter Ton, fluktuierendes Loch oder homogenes Nebelrauschen;
  Wortlaut des Nutzens muss sich gegen gemeinsamen Raum abgrenzen lassen.
  Andernfalls entfernen. Voraussetzung: SD12/SD19.
- [x] **SD25 — Shimmer behalten oder vollständig aus Produkt entfernen.**
  Oktavregeneration und Feedback im zugelassenen Register, inklusive alter
  harmonischer Farben, hoher Spektralanteile und Worst-case-Tail.
  **Abnahme:** Kein Glasalarm oder schwebender Fremdton; Tail zählt harmonisch.
  Kein Nutzen ohne hohe Aufmerksamkeit: entfernen, nicht nur Default=0.
  Voraussetzung: SD12/SD19; keine neue Sample-/FFT-Engine als Ersatz.
- [x] **SD26 — Reverse Swell strikt auf Produktnutzen begrenzen.**
  Aktiver synthetischer Vorlauf und Offline-Reverse unterscheiden. Automatische
  Generate-Vorswells bleiben aus; Admission kann ein geplantes Event absagen.
  **Abnahme:** Kein Vorlauf zu einer später abgewiesenen Note, keine Stille-
  Verletzung, keine look-ahead-Latenz im manuellen Spielen. Wenn nicht klar
  erforderlich, aus Produktbefehlen entfernen. Voraussetzung: SD07/SD13/SD19.
- [x] **SD27 — Finale Effektpalette reduzieren und Moduswechsel prüfen.**
  Alle neun Modi auf SD19–SD26 abbilden. Dream Chain ist kein Qualitätsargument;
  benötigte Kombinationen explizit wählen. Modewechsel inklusive Bypass/Fallback
  mit vorhandenen Tails prüfen.
  **Abnahme:** Kein doppelt bearbeiteter Bus, Pegeleinbruch, Tailrevival oder
  ungelöster Effektparameter. Entfernte Modi nicht durch Scene/Worldload
  reaktivierbar. Zero-Amount-/Bypass-CPU und Speicher anhand Code/Map statt
  Annahme bewerten. Voraussetzung: SD00/SD19–SD26, Migration SD36.

### 4. Natur und Geräusche mit Entscheidung zum Weglassen

- [ ] **SD28 — Naturebene rechtfertigen und separat steuern.**
  Für jede World entscheiden: ganz ohne Natur oder eine leise ortgebende Ebene.
  **Abnahme:** Die trockene musikalische Welt ist vorher eigenständig; Nature
  erhöht Ortsgefühl im Vergleich, ohne Töne/Pausen zu maskieren. Kein Wind-
  Default für alle Welten und keine Kopplung an Room. Voraussetzung: SD14–SD16/SD20.
- [ ] **SD29 — Wind nach vorhandenem Irregularitätsumbau hören.**
  Bestehende unabhängige Weather-/Eddy-Zufallsuhren und variable Gust-Zeiten
  erhalten; mehrere Zustände/Seeds getrennt auswählen. Spektrale Homogenität,
  Filterpeaks, Rumpeln und Lautstärkeverlauf prüfen.
  **Abnahme:** Kein regelmäßiges Schwingen, röhrenartiges Pfeifen oder dauerhaft
  gleiches Rauschband. Mehr Zufall alleine ist keine Abnahme. Bei unnötiger
  Präsenz wegnehmen; nur falls SD28 Wind behält.
- [ ] **SD30 — Regen und Wellen auf Ereignisse statt Looptrim prüfen.**
  Nur gewählte Naturquellen: Nah-/Fernanteil, Transienten, Schaum/Prasseln,
  spektraler Schwerpunkt und unabhängige längere Entwicklung.
  **Abnahme:** Kein nervöses Ticken, immer gleicher Schwall, pegelstarker Sweep
  oder verdecktes Sea-Hum. Mono/kleine Ausgabe prüfen; keine WAV-Samplebank.
  Voraussetzung: SD28, sonst Pfad entfernen.
- [ ] **SD31 — Alle übrigen Geräuschwege schließen.**
  Brown-Rumble, Breath, Warm/Air, Fjord/Haze/Desert, Crackle/Hiss/Sea-Hum sowie
  bisherige Texturen: Erreichbarkeit/Idle/Eintrittspunkte und Zweck dokumentieren.
  **Abnahme:** Jeder verbleibende Weg hat eine Orts- oder Artikulationsrolle
  und besteht ohne elektrischem Brumm-/Summcharakter, dominanter Bandresonanz,
  Maskierung oder nervöser Gleichförmigkeit. Sonst entfernen; keine neue Nature
  Library anlegen. Voraussetzung: SD00/SD28.

### 5. Musikalische Einflussbereiche, Pegel und Zustandswechsel

- [x] **SD32 — Vollständiges versioniertes Parameterregister schreiben.**
  Alle Familien aus Tabelle D mit den Pflichtfeldern erfassen, jeden öffentlichen
  Setter/Presetpush/native Slot zuordnen. Defaults nicht zwischen Quelle, Scene
  und World durch Reihenfolge entstehen lassen.
  **Abnahme:** Kein zugänglicher Soundwert ohne Bereich, Besitzer und definierte
  Livewirkung; ungültige Floats halten letzten gültigen Wert. Entfernte Parameter
  sind nicht scheinbar aktive Regler. Voraussetzung: Quellen-/FX-Auswahl.
- [ ] **SD33 — Activity/Color/Room als musikalische Bereiche konstruieren.**
  Je Welt Kennlinie und Extremverhalten; Activity variiert Figur/Übergabe und
  Ruhe, Color bleibt im akzeptierten Tonkörper, Room verändert Entfernung.
  **Abnahme:** Kein Bereich wird Alarm, sinnlos dicht oder leer durch Fehler;
  mittlere/default/äußerste Werte haben klaren musikalischen Nutzen. Lautstärke
  bleibt separat. Physische Zuweisung offen. Voraussetzung: SD17/SD20/SD32.
- [ ] **SD34 — Quellen-/Rollenpegel fest kalibrieren.**
  Bestehende Pluck/Bowed/Horn/Choir/Guembri-Verstärkungsfaktoren und Min/Max-Clamps
  gegen tatsächliches PCM prüfen. Register, Artikulation, Body, Send und Rollen
  gemeinsam berücksichtigen; Hookamplitude von DSP-Gain unterscheiden.
  **Abnahme:** Wechsel/Lagen überraschen nicht durch Lautheit; kleiner Velocity-
  Unterschied bleibt kontrolliert. Feste Gainkurve statt nachträglicher AGC.
  Defaultwelt und extremste erlaubte Summe haben dokumentierte Headroom.
  Voraussetzung: SD18–SD20/SD32.
- [ ] **SD35 — Kritische Parameterkombinationen prüfen.**
  Gezielt: Excitation×Damp, Color×Resonance/Drive, SHAPE×Activity, Room×Feedback,
  Width×Chorus×Bowed-Detune, Age×Blur, Shimmer×Mode/Register. Dazu sämtliche
  erlaubten Einzelgrenzen und sprung-/schnellbewegte Zielwerte.
  **Abnahme:** Keine Bandpeak-Alarme, Gainlöcher, Pitchfehler, Pumpen oder
  versteckte Dauerbelegung. Unbrauchbare Kombination begrenzen/entfernen;
  kein ungeplantes vollständiges kartesisches Sweep. Voraussetzung: SD32–SD34.
- [x] **SD36 — Katalog und Scenes ausdrücklich migrieren.**
  Fünf bestehende Worlds/sechs V2-Familien und SCN5/SCN6 kennen; neue World-IDs
  versionieren, Ersatzzuordnung und verworfene Parameter explizit dokumentieren.
  **Abnahme:** Alte ID bedeutet nicht unbemerkt etwas anderes; Recall konsistent,
  entfernte Layer/FX nicht reaktivierbar, Speicherformatbudget eingehalten.
  Migrationsfixtures mit echten bisherigen Werten. Voraussetzung: SD00/SD13/SD27/SD32.
- [ ] **SD37 — Sämtliche Worldwechsel musikalisch schließen.**
  Bei drei Welten sechs gerichtete Wechsel, einschließlich voller Releases,
  maximalem Raum, schneller mehrfacher Zieländerung und Pending-Ereignis.
  **Abnahme:** Nur jüngstes Ziel geplant; bestehende Besitzer weich beendet,
  reale Belegung und harmonische Fahnen respektiert; kein Body-Resetknacks,
  frozen noise, Hallreset oder wiederkehrender alter Ton. Voraussetzung: SD12/SD27/SD36.
- [x] **SD38 — Autonomen Betrieb als Audiobefehl fertig definieren.**
  Start, Stop, erneuter Start, Einstieg aus dichtem Manual und fehlende zulässige
  Note; keine Taste/UI hier festschreiben. Bereits vorhandenes busy-opening
  pending/retry ohne Pitch-RNG-Verbrauch beibehalten.
  **Abnahme:** Start schafft zum frühestmöglichen musikalisch zulässigen Zeitpunkt
  einen Ton, Stop verhindert neue Pläne, Wiederstart dupliziert keine Besitzer;
  Ruhe bei Ablehnung ist begrenzt diagnostizierbar. Voraussetzung: SD09/SD13/SD37.
- [x] **SD39 — Stillwerden/Clear/Mute für die gesamte Kette bauen.**
  Quellen, Releases, Natur, Dry/Send, sämtliche Delay-/Raumtöne; musikalischen
  Stop von vollständigem Stillwerden trennen.
  **Abnahme:** Stillwerden endet weich innerhalb einer festgelegten, dokumentierten
  Zeit unabhängig von Sustain/Feedback; Mute wirkt sofort nach dem Raum ohne
  Click, kein Tail bei Unmute wiederbelebt. Audio-DMA läuft weiter; keine
  Audio-Unterbrechung als Workaround. Voraussetzung: SD27/SD31/SD38.
- [x] **SD40 — Live-Key/Mode/Tuning und Besitzerwechsel prüfen.**
  Retune während gehaltenem Ton, Release, maximaler Tail und manueller Eingabe;
  festlegen, ob neue Noten umgestellt oder bestehende Stimmen weich übergeben
  werden. Kein unfreiwilliges Portamento/Sirenenretune.
  **Abnahme:** Keine falsche Tonhistorie, Pitchclamps, stuck Note oder Release an
  falsche Quelle; gemeinsame Töne oder Ruhe verbinden. Voraussetzung: SD08/SD11–SD12/SD37.
- [ ] **SD41 — Boot/Recall/Seed ohne ungewollte Ausgabe schließen.**
  Boot still, Scene-Lautstärke/Worldtargets und Seedstart definiert; vorhandene
  held/generative Besitzer nicht blind aus Persistenz wiederherstellen.
  **Abnahme:** Kein Überraschungsstart oder Lautstärkesprung, kaputte/alte Scene
  führt in dokumentierten stillen Fallback; Reset beim laufenden Raum gezielt
  testen. Keine blockierende Speicherung im Audiopfad. Voraussetzung: SD36/SD39–SD40.
- [x] **SD42 — Ganze Ausgabekette gegen harte/hohle Ergebnisse absichern.**
  Pre-/Post-FX-Gain, DC, Limiter/PCM-Konversion, Stereo/Mono, volle Belegung und
  alle Quellen/Tails einschließlich Natur; keine falsche Float-/PCMskalierung.
  **Abnahme:** Keine NaN/Inf, Clipping/Intersample-Überraschung, anhaltender DC oder
  Monoauslöschung. Startziel bei definierter Referenzlautstärke mindestens 6 dB
  digitaler True-Peak-Headroom im Worst-case-Probe; physischer SPL separat.
  Limiter fängt Ausreißer, repariert keine schlechte Gainstruktur. Voraussetzung: SD34–SD41.

### 6. Nachweis, Produktreduktion und Abschluss

- [x] **SD43 — Hör-/Renderverfahren für die ganze Kette festlegen.**
  Firmware korrekt in zulässigen Blöcken (Engine höchstens 512 Frames) rendern;
  andere zulässige Blockgrößen gegen gleiche Seed-/Eventfolge vergleichen.
  **Abnahme:** Keine stille Überschreitung oder Referenztonkontamination, SHA,
  Sample-Rate, Sourcefingerprint, Seed, Parameter, Onsettrace und feste Gainwerte
  vorhanden. Rohe Pegel plus gematchte Hörkopie; kein sampleweises Normalisieren.
  Kurze Vergleiche kompakt halten; vollständige lange Phrasen als solche
  kennzeichnen statt sie vor der musikalischen Auflösung abzuschneiden.
  Bestehende passende Harnesses weiterverwenden.
- [ ] **SD44 — World-Unterschied ohne Labels prüfen.**
  Gleiche Tonart/Stimmung, vergleichbarer Registerbereich, gleiche feste
  Referenzlautheit; Quellen-/Grammatikvergleich zunächst ohne Nature/FX, danach
  gemeinsame Roomfassung. Mehrere Seeds, unbekannte Reihenfolge, keine Namen.
  **Abnahme:** Unterschied wird durch Artikulation, Beziehungen und Zeitverhalten
  beschrieben; Erkennbarkeit beruht nicht nur auf Key/Hall/Dichte. Kurzer Vergleich
  plus längere interne Entwicklung; offenes Hörerurteil ausdrücklich markieren.
  Voraussetzung: SD17/SD35/SD43.
- [ ] **SD45 — Lange reale Audioläufe und Recovery prüfen.**
  Eventtrace UND PCM-Auswertung über mehrere Seeds/Modes/Worldzyklen, einschließlich
  Timerwrap, langen Tails, Start/Stop-Folgen, fehlender Zulassung und Scenewechsel.
  **Abnahme:** Keine eingefrorene Stimme, Endlos-Retry, verlorene Historie,
  unerklärte Dauerstille, kurzer Dauermotifloop, NaN oder zunehmende Energie.
  Aus gewählten Zeitpunkten kurze Hörproben; kein langes WAV an Nutzer.
  Voraussetzung: SD37–SD43.
- [x] **SD46 — Entfernte Produktpfade tatsächlich auslinken.**
  Nach Migration Archiv-/Altquellen, doppelte Raumwege und unbenötigte Buffer
  aus H743-Produktauswahl entfernen; Referenzcode nachvollziehbar behalten.
  **Abnahme:** Map/Callsites bestätigen Wegfall, kein Scene/Hook/Setter stellt
  sie wieder her; Bankdelta ausgewiesen. Keine vermeintliche Einsparung aus
  Ordnernamen oder Amount=0. Voraussetzung: SD00/SD10/SD27/SD31/SD36.
- [ ] **SD47 — Hotpath und Note-on-Arbeit auf Spitzenlast prüfen.**
  Quellenanregung, Parametercoefs, Schedulerretry, Worldvorbereitung und Storage-
  Handoff: keine unbounded Suche/Allocation/I/O. Vorbereiten außerhalb DMA mit
  begrenztem Statewechsel; nicht unbemerkt neue große Doppelbuffer bauen.
  **Abnahme:** Worst-case-Arbeit pro Tick/Block benannt, Output unabhängig von
  Datenrennen und Control-/Audioraten, CPU-/Stackkandidaten für SD48 dokumentiert.
  Voraussetzung: SD17/SD27/SD37/SD46.
- [ ] **SD48 — Echte H743-Zeit-/Stackabnahme durchführen.**
  Nach UX-Integration: DWT inklusive Onset, maximale SHAPE-Releases, native/alte
  Engine-Retirement, Masterraum, schnell bewegte Targets, Display/LED und Storage;
  lange Läufe bei tatsächlicher Samplerate/Blockkonfiguration.
  **Abnahme:** `peak_load < 0.60`, null Deadline-Misses, nachgewiesene Stackreserve
  pro Kontext; keine underruns oder Watchdog-Recovery im Normalbetrieb. Fehlende
  Reserve zuerst durch Reduktion lösen. Erst am Gerät möglich; SD47 + reales Build.
- [ ] **SD49 — Reale Ausgangskette und Hardwarezustände abnehmen.**
  Tatsächliche PCB/BOM zuerst feststellen: verfügbare Ausgänge, Codec/Amp,
  Lautsprecher falls vorhanden, Kopfhörer/Line-Funktion nur soweit gebaut.
  Prüfen: Gain/Impedanz, Stereo/Mono, Netz-/Batteriezustände falls unterstützt,
  DC, Clocking, Einschalt-/Mute-/Ausschaltpop und elektrisches Eigenbrummen.
  **Abnahme:** Akustischer Körper/Headroom bei echter Lautstärke, kein physischer
  Störklang; keine erfundene Hardwareeigenschaft als Prüfergebnis. Nach SD48.
- [ ] **SD50 — Ruhige Wahrnehmung mit echten Hörern abnehmen.**
  Nutzer plus freiwillige geeignete Hörer; sensible Stellen mit Zeitpunkt und
  Quelle/Parameter erfassen: Alarm/Piep, Tube, Buzz/Hum, nervöse Bewegung,
  Ermüdung, Maskierung. Auch leise und reale typische Lautstärke testen.
  **Abnahme:** Konkrete Einwände sind gelöst oder betroffener Bereich entfernt;
  keine durchschnittliche Bewertung kaschiert wiederkehrenden Störklang.
  Ergebnisse sind Hörbefunde, keine Aussage über Neurodivergenz/Heilwirkung.
  Voraussetzung: SD44/SD49; echte Nutzung länger als kurze Vergleichsdatei.
- [ ] **SD51 — Sound-Spezifikation mit exakten Endwerten einfrieren.**
  Quellen/Register/Gains, Grammatikzustände/Timing/Seeds, harmonische Regeln,
  ausgewählte FX/Natur, Parameterkennlinien/Defaults und sämtliche Übergänge
  aus den bestanden Befunden konsolidieren. Kandidatenwerte oben ersetzen.
  **Abnahme:** Ein Versionsstand reproduziert alle Produktwelten; kein ungelöster
  Pflichtparameter, Widerspruch zwischen Docs und Firmware oder stiller
  Legacy-Default. Softwarestand vor UX, Hardwarekorrekturen danach versionieren.
  Voraussetzung: SD00–SD47; endgültiger Lock zusätzlich SD48–SD50.
- [ ] **SD52 — Spätere UI auf unveränderte Audioverträge anbinden.**
  Neue Display-/Button-/Encoderzuordnung darf UIzustände ändern; Start/Stop,
  Worldtargets, Besitzer, Volume und Stillwerden nutzen obige Audiooperationen.
  Human-idle/Displayruhe dürfen keine musikalische Inaktivität simulieren.
  **Abnahme:** UIwechsel/Wake kein Retrigger, Parameterjump oder neuer Klangpfad;
  gleiche musikalischen Eingaben liefern gleiche Audioereignisse. Konkrete
  Tasten-/LED-/15-Minuten-Gestaltung bleibt spätere UX-Aufgabe. Nach SD51-Softwarestand.
- [ ] **SD53 — Sound-Definition-of-Done abschließen.**
  Jede KEEP-Quelle, jedes behaltene FX/Naturelement, jeder Parameterbereich und
  jeder Zustandswechsel hat Entwurf + Software + Hörbefund + nötigen Gerätetest.
  **Abnahme:** Kein offener soundkritischer Befund, kein ungeprüfter erreichbarer
  Legacyweg; entfernte Funktionen sauber migriert, Ressourcen-/Ausgangsreserve
  belegt, exakte Referenzversion und kurze Hörbelege verlinkt. Erst dann
  Sounddesign als Produktstand fertig nennen. Voraussetzung: sämtliche zutreffenden Aufgaben.

### Entwicklungscheckpoints und Merge-Regeln

| Checkpoint | Abgeschlossener Umfang | Integration / nächster Schwerpunkt |
|---|---|---|
| M0 | Aktuelle Transaktionen, autonome Quellenführung, Generate-Belegung, Release-Fixes, WOODLAND-Technikkorrektur und diese Roadmap | PR #129 in seine vorhandene Entwicklungsbasis integrieren; keine Produkt-/Hardwarefreigabe. Danach COAST trocken |
| M1 | Auswahl der trockenen Quellen, lokale Artikulation/Actual-Pitch-Verträge | Kleiner Quellen-Checkpoint; Hörfragen explizit offen, wenn noch unbeantwortet |
| M2 | Gemeinsame Harmonie, vollständiger Besitz/Belegung, manuelle World-Palette | Keine versteckte Begleitung; API-/Scene-Kompatibilität kennt ihre Migration |
| M3 | Ausgewählte World-Grammatiken und versionierter Katalog | Unterschiedliche musikalische Systeme; Gegenprobe bei gleichen Key/Pegeln |
| M4 | Ein Raum, gerechtfertigte kleine FX-Palette, gewählte Naturebene | Sämtliche übrigen erreichbaren FX/Geräusche entfernt oder referenz-only |
| M5 | Parameterbereiche, feste Gains, Übergänge, lange Hostläufe, H743-Ressourcen | Sounddesign softwareseitig spezifiziert; UX/UI kann darauf aufbauen |
| M6 | UX-Anbindung und reale Zeit-/Ausgangs-/Hörabnahme | Sound-Definition-of-Done; erst dieser Stand ist Produktfreigabe-Kandidat |

Nicht auf eine bestimmte Anzahl Aufgaben pro Merge festlegen: integrieren,
wenn ein zusammenhängender Vertrag geprüft ist und keine bekannte Regression
mitgeht. Vor jedem Merge tatsächlichen Head/Diff, Konflikte/Reviewbefunde und
relevante Checks prüfen; erwarteten Head verwenden. Entwicklungsmerge ist keine
Freigabe des gesamten Sounds. Keine automatische Retarget-/Merge-Kette nach
`main`; den konkreten Entwicklungszweig und Checkpoint benennen.

## TEST AM GERÄT

Die Geräteserie beginnt nach Sound-Softwarestand und UX/UI-Integration. Schon
jetzt ist definiert, was sie entscheiden muss:

| Test | Konkrete Durchführung | Bestanden, wenn |
|---|---|---|
| Trockener Körper | Jede gewählte Quelle tief/mittel/hoch, beide Ausgabekanäle und Mono, Attack/Decay/SHAPE-Grenzen | Keine Alarm-/Tube-/Buzz-Stelle; Körper bei realer leiser Wiedergabe vorhanden |
| Worldidentität | Gleiche Tonart/Referenzlautheit, unbekannte Reihenfolge, trockener und Raumdurchlauf | Drei gewählte Welten durch Tonkörper und musikalische Zeit unterscheidbar; schwache dritte wird gestrichen |
| Volle Belegung | Drei Quellen bzw. lokale kleinere Pools, maximal lange Releases; weitere Eingabe und Worldwechsel | Kein Steal-Klick, Overbudget, stuck Note oder unerklärter Pegelsprung |
| Tail-Harmonie | Farbwechsel bei langem Raum/Echo und gegebenenfalls Shimmer, dann Ruhe/Wiederstart | Alte Farben erzeugen keinen hörbaren Konflikt; System kehrt musikalisch zurück |
| Makrogrenzen | Jede Grenze plus die SD35-Risikokombinationen langsam und schnell bewegen | Ganze zugängliche Fläche bleibt musikalisch brauchbar, keine Sirene/Zipper-/NaN-Ausgabe |
| Stillwerden / Boot | Aus maximaler Ausgabe Stop, Clear, Mute/Unmute; Boot/Scene-Recall bei verschiedener gespeicherter Lautstärke | Definierte weiche Ruhe, keine alte Fahne/Note oder Überraschungslautstärke |
| UX-Verkehr | Spielen/Generate mit Display-/LED-/Encoder-/Storagearbeit und Wake | Keine Audioänderung durch idle/wake, kein verlorener Release oder Deadline-Miss |
| Ressourcen | Worst-case-DWT, Stack-Highwater und lange Sessions im endgültigen Build | Zielreserve tatsächlich vorhanden, nicht bloß mittlere CPU oder Linkerplatz |
| Physische Ausgabe | Tatsächliche Ausgänge/Lasten/Stromzustände; Pegel/DC/Noise/Pops und reale Lautstärke | Keine elektrische oder akustische Störursache; digitale Headroom und physischer Pegel separat dokumentiert |
| Langes Zuhören | Mehrere echte Sitzungen/Seeds ohne ständige Eingriffe; gezielte sensible Stellen protokollieren | Wiederkehr schafft Zusammenhang, Entwicklung bleibt ruhig; keine ungelösten störenden Stellen oder Wirkungsbehauptung |

**Aktuelle Einheiten: SD00/SD32/SD36/SD46 — reduzierte Produktauswahl und
explizite Migration; danach Quellenhörproben, Room/Nature-Messung,
langfristige Grammatik und Übergänge.** Quellen- und Gerätehörgates bleiben
offen; Referenzhistorie wird nicht als Kandidaten-Laufzeit beschrieben.
