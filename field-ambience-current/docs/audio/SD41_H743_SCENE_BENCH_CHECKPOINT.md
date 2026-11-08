# SD41 — isolierter H743-Geräte-Testbuild, 16. Oktober 2026

Ein vollständiges separates H743-Image verbindet jetzt den vorbereiteten
Flashadapter mit den echten ST-Startupvektoren, Journal, Writer und Recovery.
Basis: [PR168](https://github.com/Sancte3D/AMBIENT/pull/168), HEAD
`70ddde32dfaeaffe3399a9ee5c1f83b65cde40e3`, Tree
`955a88bf75d2d1b5635ec0d8ac5fdbfa5f3dcdd2`; dessen Run38796728990/716 ist
abschließend19/19 erfolgreich. Eigener Branch:
`codex/ambient-h743-scene-bench-2026-10-16`.

Dies ist genau die nächste einzelne Softwareeinheit. Kein Merge, Retargeting,
Hörurteil oder zusätzlicher Hauptpunkthaken. SD41 bleibt offen. Unverändert:
24/54 geschlossen,30 offen,11 technische Blöcke.

## Eigenes Image und explizite Anschlüsse

`firmware-c-next/bench/scene_flash_h743` enthält eigenes CMake, Linker,
Main, FLASH-/SysTick-Handler, BusFault-Tailbranch und SWD-Befehle. Normales
Produkt-CMake, Main, Vektoren und sämtliche263 bisherigen Firmwareinputs
sind bytegleich zur Basis. Default bleibt reference, Live-Save synchron,
Journal OFF. Keine Klang-, Quellen-, SCN7-, UI- oder Audioänderung.

RESET, SysTick, FLASH und BusFault sind im vollständigen ELF geprüft. Der
BusFault-Vektor springt ohne BL mit erhaltenem EXC_RETURN in den vorbereiteten
Entry. Echte starke HAL-Callbacks, feste Guardlabels/Bytes `00687047`, Bank1-
Ladesegmente und Initial-SP `0x20020000` sind binär geprüft. Main/IRQ/Adapter/
Fault-C werden mit `-mgeneral-regs-only` gebaut. Typisierte Debugsymbole bleiben
für die volatile, in DTCM liegende Mailbox verfügbar.

Boot prüft2048KiB/BankSwap/Vektoren und Adapter, schaltet I-/D-Cache ein und
startet ausschließlich einen schrittweisen Lesescan. Kein Board-PLL-Aufbau:
Reset-HSI, kein Audio/Display/LED, keine Aussage zur späteren Produktlast.
Mailbox berichtet Identität, Uhr, Mainfortschritt, Operation, IRQs und maximale
DWT-Zyklen um einen Schritt. Diese Höchstzahl enthält Präemption und ist keine
getrennte Start-/Poll-/Read-WCET.

## Bewusstes Schreiben statt automatischem Bootsave

Die GDB-Datei definiert nur Befehle, ohne Verbindung, Load oder Continue.
READ/RECOVER mutieren keinen Flash. ARM muss bestätigt sein, gilt weniger als
10000 laufende HAL-Millisekunden und wird von genau einem SAVE-Versuch
verbraucht. Anfrage-ID zuletzt, Bestätigung vor nächster Anfrage; Ablehnung
einer fremden Anfrage verändert nicht die laufende Operationsidentität.
Cancel drainiert echte laufende Hardware vor terminalem Abschluss. Jede
Tickiteration enthält höchstens einen IO-Callback, Read höchstens32B.

Der Schreibpayload ist ein reproduzierbares368-B-Muster mit `AMBENCH1`, keine
SCN7-Scene. Er benutzt ausdrücklich dieselben reservierten Bank2-Sektoren6/7.
Dediziertes Gerät und Sicherung/Wiederherstellung dieser Sektoren sind deshalb
Teil der [Geräteanleitung](../../firmware-c-next/bench/scene_flash_h743/README.md).
Kein Gerät wurde in dieser Einheit verbunden oder programmiert.

## Reproduzierbarer Nachweis

`tools/review_h743_scene_bench.py ... --sanitize-model` prüft den tatsächlichen
Benchcontroller mit echtem Adapter/Writer/Recovery über modellierte Hardware:
27752 Checks,2529 Ticks,2 Erase-/13 Programstarts, max1 IO/Tick. Boot ohne
Mutation, Altszene im anderen Sektor, abgelehnte/wiederholte Befehle, verbrauchte
und abgelaufene Freigabe einschließlich Timerwrap, kompletter Save/Neuscan,
in-flight Cancel, Read-CORRUPT/ERROR und Startfehler bestehen. C11/O2/Werror
und ASan/UBSan erfolgreich. IRQs/Silizium werden damit nicht simuliert.

Vollständiger ARM-Link mit GCC13.2.1 und Newlib4.4: BIN8668B, statische DTCM-
Belegung2072B, reservierter Stack16384B, danach112616B frei. Compilerframes:
Main64B, Bench-Init24B, Tick64B, neue C-IRQs0B. Das sind einzelne Frames,
keine transitive oder gemessene Stackbelegung. Bekannte Newlib-/Vendor-
Metadatenwarnungen betreffen nicht verwendete Syscallstubs/GNU-stack.

BIN SHA256 `b2fff3f68e4d2c0757b5fbf3ee3a0eddd36360f16d3923d8e91c0c3e7c0c111b`.
[SD41_H743_SCENE_BENCH_METRICS.json](SD41_H743_SCENE_BENCH_METRICS.json):3167B,
SHA256 `8670b7004907278fc17fef5594dbc1f102ae16eb8fde4b12ca87cc0d63a8b984`.
Eigener CI-Job baut unabhängig und archiviert ELF/BIN/HEX/Map, Symbole,
Disassembly, Compilerframes, JSON, GDB und Anleitung. BIN-Hash vergleicht die
Flashbytes; Debugpfade können den ELF-Hash ändern. CI-Stand im Draft-PR prüfen.

## Nächste Grenze

Echte H743-Abnahme: intakt/Single-/Double-ECC bei warmem/kaltem Cache,
gezielte MSP/PSP/FPU-Rückkehr, fremder Fault bleibt fatal; Unterbrechungen bei
Erase/Payload/Commit/Verify, Recovery des letzten gültigen Records; getrennte
Start-/Poll-/Readlatenz und IRQ/Main/Stack später unter echter Produktlast.
Das jetzige Main/MSP-Image prüft PSP/FPU nicht automatisch. Ohne Gerät als
nächste einzelne unabhängige Softwareeinheit SD47 prüfen, statt weitere
Vorbereitung als Gerätefreigabe zu zählen. UI weiter nach Soundabnahme.
