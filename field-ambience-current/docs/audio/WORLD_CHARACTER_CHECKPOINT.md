# World-Charakter V1 — Tonitor-orientierter Klangkandidat

Stand 2026-10-26. AMBIENT-Basis: PR183,
`49169462c1705a4238bd2194559689b07e22d8c5`.

Die Entfaltungs-Hörprobe wurde vom Nutzer mit „klingt gut“ beurteilt. Dieser
nächste einzelne Ausbau betrifft Klangentwürfe und zusammenhängende Bewegung
innerhalb der drei bestehenden World-Quellen. Er ist keine Übernahme der
vollständigen Tonitor-Engine und keine Freigabe aller Quellen oder Effekte.

## Konkrete Referenz und Umsetzung

Geprüft wurde wieder das hochgeladene `tonitor-main.zip`, Version 0.56,
131 Originaldateien, SHA256
`9c52dcfebb94058bb0476763aaf4649603cd384015b1d8bf0aeeb1b51e275e1a`.
Die Originaldateien wurden nicht bearbeitet. Relevant sind insbesondere:

- `firmware/src/anima.c`, Kopfkommentar und `an_move`: eine simulierte Hand
  verbindet Position, Druck und Bewegung mit Klangfarbe, Pegel und Raum;
  gespielte Tonhöhe bleibt bestehen. Orbit, Acht und ruhiges Gleiten sind
  unterschiedliche Bewegungsmodelle.
- `firmware/src/mod.c`, `mod_draw`: langsame Modulation gehört zum Entwurf
  einer anhaltenden Klangfamilie. Der Patch bekommt begrenzte, gemeinsam
  bewegte Parameter statt beliebig neue Instrumentarten.
- `druid.c`, `hive.c`, `gran.c`, `reverb.c`, `delay.c` und `layers.c` liefern
  die nächsten Kandidaten für Resonanz, Flächen, Textur, Raum und Spielablauf.

AMBIENTs `world_character.h` ist eine eigene Float-Implementierung dieser
Gestaltungsprinzipien. Es wurden keine Tonitor-Quelltexte, Tabellen oder
Effekt-Ringpuffer importiert. Namen und Referenzen dokumentieren die Anregung.
Die vollständige Tonitor-Firmware bezeichnet sich als GPL-3.0-only. Bei einem
späteren direkten Port sind die konkreten Dateihinweise maßgeblich: selbst
`druid.c` hat einen GPL-3.0-only-Dateikopf, während die Lizenzübersicht den
Rings-Ursprung als MIT führt. Der zukünftige Resonatorvergleich muss daher
seine tatsächlichen Dateien samt Hinweisen festhalten; eine Lizenzübersicht
allein ist keine Freigabe, beliebige Hilfsdateien mitzukopieren.

## Was jetzt vorhanden ist

Jede neu vorbereitete Stimme erhält aus Field-Seed, World, Besitzer und
Frequenz einen reproduzierbaren, kleinen Klangentwurf derselben Familie.
Ein späterer Seed-/Konfigurationswechsel schreibt keine aktiven Stimmen um.
Die neue Bewegung blendet sich während der ersten vier Sekunden sanft ein.

| Welt | Bewegungsmodell | Musikalische Wirkung |
|---|---|---|
| COAST | Kreisbahn plus langsamere Strömung | Körperfilter, bestehende Resonanzanteile und ein enges Stereobild bewegen sich gemeinsam. |
| WOODLAND | Zwei weich interpolierte, unregelmäßige Zielverläufe | Die lange Saite verändert Ausgangsfarbe und räumliche Balance, ohne neu angeregt zu werden. |
| HIGHLANDS | Breite Acht plus langsamere Asymmetrie | Körperfarbe und harmonische Detailanteile entwickeln sich in wiederkehrenden Bögen. |

HIGHLANDS tauscht dabei einen Teil des vorhandenen Sägezahnanteils gegen den
phasengleichen Grundton; die Grundton-Anregung bleibt kalibriert. Es kommt
kein weiterer Oszillator hinzu. COAST benutzt seine vorhandenen Resonatoren.
WOODLANDs kompletter zirkulierender Puffer, Stimmung, Feedback, natürliche
Dämpfung und Ruhestandsentscheidung bleiben bitgleich zur alten Saite.

