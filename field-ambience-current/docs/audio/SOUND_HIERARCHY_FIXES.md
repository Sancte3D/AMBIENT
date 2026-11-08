# AMBIENT — vier Hierarchieschwächen behoben, Software0.12

Stand28.10.2026. Auftrag „Fixe alles davon“, auf PR194/254ee97.
Die [Vorher-Prüfung](SOUND_HIERARCHY_REVIEW.md) und deren Messwerte bleiben
als Historie erhalten. Trockene V2-Tonkörper, natürliche WOODLAND-Schwingung,
Stimmenbudget und Encoderzuordnung bleiben erhalten. PCB noch nicht festgelegt.

## Ergebnis

Alle vier konkret benannten Softwarepunkte sind umgesetzt und mit echten
DSP-Ausgaben geprüft: Nature bleibt auch bei leisen Cells zurück, beide Räume
folgen einer vergleichbaren Pegelkurve, Farben wirken auf beide Räume und
Grain0 hält keine verborgene Freeze-Sperre. Hörfreigabe und Geräteabnahme werden
damit nicht vorweggenommen. Die normalen Ausgangs- und Kandidatenflags bleiben
wie vorher; die neue Regel gehört ausschließlich zum Full-Sound-Kandidaten.

| Befund | Implementierte Korrektur |
|---|---|
| Nature war bei leiser WOODLAND-Fahne nur2,64dB darunter | Eigene Pegelregel reduziert nur den Wetter-Rückweg, Ziel etwa28dB unter Dry. Nach tatsächlicher Ruhe langsame Erholung. Überlebende Freeze-/Raumklänge bleiben Referenz, wenn die direkten Quellen weg sind. |
| Gleicher Room-Wert ergab bis12,64dB andere Stärke | Beide Tanks erhalten dieselbe begrenzte Rückwegkalibrierung und denselben Amount. Nominal0,25*Room gegenüber Dry; die Klangstruktur der Tanks bleibt verschieden. |
| Farben umgingen den FDN-Hall | FDN wird vor der gemeinsamen Drive-/Crush-/Phaser-Stufe mit Plate/Delay/Grain zusammengeführt. Jede Färbung wirkt einzeln auf beide Hallmodelle. |
| Unhörbarer Freeze/Grain0 blockierte E3 | Grain unter einem Q16-Schritt deaktiviert Freeze bereits in der Main-Transaktion. Leerer Capture schützt keine erfundene Tonhöhe. Tatsächlich eingefrorenes Material behält seinen Harmonieschutz; echte Ablehnungen bekommen eine sichtbare Ursache. |

## Nature im Vordergrundvergleich

Gleiche D3-Cell, dieselbe leise Eingabeuntergrenze0,227272734, Room0,5,
Plate, Nature100%, Seed1234, neutrale Sourceparameter. Schlechtestes1-s-Fenster
im6–24s-Bereich. Keine Hörnormalisierung und kein Quellen-Gain.

| World | Vorher unter Dry | Jetzt unter Dry |
|---|---:|---:|
| COAST | -15.68 dB | -28.05 dB |
| WOODLAND | -2.64 dB | -28.18 dB |
| HIGHLANDS | -22.62 dB | -28.29 dB |

Über alle180 Fälle bleibt das lauteste relative1-s-Naturefenster mindestens
27,62dB unter dem direkten Cell-RMS; der Mittelwert bleibt ebenfalls unter
der geforderten−24-dB-Grenze. Die alte schwache WOODLAND-Fahne wird dafür
nicht angehoben oder gekürzt. In den geprüften letzten Freeze-Sekunden ohne
aktiven Dry-Ton liegt Nature auch unter dem überlebenden FX-Rückweg.

Technisch:20-ms-AC-Leistungsmessung, Ziel etwa−28dB,10-ms-Absenkung,
2-s-Erholung, ruhige Rückkehr nach Hysterese. Zuerst der direkte Tonkörper;
unter dessen Floor überlebendes Musik-Audio. Nur Wetter wird reduziert.
Die Grenzpegel und der prozedurale Wetterverlauf/RNG bleiben unverändert.
Ein separates18-s-Floatfixture prüft leise Direktquelle sowie alleinigen
Musik-/Freeze-Rückweg, komplette37/512-Framegleichheit und Rückkehr auf
95,9–98,9% des Solo-RMS im16–18s-Fenster nach Stille ab10s.

