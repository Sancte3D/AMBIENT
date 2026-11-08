# SD11 — öffentliche Harmonik und Register technisch geprüft

Stand2026-10-18, auf PR174/Commit
`e7c16650c10f466cb602ab2f3b24643a83f014a9`. Genau eine technische Einheit:
die systematische öffentliche Zulassung ergänzt die schon bestehenden echten
All-key-Generatorprüfungen. Kein weiterer Runtimefehler gefunden. Alle263
bestehenden Firmware-/Header-/HAL-/Startup-/Linker-/CMakeinputs bleiben zur
Basis bytegleich. Kein neuer Parameter, State, Buffer oder Klangalgorithmus.

**SD11 ist softwareseitig geprüft und zählt jetzt zur Hörgruppe.** Der
Hauptpunkt bleibt ungecheckt: Quellen-/Teilton- und Registerwahl müssen am
Klang und später an der tatsächlichen Ausgangskette entschieden werden.
24/54 Hauptpunkte geschlossen,30 offen;10 technische Blöcke verbleiben
(8 Software/Prüfung+2 Gerät),19 Hörentscheidungen und1 Gesamtcheck.

## Konkreter Abschluss der technischen Lücke

Die heutigen drei Generatoren besitzen bereits48 echte Phrasenprüfungen je
World über12 Keys×2 Collections×2 Stimmungen, einschließlich tatsächlicher
DSP-Starts, Quelle/Hz, gehaltenem gemeinsamen Ton und Retirement. Die
öffentlichen Note-Eingänge waren dagegen vor allem mit einzelnen Beispielen
geprüft. Die alte sechsmodige Referenz-Leadprüfung ist kein Nachweis des
heutigen Product-Ensembles und wird hier nicht dafür verwendet.

`test_product_harmony.c` verwendet ausschließlich die wirklichen öffentlichen
Engine-APIs und die unveränderten Produktquellen. Keine private `admit`- oder
`compatible`-Funktion als Erwartungswert, kein Ersatz-DSP. Die Kontrolle
berechnet Equal/Just mit double-Genauigkeit, festen Core-Bitsets und einer
deklarierten Liste gesperrter Centbereiche aus dem Vertrag. Der Octave-fold
des Produktcodes wird dafür nicht wiederverwendet.

| Dimension | Vollständig geprüfter Bereich dieser Fixture |
|---|---|
| Familie/World |COAST, WOODLAND, HIGHLANDS |
| Tonart |Alle12 Pitch Classes |
| Collection |Major-Core0/2/4/7/9, Minor-Core0/3/5/7/10 |
| Stimmung |Equal und keybezogenes5-limit Just, einschließlich Töne unterhalb des Key-Roots |
| Öffentliche Eingänge |Manual mit Owner0/1 und World-Note-API mit Owner6/7; letzterer im ausdrücklichen Generate-Modus, ohne autonomen Tick |
| Einzeltonscan |MIDI45..80 chromatisch; innerhalb des öffentlichen Core zusätzlich±2,9/±3,1Cent |
| Paare |Jedes geordnete Paar der gültigen öffentlichen Core-Töne, einschließlich live Unisono und beiden Reihenfolgen |
| Referenzparameter |Dry, Nature0, Attack0, Release1, Color1, Master0,6; kein kartesischer Makrosweep |

## Tatsächliche Ergebnisse

| Prüfung | Ergebnis |
|---|---:|
| Einzeltonfälle |19968 |
| angenommene Einzeltöne |7200 |
| geordnete Paarfälle |20064 |
| angenommene Paare |16128 |
| abgewiesene Paare |3936 |
| vollständig bytegleiches Fortsetzungs-PCM bei abgewiesenen Paaren |8060928 Stereo-Frames |
| kleinste/größte angenommene Eingangsfrequenz im Scan, inklusive±2,9Cent |145,268219 /445,240173Hz |

Insgesamt40032 Fälle. Die unverschobenen `tuning_hz`-Werte entsprechen dem
unabhängigen Equal-/Just-Bezug mit<0,003Cent Rechenabweichung; die zusätzlichen
±2,9Cent sind absichtliche Eingangsprobes innerhalb der3-Cent-Zulassung.
Dies ist **kein** gemessener Fehler der aus PCM geschätzten Grundfrequenz.
Angewandte Hz und echte
DSP-Ack-Hooks/Ledger bleiben für angenommene Quellen exakt erhalten.
Jeder abgewiesene Einzelton bleibt ohne Quelle, Onset oder PCM.

Für jedes erste Paarmitglied wird nach512 echten Frames eine2048-Frame-
Kontrollfortsetzung ohne zweite Anfrage gerendert. Jeder abgewiesene Partner
muss diese ganze Fortsetzung bytegleich erhalten, einschließlich des langen
WOODLAND-Quellenverlaufs. Ein Ablehnungszähler darf sich ändern; das ist kein
gehörtes Ereignis und kein Eingriff in die klingende Quelle. Bei angenommenen
Paaren werden zwei echte Besitzer/Hz/Acks geprüft. Kein Nonfinite/Limiterfall.

C11/O2/Wall/Wextra/Werror sowie ASan/UBSan bestanden, dieselbe native und
sanitisierte Ergebnismetrik. Der normale Hostrunner führt die Fixture ohne
historische Gitobjekte aus; der eigene aktuelle CI-Job prüft zusätzlich alle
263 bestehenden Kompilierinputs gegen die exakte Basis.

