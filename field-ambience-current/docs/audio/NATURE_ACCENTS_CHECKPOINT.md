# AMBIENT — leise Naturkulissen, 27.10.2026

Auf Wunsch von Michalis ergänzt der vollständige Soundkandidat eine optionale
prozedurale Hintergrundebene: Strandwellen, verschiedene Brisen sowie leiser
Regen mit seltenem fernem Donner. Basis ist der gesicherte Soundstand PR187,
Commit `8381a1dc8ff85cac71d6b4941b94a6ef82d22e17`, Tree
`da696d3a37723ac6323883594bface6f488dee3f`. Dessen aktueller Hostjob und
H743-Build einschließlich Soundkandidat bestehen in CI756/40420032616.
Die Naturergänzung braucht ihren eigenen neuen ARM-Nachweis; der bisherige
Build ist keine Freigabe des neuen Codes.

## Bedienung

Der vorhandene Menüparameter **Nature** bleibt der einzige Regler. 0 % ist
aus und weiterhin der Startwert. Positive Werte öffnen die Ebene auch ohne
eine gespielte Note. Die Hörprobe verwendet 65 %, keine neue Voreinstellung.
Die bereits vorhandene Nature-Speicherung bleibt im bestehenden Scene-Feld;
kein neues Format, Menü oder Encoderlayout.

| World | Positive Nature |
|---|---|
| COAST | Sanft anschwellende Strandbrandung, zurückweichender Schaum und leise Brisen. Wellen dauern ungefähr 6–12 s, danach 3–12 s Abstand. |
| WOODLAND | Weicher leichter Regen, gedämpfte ungestimmte Tropfen, geschützter Wind und seltener ferner Donner. Die ganze Ebene hat zusätzlich 9,1 dB Abstand für die leiseren Saitenausklänge. |
| HIGHLANDS | Wind allein: lange weiche Atemzüge, wärmere rollende Böen und kürzere luftigere Brisen. Diese drei Arten wechseln selbstständig. |

Windarten haben jeweils neue Dauer, Filterfarbe und unregelmäßige Pausen von
2–14 s. Die anderen Worlds bekommen ebenfalls leisen Wind. Es gibt weder
einen ständigen Rauschteppich noch gestimmte Drips, Möwen, Vogelrufe oder
pitched Donner. Regen entwickelt sich über mehrere Sekunden und hat eigene
Pausen. Donner schwillt über mindestens 1,2 s an, klingt weich aus und lässt
mindestens 28 s Abstand; bei einer Regenpause wartet er zusätzlich.

## Klang- und Laufzeitvertrag

Die Erweiterung gehört zu `FAM_SOUND_FINISH_CANDIDATE=ON` im Product-Profil.
`nature.c` bindet dafür die private Implementierung `nature_accents.h` ein.
Ohne diesen schon vorhandenen Schalter bleibt die bisherige Nature exakt
erhalten. Reference bleibt der Default. Die musikalischen Quellen, ihre
langen Töne, drei gemeinsamen Quellplätze und Harmonie-/Tailzulassung sind
unverändert. Natur registriert weder einen Besitzer noch eine Tonhöhe.

Die Ebene mischt weiter **nach** Room/Delay/Grains und vor dem gemeinsamen
Master/DC-/Ausgangspfad. Sie kann keine Note, keinen Freeze-Inhalt und keinen
Room-Tail erzeugen. Eigenständige RNGs beeinflussen keine musikalische Phrase.
Keine Samples, Delaypuffer, dynamische Allokation oder blockierende Arbeit.
Host-Wetterzustand: 832 B plus drei 32-Bit-Ziele. Der Linkaudit kontrolliert
den echten ARM-Zustand in D1 mit maximal 2048 B und erfasst auch die im
privaten Header definierten Compiler-Stackframes. D2 bleibt dem Raum/FX-Bus.
Die tatsächliche IRQ-Zeit und gesamte verschachtelte Stackhöhe sind weiterhin
am Gerät zu messen.

Audio besitzt sämtliche Wetterzustände. Main publiziert nur Amount, World
und Seed mit drei unabhängigen 32-Bit-Atomics; Audio liest sie einmal pro
Renderaufruf. Amount läuft über 2 s weich nach. World/Seed werden hinter
einer 80-ms-Ausblendung zurückgesetzt und über 2 s eingeblendet. Bei fertig
abgesenktem Amount0 ist die Ebene exakt still und bearbeitet kein verborgenes
Wetter weiter. Clear/Mute löschen hinter dem gemeinsamen Nullausgang auch
diese Zustände. Alte Natur kann danach nicht wieder auftauchen.

Die weiche Amplitudenbegrenzung bleibt bei Nature 100 % vor Master/DC unter
0,004 (−48 dBFS), in WOODLAND zusätzlich mal 0,35. Donner ist vorher separat
unter 0,0007 begrenzt, danach gelten noch World-/Nature-/Masterpegel.
Die normale Mischung nutzt diese Grenzen nicht als Lautheitsziel; es gibt
keine automatische Verstärkung leiser Natur oder Ausklänge.

