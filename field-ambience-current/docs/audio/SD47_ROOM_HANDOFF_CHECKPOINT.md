# SD47 — vollständige Room-Zielwerte ohne Audio-Warten, 16. Oktober 2026

Diese einzelne Einheit behebt einen konkreten Übergabefehler zwischen Main und
Audio. Basis [PR169](https://github.com/Sancte3D/AMBIENT/pull/169), HEAD
`e2ede28c0ccc653f1d641741285d58eb7d02e587`, Tree
`60ad8f8b31f3367bc72b6c8b90deb9337d736222`; Run38887942690/720 abschließend
20/20 erfolgreich. Eigener Branch: `codex/ambient-hotpath-2026-10-16`.
Kein Merge oder Retargeting. SD47 bleibt für die übrige Hotpathprüfung offen;
24/54 Hauptpunkte geschlossen,30 offen,11 technische Blöcke.

## Reproduzierbarer Befund

`ambient_room_set` veröffentlichte acht Feedbackziele, Amount und T60 als
einzelne atomare Wörter. Diese Wörter hatten keine gemeinsame Generation.
Ein Audio-Interrupt nach dem ersten Gainstore las deshalb neue Teilkoeffizienten
zusammen mit alten übrigen Gains und altem Amount. Das war kein einzelner
zerrissener Float, aber auch kein vollständiger alter oder neuer Reglerstand.

Die Gegenprobe kompiliert den echten alten Room mit ausschließlich
Beobachtungshooks zwischen seinen Stores. Sie scheitert reproduzierbar:
`old=0 next=1 point=2 block=64 pauses=1 first_sample=5492`. Verglichen wird
ganze tatsächliche Float-Ausgabe samt Folgeausklang, kein Ersatzmodell der
Parameterrechnung. Daraus wird kein bereits gehörter Knack oder Klanggewinn
behauptet.

## Begrenzte Übergabe

Ein Main-Schreiber berechnet dieselben acht `expf`-Koeffizienten zuerst auf
seinem Stack. Danach veröffentlicht er eine ungerade Generation, die vorbereiteten
Wörter und abschließend die nächste gerade Generation. Versions- und Payload-
Wörter sind C11-seq_cst, damit ein gleichbleibender gerader Vorher-/Nachherwert
eine vollständige Publikation einschließt. Unsigned Generationwrap ist geprüft.

Der einzige Audio-Leser macht **genau einen Versuch**: höchstens elf atomare
Wortloads, bei ungerader Generation nur einer. Bei laufendem oder inzwischen
verändertem Write bleibt sein letzter vollständiger Zielstand erhalten.
Kein Spin, Retry, Main-Lock, IRQmaskieren, Heap oder Koeffizientenrechnen im
Leser. Übernahme des neu fertigen Standes an einer folgenden Rendergrenze.
36B Audio-Zielcache plus4B Version; keine zweite Hallarena oder Sourcebuffer.
Die Audiofunktion benutzt ausschließlich ihren eigenen angenommenen Cache.

## Wirklich geprüfte Fälle

- 335 gerichtete Fälle,5977773 Assertions:325 simulierte Main→Audio-
  Unterbrechungen an allen13 Veröffentlichungsstellen,64/512er Aufteilung,
  eine oder drei aufeinanderfolgende Audiohälften bei angehaltenem Schreiber,
  sechs Wertepaare einschließlich0/1, gleicher Werte und Clamps.
- Zusätzliche Wrapfälle, NaN/Inf ohne Veröffentlichung und1000 Edits vor einem
  Render. Zehn weitere Writer-Interleavings zwischen Leserloads: ein geänderter
  Kandidat wird ganz abgewiesen. Diese Gegenrichtung ist ein C11-Modellfall;
  Main unterbricht am H743 keinen höher priorisierten Audio-IRQ.
- C11/O2/Wall/Wextra/Werror und ASan/UBSan bestehen. Hooks fehlen im normalen
  ARM-Objekt; sie sind kein Bestandteil des Produktablaufs.
- 24 gepaarte vollständige12-s-Product-Ausgaben: drei Worlds ×Manual/Generate
  ×Nature0/1 ×64/512. Vorher/nachher ganze PCM- und tatsächliche Hookdateien
  bytegleich; auch die beiden Blockgrößen liefern gleiche ganze PCM. Mehrfache
  Roomziele, Dry/Return, ungültige Werte, Clear und Neustart enthalten.
  12700800 Stereo-Frames je Version; getrennte Fälle, kein Langzeit-/Hörurteil.
- Alle262 anderen der263 bestehenden Firmwareinputs bytegleich PR169.
  Quellen, lange WOODLAND-Holds, Grammatik, Room-DSP/Kalibrierung, SCN7 und
  Defaults bleiben erhalten. Kein neues Hörpaket erforderlich.

`test/run_tests.sh` enthält den neuen Test und prüft weiterhin die tatsächlichen
Product-/Scene-/Writer-/Recovery-/Generatorpfade. Gesamter Host- und CI-Stand
steht im Draft-PR/aktuellen Recoverykopf; kein Erfolg aus einem noch laufenden
Gesamtrunner ableiten.

## ARM und Ressourcen

Echter vollständiger H743-Product-Link, GCC13.2.1/Newlib4.4, mit dem tatsächlichen
M7/hard-float-Toolchainplan. Gegenlink benutzt dieselben übrigen Objekte und
denselben Linker, ersetzt nur Room durch dessen alten Source. Dessen Objektname
bleibt `ambient_room.c.obj`: der reale Linker ordnet Room-BSS anhand dieses
Namens D2 zu. Beide Links prüfen Tank/Diffusion im richtigen RAM-Bankbereich.

BIN196620→196700B, Flash **+80B**; D2-BSS **+40B**, D1-BSS/DTCM unverändert.
Keine unaufgelösten Atomic-Runtimehelfer oder Testhooks. Eigene Compilerframes:
Room-Process192→160B, neuer Accept-Helper40B; Main-Set40→80B. Der Helper ist
ein zusätzlicher Callee, keine behauptete32-B-Gesamtstackersparnis. Keine
transitive Bibliotheks-/IRQ- oder reale High-water-Messung. Kein DWT-/Deadline-
oder Lastreservenversprechen aus Hostzeit, Linkerplatz oder Compilerframes.

[SD47_ROOM_HANDOFF_METRICS.json](SD47_ROOM_HANDOFF_METRICS.json) enthält
Quellhashes, alle PCM-/Hookhashes, Gegenprobe, Sektionen und Compilerframes.
Reproduktion: `python3 tools/review_room_handoff.py build-room-handoff-review --sanitize --arm`.
Eigener CI-Job erzeugt den aktuellen Nachweis und das Product-ARM-Image.
Sieben frühere Bytegleichheitsaudits laufen unverändert am exakten, vollständig
grünen PR169-Stand weiter; ihre strengen Guards werden nicht gelockert. Neue
Übergabe, aktuelle PCM und normale Host-/ARM-Jobs prüfen den neuen Source.

## Nächste einzelne SD47-Einheit

Quellenvorbereitung, Start-Acknowledgement und Retirement unter gezielter
Main/Audio-Preemption prüfen. Die übrige Quellen-/Scheduler-/World-/Storage-
Hotpathaufnahme ist damit noch nicht vollständig abgenommen. Audio-Clear
mit Tankmemset, kalte Cachepfade, Filterupdates und DMA-Clean bleiben konkrete
Spitzenlastkandidaten für die echte H743-Abnahme SD48. SD41-Geräte- und finale
World-/Quellen-/Raum-/Naturhörgates bleiben offen; keine UI vor Soundabnahme.
