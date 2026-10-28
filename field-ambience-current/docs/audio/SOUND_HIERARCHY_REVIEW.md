# AMBIENT — Hierarchieprüfung von Cells, Effekten und Nature

Stand 28.10.2026, tatsächliche Firmwarebasis
`457bba1c4f4f9f381acab8fa84cc79264c71c128` / PR193.
Auftrag: Hierarchie prüfen. Das PCB steht nach Michalis' Klarstellung noch
nicht fest. BOM und bisheriger Messplan sind Entwurfsstände, keine bestätigte
Bestückung. Diese Prüfung benötigt keine festgelegte analoge Ausgangskette.

## Ergebnis

Die direkten Cell-Tonkörper bleiben bei den geprüften Standardwerten vor dem
Musik-Rückweg. Das erfüllt noch keine konsistente Hierarchie über alle Regler
und Spielstärken. Konkrete offene Punkte: Nature bei leisen/ausklingenden
Cells, abweichende Room-Kennlinien, Farbwirkung abhängig vom Raummodell und
unsichtbare Tonzulassung durch stummen Freeze-Inhalt. Keine DSP-Werte geändert,
keine Encoderzuordnung festgelegt und kein offenes Hör-/Gerätegate geschlossen.

## Tatsächliche Abhängigkeiten

| Ebene | Tatsächliche Rolle | Hierarchiebefund |
|---|---|---|
| Cell / World / Ausdruck | Direkter V2-Tonkörper; gleicher Klangpfad für Manual/Generate | Primärquelle. Effekt- und Natureziele verändern in der Matrix weder ihren Fingerprint noch ihre bestätigten Start-/Stopereignisse. |
| Room | Gemeinsamer Rückweg; skaliert auch Delay/Grain und verändert zugleich den Raumdecay | Übergeordnetes FX-Makro, kein unabhängiger Hallanteil. Bei Room0 wird der ganze Musik-Rückweg nach dem Fade kalt. |
| Delay / Grain | Zusätze im gemeinsamen Rückweg, eigener Anteil und Zeitparameter | Dem Room-Makro untergeordnet. Höherer Einzelanteil kann räumliche Vordergrundwirkung übernehmen. |
| Drive / Crush / Phaser | Färbung des SF-Rückwegs | Bearbeiten Plate/Delay/Grain. Der separate FDN-Hallrückweg umgeht sie. Keine gleichrangigen Mastereffekte. |
| Freeze | Stoppt Grain-Neuschreiben; schützt zurückgehaltene Tonhöhen | Zustandsmodus mit Einfluss auf Spielbarkeit, kein weiterer Mixregler. |
| Nature | Separater additiver Wetterbus nach Musik-FX, vor Master/DC/PCM | Room-unabhängig und default0. Statische Begrenzung, keine automatische Absenkung gegenüber Cells. |
| Master / Mute / Clear | Gesamtausgabe | Gemeinsame letzte Instanz, keine Nature-/Hall-Ausnahme. Vorhandener Vertrag bleibt unverändert. |

## Gemessene Pegelhierarchie

D3, eine physisch gehaltene Cell von2 bis26s, Seed1234, neutrales
Color/Attack/Release, Plate, Room0,5. Ausgewertet6–24s, kein Dateigain.
Effektwerte Standard. **Nature hier absichtlich100%; sein Bootdefault bleibt0.**
Normal=.7 Engine-Velocity. Leise ist die aus dem echten `cells.h` kompilierte
Hall-Eingabeuntergrenze0,227272734 (=CELL_AMP_MIN/CELL_AMP_MAX), kein hypothetischer
fast-null MIDIwert. Werte sind RMS-Abstände zum direkten Tonkörper; negativer
bedeutet darunter. Die letzte Spalte ist das lauteste relative1-s-Fenster.

| World | Musik-Rückweg, normal | Nature, normal | Nature, leise | Nature, leise, schlechtestes1s |
|---|---:|---:|---:|---:|
| COAST | -16.85 dB | -29.72 dB | -19.95 dB | -15.68 dB |
| WOODLAND | -14.46 dB | -29.43 dB | -19.66 dB | -2.64 dB |
| HIGHLANDS | -19.64 dB | -39.84 dB | -30.07 dB | -22.62 dB |

