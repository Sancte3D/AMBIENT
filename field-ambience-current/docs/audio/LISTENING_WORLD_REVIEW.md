# Generate — autonome Klangwelten, Paket 2026-09-21

## KERNURTEIL

Generate folgt jetzt der bewusst gewählten World statt einer zufällig zuvor
gewählten manuellen Synth-Engine. Die größte verbleibende Schwäche ist die
unvollständige Übergangs-/Ruhelogik: vollständige Quell-Releases bei Character-
Rückkehr und Display-Idle fehlen. Eigenständige World-Phrasen sind ein
musikalischer Entwurf, noch keine subjektive Klangfreigabe.

## FUNDAMENTAL FALSCH

Behoben: Die bisherige Character-Auswahl konnte autonome Wiedergabe verhindern.
Die Melodie hing an einer manuell überschreibbaren Voice; gehaltene
Bowed/Horn/Choir-Einsätze waren unabhängige One-shots, obwohl der Composer
lange Melodiehaltezeiten plante. Die World übernimmt jetzt ihre eigene Stimme;
die drei gehaltenen Timbres gehören zu Melodiequelle 15 und folgen deren
Release, Unterdrückung und Generate-Stopp. Guembri bleibt natürlich abklingend.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Rückkehr zur gemerkten manuellen Engine verwendet den bestehenden etwa
  15-ms-Crossfade. Shared-FX-Tails bleiben, volle Ambient-Quell-Tails nicht.
- Character/Voice-Menüwerte benötigen im Hörmodus eine klare Kennzeichnung
  ihrer manuellen Rolle. Sonst scheint eine Änderung wirkungslos.
- Der 15-Minuten-Displaytimer und klangneutrales Wake-Verhalten fehlen.
- Lange World-Stimmen können das Verhältnis Vordergrund/Pad verändern.
  Pegel und Effekte wurden in diesem Paket nicht pauschal neu abgestimmt.
- Gleiche Harmonie-/Composer-Architektur bleibt unter allen Worlds bestehen.
  Unterschiedliche Zeitprofile allein beweisen keine unverwechselbaren Welten.

## LOCKED

Generate: gesperrte fünf Spielflächen, Hold und Drone; kein Steer-Ersatz.
Beim Start alte Eingaben/Latches und laufende Gesture beenden. Generate erneut
oder Clear (auch Shift+Clear) beendet das Hören; Volume wird nicht gesperrt.
Generate beginnt ohne alte Presence-Wartezeit und verwendet Ambient aus jeder
manuellen Engine. Eine während des Hörens gewählte manuelle Engine wird erst
beim Ausstieg aktiv. World-Wechsel bleiben bewusst über die bestehende UI.

Implementierte Statusrückmeldung: LISTENING/PLAY-Overlay und 4-s-Lichtpuls bis
zu einem Drittel der bisherigen weißen LED-Duty; Fade-Engine bleibt aktiv.
Lichtrhythmus ist vom musikalischen Timing unabhängig.

## REMOVE / MERGE / REDESIGN

Keine neuen Oszillatoren, Buffers oder Effektketten. Vorhandene World-Timbres
werden mit ihrer Phrasensteuerung zusammengeführt. Neue konstante Tabelle:
25 Byte; zwei zusätzliche Engine-Indizes: auf H743 zusammen 8 Byte Zustands-
daten. Compiler-/Linker-Auswirkung nicht gemessen. Keine dynamische Allokation,
keine neue Arbeit pro Audiosample; länger aktive Stimmen können die gemessene
Lastverteilung dennoch verändern. DWT muss am Gerät folgen.

| World | Melodiehaltezeit | Grundpause | Dichtefaktor vor Composer | Rolle |
|---|---:|---:|---:|---|
| Alps | 8–16 s | 5–12 s | 0,65 | lange getrennte Rufe |
| Open Sea | 9–16 s | 3–8 s | 0,75 | verbundene Bögen |
| Fjords | 10–16 s | 6–14 s | 0,55 | lange Töne mit größerem Abstand |
| Moss Fields | 8–14 s | 6–12 s | 0,60 | zurückgenommene gehaltene Linien |
| Desert | 4–8 s | 8–16 s | 0,45 | vereinzelte gezupfte Ereignisse |

