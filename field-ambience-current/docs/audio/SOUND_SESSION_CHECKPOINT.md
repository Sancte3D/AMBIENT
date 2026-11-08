# Effektparameter und große Sound-Sessions, 28.10.2026

Auftrag: technischen Ausbau fortsetzen, Encoderbelegung erst am Ende.
Basis PR191, `327215295b283d956dccab63112825bb95107549`.
Die von Michalis als besser bewerteten V2-Worlds und WOODLANDs lange,
harmonische Töne bleiben die Klangbasis. Keine neue World in diesem Ausbau.

## Effektsteuerung

Alle Effektziele gehören jetzt einem gemeinsamen Main-Zustand. Fünf
Mischanteile, Freeze, Raummodell und die fünf neuen Feinparameter werden als
eine versionierte Audio-Transaktion übernommen. Ein unterbrochener Main-
Schreibvorgang lässt den letzten vollständigen Audio-Snapshot stehen.
Get/Apply und die bestehende Makro-API verwenden denselben Zustand; alte
Makroaufrufe überschreiben die neuen Feinwerte nicht.

| Parameter | Bereich | Bisheriger Standard |
|---|---|---|
| Delay links/rechts | je 40 bis 700 ms | 210 / 267 ms |
| Delay-Feedback | 0 bis 0,5 | 0,32 |
| Grain-Länge | 40 bis 260 ms | 8192 Samples, etwa 185,76 ms |
| Phaser-Tempo | 0,01 bis 0,5 Hz | 0,07 Hz |

Delay-Zeitänderungen überblenden 40 ms zwischen festen Lesepositionen.
Es gibt keine kontinuierlich bewegte Delay-Position und damit keine daraus
erzeugte Tonhöhenrutsche. Während einer Überblendung folgt die nächste dem
aktuellsten Ziel. Feedback und Phaser-Tempo glätten über etwa 80 ms.
Grain-Längenwechsel blenden nur den Grain-Rückweg über 40 ms aus, setzen
die zwei überlappenden Fenster neu und blenden wieder ein. Der Ringinhalt
bleibt bestehen, auch bei Freeze; die Lesegeschwindigkeit bleibt eins.

Der Stereo-Delay nutzt 262144 Byte internes D1-RAM. Ein Gültigkeitszähler
schützt die absichtlich nicht initialisierte NOLOAD-History: Vor Clear
gespeicherte oder noch nicht geschriebene Samples können nicht gelesen
werden. Clear braucht deshalb kein zusätzliches 256-KiB-Memset im Audio-
Interrupt. Der bestehende Room-/Plate-/Grain-Zustand liegt weiter in D2.
Der ARM-Link-Audit prüft die tatsächliche Lage und Größe beider Bereiche.

FXP1 speichert Zielwerte mit explizitem Float32-Little-Endian, Flags und
CRC32 in 52 Byte. Nichtendliche Werte werden ohne Teiländerung abgelehnt.
Apply begrenzt gültige Zahlen auf die Bereiche; das Dateiformat lehnt
ungültige Werte auch mit korrekter Prüfsumme ab.

## Vollständige Scene und Flashanbindung

Der zusätzliche Schalter `FAM_SOUND_SESSION_CANDIDATE=ON` erfordert H743,
Product und den vollständigen Sound-Kandidaten. Er ist standardmäßig OFF.
Er kann nicht mit dem alten experimentellen Scene-Journal gleichzeitig
aktiviert werden. Die neuen Effektparameter selbst benötigen nur den
bestehenden vollständigen Sound-Kandidaten.

SCN8 erweitert fünf Scene-Slots auf je 1168 Byte. Eine Transaktion enthält
5848 Byte mit World-/Menüwerten, Collection, Helligkeit, Seed, FXP1 und einer
GLP1-Gestenaufnahme mit maximal 128 Ereignissen je Slot. Das neue JSS1-
Journal hat 8192 Byte Payload pro Record und reserviert Bank 2, Sektoren
6 und 7. Seine eigene Kennung unterscheidet es vom alten 512-Byte-JSC1.

Der tatsächliche bestehende asynchrone H743-Adapter ist nun eingebunden:
ein Flashbesitzer, HAL-IRQ-Schreiben, ECC-sichere Reads, strenge BusFault-
Frame-Prüfung und die passenden FLASH-/BusFault-Vektoren. Der synchrone
Flash-Treiber wird im Session-Build nicht verlinkt. Der Link-Audit prüft
auch die starken Vektoren und das Fehlen des synchronen Journal-Writers.
Firmware-Code und Konstanten müssen vollständig in Bank 1 bleiben.

