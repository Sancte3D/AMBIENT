# WOODLAND — eigenständige lange Resonatorquelle

Stand 26.10.2026. Basis PR185, Commit
`b6b62dbf3f33230ea4c659a8f5976efbf4ff31d2`, Tree
`49b53ec0cffe37981904b1c68faac3e640afe768`.
Branch `codex/ambient-woodland-resonator-ui-2026-10-26`.
Michalis hat die vorherige Körper-Hörprobe mit „Das ist sehr gut“ beurteilt.
Das ist keine pauschale Geräte-/Produktfreigabe für den neuen Ersatz.

## Einzelne Klangänderung

Model 1 ist ein eigener harmonischer modaler Resonator: bis zu acht gedämpfte
Teilschwingungen bei der tatsächlich zugelassenen Frequenz ×1…×8. Oberhalb
0,45×Samplerate entfallen Modi. Keine gerundeten MIDI-Frequenzen,
inharmonische Steifigkeit, fremde Tonkollektionen oder weitere Audiostimmen.
Die anfängliche Auslenkung regt den Körper einmal an; danach klingt er ohne
laufende Fremdanregung aus. Keine Tonitor-Sourcen oder Tabellen kopiert;
Referenzprinzip ist DRUIDs modaler Körper aus dem geprüften Tonitor-0.56-ZIP.

Die bestehende WOODLAND-Quelle besitzt weiterhin zwei lokale Stimmen unter
der globalen Drei-Quellen-Zulassung. Ein Union teilt die schon reservierten
4096 B je Stimme zwischen Saitenpuffer und Resonatorzustand. Im Resonatorfall
läuft die Saite nicht, auch die vorige Körperergänzung ist dann inaktiv.
Der neue Modus ist ein Ersatz innerhalb desselben Besitzers/Quellplatzes.
Es gibt keinen zusätzlichen Delay-/Stimmenpool.

Vorbereitung ist Main-only; Audio besitzt die laufenden Zustände. Umschalten
betrifft ausschließlich künftige Noten. Gleich bleiben die vorhandenen
Attack-/Release-/Kurzstoprampen, Entfaltung, World-Charakter, Color-Shelf,
Panning, dry/send und Room. Natürlicher Ruhestand folgt der tatsächlich
neuen Quellenergie; er wird nicht künstlich auf den alten Saitenframe gesetzt.
Clear/reuse initialisiert den kompletten Unionzustand. Main-Konfiguration
wird durch Audio-Clear nicht verändert.

Grundton-T60 nominal 36 s bei neutralem Release, 9…144 s über dessen bisherige
Skala. Höhere Teiltöne verklingen schneller. Die reale Ruhestandsentscheidung
liegt bei der vorhandenen Energieschwelle; 36 s ist keine zugesagte Gesamtdauer.

## Aktivierung und Nachweis

`FAM_WOODLAND_RESONATOR_CANDIDATE` ist standardmäßig OFF, unabhängig von den
vorherigen Kandidatenschaltern. `engine_set_woodland_resonator(bool)` erfasst
künftige lange Noten; kein neuer Scene-/UI-/MIDI-Parameter.

```sh
python3 tools/review_woodland_resonator.py build-woodland-resonator
cmake -S . -B build-h743-resonator -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_WORLD_CHARACTER_CANDIDATE=ON -DFAM_WOODLAND_RESONATOR_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-resonator
python3 tools/check_product_link.py build-h743-resonator
python3 tools/device_capture.py build-h743-resonator
```

Abgeschlossene native/ASan-/UBSan-Prüfungen:

- Tatsächlicher Resonator bei 60, 61,1234, 196, 246,94165, 470 und 8000 Hz,
  je Release 0/0,5/1: größte Grundtonabweichung 0,000110 Cent; größte
  T60-Abweichung durch Float-Koeffizienten rund 2,45 %. Harmonik-/Peakgrenze,
  Modusauslassung, ungültige Eingaben und lange freie Schwingung geprüft.