Die Haltezeit gilt auch für die Melodie-Padstimme; ein gezupfter Klang wird
nicht künstlich gestreckt. Composer und Phrasenatmung verändern Pausen und
Dichte; die Grundpause ist keine garantierte minimale/maximale Gesamtdauer.
Tonhöhenfilter, Motivreplay und mindestens 1,4 s zwischen automatischen
Einsätzen bleiben erhalten. Keine geänderten World-/Scene-/Core-IDs.

## BESTE VERSION

Eine World hat eigene Harmonik, Material, Atem und Ruhe. Generate macht diese
Zusammenhänge unmittelbar hörbar. Die nächste Klangeinheit muss prüfen, ob
Vordergrund, Pad, Naturtextur und Raum gemeinsam funktionieren; zuerst deren
Pegel/Bewegung und Überlappungen reduzieren, bevor neue Schichten hinzukommen.

## Prüfung auf dem Host

`bash test/run_tests.sh`: vollständige Suite einschließlich Audio-Hotpath-Lint,
Produkt-Routing, Controls, LEDs, World-Profilen, Engine und FX: bestanden.
Produkt-Engine: 15.100 Checks ohne Fehler; FX: 478.860 ohne Fehler.
Die bestehenden Pluck-only-Autoplay-Assertions wurden auf die tatsächliche
Default-World Alps/Horn korrigiert, nicht entfernt.

- HAL-Routing-Test: Generate sperrt neue Presses; alte Releases erreichen ihren
  Besitzer; neun Modekombinationen, Cleanup und ungültige Ereignisse.
- Controls: Zellen/Hold/Drone gesperrt; Clear verlässt Hören; danach spielbar.
- Sechs Engines: Generate startet tatsächlich Audio und merkt/restauriert die
  manuelle Auswahl, auch nach Änderung während des Hörens.
- World-Scheduler: je 15 min simulierte Steuerzeit; 29/34/27/34/31 vollständig
  abgeschlossene Melodiehaltezeiten, korrekte World-Grenzen und Grundpausen.
  Das sind Ablaufprüfungen mit verkürztem Audiofortschritt, keine 75-min-Hörprobe.
- Gehaltene World-Stimmen: expliziter Test auf Quellenbesitz auch jenseits der
  früheren One-shot-Dauer und vollständige Freigabe nach Generate-Stopp.
- LED: sichtbar veränderlich, begrenzte Helligkeit, keine Periodenkante, aus
  nach Exit. Keine Aussage über tatsächliche optische Helligkeit.

### Hörprobe

`tools/review_listening_worlds.py --world 1 --output /tmp/open-sea`

28 s, Open Sea, echter C-Produktpfad mit World-Makros und gemeinsamem Dream-
Raum. Automatik startet bei 0 s, Stop bei 24,009 s; kein manuell komponierter
Notenablauf. Melodiestimme setzt in dieser Startphrase erst bei 21,408 s ein.
Das zeigt den Start und dessen Zurückhaltung, keinen ganzen World-Zyklus.

| Messung | Ergebnis |
|---|---:|
| Rohdatei bei Master 0,5 | −35,8 LUFS; −21,3 dBTP |
| Konstanter Export-Gain | +9,3 dB |
| Ausgelieferte 28-s-WAV | −26,5 LUFS; −12,0 dBTP |
| Rohdatei geclippte Samples | 0 |
| Rohdatei absoluter Kanal-DC | <0,000001 FS |

Nur je 200 ms Randfade für den Ausschnitt; keine Kompression oder abschnitts-
weise Normalisierung. Der letzte Fade begrenzt die Datei auf 28 s, er ist
keine Firmware-Hüllkurve. Hörlautheit auf Kopfhörern folgt auch deren Hardware.
Der Render wurde gemessen; daraus folgt keine subjektive Hörbewertung.

## TEST AM GERÄT

Vollständige Liste in `PRODUCT_REVIEW_EXECUTION.md`. In diesem Paket besonders:
Start/Stop aus allen Engines, Lock auch bei Shift/Gesture/Scenes, natürliche
Übergänge, langer Vordergrund gegen Bett und Raum, ruhige LED im Dunkeln.
Aktueller ARM-Cross-Build/Map, Stack, DWT und echte Ausgänge sind ungemessen;
die ältere Speicherfreigabe gilt nicht automatisch für diesen Build.
