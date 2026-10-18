# SD47 — abschließendes Arbeits- und Übergabeinventar

Stand 2026-10-18, auf PR173/Commit
`b352dbf9799b3fa618eeb01a833ac2a776f1c64e`. Dessen
[CI39081685904/728](https://github.com/Sancte3D/AMBIENT/actions/runs/39081685904)
ist vollständig24/24 erfolgreich. Das bereits geprüfte Room-Clear-Artefakt
und sein tatsächlicher Product-Link bleiben der ARM-Bezug dieses Inventars.
Alle263 bestehenden Core-/Header-/HAL-/Startup-/Linker-/CMakeinputs sind dazu
bytegleich. Kein neuer Runtimecode, State, Buffer oder Klangparameter.

Das bisher unspezifische „übrige SD47-Inventar“ ist hiermit abgearbeitet.
Der DSP-/Main-Audio-Softwareaudit hat keinen weiteren belegten Fehler ergeben.
**Der Hauptpunkt SD47 bleibt offen:** Live-Save blockiert weiterhin Main
(SD41); echte Spitzenzeit/Stackreserve ist weiterhin ungemessen (SD48).
Diese beiden Abhängigkeiten werden nicht durch weitere Room-/Quellenprobes
oder einen Hostzeitwert ersetzt.24/54 geschlossen,30 offen,11 technische Blöcke.

## Vollständige Kriterienmatrix

| SD47-Kriterium | Stand des Product-Kandidaten | Noch erforderlicher Nachweis |
|---|---|---|
| Quellenanregung außerhalb DMA, feste Arbeitsgrenzen | Alle drei Quellen bereiten in Main vor; Pluck-Pässe und Publikation unter echter Audiofunktion geprüft | Hardwarelatenz von Onset/kaltem Cache in SD48 |
| Koeffizienten und Targets ohne Audio-Retry | Room: acht Main-expf, ein Audio-Leseversuch; Quellen: feste32-Sample-Controlrate; Nature: drei atomare Zielwörter und feste64-Sample-Controlrate | Gesamte IRQ-Zeit inklusive tatsächlicher tanf-/libm-Pfade in SD48 |
| Scheduler/Worldvorbereitung ohne unbounded Suche | Drei Slots,16 Tails, feste Pitch-/Rollenbereiche; höchstens zwei Starts pro COAST-Tick, sonst einer; Retry vertagt250ms | Kein weiterer identifizierter Softwarebefund; musikalische SD17/SD37-Hörwahl bleibt offen |
| Besitz/Output bei Main-Audio-Unterbrechung | Quellen-Prep, alte Stopps, Roomgeneration und nun Nature-Publikationen mit vollständigen Kontrollausgaben geprüft | Gerichtete C-Grenzen decken nicht jede Maschineninstruktion oder beliebige weitere Schreiber ab |
| Storage-Handoff ohne blockierende Live-Planung | Audio enthält kein Storage-I/O; vorbereitete Writer/Recovery/Adapter/Bench sind separat begrenzt | **Offen SD41:** Gerät prüfen, danach Productanschluss; Default-Save ist weiterhin synchron |
| Worst-case-Kandidaten und Speicher/Stack benannt | Schleifengrenzen, Room-Clear-Zielbytes und tatsächliche ARM-Einzelframes unten | **Offen SD48:** kompletter DMA-IRQ, UI/LED/MIDI/Storage, Deadline-Misses und realer Stack-High-water |

## Callsites, Besitzer und feste Arbeitsgrenzen

Die Angaben zählen C-Arbeit beziehungsweise feste Zustandsgrößen. Sie sind
keine gemessenen Cortex-M7-Zyklen. Ein Renderaufruf wird in Abschnitte
`1≤n≤512` zerlegt; nur die Zahl dieser Abschnitte hängt von seiner Länge ab.

| Pfad / konkrete Callsites | Kontext und Besitz | Grenze / Übergabe |
|---|---|---|
| `engine_product.c: admit`, `in_collection`, `pitch_clear`, `reap` | Main: Zulassung, tatsächliche Hz, Slots, Tails und Hooks |3 Slots/16 Tails; generierter Pitchscan45..80 höchstens36 Kandidaten (World enger), manuelles Core-Register50..69; kein Suche-bis-Erfolg. Geschachtelte Scans haben ebenfalls feste Enden |
| `bowed.c/horn.c: prepare_note`, `*_try_note_on` | Main bereitet einen freien Pendingrecord; Audio übernimmt erst publizierten Record | Je3 feste Quellenrecords, kein Steal im Productpfad; echte Prep-/Publikationsunterbrechungen siehe Quellen-Prep-Checkpoint |
| `pluck.c: pluck_note_on` | Main schreibt nur eine nach Audio-Retirement freie Saite |2 feste1024-Float-Saiten;≤8 Anregungsmoden;1024 Füllschritte plus2n Zentrier-/Normierschritte. Product manuell n≤316, Generate n≤421:≤1866 Bufferbearbeitungen pro Start; direkte öffentliche API n≤736:≤2496. Innerer Modenscan ist zusätzlich≤8, nicht mit Bufferbearbeitungen gleichsetzen |
| `engine_product.c: admit`, `engine_note_off`, `retire_context` | Main einziger Stopp-Schreiber; Audio konsumiert Besitzermasken | Zwei Main-Wort-RMWs verwerfen nur alte Stopps vor Neubelegung. Main-Exclusive-Retries bei IRQ sind keine feste Main-WCET. Kein neuer Audio-Retry |
| `engine_product.c: engine_generative_tick`, `coast_tick`, `woodland_tick`, `highlands_tick` | Main: Scores, Tickets, Fälligkeits-/Retryzeit |≤2 Vorbereitungen/COAST-Tick, sonst≤1;3 Rollen, endliche Phrasentabellen. Ablehnung setzt nächsten Versuch auf+250ms, keine sofortige Wiederholschleife |
| `engine_set_world`, `engine_set_gen_seed`, `reset_score_context` und `world_grammar.c` | Main setzt feste Scores neu; Audio behält alte physische Quellen/Hz bis Retirement |3 feste Phrasenzustände, keine Audio-Scoreinitialisierung. Gehaltene manuelle Quelle bleibt ursprünglicher Familie zugeordnet; Generate-Kontextstop100ms. Kein World-Setter löscht den Room |
| `engine_render` → `bowed_render_mix`, `horn_render_mix`, `pluck_render_mix` | Audio: Quellen-DSP, Release, Retirement | Feste Pools3/3/2, globale Product-Zulassung3 inklusive Releases/Pending. Je aktive Bowed-/Hornquelle Filterupdate alle32 Samples; pro512er Abschnitt≤16 Controlupdates. Kein Anregungsloop im Render |
| `ambient_room_set` → `accept_targets` → `ambient_room_process` | Main:8 expf und vollständige Publikation; Audio: letzter ganzer Cache/Roomstate | Ein Leseversuch,≤11 SC-Wortloads (bei laufender Publikation1), kein Spin. Pro Sample8 Feedbacklinien,4 Allpässe; beim kalten Bypass kein Tank-DSP |
| `ambient_room_clear` / Dry-null / Engine-Clear | Nur Audio, ganze Kette hinter Null | Warm60720 explizite C-memset-Zielbytes, kalt0. Gleichzeitiger Dry-/Clearabschluss maximal einmal60720 statt zweimal121440. Nicht der gesamte Engine-Clear und kein gemessener Busverkehr |
| `nature_set_amount/world/seed` → `nature_render` | Main: je ein atomarer Store; Audio: alle Filter-, Wetter-, Event- und Fadezustände | Genau3 Zielwortloads/Abschnitt. World/Seed sind unabhängige Targets, keine versprochene transaktionale Scenegruppe. Audio blendet80ms aus, übernimmt letztes Ziel, blendet2s ein; Amount glättet2s |
| `nature.c: wind_tick`, `reset_events`, Woodland-Drops | Audio | Wind2 SVF-Koeffizienten je64 Samples; höchstens1 Eventreset/Abschnitt mit2 Wasserkoeffizienten. Konservative Schranke `2+2*(ceil(n/64)+1)` =20 für512; im neuen Test höchstens18 beobachtet.≤8 Dropplätze, keine unbegrenzte Platzsuche. Idle ohne neues Ziel:3 Loads, kein Noise-/Filter-/Wetter-DSP |
| `engine_render`: Busse, Master/DC/Peak/PCM und Clear | Audio |4 explizite n-Float-Busclears (≤8192 Zielbytes), feste Besitzerprüfung16, n-Sample-Endstufe.40ms-Gate/120ms-Volume, feste Quellen-/Nature-Reinitialisierung hinter Null; mögliche libm-/Clearspitze in SD48 messen |
| `scenes_save` → `s_write` → `scenes_flash_write` | **Default Main synchron**, außerhalb Audio-IRQ | Defaultpayload≤512B,≤16 Flashwords plus1 Sektorerase. Begrenzte Datenmenge beweist keine begrenzte Main-Stallzeit; HAL-Erase/Program warten synchron. Das bleibt ein offener Produktbefund |
| Vorbereitete `scene_journal_async_step`, `scene_journal_recovery_step`, H743-Adapter und separater Bench | Separater Main-/IRQ-Vertrag, aktuell kein Default-Productanschluss | Ein IO-Callback pro Schritt, Lesen≤32B; Begin ohne IO, laufende Operation wird vor Cancel-Abschluss drainiert. Kein synchroner Gesamtscan in einem vorbereiteten Schritt; tatsächliches Silizium/ECC/IRQ noch SD41 |
| `audio_h743.c: fill_half` | DMA-Audio-IRQ |512 Stereo-PCM16-Frames =2048B D-Cache-Clean. Aktueller DWT-Scope endet vor Clipscan und Cache-Clean; kompletter IRQ ist bisher **nicht** gemessen |

Quellorte unter `firmware-c-next/src`, HAL unter `src/hal_h743`. Ein Main-
Schreiber, keine reentranten Engine-Setter aus dem Note-Hook und kein Main
innerhalb des Audiokontexts gehören zum geprüften Besitzvertrag. Ein atomarer
Wert ist kein Versprechen, dass unabhängig bediente Regler im selben Sample
wirken.64/512-Vergleiche benutzen dieselben Befehle am selben tatsächlichen
Audioframe; beliebige veränderte Controlraten werden nicht als identisch behauptet.

## Abschließende Nature-Gegenprobe

Die echte `nature.c` wird zweimal kompiliert. Das unveränderte Originalobjekt
liefert die Kontrolle; nur das zweite Hostobjekt beobachtet atomare Stores/
Loads und reale `dsp_svf_set`-Aufrufe. Es gibt weder einen Ersatzsynth noch
Beobachter im Firmwarecode. Vor/nach genau einem realen Main-Store läuft eine
vollständige64er beziehungsweise512er Audiohälfte; Main läuft nie in Audio.

120 Fälle: jede der3 Ausgangswelten ×(3 Worldwerte +4 Seedwerte +3 Amountwerte)
×vor/nach Store ×64/512. Seed0/1/Default/UINT32_MAX, Amount0/0,2/1; alle neun
Worldpaare einschließlich idempotenter Wahl. Drei Sekunden echter Warm-up
und4096 Folgeframes pro Fall. Ganze Stereo-Floatausgaben entsprechen dem
unbeobachteten Modul mit dem Auftrag vor beziehungsweise nach derselben Hälfte.

Zusätzlich je World ein38-s-Verlauf:18 World-/Seed-/Amount-Bursts im Abstand
221 Samples, ungültige Inputs, vollständiges Ausklingen bei Amount0, exakt
stille Endphase, audioeigener Clear, neue letzte World/Seed0 und Neustart.
Ganze Ausgabe64/512 bytegleich; beobachtetes512er Objekt ebenso bytegleich
zum Original. Insgesamt10580880 Stereo-Frames vollständig verglichen
(Warm-up zusätzlich ausgeführt, nicht mitgezählt). Keine WAV/Hörabnahme.
C11/O2/Wall/Wextra/Werror und ASan/UBSan bestanden. Pro beobachtetem Render
genau3 Loads, maximal18 beobachtete SVF-Koeffizientensetzungen; die analytische
konservative512er Schranke20 wird dadurch nicht auf18 herabgesetzt.

Reproduktion: im `firmware-c-next`
`python3 tools/review_nature_handoff.py build-nature-handoff --sanitize`.
Der normale Hostrunner verwendet `--fixture-only`, ohne alte Gitobjekte zu
benötigen. Der eigene CI-Job prüft zusätzlich alle263 bestehenden Inputs
gegen den exakten PR173-Commit. Daten:
[SD47_NATURE_HANDOFF_METRICS.json](SD47_NATURE_HANDOFF_METRICS.json).

## Vorhandene Belege und Grenzen

| Nachweis | Status dieses Inventars |
|---|---|
| [Roomgeneration](SD47_ROOM_HANDOFF_CHECKPOINT.md) |335 Fälle,5977773 Assertions; kein weiterer Roomparameteraudit erforderlich |
| [Quellenbesitzer](SD47_SOURCE_HANDOFF_CHECKPOINT.md) |645 Fälle,48809 Assertions/1395 Unterbrechungen; alte Stopps vor neuer Vorbereitung entfernt |
| [Quellenvorbereitung](SD47_SOURCE_PREPARATION_CHECKPOINT.md) |4605 Fälle/5082 Audiohalb-Unterbrechungen, einschließlich jeder Pluck-Passiteration |
| [Room-Clear-Arbeit](SD47_ROOM_CLEAR_WORK_CHECKPOINT.md) |36 reale Clear-/Mute-Fälle und24 ganze normale12-s-Product-Paare; PR173 vollständig24/24 grün |
| [Worldübergaben](SD37_WORLD_TRANSITION_CHECKPOINT.md) |Bestehender gerichteter54-Szenarien-Nachweis/64–512, spätere Quellenstop-Korrektur zusätzlich PR171; keine neuen220,8min Render in diesem Inventar behauptet |
| [Storagevorbereitung](SD41_H743_SCENE_BENCH_CHECKPOINT.md) |Writer/Recovery/Adapter/Bench vorhanden; Defaultanschluss und Geräteabnahme bleiben offen |

Die frühere ARM-Product-BIN bleibt196844B, SHA256
`7ba0c2db6914cdc581fcfc3ad72a9adbeb5712835ef9f5a5ef3dd1276f77c285`;
ihre CI-/lokale Gleichheit wurde bereits bei PR173 geprüft. Hier keine neue
gepaarte ARM-Optimierung oder Laufzeitmessung. Aus dessen tatsächlichem
GCC13.2.1-Audit: eigene Compilerframes Room-Process160B, Nature136B,
SceneSave104B, Engine-Render88B, GenerateTick80B, Fill-half16B, Room-Clear16B.
Kein transitive-Stack-/IRQ-/FPU-/libm-High-water aus diesen Einzelframes.

## Endliche Restliste für SD47

1. **SD41:** den vorbereiteten H743-Storagepfad am Gerät abnehmen und danach
   den bestätigten, schrittweisen Save-/Recovery-Handoff in den Product-Main
   integrieren. Journal bleibt bis dahin OFF, Live-Save synchron. Allein ein
   neues Flag oder der isolierte Bench erfüllt diesen Punkt nicht.
2. **SD48:** echte512er DMA-Hälften einschließlich Clipscan/Cache-Clean,
   kalter Onsets, maximaler Release-/Worldwechsel, Room-/Nature-Reset,
   schnell bedienter Targets und UI/LED/MIDI/Storage messen. Ziel bleibt
   `peak_load<0.60`, null Misses, belegte Stackreserve in jedem Kontext.

Die SD17-/SD37-Musik-/Übergangshörwahl bleibt daneben erhalten. Keine weitere
ungeklärte SD47-DSP-Unteraufgabe aus dem Wort „Inventar“ ableiten. Als nächste
unabhängige offene Software-/Prüfeinheit nach aktueller Reihenfolge SD11
an seinen tatsächlichen Quellen-/Teilton-/Registerkriterien prüfen; vorhandene
Nachweise zuerst lesen und keine Hörentscheidung aus Messdaten ableiten.