Die Nature-Solodatei bleibt bytegleich zur bisherigen Wetterimplementierung;
unter Musik ist strikte Additivität absichtlich durch die Pegelregel ersetzt.
Die vorhandene Nature-Regression prüft deshalb weiterhin unveränderte Solo-
Wetter- und OFF-Profil-Ausgabe, gleiche tatsächliche Cell-/Generate-Noten und
jetzt die reduzierte Rückgabe. Keine heimliche Normalisierung der Musik.

## Ein Room-Bereich für beide Modelle

Der reine Raum-Rückweg folgt nominal−24,08/−18,06/−14,54/−12,04dB gegenüber
Dry bei Room0,25/0,5/0,75/1. In allen zwölf geprüften World-/Room-Paaren
steigt der gemessene Raumanteil monoton; der größte Unterschied zwischen
Plate und FDN beträgt0,365dB, zuvor bis12,64dB. Das gilt für die gemessenen
D3-Fälle mit neutraler Farbe, nicht als universale psychoakustische Gleichheit.

250-ms-Energiebeobachtung,500-ms-Gainrampe, Gain0,125..8. Nur der Raumpfad
wird kalibriert; Delay/Grain behalten ihre eigenen Anteile und Zeiten. Wenn
kein neuer Eingang anliegt, bleibt der zuletzt gewählte Gain für die Fahne
stehen. Keine Quellen-/Master-AGC und keine Änderung des Tankfeedbacks oder
seines1,2+3,6*r*r-s-Vertrags. Beide neuen Pegelregeln berechnen je ein sqrtf
nur alle64 echten Samples, unabhängig von der Audioblockgröße.

## Jede Farbe bearbeitet beide Räume

Die gesonderte native und ASan/UBSan-Prüfung setzt Delay/Grain0 und verändert
jeweils nur eine Farbe. Relativer RMS-Signalunterschied gegenüber neutral:

| Modell | Drive1 | Crush1 | Phaser1 |
|---|---:|---:|---:|
| FDN | 8,57% | 32,62% | 20,07% |
| Plate | 11,59% | 19,19% | 17,50% |

Das misst tatsächliche Wellenformänderung, keine Prozentzahl für gehörte
Qualität oder Lautheit. Die trockenen Cells umgehen weiterhin diese Farben.
Die ganze FDN-Ausgabe mit Farben0 gegenüber allen drei Farben1 ist jetzt
in allen drei Worlds verschieden; die alte unbemerkte Bypassabhängigkeit entfällt.

## Freeze mit ehrlichem Zustand und Rückmeldung

Die Normalize-/Get-/Apply-Transaktion setzt Freeze bei effektivem Grain0 auf
false. Audio schreibt den Grainring wieder mit frischem Eingang; beim Wieder-
aufdrehen erscheint kein dauerhaft konservierter alter D3-Capture. Bis alte
hörbare Tails weg sind, gilt weiter der konservative14-s-Harmoniehorizont.
Leere Aufnahme wird anhand des realen Rings erkannt, nicht nur anhand eines
aktivierten Schalters. Die feste Inhaltsgrenze1e−7 liegt im Grainpfad deutlich
unter einem PCM16-Schritt; sie gilt nicht als Geräte-Hörschwelle.

Der ursprüngliche60-s-Fehlerfall akzeptiert E3 nach dem Keywechsel jetzt mit
und ohne angefragtes Freeze bei Grain0. Positiver Grain-Capture schützt D3
weiter und weist E3 korrekt mit ENGINE_NOTE_FROZEN_PITCH ab. Nullsetzen,
Abklingen und Wiederöffnen sind geprüft. Das Pitchledger wird nicht pauschal
entfernt. Ein noch laufender Besitzer beziehungsweise sein Release bekommt
den passenden Grund, statt fälschlich „alle Stimmen voll“ zu behaupten.

Im echten H743-Eingabepfad zeigen abgewiesene physische Noten:

- FREEZE / CLEAR TO PLAY für eingefrorene inkompatible History;
- NOTES / VOICES FULL für die echte Belegungsgrenze;
- HARMONY / NOTE ACTIVE oder TAIL ACTIVE für geschützte aktuelle Töne/Fahnen.

Die zusätzliche Fixture betätigt die real extrahierten Cell-/Clear-Routen:
Ablehnung erzeugt Overlay, Clear verwirft den Capture über das vorhandene
40-ms-Gate, nächste Cell spielt. Keine neue Encoderzuordnung. Ein Overlay
ist keine Behauptung, dass das Board bereits gebaut oder gehört wurde.