Die Bewegung hat feste Grenzen: Körperfilter-Faktor 0,685–1,315,
Detail-Faktor 0,74–1,26, Pegelfaktor 0,945–1,055 und maximal 0,035 zusätzliche
Pan-Gewichtung. WOODLANDs Ausgangsfarbe bleibt zusätzlich im vorhandenen
Shelf-Bereich begrenzt. Es gibt kein Pitch-/Feedback-/Ereignis-Modulationsziel.
Die vorhandene physische Cell-Entfaltung bleibt davon unabhängig nutzbar.

## Aktivierung und Reproduzierbarkeit

`FAM_WORLD_CHARACTER_CANDIDATE` ist standardmäßig **OFF**. Der bisherige
Product-Kandidat und bestehende Scenes behalten so ihr bisheriges PCM.
Für den ausdrücklich neuen Hörkandidaten:

```sh
cmake -S . -B build-h743-character -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_WORLD_CHARACTER_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-character
```

Der Main-only-Aufruf `engine_set_world_character(bool)` setzt die Vorlage
für zukünftige Noten. Ein Umschalten bearbeitet keine laufenden Filter oder
Voice-Zustände. Es ist noch kein neuer UI-/MIDI-/Scene-Parameter.
`WORLD_CHARACTER_MODEL=1` bezeichnet dieses Entwicklungsmodell.
Eine spätere reguläre Aktivierung mit gespeicherten Varianten benötigt eine
explizite Modell-/Presetversion, statt nur alte Seeds neu zu interpretieren.
Scene-Journal bleibt OFF und Product-Save synchron. Kein Gerät wurde programmiert.

## Echtzeit und Nachweise

Main bereitet die wenigen Hash-/Raten-/Variantenwerte beim Note-on vor.
Die IRQ-Seite liest nur den bereits veröffentlichten Voice-Snapshot und
arbeitet mit einer lokalen 32-Sample-Uhr. Größere Audio-/DMA-Blöcke ändern
somit weder Geschwindigkeit noch Verlauf. Der Audiopfad verwendet LUT-Sinus,
Polynome und begrenzte Arithmetik; keine neue Allokation, Delaylinie,
blockierende Synchronisation oder transzendentale Berechnung.

- Alle bestehenden Host-Testblöcke bestanden einschließlich Hotpath-Lint,
  Reference-Tests, Long-World-/Scene-/Tail-/Harmonieprüfungen und Effects-Suite.
- `test_world_character.c`: 72 simulierte Minuten begrenzte Steuerverläufe;
  echte Product-PCM mit 37/512 Frames byte-identisch; Seedvarianten,
  physische Entfaltung, HOLD, Clear und neue Besitzer geprüft.
- `test_world_character_sources.c`: tatsächliche private DSP-Zustände;
  unveränderte Nachbarstimme, Pitch-/Oszillatorphase und Hüllkurve;
  WOODLAND zusätzlich mit komplett identischem Saitenpuffer/Feedback/Retirement.
- `test_world_character_stress.c`: 48 hörbare 12,5-s-Fälle bei voller
  Anschlags-/Masterlautstärke, maximalem Raum/Natur, Klangfarben-Enden,
  Major/Minor, ET/JI und langen Hüllkurven; Clear/Mute am Ende exakt still.
  Keine Nonfinite-/Output-Limiter-Ereignisse. Native und ASan/UBSan bestanden.
- `review_world_character.py`: 6.350.400 vollständige Stereo-Frames bei OFF
  exakt wie PR183; bei ON dieselben echten Noten-/Owner-/Hz-/Zeit-/Velocity-
  Traces. ON ist auch zwischen 37/512 Frames vollständig byte-identisch.
- ARM GCC 13.2.1: Product OFF und explizit ON gebaut. Link-/DMA-/Room-/
  Compiler-Frameaudit bestanden. Das ON-Auslesepaket passt zu seiner eigenen
  ELF/BIN-Paarung. Kein Timing- oder Stack-High-Water-Nachweis auf Silizium.

