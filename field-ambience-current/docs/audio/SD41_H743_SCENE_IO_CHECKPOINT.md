# SD41 — vorbereiteter H743-Flashadapter, 15. Oktober 2026

Diese einzelne Einheit ergänzt die vorbereitete Scene-Save-API um einen
separaten STM32H743-Adapter. Basis ist der vollständig grüne
[PR167](https://github.com/Sancte3D/AMBIENT/pull/167), HEAD
`45a71dc9615271e596aa984a4c89cb8403e91686`, Tree
`25fb5b4bca5bcf02de1e44ed6a77962e32a86991` (CI38638263991:18/18 erfolgreich).
Eigener Branch: `codex/ambient-h743-scene-io-2026-10-15`.
Keine Default-/UI-/Main-/Vektor-/CMakeaktivierung; Live-Save synchron, Journal
OFF, SD41 offen. Keine Soundänderung oder Hörfreigabe aus diesem Checkpoint.

## Asynchroner Besitz und Abschluss

`scenes_flash_h743_async_init/make_io` liefert passende Interfaces für den
vorhandenen schrittweisen Writer und Recovery. Ein globaler FLASH-Besitzer,
Main-only, keine konkurrierenden HAL-Flashoperationen. Reinit/Rebind kann
weder einen laufenden Zugriff noch ein unbestätigtes IRQ-Ergebnis verlassen.
Die synchronen Mutationshooks sind ausdrücklich fail-closed, nicht blockierend.
Dieser Adapter unterstützt den schrittweisen Recovery-/Writerpfad, nicht den
alten synchronen Whole-record-Read-/Writeadapter.

Bank2/Sektoren6–7, je128KiB, Basis0x081C0000. Nur Bank2 wird entsperrt/gesperrt.
Preflight verlangt ruhende Hardware und HAL, bevor die realen HAL-Erase_IT- oder
Program_IT-Aufrufe erfolgen. Deren vorhandener Wait-Helper hat dann unter dem
exklusiven Besitz keine laufende Operation abzuwarten. Das ist keine gemessene
Start-WCET: HAL-Barrieren, AXI und tatsächliche Programmierzeiten bleiben Gerätethemen.
Der32-B-Programmpuffer ist ausgerichtet und eine eigene feste Kopie.

Callbacks melden lediglich EOP/Fehler. Fehler gewinnt auch dann, wenn EOP zuerst
im selben HAL-IRQ gemeldet wurde; falsche Completionadresse kann nicht DONE
werden. Poll bleibt BUSY, bis Hardware und HAL beide quieszent sind. Erst dann
Bank2-Lock und terminales DONE/ERROR; vorher kein Pufferrelease/Cancelabschluss.
Ein unzulässiger Kontext liefert bei laufendem Auftrag weiter BUSY. Kein Timeout
befreit noch laufende Hardware oder ihren Puffer. Spätere ganze Journalprüfung
und bestätigte RAM-Publikation bleiben Aufgabe von Writer/Scene-API.

## Begrenzter Leseweg und konservativer ECC-Kandidat

Read verlangt gültigen Besitzer, ruhende Hardware und privilegierten Main ohne
Interruptmasken; maximal32B. Nur betroffene32-B-Flashcachelines werden invalidiert.
Höchstens neun ausgerichtete32-bit-Loads, danach Publikation aus32-B-Scratch.
ERROR/CORRUPT verändern den Ausgabepuffer nicht. Keine Audio-DMA-Cacheoperation.

Einzelne korrigierte ECC-Werte dürfen gelesen werden. Ihr Flag wird gezielt
vor/nach dem Load bereinigt, damit die Fehleradresse des nächsten Reads zuordenbar
bleibt. Ein vorbestehender unzugeordneter Doppel-ECC-/CPU-Fault ergibt ERROR,
keine pauschale Löschung. Der Gerätecode führt genau `ldr r0,[r0]; bx lr` in einer
eigenen Assemblyroutine aus. Der BusFault-Einstieg wählt originalen MSP/PSP und
bewahrt EXC_RETURN. Der gemeinsame, nativ geprüfte Framevalidator akzeptiert nur:

- aktiven Guard und exakt den festen LDR-PC;
- ausschließlich präzisen Data-BusFault mit gültiger, exakt passender BFAR;
- Thread-Rückkehr über MSP/PSP, mit oder ohne FPU, gültiges Thumb-xPSR;
- Bank2-Doppel-ECC im exakt passenden32-B-Flashword, kein Bank1-Doppel-ECC.

Nur dann wird der gestackte PC auf das feste BX-LR-Label gesetzt und der Read
CORRUPT. Nur die zugehörigen CPU-/Bank2-ECC-Flags werden quittiert. Unbekannte,
gemischte, imprecise, Stacking-, Handler- oder Instructionfaults bleiben fatal;
kein allgemeiner HardFault-Bypass. PM0253 Fig11: Auch im erweiterten FPU-Frame
stehen R0–xPSR direkt am Stackanfang; FP-Daten folgen oberhalb. Die beiden
Handler-C-Dateien werden mit `-mgeneral-regs-only` geprüft, ohne eigene FP-Arbeit.

Grundlagen: [ST RM0433 §4.7.7](https://www.st.com/resource/en/reference_manual/dm00314099.pdf)
und [ST PM0253 Rev6 Fig11/§2.4.7/BFSR](https://www.st.com/resource/en/programming_manual/pm0253-stm32f7-series-and-stm32h7-series-cortexm7-processor-programming-manual-stmicroelectronics.pdf).
Dies ist ein aus den dokumentierten Fehlerregistern abgeleiteter Kandidat;
Cache-/AXI-/Faultverhalten auf echtem H743 ist noch nicht abgenommen.

## Reproduzierbare Prüfung

`tools/review_h743_scene_io.py` prüft alle258 bisherigen Firmware-/Header-/
HAL-/Linker-/Startup-/CMakeinputs bytegleich zur exakten Basis. Neue Dateien sind
nicht im expliziten Firmware-Sourceplan. Bestehende Sound-/Sceneaudits behalten
ihre unveränderten Baselineguards; keine heimliche Entschärfung alter Prüfungen.

Native Prüfung: C11/O2/Wall/Wextra/Werror und ASan/UBSan,70740 Checks,
4480 positive Lesevarianten plus Rand-/Fehlerfälle.328 gerichtete Architektur-
Frames über alle vier gültigen Thread-EXC_RETURN-Formen: jedes einzelne falsche
CFSR-/PC-Bit, falsche BFAR/ECC-Bank/-Adresse, unzulässige Rückkehr, falsches xPSR,
fehlender Guard und echte FP-Tailfixtures nach PM0253. Abgewiesene Fehler ändern
weder Frame noch Guard. Beide EOP-/Fehlerreihenfolgen, Callback vor Hardwareende,
Reinit/Rebindschutz, falsche Completion, Lock-/Startfehler, ungültiger Kontext,
unverändertes geliehenes Word. Echte vorhandene Recovery/Writer führen über
diesen Adapter einen368-B-Save, vollständigen Neuscan und in-flight Cancel aus.
Die native Hardwareunterseite ist modelliert; CPU-Faultentry wird nicht simuliert.

ARM: GCC13.2.1/Newlib4.4, echter H743-Vendorheader, neue C-/Assemblydateien plus
Scene-/Controller-/Journal-/Writer-/Recovery- und reale ST-HAL-Flashdateien
kompiliert und partiell verbunden. Keine unaufgelösten Scene-Save-/Adapter-/
HAL-Flash-/pFlashsymbole. Guardbytes `00687047` und feste Labels0/2 geprüft.
Partielles Objekt20152B, kein neu integriertes Flashimage. Direkte statische
Frames: Start-Erase40, Start-Program24, Poll8, Read96, Init24, Make-IO4,
BusFault-Helper40, Dispatcher8, Framevalidator12B. Keine transitive oder
physisch gemessene Stackreserve. Die vendorte HAL enthält keine GNU-stack-Notiz;
der Teillinker meldet diese Metadatenwarnung, kein neuer Geräte-Stacknachweis.

[SD41_H743_SCENE_IO_METRICS.json](SD41_H743_SCENE_IO_METRICS.json) enthält
Inputs, Hashes und genaue Werte; eigener CI-Job reproduziert Modell/ARMprüfung
und lädt die Objekte/Stackreports hoch. Aktuellen CI-Stand im eigenen Draft-PR
und im Recoverykopf nachsehen.

## Nächste einzelne Einheit

Isolierter Geräte-Testbuild mit expliziten Vektor-/Adapteranschlüssen, ohne
Scene-UI-Umschaltung. Vorher sichere Bank1-Ausführung und reservierte Bank2-
Sektoren im realen Link prüfen, BankSwap aus, exklusiver HAL-Flashbesitz,
ECC-IRQs aus und BusFault-Einstieg passend angeschlossen. Aktuelle laufende
Firmware erfüllt diesen Anschluss noch nicht.

Danach am H743: intakte/Single-/Double-ECC-Worte mit warmem/kaltem D-Cache,
MSP/PSP/FPU-Rückkehr und unverwandtem Fault prüfen; gezielte Unterbrechungen
bei Erase/Payload/Commit/Verify und Recovery des letzten gültigen Records.
Start-/Poll-/Readlatenz, Mainfortschritt und Audio-IRQ/DWT/Stack unter echten
Display-/LED-/Audiolasten messen. Kein Gerätemesswert wird aus Hostticks oder
Compilerframes abgeleitet. UI-Anschluss weiter nach Soundabnahme.

24/54 Hauptpunkte geschlossen,30 offen,11 technische Blöcke. Dieser
Softwareteil setzt keinen zusätzlichen Hauptpunkthaken.
