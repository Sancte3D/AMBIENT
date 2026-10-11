# SD33 — COAST Activity kalibrieren

Stand 2026-10-11, Product Sound Candidate Spec0.9. Eine einzelne Arbeitseinheit:
die in PR155 fast wirkungslose COAST-Kennlinie. SD33 bleibt als ganzer Punkt offen.

## Verhalten

Vorher wurden die geplanten Activity-Abstände schon von den tatsächlichen
Außenreleases verbraucht. COAST erhielt bei0/0,5/1 praktisch dieselben Einsätze.
Jetzt folgt auf das tatsächliche Ende beider Außenbesitzer eine einmal geplante
Wartezeit6/3/0 s. Der gemeinsame Owner15 bleibt ohne Wiederanschlag stehen.
Nach dem Ende aller drei Quellen wartet die nächste Phrase10/5,5/1 s.
Der bestehende Entwicklungsfaktor1/1,08/1,12/0,96 wirkt auch auf diese Zeiten.
Früheste bisherige Deadlines und Pitch-/Room-History bleiben wirksam.

Laufende Wartezeiten und bestätigte Holds werden durch Activity nicht umgestellt.
Die nächste Wartezeit benutzt den dann aktuellen Wert. Keine neue Stimme,
Quellenänderung oder Umgehung des bisherigen Besitzer-/Tailvertrags.
SCN7-Schema und gespeicherter Default0,5 sind unverändert; derselbe COAST-Wert
hat ab Spec0.9 die neue Zeitsemantik. Reference bleibt Default, Journal OFF.

## Tatsächliche Engine

Gleicher Renderer, gleicher Seed, D Major/ET, Color/Attack/Release/Room0,5,
Nature0, Volume0,6. Exakter Baselinecommit
`4de325a0d4c6255909801e02e0cfe62889a6777e`, Tree
`f515c8c68420ceec81d4ff26f80b3e40c8b8a69b`.
Nur engine_product.c, world_grammar.c und dessen Header ändern Compilationinputs.

| Seed | Activity | Baseline Starts bis568 s | Kalibriert | Erste erinnerte Rückkehr vorher / jetzt |
|---|---|---:|---:|---|
|1234|0|79|56|178,31 /248,66 s|
|1234|0,5|79|64|178,23 /215,13 s|
|1234|1|79|77|178,38 /181,58 s|
|42|0|79|56|179,31 /249,66 s|
|42|0,5|79|64|178,99 /215,89 s|
|42|1|79|77|178,91 /182,11 s|

Je Seed echte Dichte5,92/6,76/8,13 Starts pro Minute. Vier Phrasenarten und
vollständig erinnerte Rückkehr bleiben bei allen Werten erreichbar.
Alle gemeinsamen gehörten Präfixe stimmen in Pitch/Rolle/Velocity/Memory
überein; abgeschlossene Außenholds unterscheiden sich höchstens um10 ms.
Owner15 darf mit den späteren Partnern länger stehen. Echte DSP-Masken belegen
zusätzliche Waits nach Retirement und den erhaltenen gemeinsamen Ton.
Das erste Renderblock-Ack folgt auf Main-Zulassung beim nächsten10-ms-Tick;
die Retirementprüfung schließt die neu vorbereitete DSP-Quelle ausdrücklich aus.

12 feste600-s-Fälle plus zwei Live-Fälle, jeweils64/512 bytegleiches PCM,
komplette Note-on/off- und Statusdaten. Livewerte bei10/32/64/96/128/160 s:
0/1/0,5/0/1/0,5; Setter löst keinen sofortigen Start aus, Tonfolge/Holds bleiben.
Vier weitere112-s-Fälle je Baseline/Kandidat und64/512 beweisen WOODLAND und
HIGHLANDS bei beiden Seeds vollständig bytegleich in PCM/Ereignissen/Status.
Insgesamt44 volle Renderläufe,309,87 min PCM. Große Performances bleiben unter
OS/tmp; vor dem Export werden Länge und sämtliche PCM-/Tracehashes erneut geprüft.