## Tatsächlich gemessener Abstand zu Cells

Zwei physisch gehaltene Cells mit Velocity 0,7, Starts bei 8/18 s und Release
bei 30 s, Room 0,5 und Master 0,75. Gleiche Eingaben mit Nature 0 und Nature 1;
Differenz des echten quantisierten Engine-PCM. Messfenster 10–30 s; außerdem
jedes einzelne 1-s-Fenster darin. Keine getrennte Normalisierung.

| World | Abstand Nature / Cells, Seeds 1234 und 42 | Lautestes 1-s-Verhältnis beider Seeds |
|---|---:|---:|
| COAST | −34,46 / −32,52 dB RMS | −26,22 dB |
| WOODLAND | −38,55 / −36,74 dB RMS | −21,10 dB |
| HIGHLANDS | −44,58 / −40,25 dB RMS | −32,99 dB |

Das gilt für diese tatsächlich geprüften gehaltenen Töne, nicht für jeden
beliebig leisen Anschlag oder das Ende eines vollständig verklungenen Tons.
Nature bleibt in musikalischen Pausen bewusst eigenständig hörbar. Pegel
belegen keine subjektive Natürlichkeit oder Hörfreigabe.

## Reproduzierbare Prüfungen und Hördateien

```sh
python3 tools/review_nature_accents.py build-nature
python3 tools/review_sound_finish.py build-sound-finish
python3 tools/review_ui_idle.py build-nature/ui --fixture-only --finish-candidate --sanitize
cmake -S . -B build-h743-sound-finish -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_SOUND_FINISH_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-sound-finish
python3 tools/check_product_link.py build-h743-sound-finish
python3 tools/device_capture.py build-h743-sound-finish
```

Native und ASan/UBSan prüfen jeweils 18 Minuten vollständige Float-Wetter-
Samples: drei Worlds, zwei Seeds, volle 120-s-Vergleiche 37/512 sampleweise
bitgleich, drei Windarten, wechselnde Intervalle, echte nicht-null Donner-
Samples mit gemessener eigener Spitze 0,000674255, Grenzen, Mono/DC, Cold Idle,
fertiges Amount0 und Clear. Die beobachteten Main-Veröffentlichungen bestehen
120 echte Unterbrechungsfälle und vollständige World-/Seed-/Idle-/Restart-
Verläufe; drei Atomics pro Render, höchstens 20 Filterkoeffizientenupdates.

63 tatsächliche Engine-Captures prüfen 67.737.600 Stereo-Frames bytegleich
über Partitionen, Room und frühere Builds. Kandidat mit Nature 0 entspricht
PR187; alter Build ohne Schalter entspricht PR187 auch mit positiver Nature.
Alle Note-Traces sind innerhalb der jeweiligen musikalischen Variante
Nature-unabhängig. Nature allein braucht keinen Phantomton und bleibt Room-
unabhängig. Additivität unter Cells ist bis auf höchstens 2 PCM-LSB geprüft;
der erste 40-ms-Ausgangsstart von Nature0 ist ausdrücklich ausgenommen, da
positive Nature den gemeinsamen Ausgang schon vor dem ersten Cell öffnet.

Zusätzlich besteht die vollständige Sound-Regression native und sanitisiert:
FX/Freeze, eigene Loopbesitzer, Stimmführung, 48 laute Endpunktmischungen mit
Nature 100 %, Clear/Mute und die tatsächlichen 18-min-Musikverläufe. Die alte
96-s-Hörprobe mit Nature 0 hat weiterhin exakt denselben Hash.
Der echte H743-UI-Fixture prüft nun auch den vorhandenen Nature-Editor mit
0→65→0 % und echtem quellenlosem Audio in allen Worlds; außerdem bleiben
126 vollständige 12-s-Wake-Vergleiche pro Modus und Cell-Releases bestanden.

Die neuen Hördateien dauern je 144 s: **COAST 0:00, WOODLAND 0:48,
HIGHLANDS 1:36**. Pro 48-s-World zuerst 8 s Natur allein, dann die zwei Cells;
in der Solo-Datei läuft dieselbe Natur ohne Cells. WAV ist unverändertes
Stereo-PCM 16/44100 Hz. Beide MP3s bekommen denselben festen Abhörfaktor 4
(+12,04 dB), keine einzelne World-/Pausenverstärkung, Kompression oder
künstliche Schlussblende. Vollständiges MP3-Decoding kontrolliert die Länge.
Quellhashes bleiben vor/nach sämtlichen Prüfungen identisch.
Die genauen Messdaten und Datei-Hashes stehen in
[NATURE_ACCENTS_METRICS.json](NATURE_ACCENTS_METRICS.json).

Diese Fassung ist ein implementierter Hörkandidat. Hörabnahme, physische
H743-Ausgabe, IRQ-/Stackmessung und die alten offenen SD-Gates bleiben offen.
Keine Firmware wurde auf ein Gerät geflasht; kein PR wurde gemerged oder
retargetet.