| ARM Release | PR183 | World-Charakter OFF/ON |
|---|---:|---:|
| text | 198.924 B | 203.580 B |
| data | 240 B | 240 B |
| bss | 142.864 B | 143.888 B |
| Bank-1-Loadimage | 199.172 B | 203.828 B |

Zusätzlich 1.024 B statischer Zustand, keine neue Sample-/Delay-Speicherbank.
`engine_render` bleibt als einzelner Compiler-Frame bei 88 B; das ist keine
Aussage über gemessene Gesamt-Stackhöhe oder Echtzeitreserve.
Die bestehenden CI-Jobs prüfen den aktuellen Kandidaten und bauen das ON-
Hörimage zusätzlich im vorhandenen ARM-Job; die Jobzahl bleibt 30.

## Hördateien

`AMBIENT_Welten_Charakter_V1.wav`: 72 s, Stereo PCM16 / 44.100 Hz,
12.700.844 B. COAST 00:00, WOODLAND 00:24, HIGHLANDS 00:48.
Jede Welt spielt ihre echte Firmware-Grammatik mit Seed 1234. Raum 0,5,
Natur aus, Master 0,75; nach 18 s wird Generate beendet und natürlich
ausgeklungen. Keine externe Komposition, kein nachträglicher Gain/AGC.
SHA256: `96f6a5e15aee60386dffb6ae1ee2139d267e5c7cfed4e59dad352697e25bbe40`.

Der vollständige Vergleich `AMBIENT_Welten_Charakter_AB.wav` ist 144 s lang:
pro Welt 24 s bisher / 24 s Charakter. SHA256
`85e60dfd2f05f90a0a4f7b591e442b887da199d282f548408e2d486c65235045`.

Reproduktion: `python3 tools/review_world_character.py build-world-character`;
der oben genannte PR183-Commit muss lokal verfügbar sein. CI-Artefakte:
`ambient-world-character-current-proof-and-wav` sowie
`firmware-c-next-h743-world-character-hearing` für den bewusst separaten Build.
Die musikalische Wirkung ist weiterhin eine Hörentscheidung.

## Verbindliche nächste Blöcke — einzeln abarbeiten

| Reihenfolge | Aus Tonitor übernehmen/ableiten | Nächster konkreter Umfang |
|---|---|---|
| 1 | Familie + zusammenhängende Bewegung | Dieser V1-Kandidat ist implementiert und technisch geprüft; Hörvergleich offen. |
| 2 | DRUID/Rings und ausgewählte HIVE-Flächen | Zuerst eine lange Resonatorquelle als Ersatz vergleichen; danach eine tonal begrenzte Flächenalternative. Drei globale Quellen beibehalten. |
| 3 | Akkord-/Voicing-Varianten, gebundene Gestenloops | Rollen und Stimmführung pro Welt erweitern; Loop-Ties mit eigenen Playback-Besitzern. Jedes Ereignis durch dieselbe Harmonie-/Stimmenzulassung. |
| 4 | Raum und Delay | Dattorro-Raum als Alternative zum aktuellen Raum vergleichen; danach ein gemeinsamer Musik-Delay. Tatsächlich hörbare alte Tonhöhen/Tails mitführen, Clear/Mute vollständig einbeziehen. |
| 5 | Granular/Freeze und übrige Farben | Zuerst untransponierte lange Körner in einem gemeinsamen Puffer; Freeze nur mit passender Harmoniehistorie. Filter, sanfter Drive, Crush/Rate und Phaser/Chorus danach gezielt bewerten, statt jeden Effekt jeder Welt aufzuzwingen. |

Neue Quellen zuerst ersetzen vorhandene Quellen. Neue Effekte zuerst als
Alternative oder ein gemeinsamer Bus mit sauberem Bypass und Pegelabgleich.
Nature bleibt ein bewusst getrennt gemischter Anteil. Für jeden Block:
kurzer A/B-Vergleich, tatsächliche Quellen-/Tail-/Wechsel-/Clear-/Mute-Prüfung,
Host-/H743-Nachweise und eigene Hörabnahme. Die Software-Belege schließen
keine der bisherigen Geräte-/Hörentscheidungen automatisch.
