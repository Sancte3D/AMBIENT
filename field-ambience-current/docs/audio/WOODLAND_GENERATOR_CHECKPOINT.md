# WOODLAND — lange Quelle und tatsächlicher Generator, 8. Oktober 2026

WOODLAND verwendet jetzt im Product-Kandidaten lange, weich einsetzende Saiten
und eine eigene Akkordrollen-Regel. Der verworfene kurze Pluck-Entwurf wird
nicht als musikalische Richtung weitergeführt. Der neue Klang braucht noch
ein Hörurteil; das ist keine gesamte Sound-/Gerätefreigabe. `reference` bleibt
CMake-Default. Nächster einzelner Integrationspunkt ist HIGHLANDS.

## Quelle und musikalischer Ablauf

Die zwei bestehenden Karplus-Strong-Delay-Linien bleiben. Die Product-Engine
wählt nach Init/Clear fest dieselbe Ambient-Artikulation für Manual/Generate.
Der direkte historische Quellenweg nach `pluck_init()` behält seine kurze
Artikulation; `pluck_set_ambient` ist kein Produktregler und keine neue Welt.

- Nominaler Loop-T6036 s ×Releasefaktor; Filter/Interpolation verkürzen den
  tatsächlichen Decay. Attackbasis0,8 s ×Attackfaktor, innerhalb der normalen
  Product-Makros ca.0,429..1,493 s. Kein Resampling, keine wiederholte Anregung.
- Color-Damp0,025..0,005, default0,015; weiterhin symmetrischer FIR und80-ms-
  Smoother. Feste Quellenkalibrierung0,22 bleibt, keine AGC/Kompression.
- Manual-Key-up behält den natürlichen Saitenausklang mit ursprünglichem Owner.
  Generate plant Halteabsichten6..18,5 s, dann den eigenen musikalischen
  Release2 s ×Releasefaktor (ca.1,32..3,03 s), pro Note gespeichert.
- Kontextübergabe kann bestehende Releases auf100 ms verkürzen; erneute
  Stop-Anforderung verlängert keine Rampe. Master-Clear/Mute bleiben40 ms.
  Die alte20-ms-Referenzrampe besteht ihre unveränderte PCM-Prüfung.

Seed1234/D-Dur: **D3/F#3 → B3/F#3 → B3/D4 → F#3/D4 → F#3/A3**.
Nur eine Stimme wird jeweils neu begonnen; gemeinsame Akkordtöne bleiben in
ihrem realen Besitzer. Die untere und obere Rolle haben ungleiche Haltezeiten
und Akzente. Seed kann den letzten Ton zu einer verwandten Sext/Septim ändern.
Basiszwischenräume2/12/9/4/7 s, Activity-Faktor1,35..0,65, minimale Lücke1,4 s;
Activity verändert keine Holds oder Klangfarbe. Ausklang/Pitchhistory können
geplante Einsätze verlängern. Nach letztem Hold folgen5..9 s geplante Ruhe;
die nächste Phrase wartet außerdem beide tatsächlichen Quellenenden ab.

`woodland_phrase_t` hält sechs begrenzte Phasen, RNG/Timing und gehörte Zähler.
Ein reiner Vorschlag verändert nichts. Erst tatsächlicher DSP-Start bestätigt
Phase/RNG und setzt die Haltefrist. Verworfenes Pending zählt nicht. Owner6/7
werden erst nach tatsächlicher Retirement wiederbelegt; zwei lokale und drei
globale Slots einschließlich Releases,16 Tail-Einträge und7,2-s-Grenze bleiben.
Interner WOODLAND-Generator: Core-MIDI45..68, Wächter105..470 Hz.
Manual/öffentliche Note-API behält140..470 Hz/MIDI50..69. Keine verdeckte
Tontransposition. Key/Collection/Tuning, World/Seed, Generate/Autoplay,
Präsenz, Mute und Clear verwerfen offene Phrasen; klingende Hz werden nicht retuned.

## Echte Firmware-Hörprobe und Prüfungen

`review_woodland_generator.py` kompiliert die tatsächliche Product-C-Kette.
`render_woodland_generator.c` verwendet nur Engine-APIs und10-ms-Main-Ticks:
D-Dur/Equal, Seed1234, Activity/Color/Attack/Release0,5, Volume0,6,
Room0,24, Nature0. Keine direkte Quellenfolge, temporäre DSP-Kopie oder
SHAPE-Sonderwerte. Sechs tatsächliche Starts, maximal zwei Quellen inklusive
Releases, natürliche Freigabe vor50-s-Dateiende. Letzter Start bei34,06 s;
Generate stoppt6,5 s danach, ohne Clear oder künstlichen Schlussfade.