Beim Boot läuft eine reine Leserekonstruktion vor `audio_init`. Während
des Spiels startet oder pollt jeder Main-Schritt höchstens eine Operation
oder liest höchstens 32 Byte. Die Audio-Interrupts bleiben freigegeben.
Payload zuerst, Commit-Header zuletzt, vollständige Lesebestätigung vor
der RAM-/UI-Erfolgsmeldung. Der vorige gültige Record bleibt erhalten;
ein Neustart hängt keine Daten an einen möglicherweise beschädigten Sektor.
Ein physisch vollständiger, aber noch nicht bestätigter Commit darf nach
einem Reset wiederhergestellt werden.

SCN5/6/7 werden nur lesend migriert. Alte Scenes erhalten Standardeffekte
und einen leeren Loop. Der erste neue Commit liegt im anderen Sektor als
der bisherige rohe SCN7-Store. Ein erkennbares altes experimentelles JSC1-
Journal blockiert neue Schreibvorgänge; es wird nicht stillschweigend
überschrieben. Unverfügbare Reads verhindern ebenfalls die Aktivierung.

Das bestehende Scene-Bedienen bleibt: SHIFT+Cell speichert, Cell lädt.
Der Status zeigt SAVING, SAVED oder SAVE FAILED. Ein zweiter Save/Recall
während BUSY wird abgelehnt; spätere Klangänderungen verändern den bereits
angenommenen Snapshot nicht. Eine laufende REC muss zuerst beendet werden.
Geladene Loops bleiben gestoppt und starten keine Noten. Freeze-Audio und
andere flüchtige Effekthistories werden nicht gespeichert; ein Scene-Recall
stellt Freeze ausdrücklich auf aus. Es gibt keine neue Encoderbelegung.

## Prüfung und praktische Grenzen

`review_sound_session.py --sanitize` prüft die echten DSP-, Scene-, Journal-
und H743-Adaptermodule, nur die NOR-/ECC-Hardware ist im Host modelliert:

- genaue Impulspositionen bei 210/700 ms, schnelle Zeit-/Längen-/Tempo-
  Änderungen und sechs Sekunden bitgleiches PCM zwischen 37/512 Frames;
- maximale 700-ms-/0,5-Feedback-/260-ms-Konfiguration, endlicher Ausgang
  und Abklingen innerhalb des getesteten Nachhallfensters;
- Codec-Beschädigungen, ungültige Zielwerte und kein Wiederauftauchen
  alter Delay-History nach Clear;
- SCN7-Migration ohne Boot-Schreiben, unveränderliche Snapshots, alle fünf
  Slots, 128 Ereignisse, Rollover, Abbruch, Teilprogrammierung und Retry;
- 1104 simulierte Stromabbrüche, sechs Programmierbreiten an jeder
  Payload-/Header-Flashword-Grenze, Recovery und erneutes Speichern;
- neun vollständige 12-s-Engine-Captures in allen drei Worlds: mit und
  ohne Speichern, 37/512 Frames, identisches PCM und Planer-Notentraces.

Native und ASan/UBSan müssen bestehen. Der vollständige Sound-Review
prüft zusätzlich Freeze, Besitzer, Harmonie, World-Endpunkte und die
bisherige 96-s-Musik; Nature- und H743-Bedienreviews bleiben erforderlich.
CI baut einen eigenen Session-H743-Kandidaten und erzeugt Speicher-,
Stackframe- und Profiler-Capture-Artefakte. Das ist keine Geräteabnahme.
Echte Flash-/Audio-Laufzeiten, ECC/Cache, Stromunterbrechungen und
Stack-High-Water müssen am H743 gemessen werden. Keine neue Hörfreigabe,
kein Merge, kein Flashen.

```sh
python3 tools/review_sound_session.py build-sound-session --sanitize
python3 tools/review_sound_finish.py build-sound-finish
cmake -S . -B build-h743-sound-session -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_SOUND_FINISH_CANDIDATE=ON -DFAM_SOUND_SESSION_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-sound-session
python3 tools/check_product_link.py build-h743-sound-session
python3 tools/device_capture.py build-h743-sound-session
```
