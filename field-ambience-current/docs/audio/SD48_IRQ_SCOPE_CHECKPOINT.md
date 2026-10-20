# SD48 — getrennte Render- und DMA-C-Messfenster

Stand2026-10-20. Vorbereitete Messinstrumentierung, keine Geräteabnahme.
Basis PR180/de732f42d4c1973a82f256d9cdec129394dba4b1, CI740 vollständig29/29
erfolgreich. SD48 bleibt offen;25/54 Aufgaben erledigt,29 offen.

Der alte Profiler umfasst weiterhin ausschließlich die Renderfunktion.
Ein separater56-B-Profiler beginnt jetzt im DMA1-Stream0-C-Handler vor dem
HAL-Aufruf und endet nach dessen Rückkehr. Damit umfasst er HAL-Dispatch,
Refill einschließlich alter Renderstatistik, echten Clipscan und D-Cache-Clean.
Beide werden vor Aktivierung des Pumps zurückgesetzt. Ein IRQ behält immer
das Budget einer512-Frame-Hälfte (5572789 Zyklen bei480MHz/44100Hz), auch wenn
beide Halbflags im selben Handler bedient werden. Kein verdoppeltes Budget
versteckt diesen Rückstand. Die Diagnostics zeigen Render-/IRQ-C-Prozent,
jeweilige maximale Zyklen und Budgetüberschreitungen getrennt; bestehende
Clips/Voices/Batterie/PSRAM bleiben sichtbar und passen in320×170.

Das ist ausdrücklich **keine vollständige Exception-Zeitmessung**: Exception-
Entry/Exit, Compilerprolog/-epilog außerhalb der Clockreads, abschließende
Profilbuchhaltung und Verzögerung vor Handlerbeginn fehlen. Zwischen den
Messgrenzen liegende Präemption gehört zur gemessenen Zeit. Overrun heißt
Messfenster≥Halbperiodenbudget, kein nachgewiesener Hardware-Underrun. Live-
Diagnosefelder sind Einzelbeobachtungen, kein atomarer Mehrfeldsnapshot.
Der zusätzliche Profiler verursacht selbst Arbeit; reales Worst-case-/IRQ-/
Stack-/Dispatch-/DMAfehler- und Ausgangsverhalten bleibt SD48/SD49.

## Geprüfter Softwarevertrag

Die Hostfixture kompiliert unverändert extrahierte tatsächliche Reset-,
IRQ-, Refill-/Callback- und Diagnosefunktionen. Nur Hardware/DWT-Kosten sind
injiziert; der Clipscan selbst bleibt echter Code, der Cachecall wird beobachtet.
64 Kontofälle:8 synthetische Renderkosten ×0/1/2 ausstehende Hälften ×normale/
wrappende Uhr. Renderer-/IRQ-Zähler, unveränderte Cliperkennung, volle Halb-
periodenbudgets sowie exakte Scan-/Cacheaufrufe bestehen. Zusätzlich eine
gerichtete Gegenprobe: Renderer unter Budget, Postwork darüber; nur der neue
äußere Zähler erkennt die Überschreitung. Reset leert beide Profiler.

Zwölf ganze Product-PCMgegenproben:3 Worlds×64/512 Rendercaps×DWTwrap normal/
wrap, Nature0,3/Room0,5, tatsächliche Quelle220Hz; je32 tatsächliche512-Frame-
Refills über beide HAL-Callbacks.196608 ganze Stereo-Frames pro Prüfmodus
bytegleich zur direkten unveränderten Engine. Kein NaN/Limiter.33 tatsächliche
Diagnosezeilen einschließlich Header/PSRAMzuständen passen auch bei breiten
Zählertexten in das Panel. Keine exportierten WAVs oder Hörbehauptung.

Native C11/O2/Wall/Wextra/Werror und ASan/UBSan bestehen mit identischen Daten:
[SD48_IRQ_SCOPE_METRICS.json](SD48_IRQ_SCOPE_METRICS.json).
Der unveränderte bestehende Profiler-Test besteht zusätzlich17 Checks.
Product und Reference vollständig für Cortex-M7 gelinkt; Product-Linkaudit
prüft ausgeschlossene Legacy-DSPpfade, D2-Raum und ausgerichtete D1-DMA.
Product BIN200132B, SHA256
`c07844a1f18ca60b5de84f6d91fbe38ce482875d4cff8fcb65cd2b7a6a7b73a5`;
Reference BIN260356B. Neuer Profiler im echten ARM-ELF56B, keine zusätzlichen
Audiobuffer. Keine CPU-/Stackreserve aus diesen Größen ableiten.

Vier bestehende HAL-Dateien ändern sich;261 andere265-basierte Firmwareinputs
bleiben bytegleich PR180. Engine/Quellen/Grammatiken/Parameter/Scene/Audioformat/
DMAgröße/Linker und UI-Ruhemodus bleiben unverändert. Die gehashte SD51-Datei
ist der frühere SD52-Checkpoint; aktuelle Instrumentänderungen sind hier mit
eigenen Hashes versioniert. Reference Default, Product Kandidat, Journal OFF,
Live-Save synchron. Kein Flashen, Merge oder Retargeting.

Reproduktion: `python3 tools/review_h743_irq_scope.py <output> --sanitize`.
Normaler Hostrunner enthält die aktuelle Fixture ohne Gitbasisabhängigkeit;
eigener CI-Job prüft Sanitizer und unveränderte Inputs. Der strikte SD52-Job
bleibt unverändert auf dem grünen PR180-Head als archivierter Beleg; aktuelle
Host-/ARM-Jobs laufen auf dem neuen Head. Keine erneute ganze lange Hostsuite
lokal für unveränderten DSP, sondern gezielte reale HAL-/PCMprüfungen und ARM.

Nächste Grenze bleibt echte Geräteprüfung: volle Exception/Dispatchlatenz,
Worst-case inklusive Display/LED/MIDI/Storage, Deadline-/DMAfehler und realer
Stack-High-water. Die getrennten Werte schaffen dafür eine zusätzliche
Beobachtung; sie ersetzen die Abnahme nicht.