## SD11-Kriterienmatrix und klare Zuständigkeit

| Kriterium | Technischer Nachweis | Verbleibende Entscheidung |
|---|---|---|
| Core und alte Modes |Product hat ausdrücklich2 Collections; `engine_set_mode` nimmt nur0/1 an, alte Scene-IDs werden migriert. Die alten6 Modi sind kein versteckter Produktumfang |Musikalischer Nutzen der2 Cores hören |
| Tatsächliche Frequenzen und Stimmung |Neue unabhängige Equal/Just-/Zulassungsmatrix; bereits vorhandene echte Generator-Acks und Hz-/Besitzprüfung |Physischer Tonkörper und wahrgenommene Stimmung unter Ausgangslast |
| Tiefe Abstände und Kollisionen |Alle geordneten öffentlichen Paare prüfen gesperrte100±45-/600±45-Cent-Klassen, live Unisoni/nahe Varianten und die249,5-/299,5-Cent-Untergrenzen am tatsächlichen Hz-Wert |Keine universelle Rauigkeits- oder Konsonanzfreigabe daraus |
| Transposition/Registerflucht |Öffentlich weiterhin MIDI50..69/140..470Hz; Scan45..80 öffnet keinen privaten Bypass. Interne COAST45..80/105..850, WOODLAND45..68/105..470, HIGHLANDS45..75/105..650 durch vorhandene echte All-key-Phrasen geprüft |Tragfähigen gehörten Quellenbereich final wählen |
| Rollen/Stimmführung |Gegenläufige COAST-Außenstimmen/ein Ziel, lange WOODLAND-Akkordrollen und HIGHLANDS gemeinsame Stimme/echte Pause bereits integriert und geprüft |Form, Eigenständigkeit und Übergänge hören, SD14–SD17/SD37 |
| Relevante Teiltonkonflikte |Bestehende isolierte Komponenten-/Mehrstimmigkeitsbelege unten; keine generelle neue Obertongrenze oder zusätzliche Pitch-Verbote aus einer Spektralzahl |Explizites Quellen-/Teilton-Hörgate SD02/SD04/SD06 und Registerhörwahl SD11 |

Die Anzahl10 technischer Blöcke folgt dieser tatsächlichen Zuständigkeit.
Ein ungechecktes SD11-Hörgate wird nicht weiterhin als offene Programmierung
gezählt. Ein späterer konkreter Hörbefund kann eine neue technische Korrektur
begründen; die jetzige Prüfung erfindet dafür keine pauschale Regel.

## Teiltonbelege am richtigen Quellenstand

Die Dateien wurden mit den aktuellen Quellenhashes abgeglichen:

- `bowed.c` und `horn.c` sind exakt wie in
  [SD02-Komponenten](SD02_COAST_COMPONENT_CHECKPOINT.md),
  [SD03-Mehrstimmigkeit](SD03_COAST_INDEPENDENCE_CHECKPOINT.md) und
  [SD04-Quellenvergleich](SD04_HIGHLANDS_COAST_CHECKPOINT.md).
  Dort sind reale Grundton-/Obertonfenster und isolierte Zusätze dokumentiert.
  Beispielsweise liegt COASTs1,5f-Band bei D3/D4/A4 im vollständigen colour0-
  Körper etwa59..62dB unter dem Grundton; die isolierte Gegenprobe schreibt
  diesen schmalen Rest hauptsächlich Grain zu.2f ist dagegen ein wesentlicher
  harmonischer Bestandteil. Keine eigenständige laute Quinte allein aus dem
  Namen „Resonator1,5f“ ableiten.
- `pluck.c` ist exakt wie im aktuellen
  [langen WOODLAND-Colornachweis](SD33_WOODLAND_COLOR_CHECKPOINT.md).
  Die alten kurzen SD02/SD04-Pluckhashes sind verschieden und werden ausdrücklich
  nicht als heutige lange Saite ausgegeben. Gehaltene Grundton-/Farb-/Ausklang-
  messungen der langen Quelle, Generatorrollen und tatsächlicher Besitzer gelten.

Diese Belege berücksichtigen reale Teiltonenergie, ersetzen aber keine
Wahrnehmungsentscheidung für jeden zugelassenen Akkord. Harmonische2f/3f-
Überlagerung oder ein geringer Schwebungsabstand ist allein kein begründeter
neuer Verbotsgrund. Die gewünschte harmonische Ensemblebewegung bleibt erhalten.

## Reproduktion und nächste einzelne Einheit

Im `firmware-c-next`:
`python3 tools/review_product_harmony.py build-product-harmony --sanitize`.
Daten:[SD11_PUBLIC_HARMONY_METRICS.json](SD11_PUBLIC_HARMONY_METRICS.json).
Kein neues Hör-WAV, ARM-Zeit-/Stackversprechen oder Hardwaretest aus dieser
Einheit. Bestehender H743-Link und neue aktuelle CI bleiben getrennte Nachweise.

Als nächster unabhängiger technischer Block folgt **SD12**: tatsächliche
Source-Releases/Room-Hz-Historie und deren technische Restkriterien lesen;
vorhandene Ruhe-/Timeout-/Hörrelevanzbelege verwenden, keine bereits grüne
SD11-Matrix nochmals als neue Aufgabe vorbereiten. Auch dort softwareseitige
Erfüllung von einer gehörten Tail-Schwelle unterscheiden.