- Echte Quellstimmen: Nachbar bytegleich, keine laufende Delayberechnung,
  belegter Pool/Owner/ungültige Frequenzen abgewiesen, aktive Snapshots
  unverändert, schnelle Besitzerfreigabe, ungehörte Vorbereitung storniert,
  saubere Wiederverwendung. Bei 196 Hz weiterhin aktiv nach 20 s; natürlicher
  Ruhestand im Fixture nach 1.435.085 Frames, rund 32,54 s.
- Echte Product-Engine, 37/512-Frameblöcke mit gleichem 10-ms-Mainraster:
  bytegleiche PCM-Partitionierung. Physische Cells, Hold, Entfaltung, Clear
  und Mute geprüft. 48 laute Endpunktfälle mit Major/Minor, ET/JI, voller
  Velocity/Master/Room/Nature, Color-Enden und langen Hüllkurven bestehen.
- Zusätzlich 48 vollständige echte WOODLAND-Phrasen: alle Grundtöne,
  Major/Minor und ET/JI, Attack/Release/Color-Enden, tatsächliche Besitzer,
  gehaltene gemeinsame Töne und natürlicher Abschluss bestehen.
- Resonator OFF stimmt über 25.401.600 Stereo-Frames mit PR185 überein,
  einschließlich Character-/Body-Konfigurationen. COAST/HIGHLANDS bleiben
  auch bei Resonator ON bytegleich. In der neuen 24-s-Generatorpassage
  bleiben tatsächliche Besitzer-/Hz-/Velocity-/On-/Off-Zeitspuren gleich.

Der Hostrunner enthält die neuen Kandidatenprüfungen. Die bestehende CI
baut im selben H743-Job einen eigenen Resonator-ON-Kandidaten mit Linkaudit
und passendem Auslesepaket; die Workflowzahl bleibt 30. Aktuellen ARM-Status
am jeweiligen Commit prüfen. Ein Linkpass ersetzt keine Gerätezeitmessung.
Journal bleibt OFF, Live-Save synchron. Kein Merge/Retarget oder Flashen.

## Neue Hördateien

Echte C-Firmware, Stereo PCM16/44.100 Hz, Seed1234, World-Charakter ON,
Room0,5, Nature0, Master0,75. Generate endet je Passage bei 18 s. Keine
externe Komposition, Normalisierung oder Schlussblende.

| Datei | Inhalt | SHA256 |
|---|---|---|
| `AMBIENT_Woodland_Resonator_AB.wav` | 48 s, erst24 s bisheriger Körper, dann24 s Modalersatz | `4b173b55e7ab50a432cff81e766f82227acfdefeffa882add6a030ac4a5d69ef` |
| `AMBIENT_Woodland_Resonator.wav` | 24 s nur Modalersatz | `483e03bfbed3a59d324f4afa10f6aaa6cff5904a42d3c818a06e6119060208d9` |

Der erste A/B-Abschnitt stimmt exakt mit der zuvor bereitgestellten
`AMBIENT_Woodland_Body_V2.wav` überein. Im Rohvergleich RMS −33,821 / −35,624
dBFS, Samplepeak −20,082 / −22,914 dBFS. Der Ersatz ist in dieser Passage
also etwa 1,8 dB leiser; keine Dateiverstärkung kaschiert den Unterschied.
Null Nonfinite-/Limiterereignisse. Eigene Hörabnahme des Ersatzes bleibt offen.

## Display-Untersuchung und nächster Schritt

Zusätzlich beauftragt: aus der Referenzfirmware für die Erstellung unserer
UI-Dateien lernen. Ergebnis und Quellstellen stehen in
[UI_ASSET_PIPELINE_TONITOR.md](../UI_ASSET_PIPELINE_TONITOR.md).
Ein unabhängiger Offline-PNG-Exporter prüft native Maße, RGB565-Bytefolge,
4-Bit-Grau-/Alphazeilen, Transparenz und generiertes C. Keine neue
Encoderbelegung und keine bereits behauptete LVGL-/Geräteintegration.

Nächste einzelne Soundeinheit bleibt die tonal begrenzte Flächenalternative;
danach Stimmführung/Gestenloop-Ties und die vereinbarten Effektstufen. Alle
übrigen SD-Hör-/Gerätegates werden durch diese neue Quelle nicht geschlossen.