Maximal drei tatsächliche Quellen einschließlich Releases. NaN/Limiter0,
Raw True Peak höchstens−20,0 dBFS, Mono-Energieverhältnis mindestens0,96987,
DC höchstens1,23e−8. Stop568 s, alle Besitzer spätestens577,21 s frei,
letzte Sekunde der600-s-Aufnahmen exakt still; keine künstliche Stop-Blende.
576 Pure-Cases über acht Phrasen/33792 bestätigte Intents,1536 HIGHLANDS-
Pausenintents sowie COAST-Wait-Gültigkeit, unveränderte Memory/RNG,
einmalige Planung, Neustart und exakt0 als Timerwrap-Deadline bestehen.

## Eine kurze Hördatei

`AMBIENT_COAST_Activity_Calibrated_0_50_100_38p5s.wav`, Stereo16-bit/44,1 kHz:

| Ausgabeposition | Activity | Echter Ausschnitt |
|---|---|---|
|0–12,5 s|0|8–20,5 s|
|13–25,5 s|0,5|8–20,5 s|
|26–38,5 s|1|8–20,5 s|

Die gleiche erste Phrase bei Seed1234 zeigt die erste tatsächliche Übergabe
später/früher, während der Akkordton weiterklingt. Ein gemeinsames festes Gain
+9,9 dB, zwei0,5-s-Vergleichslücken und40-ms-Schnittkanten. Kein AGC oder
einzelnes Hochziehen des langsamen Verlaufs. Gesamt−23,3 LUFS/−10,7 dBTP.
SHA256 `a79641b9c820dc7b139bdee4ec1751d2ea792d574516624237c1ab739a54925f`.
Hörurteil und musikalischer Nutzen bleiben offen.

## Host und H743

Der vollständige ununterbrochene firmware-c-next-Hostrunner besteht mit dem
neuen Core, einschließlich48 COAST-Phrasen, Makrogrenzen,90-min-Longform,
Übergaben, Sceneverhalten und Reference-Vertrag. Beide echten H743-Product-
Releasebuilds verwenden dieselben unabhängig SHA256-geprüften Ubuntu-Noble-
Pakete GCC13.2.1/Newlib4.4; Baseline reproduziert das frühere ELF exakt.

| Beleg | Baseline | Kalibriert |
|---|---:|---:|
|Bank1-Loadimage|195928 B|196384 B (+456 B)|
|D1 RAM|65664 B|65664 B|
|D2 RAM|60808 B|60808 B|
|reservierter DTCM-Stack|16384 B|16384 B|
|COAST-Phrasenzustand|52 B|56 B|
|individuelle Core-Compilerframes|182|183|
|größter Coreframe|192 B|192 B|

Room-Tank55352 B und Diffusion5296 B bleiben intern in D2, DMA4096 B intern
in D1/32-B-aligned. Reference-/Archiv-DSP ausgeschlossen, Image bleibt in Bank1.
Compilerframes sind kein Hardware-CPU-/Stack-Highwaternachweis; SD48 bleibt offen.
ELF vorher `9dff333c483d849357250246b755dd466278802f982504edd6ef194ad84c039a`,
jetzt `d99fb5c0b0536c2e376fe2447a25542287356f93f41cd143f5a1f675f4d18105`.

CI trennt historische Spec0.8-Hörpakete mit unveränderten Hashassertions auf
PR155 vom neuen aktuellen COAST-Audit. Alle normalen Host-/H743-Builds prüfen
den aktuellen Branch. Keine Retargets oder Merges.

Messdaten: [SD33_COAST_ACTIVITY_METRICS.json](SD33_COAST_ACTIVITY_METRICS.json),
[SD33_COAST_ACTIVITY_ARM_METRICS.json](SD33_COAST_ACTIVITY_ARM_METRICS.json).
Nächste einzelne Softwarevorbereitung: SD33 Color0/0,5/1 in den tatsächlichen
musikalischen Worlds; Quellen-, Room-, Nature- und Langzeithörwahl bleiben offen.
Kein SD33-Haken, keine UI-/Gerätefreigabe.