Die Mittelwerte können das späte Ausklingen verdecken. WOODLAND ist ein einmal
angeregter Körper und wird auch bei weiter gedrückter Cell natürlich leiser.
Im23.–24.-s-Fenster liegt dessen leiser direkter Ton bei−72,37dBFS vor Master,
Nature bei−75,01dBFS: nur2,64dB Abstand. Beide sind absolut sehr leise;
das beweist keine gehörte Maskierung, widerlegt aber eine universale20-dB-
Unterordnung. Bereits bei Nature35% beträgt der schlechteste Abstand derselben
leisen WOODLAND-Cell nur11,77dB. Die bisherigen−24dB im Mittel/−20dB pro1s
waren Bedingungen eines Zwei-Cell-Tests und werden hier nur diagnostisch benutzt.

**Folgerung:** Eine klare Produktregel muss zwischen Cell-Einsatz/tragender
Phase und natürlichem Ausklingen/Pausen unterscheiden. Unveränderte Natur in
Pausen kann gewollt sein. Ein Wetterducker, der jeder Fahne bis Null folgt,
wäre keine automatisch gute Lösung und ist hier nicht implementiert.

## Kennlinien und Rollen sind noch nicht einheitlich

Reiner Raum, D3/.7/Seed1234, Natur100%, jeweils derselbe6–24s-Zeitraum:

| World / Modell | Room0,25 | Room0,5 | Room0,75 | Room1 |
|---|---:|---:|---:|---:|
| COAST / Plate | -21.06 dB | -17.16 dB | -16.11 dB | -15.83 dB |
| COAST / FDN | -24.31 dB | -16.26 dB | -11.15 dB | -7.76 dB |
| WOODLAND / Plate | -19.48 dB | -14.51 dB | -12.10 dB | -10.55 dB |
| WOODLAND / FDN | -22.84 dB | -14.74 dB | -9.63 dB | -6.29 dB |
| HIGHLANDS / Plate | -23.67 dB | -19.95 dB | -19.50 dB | -20.15 dB |
| HIGHLANDS / FDN | -24.49 dB | -16.33 dB | -11.07 dB | -7.51 dB |

Beim HIGHLANDS-Raummaximum ist der FDN-Rückweg12,64dB stärker als Plate.
Der Plate wird dort von0,75 auf1 um0,65dB schwächer im gemessenen RMS,
obwohl der nominale Decay länger wird. Room steuert beides; längerer Decay
ist kein Beleg für monoton größere Entfernung oder einheitliche Stärke.

Drei vollständige40-s-Paare zeigen: Im FDN-Modell, Delay/Grain0, bleibt
die gesamte PCM-Ausgabe bei Drive/Crush/Phaser0 gegenüber allen drei auf1
**bytegleich**. Der FDN wird separat zugemischt; die Farben bearbeiten nur
den anschließend addierten SF-Rückweg.
Das ist eine reale Pfadabhängigkeit, kein defekter Setter. Vor Bedienelementen
muss klar sein, ob Farbe bewusst nur die zusätzliche Textur oder den ganzen
Raum formen soll.

An geprüften Endpunkten erreicht der gesamte Musik-Rückweg im Mittel bis
−4,80dB gegenüber Dry (WOODLAND/FDN/Delay1/Room1). Mit eingefrorenem Grain
liegt er beim späten WOODLAND-Ton in einem1-s-Fenster bis+7,17dB darüber.
Freeze kann die Fläche also bewusst übernehmen. Solche Bereiche sind eine
Sonderfunktion und keine pauschal leise Hintergrundveredelung. Da Drive/Crush/
Phaser ebenfalls Pegel/Form verändern, ist „alles1“ nicht der lauteste Fall.

## Zustands- und Quellenpriorität

Echte Engine-Zulassung: COAST/HIGHLANDS drei, WOODLAND zwei Quellen bei vier
angefragten kompatiblen Tönen. Manual ist während Generate gesperrt. Diese
Grenzen funktionieren; sie liefern noch keine Erklärung für abgewiesene
Tastendrücke. Bestehende Quellen werden nicht zugunsten eines neuen Eingangs
blind überschrieben.

Stummer Freeze-Fall, nativ und ASan/UBSan reproduziert: D3 spielen, nach2s
lösen, alle zusätzlichen FX-Anteile0, FDN/Room0,5. Nach weiteren60s sind
aktive Quellen0 und die letzte PCM-Sekunde exakt0. Ohne Freeze wird E3 nach
Keywechsel zu E akzeptiert. Mit Freeze bleibt eine D3-History reserviert und
E3 wird abgewiesen. Auch bei Grain0 darf die eingefrorene Tonhöhe später
wieder hörbar werden; die Sperre schützt diese Harmonie. **Die History einfach
freizugeben wäre keine korrekte Reparatur.** Offen ist eine verständliche
Freeze-/Verwerfen-/Rückmeldungspolitik. Der neue Test dokumentiert den Zustand,
er erklärt ihn nicht zur akzeptierten Bedienung.

