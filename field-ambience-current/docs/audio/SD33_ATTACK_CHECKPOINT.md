# SD33 — Attack im tatsächlichen musikalischen Ensemble

Stand2026-10-12. Eine einzelne Prüf-/Vorbereitungseinheit, Release bleibt0,5.
Firmware und Spec0.10 sind unverändert, kein neuer Soundumbau oder SD33-Hörhaken.
Exakter PR158 `1f21ade3c78acc2a1c513eefaa4fa1ba1561dd95`, Tree
`1f7d566ab56aee640d60beefb54b06769fbf74fa`.
251 Core-/Header-/Startup-/Linker-/CMakeinputs vor/nach Prüfung bytegleich.
PR158 CI38284802990/run691 wurde frisch completed/success geprüft.
Reference bleibt Default, Product Kandidat, Journal OFF, SCN7 unverändert.

## Aktueller Parametervertrag

Attack0/0,5/1 wird über0,35+0,30*Attack an SHAPE übertragen; tatsächlich
gelesener Zeitfaktor0,5358867/1/1,8660657. Diese Werte skalieren die bestehende
Quellenartikulation und werden beim Vorbereiten eines neuen Tons aufgenommen.
Ein bereits vorbereiteter oder gehaltener Ton bekommt seine Envelope nicht
rückwirkend umgestellt. Release bleibt getrennt und unverändert beim Faktor1.
Gemessener normalisierter Onset-RMS: erste200ms geteilt durch Körper1,65–1,95s,
beides echtes ungefadetes Stereo-PCM, gleicher Anfang/Seed/Notenplan.
Das ist kein T10/T90-Messwert und keine allgemeine Beruhigungs-/Hörfreigabe.

| World | Nominale Quellen-Attack bei0 /0,5 /1 | Onset-/Körper-RMS bei Seed1234 |
|---|---|---|
|COAST|0.161/0.300/0.560s|0.612/0.347/0.186|
|WOODLAND|0.429/0.800/1.493s|0.278/0.092/0.028|
|HIGHLANDS|0.241/0.450/0.840s|0.331/0.187/0.103|

Beide Seeds1234/42 bestätigen strikt abnehmende frühe Energie bei größerem
Attackwert. WOODLANDs Minimum ist bereits ein weicher≈429-ms-Einsatz;
seine lange Schwingung, Anregung und Releases werden für diesen Vergleich erhalten.

## Tatsächliche vollständige Engine

18 feste112-s-Fälle (drei Worlds ×zwei Seeds ×drei Attackwerte) plus drei
Livefälle, jeder64/512:42 ganze Render/78,4min PCM. Sechs volle Defaultkontrollen
mit dem bestehenden normalen Renderer ergeben48 WAV-Render/89,6min PCM.
Die sechs Default-PCM/Events/gemeinsamen Statusdaten entsprechen ihm exakt;
neues Statusformat enthält zusätzlich Memoryreturns. Alle ganzen Kandidaten
sind64/512 bytegleich in PCM/Ereignissen/Status. Alle Attackwerte und Livefälle
haben vollständige identische Noten/Hz/Velocity/Holds/Owner/Note-on-off/Phrasen/
Memory/Zulassung/Retirement-/Übergabedaten. Keine nur passende Hörschnipselprüfung.
Live initial0,5, bei10/20/32/48/64/72s auf0/1/0,5/0/1/0,5. Setter keine Sofortnote.
Quellen3/2/3 inklusive Releases, NaN/Limiter0, Raw True Peak≤−20,4dBFS,
Mono≥0,93175, DC≤1,30e−8. Stop80s, spätestens88,84s alle Besitzer frei,
letzte Sekunde aller112-s-Aufnahmen exakt still ohne künstliches Endfade.
Maximaler benachbarter PCM-Sprung um Livewechsel0,02079; keine Klick-Hörabnahme.

Separater echter Product-PCM-Vertrag: zwölf vorbereitete/held Livewechsel,
neun zukünftige Startfälle64/512, sechs Clampfälle und ungültige Eingaben.
Änderung nach Vorbereitung vor dem ersten Sample sowie während des gehaltenen
Tons bleibt komplette vier Sekunden PCM exakt; Release1 und Owner bleiben.
Zusätzlich bestehender Product-Vertragscheck mit12min tatsächlicher Musik,
Block-/Invalid-Input-Invarianz, Besitz/Kapazität/Storno/Tuning-Tails bestanden.
Kein neuer ganzer Hostrunner oder ARM-Build behauptet: alle251 Coreinputs
sind exakt PR158, dessen voller Host/H743-Beleg und grünes CI bleiben maßgeblich.
Keine CPU-/Runtime-/ISR-Stack-/Hardware-/Hörabnahme aus diesen Tests.

## Drei kurze WAVs

Je38,5s: derselbe tatsächliche Anfang0–12,5s bei Attack0/0,5/1.
Ausgabepositionen0–12,5s /13–25,5s /26–38,5s. Ein Gain für alle drei Teile je
World, kein einzelnes Normalisieren oder AGC. Der tatsächliche Beginn ist
ungefadet, startet bereits exakt still und bewahrt die gemessene Attackform.
Nur die Schnittenden bekommen40ms Fade; zwei0,5-s-Vergleichslücken.

| Datei | Gemeinsames Gain | LUFS /dBTP |
|---|---:|---|
|AMBIENT_COAST_Attack_0_50_100_38p5s.wav|+10,0dB|−23,2 /−12,3|
|AMBIENT_WOODLAND_Attack_0_50_100_38p5s.wav|+8,5dB|−23,6 /−13,2|
|AMBIENT_HIGHLANDS_Attack_0_50_100_38p5s.wav|+6,7dB|−23,1 /−15,6|

RAWs ausschließlich OS/tmp, nach versiegelter Prüfung entfernt. Exportiert
werden drei WAVs, vollständige Status-/Ereignis-CSVs und JSON. Jede Aufnahme
und alle Quellen-/Auditinputs werden nach dem ganzen Durchlauf erneut geprüft.
Aktuelle Attack-CI reproduziert diese Tools; vorherige WOODLAND-Colorprüfung
ist zur Wahrung ihres exakten Checkpoints jetzt auf PR158 gepinnt.
Alle Daten/Hashes: [SD33_ATTACK_METRICS.json](SD33_ATTACK_METRICS.json).
Nächste einzelne Arbeitseinheit: Release0/0,5/1 getrennt im Ensemble prüfen.
