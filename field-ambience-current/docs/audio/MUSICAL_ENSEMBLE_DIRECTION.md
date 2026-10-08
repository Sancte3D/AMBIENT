# AMBIENT — musikalisches Ensemble zuerst, 8. Oktober 2026

## Nutzerurteil und neue Priorität

Die trockenen Einzelquellenvergleiche wurden als RAW, langweilig, zu ähnlich
und ohne musikalische Besonderheiten bewertet. Das Urteil wird nicht durch
technische Prüfwerte entkräftet. Gewünscht sind zusammenwirkende gehaltene
Akkordtöne, gemeinsame Töne über Wechsel, Oktav-/Registerwechsel und eigene
Dauern: eine erkennbare musikalische Handschrift im Ensemble.

Die nächste Produktarbeit priorisiert deshalb **SD11/SD14: Akkordbogen und
Stimmführung**, bevor SD05 und weitere isolierte Quellen-/Endpointpakete
fortgesetzt werden. SD02–SD04 sind keine bestätigte Familienwahl. Weiterhin
24 technisch geschlossene und 30 offene Gesamtgates; kein Sound-fertig-Urteil.

## Tatsächlicher Befund

Der jetzige `world_grammar.c` besitzt Ereignis-/Motiverinnerung und unterschiedliche
Abstände, aber keinen Akkordzustand oder Funktionsbogen. Sein Index bleibt bei
0–7. Der D-Dur-Core ist pentatonisch; G und Cis fehlen. Er vermeidet Konflikte,
organisiert aber keinen Wechsel von harmonischem Ziel, Spannung und Auflösung.
Drei isolierte Quellen allein beantworten diese kompositorische Frage nicht.

Ziel: Ein begrenzter Akkordplan für eine Phrase; darin klare Grund-/Verbindungs-/
Oberstimmenrollen. Gemeinsame Töne behalten Besitzer und Verlauf. Andere Stimmen
wechseln gezielt Ton/Register, mit eigenen Dauern und Antworten. Die Handschrift
der Welten entsteht später aus unterschiedlicher Stimmführung und Zeitgestaltung
auf dieser musikalischen Grundlage. Gehaltene Eingaben müssen musikalisch
zusammenhängende Stimmgruppen tragen können. Keine automatisch zusätzliche Bassspur.

## Ein kleiner hörbarer Entwurf

`render_ensemble_sketch.c` spielt eine eigene komponierte 27-s-Phrase:
**D-Dur → h-Moll/Fis → G-Dur → A sus4 → A-Dur**. Fis wird zwischen F#4/F#3
weitergegeben; H und D tragen den mittleren Wechsel. D bleibt lange stehen
und löst sich am Ende nach Cis auf. Einsätze sind versetzt; Haltezeiten sind
unterschiedlich. Das Radiohead-Beispiel war eine Erklärung des musikalischen
Prinzips, keine Vorgabe für eine Songkopie.

Die Probe verwendet den echten unveränderten Product-Bowed-Tonkörper, drei
reale Stimmen einschließlich Releases und denselben vorhandenen C-Raum bei
Amount 0,24. Keine Samples, externen Synths, neue Layer oder neue Effekte.
Dry und Room spielen denselben Score. Volume 0,6, Quellenkalibrierung 0,5;
LISTEN ist ein konstanter +6,7-dB-Gain auf −23 LUFS, True Peak −12,5 dBFS.

**Ausdrücklich eine Host-Kompositionsprobe:** direkte Quellenaufrufe statt
normaler Product-Zulassung; direkte SHAPE-Release=0 (Faktor 0,25), individuelle
Attackwerte und erweiterter Tonvorrat einschließlich G/Cis. Das liegt außerhalb
der aktuellen Makro-/Pentatonik-Grenzen. Product-DSP, Header, UI und Defaults
wurden nicht geändert. Der autonome Generator spielt diese Phrase noch nicht.

## Prüfung

- Warnungsfreier C-Build mit `FAM_SOUND_PRODUCT`, `-O2 -Wall -Wextra -Werror`.
- Zehn tatsächlich angenommene Quellenstarts; maximal drei reale Stimmen
  einschließlich Releases; alle Quellen enden natürlich vor 27 s.
- Keine Clear-/Schlussfade-Montage. Der vorhandene Raum darf seinen Resttail
  am Dateiende noch enthalten; kein vollständiges Room-Retirement behauptet.
- Dry und Room bei 64/512 Frames jeweils bytegleiches PCM und identische Traces.
- Alle Float-Samples finite und unter Clipping; Format, True Peak, Mono und DC
  geprüft. Raw und Listen getrennt; keine AGC oder Kompression.
- Source-/Header-/Tool-Hashes, reale Commands, Pegel und Berichte gespeichert.
- [PR143-CI](https://github.com/Sancte3D/AMBIENT/actions/runs/37783241714)
  ist inzwischen bestanden und belegt den unveränderten Firmware-Ausgangspunkt.
  Keine neue Geräte- oder Wahrnehmungsfreigabe.

## Nächster einzelner Softwarepunkt

SD14 erhält einen echten Akkord-/Voicing-Zustand statt einzelner zufälliger
Skalenereignisse. Seine Übergabe braucht gemeinsame Töne, nachvollziehbare
Stimmbewegungen, tatsächliche Hz-/Besitzerhistorie und einen planbaren Umgang
mit Releases und Raumtönen. Der Tonvorrat muss funktionale Harmonien zulassen;
die bisherige pauschale Pentatonik-/Intervallabweisung darf diesen Verlauf nicht
unbemerkt verhindern. Geprüfte maximale Belegung und sichere Audioübergaben
bleiben Anforderungen. Die Probe ist ein Vorschlag für diese Übertragung,
keine nachträgliche Behauptung bereits implementierter Generatorfähigkeiten.