Dry/Room-WAV, PCM und Main-Ack-Traces sind bei64/512-Frame-Audioblöcken jeweils
bytegleich, mit identischem10-ms-Steuerungsraster. Keine Identitätsbehauptung
für andere Control-Raten. RAW−33,4LUFS/−22,6dBFS True Peak; LISTEN mit
konstant+10,4dB bei−23LUFS/−12,2dBFS True Peak. Format/finite/Mono/DC,
Datei-/Quellen-/Header-/Toolhashes:
[WOODLAND_GENERATOR_METRICS.json](WOODLAND_GENERATOR_METRICS.json).

- Default-Quelle D3: erste100 ms zu2..3-s-Body-RMS0,03100;
  Body bei8..9 s zu2..3 s0,29785. Technischer Zeit-/Energiebeleg, kein Hörurteil.
- Reiner Vertrag:12 Keys×2 Collections×32 Seeds, wiederholte unveränderte
  Vorschläge, doppelte Ack-Abweisung und explizite Null-Deadline am Timerwrap.
- 48 vollständige tatsächliche Engine-Phrasen: alle Keys/Collections und
  Equal/Just; getestete Attack-/Release-/Activity-/Color-Endpunkte über Keys
  verteilt, Volume/Room1. Je sechs Starts, tatsächlicher gemeinsamer Ton bei
  beiden entsprechenden Akkordübergängen, max zwei Quellen, natürliche
  Freigabe, finite PCM, kein Limiter, Samplepeak unter−6dBFS. Private tiefe
  Noten bleiben über Manual/öffentliche World-Note-API abgewiesen.
- Vollständige `test/run_tests.sh`-Suite bestanden: reale12-Minuten-Product-
  Läufe,36-Minuten-Stress, sechs Worldrichtungen, Pending/Mute/Präsenz/Context/
  Timerwrap, Source-/Room-/Nature-Prüfungen, Migration und Journalmodell.
- COAST-Audition erneut aus aktueller Engine gerendert: Dry/Room-Raw, Hör-WAV
  und Traces exakt wie PR148. COASTs musikalischer Ablauf ist unverändert.
- Das bisherige Product-Hörpaket besteht erneut alle42 Summen-/Grenzproben:
  schlechtester True Peak−9,66dBFS, geringste Mono-Energie0,91904. Historischer
  kurzer WOODLAND-/HIGHLANDS-Hostvergleich und langer45-s-Quellenentwurf
  bleiben über ihren ausdrücklich direkten Quellenweg reproduzierbar.

## H743 und verbleibende Gates

Product Release mit ARM-GCC13.2.1/Newlib4.4 und `check_product_link.py` bestanden.
Vergleich zum vorherigen PR148-Build mit demselben Toolchainstand:

| Ressource | PR148 | langes WOODLAND | Änderung |
|---|---:|---:|---:|
| Bank1-Flash-Ladebild |193360 B|195496 B|+2136 B|
| RAM_D1 |65696 B|65760 B|+64 B|
| RAM_D2 |60808 B|60808 B|0 B|
| reservierter DTCM-Stack |16384 B|16384 B|0 B|
| größter einzelner Product-Kern-Compilerframe |216 B|216 B|0 B|

Ein interner D2-Raum und ausgerichteter4096-B-D1-DMA-Puffer, keine Archive/
Referenz-DSP-Familien im Produktlink, Bank1 unter1MiB. ELF-/Compilerbelege:
[WOODLAND_GENERATOR_ARM_METRICS.json](WOODLAND_GENERATOR_ARM_METRICS.json).
Compilerframes sind kein Stack-Highwater oder CPU-Nachweis. DWT/Deadline,
ISR/Display/LED/Storage-Verkehr und reale Ausgangskette bleiben Gerätetests.
Journal bleibt OFF; ECC/Power-cut und nonblocking Save bleiben offen.

SCN7 behält Felder/World-IDs und normierte Parameter, Spec0.5 dokumentiert
bewusst die neue WOODLAND-Artikulation. Frühere Klangstände reproduziert ihr
jeweiliger Git-Checkpoint. Neues Quellen-/World-Hörurteil, langfristige
musikalische Entwicklung und Produkt-/Geräteabnahme bleiben offen.
SD15 ist implementiert, Gesamtstand weiterhin24 geschlossen/30 offen.