## Nachweise und Reichweite

- [Aktuelle180-Fälle-Matrix](SOUND_HIERARCHY_FIXED_METRICS.json):201 vollständige40-s-Captures;
  Original/Observer und37/512 bytegleich, nativ/ASan-UBSan und FDN-Farben geprüft,
  insgesamt42.336.000 Stereo-Frames in24 ganzen Paaren verglichen.
- [Direkter Vorher/Nachher-Vergleich](HIERARCHY_PAIRED_METRICS.json): auch der ganze96-s-
  Generate-Notentrace ist bytegleich zum Parent, der FX-Klang bewusst verändert.
  Direkte Source-Fingerprints aller180 Fälle exakt wie vor den Korrekturen;
  unveränderte tatsächliche Noten-/Ownerereignisse. Keine andere World als Tarnung.
- [Sound-Regression](HIERARCHY_SOUND_METRICS.json):14 neue/fortgesetzte Prüfblöcke
  nativ/sanitisiert, einschließlich18min echter Musik pro Modus über zwölf
  World-/Key-/Collection-/ET-JI-Fälle,48 laute Endpunkte, langer WOODLAND-Verlauf,
  Freeze/Clear/Loops,45-s-Effektwechsel und Parameterinvalidität.
- [Nature-Regressionswerte](HIERARCHY_NATURE_METRICS.json): eigenständige Wetter-
  und Publikationsprüfungen, komplette Engine-Traces/Partitionen und alte
  OFF-/Solo-Ausgaben erhalten; keine behauptete unveränderte volle Musikmischung.
- [Session/Messmodus](HIERARCHY_SESSION_METRICS.json):1104 Stromabbrüche und
  27.041.631 Modellchecks je Modus; neun echte12-s-Captures je Modus, Save/kein
  Save und37/512 bytegleich, Main-Planertraces unverändert.
- Echter Headerkontext des H743-Einstiegs wird zusätzlich für Reference,
  Basic Product und Full-Sound separat kompiliert. Die Fixture-eigenen Includes
  können dadurch keine fehlende Abhängigkeit mehr verdecken.
- [Tatsächlicher H743-Bedienfixture](HIERARCHY_UI_METRICS.json) nativ/sanitisiert:126 vollständige12-s-
  Wakevergleiche je Modus, dazu neue Ablehnungs-/Clearprüfung sowie frühere
  Scene-Releases, SHIFT, REC, Natureeditor und physische Entfaltung.
- Voller Hostrunner und Hotpath-Lint bestehen. Der Guard erfasst jetzt auch
  die beiden privaten FX-/Nature-Header; keine Allokation oder blockierenden Calls.
- Aktuelles278-Input-/Zehn-Ziel-Register stimmt. SF-State109948B, Delay262144B,
  zusätzlicher Nature-Pegelzustand56B; reales ARM-Link-/Frameaudit separat in CI.

Die Matrix deckt ausgewählte Noten, zwei Spielstärken, zwei Seeds, beide Räume,
Room-Stufen und FX-Enden ab; keine vollständige Kombination aller Keys/Color/
Attack/Release-Werte. RMS ist keine subjektive Lautheit oder Verdeckung.
Die neue0.12-Kalibrierung verändert den FX-Klang gespeicherter numerischer
Scenes bewusst; SCN8/JSS1/FXP1-Layout und Migration bleiben unverändert.
Frühere Hördateien bleiben historische Fassungen. Finaler Ambient-/Effekt-
Hörvergleich und Messung am künftigen PCB stehen weiterhin aus.

Reproduktion aus `firmware-c-next`:

```sh
python3 tools/review_sound_hierarchy.py build-hierarchy
python3 tools/review_sound_finish.py build-hierarchy-sound
python3 tools/review_nature_accents.py build-hierarchy-nature
python3 tools/review_sound_session.py build-hierarchy-session --device-bench --sanitize
python3 tools/review_ui_idle.py build-hierarchy-ui --fixture-only --finish-candidate --sanitize
python3 tools/freeze_sound_candidate.py ../docs/audio/SOUND_CANDIDATE_INPUTS.json --check
```

Default weiter Reference/Kandidatenflags OFF. Kein Merge, Retarget oder
physisches Flashen. PCB noch offen, Encoder zuletzt. Die ursprünglichen
Hör-/Geräte-/Gesamtgates werden nicht aus diesen Softwarewerten abgehakt.