## Sinnvolle Produkthierarchie vor Encoderzuordnung

1. World-Tonkörper, Note und Ausdruck tragen die Identität.
2. Ein musikalisch konsistenter Room-Bereich bestimmt den Raum.
3. Delay/Grain ergänzen ihn; starke Textur/Freeze haben einen bewussten Modus.
4. Drive/Crush/Phaser sind nachgeordnete Färbungen mit ausdrücklich festgelegtem
   Wirkungsbereich. Der technische Ausbau verpflichtet nicht zu drei Hauptreglern.
5. Nature bleibt eine eigenständige leise Kulisse; seine Beziehung zu einer
   tragenden Cell und zu einer verklungenen Note braucht getrennte Kriterien.

Das ist eine aus den Befunden abgeleitete Empfehlung, keine bereits freigegebene
UX oder Encoderbelegung. Keine vierte World als Lösung dieser Hierarchiefragen.

## Umfang, Nachweis und Grenzen

- 180 unterschiedliche40-s-Fälle,201 tatsächliche Captures/134min gerendertes
 Audio; drei Worlds, MIDI50/57/62, zwei Spielstärken, fünf Room-Werte, beide
 Räume, Nature0/0,35/1, einzelne und kombinierte FX-Endpunkte, Freeze und
 Maximalbelegung. Seed42 nur für ausgewählte D3-Fälle, sonst1234.
- Je neun ganze40-s-Paare Original/Messwrapper und37/512, drei ganze nativ/
 ASan-UBSan-Paare und drei FDN-Farbpaare:42.336.000 Stereo-Frames tatsächlich
 byteweise verglichen. Messwrapper ruft ausschließlich originale Busfunktionen.
- NaN-/Inf- und Limiterzähler in allen201 Captures0; höchste PCM-Sample-Spitze
 0,3300171. Das ist keine Garantie für alle möglichen Parameterkombinationen.
- Native und ASan/UBSan für Quellenlimit/Generate und stummen Freeze. Der
 sanitierte Vergleich rendert das Original, der Beobachter wird nativ mit
 dem Original byteweise abgeglichen. Kein behaupteter sanitierter Busbeobachter.
- 278 aktuelle eigene Runtime-/Header-/HAL-/Startup-/Linker-/CMakeinputs und
 zehn FX-Grenzen passen weiter zum unveränderten Kandidatenregister.
- [Vollständige Werte samt1-s-Verläufen und Quellhashes](SOUND_HIERARCHY_METRICS.json).

Die per Bus gemessenen Float-Differenzen werden mit demselben DC-Koeffizienten
wie die Endstufe ausgewertet, vor gemeinsamer Volume-/Gate-/PCM-Stufe. Sie
sind keine wahrnehmungsgewichtete Lautheit, kein Hörtest und kein Beweis für
Verdeckung, Stereo-Identität oder eine fertige World-Unterscheidung. Mono-
RMS-Verhältnisse stehen in den Rohwerten, ersetzen ebenfalls keine Mono-Hörwahl.
Parameter-Sweeps sind eine gezielte Matrix, keine vollständige Kombination
aller Werte, Keys, Collections, Attack/Release/Color oder Langzeitentwicklung.
Das unfertige PCB blockiert diese Softwarebefunde nicht; reale Messungen an
Clock/DAC/Lasten/Stack/IRQ folgen nach Festlegung und Aufbau des Boards.

Reproduktion aus `firmware-c-next` (Host-C-Compiler und Python3):

```sh
python3 tools/review_sound_hierarchy.py build-sound-hierarchy
python3 tools/freeze_sound_candidate.py ../docs/audio/SOUND_CANDIDATE_INPUTS.json --check
```

278 Runtimeinputs unverändert; neue C-Datei ist ausschließlich ein Hostwerkzeug,
kein CMake-/Firmware-Eintrag. Hörgates SD19/28–35/44/50/51 sowie die bisherigen
Geräte-/Abschlussgates bleiben offen. Ein grüner Test bedeutet hier, dass die
Befunde reproduzierbar und die verglichenen Pfade unverändert sind.
