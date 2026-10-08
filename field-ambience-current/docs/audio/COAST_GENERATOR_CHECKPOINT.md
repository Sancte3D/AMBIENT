# COAST — tatsächlicher Generator, 8. Oktober 2026

COAST führt jetzt in `FAM_SOUND_PROFILE=product` zwei äußere Stimmen in
Gegenbewegung zu einer gemeinsamen Mitte. Ein leiser Akkordton verbindet den
Verlauf. Der positiv gehörte direkte Host-Entwurf aus PR145 ist damit in einen
echten, release-sicheren Firmwareablauf übertragen. Die neue Firmwareprobe
ist noch keine abschließende Hör-/Gerätefreigabe. `reference` bleibt Default.

## Musikalischer und technischer Vertrag

- Seed1234/D-Dur: unten D3→A3→B3→D4; oben D5→B4→F#4→D4;
  F#3 bleibt als gemeinsame Stimme erhalten. Seed kann die obere Core-Route,
  kleine Akzente und Haltezeiten ändern. Transposition und Minor sind definiert.
- Drei feste Besitzer6/7/15, insgesamt acht tatsächliche Starts. Beide äußeren
  Zielabsichten werden einmal in Owner6 ausgeführt. Kein doppeltes Unisono,
  kein neuer Layer oder Tonkörper, kein held Steal.
- Der reine Vorschlag ändert keine gehörte Phase und keinen RNG. Nur bestätigte
  DSP-Starts schalten die Phase weiter; verworfene Vorbereitungen zählen nicht.
  Außenrollen werden erst nach wirklichem Quellenende wiederverwendet. Der
  vorhandene gehaltene Akkordton erhält sein Ende erst nach Ziel-DSP-Ack.
- Drei globale Slots einschließlich Releases,16 Tail-Einträge und deren
 7,2-s-Grenze bleiben bestehen. Main plant feste, begrenzte Rollenpaare;
  der Audio-Renderpfad und alle Quellen-DSP-Dateien bleiben unverändert.
- Nur intern für COAST: Core-Noten MIDI45..80, Frequenzwächter105..850 Hz.
  Manual/öffentliche World-Note-API bleiben140..470 Hz/MIDI50..69.
  Beide Collections und Equal/Just verwenden dieselbe reale Hz-/Kollisionsprüfung.
- Neue Key/Collection/Tuning-Kontexte, World/Seed, Generate/Autoplay,
  Präsenzunterdrückung, Mute und Clear verwerfen offene COAST-Absichten.
  Gehörte Quellen behalten ihre tatsächlichen Hz und Originalbesitzer;
  Generate-Quellen gehen beim Kontextwechsel in natürlichen Release.

## Hörbeleg aus der echten Engine

`tools/review_coast_generator.py` kompiliert die tatsächliche Product-C-Kette.
`render_coast_generator.c` steuert nur Engine-APIs, keine direkten Quellenstarts,
temporären DSP-Änderungen oder SHAPE-Sonderwerte. Parameter: Seed1234,
D-Dur/Equal, Activity/Color/Attack/Release0,5, Volume0,6, Room0,24, Nature0.

Die70-s-Aufnahme enthält acht bestätigte Starts, maximal drei Quellen und
natürliches Quellenende vor Dateiende. Die Mitte beginnt bei43,8 s; sechs
Sekunden später beendet Generate seine Planung und lässt alles ausklingen.
Die normalen Produktreleases sind länger als im direkten27-s-Host-Entwurf.
Kein Clear oder künstlicher Schlussfade. Room darf einen leisen Resttail haben.

Dry/Room-PCM, WAV und Starttraces sind bei64/512-Frame-Audioblöcken jeweils
bytegleich, bei identischem10-ms-Main-Steuerungsraster. Getter mit Main-Ack-
Arbeit laufen ebenfalls auf diesem Raster. Beliebige andere Control-Frequenzen
werden dadurch nicht als identisch behauptet. Tracezeiten sind Main-Beobachtung
des DSP-Starts, keine Behauptung des exakten ersten Oszillatorsamples.

RAW−33,4LUFS/−20,7dBFS True Peak; LISTEN mit konstant+10,4dB bei−23LUFS/
−10,3dBFS True Peak. Kein AGC oder Kompressor. Format/finite/Mono/DC und
Datei-/Quellen-/Header-/Toolhashes stehen in
[COAST_GENERATOR_METRICS.json](COAST_GENERATOR_METRICS.json).

## Verifikation und H743-Ressourcen

- Vollständige `test/run_tests.sh`-Suite bestanden: reale Product-Audioläufe,
 36-Minuten-Stress, alle sechs Worldrichtungen, Pending-/Mute-/Präsenz-/Kontext-
  und Timerwrap-Prüfungen sowie Quellen, Raum/Nature, Migration und Journalmodell.
- Neu: reine Phrasenverträge für12 Keys×2 Collections×32 Seeds; unveränderte
  wiederholte Vorschläge, doppelte Ack-Abweisung und exakte Null-Deadline am Wrap.
- Neu:48 vollständige echte Engine-Phrasen über12 Keys×2 Collections×2
  Stimmungen. Beide Release-/Activity-Endpunkte über die Tonartfälle verteilt,
  Volume/Color/Room1. Jeweils acht Starts, monotone Gegenwege, ein Ziel,
  ein ununterbrochener Akkordton, max drei reale Quellen, natürliche Retirement,
  finite PCM, kein Limiter und Samplepeak unter−6dBFS. Keine exhaustive
  kartesische Parameterprüfung oder Hörabnahme daraus ableiten.
- H743 Product Release-Build und `check_product_link.py` bestanden. Vergleich
  zum vorherigen PR147-Baum mit demselben ARM-GCC13.2.1/Newlib4.4-Toolchainstand:

| Ressource | PR147 | COAST integriert | Änderung |
|---|---:|---:|---:|
| Bank1-Flash-Ladebild |190724 B|193360 B|+2636 B|
| RAM_D1 |65600 B|65696 B|+96 B|
| RAM_D2 |60808 B|60808 B|0 B|
| reservierter DTCM-Stack |16384 B|16384 B|0 B|
| größter einzelner Compilerframe im Product-Kern |192 B|216 B|+24 B|

Ein interner D2-Raum, ausgerichteter4096-B-D1-DMA-Puffer, keine Archive/
Referenz-DSP-Wege im Produktlink. Bank1 bleibt unter1MiB. ELF-/Compilerbelege:
[COAST_GENERATOR_ARM_METRICS.json](COAST_GENERATOR_ARM_METRICS.json).
Compilerframes sind keine Stack-Highwater-Messung; DWT, ISR-Nesting,
Display/LED/Storage-Verkehr und reale Ausgangskette bleiben Gerätetests.
Journal bleibt OFF; ECC/Power-cut und nonblocking Save bleiben offen.

## Nächster einzelner Punkt

Die lange WOODLAND-Richtung in Quelle und Generator übertragen: weicher Beginn,
lange tragende Saiten, gemeinsame Akkordtöne und ungleiche Dauern. Der kurze
gezupfte Entwurf bleibt verworfen. HIGHLANDS folgt separat. Keine neue UI,
kein automatischer Merge und keine pauschale Freigabe der drei Worlds.
SD14 ist softwareseitig integriert; Gesamtstand weiter24 geschlossen/30 offen.
