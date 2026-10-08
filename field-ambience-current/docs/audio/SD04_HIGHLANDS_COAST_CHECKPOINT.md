# SD04 — HIGHLANDS trocken gegen COAST, 8. Oktober 2026

**Softwarevorbereitung geprüft; KEEP/REMOVE-Hörentscheidung offen.**
Weiterhin 24 geschlossene und 30 offene Gesamtaufgaben. COAST ist der
bestehende Kandidat, dessen endgültige Quellenwahl aus SD02 ebenfalls offen ist.

Basis: PR141, `ba0de824055d2f67ea7dc7f4981fb83b2ccd0c59`.
[CI37611452027](https://github.com/Sancte3D/AMBIENT/actions/runs/37611452027)
ist inzwischen vollständig bestanden. Dieser Durchlauf bearbeitet nur SD04.

## Vergleich

`review_highlands_coast.py` kompiliert den bestehenden echten Product-Renderer
und unveränderte C-Quellen mit `-O2 -Wall -Wextra -Werror`. Zwei trockene
Registerdurchläufe verwenden dieselben Noten D3/D4/A4, Volume 0,6, Velocity
0,75, Color/Attack/Release 0,5 und Seed 1234. Room ist Dry, Nature aus,
Generate aus. Kein Quellen-, Parameter- oder Firmwareumbau.

Die vollständigen Dateien sind je 27 s lang: Einsätze bei 0/9/18 s,
Key-up bei 3/12/21 s und Clear bei 8,75/17,75/26,75 s. Die CSV-Traces sind
bytegleich. Startbestätigungen werden im vorhandenen Renderer nach dem ersten
512-Frame-Block beobachtet; das ist keine zusätzliche Audioeinsatzverzögerung.
Der Clear-Abschluss wird ausdrücklich nicht als natürlicher vollständiger
Release-Nachweis gewertet. SD03 hatte dafür eigene ungeclearte Gegenproben.

## Befund und Grenzen

- Beide tatsächlichen Render: jeweils drei bestätigte Starts und Key-ups,
  null Nonfinite- und Limiter-Ereignisse, korrektes PCM16/Stereo/44,1-kHz-Format.
- RAW: COAST −31,2 LUFS / −23,9 dBFS True Peak;
  HIGHLANDS −30,7 LUFS / −27,0 dBFS True Peak.
- Vollständige LISTEN-Dateien: je −26 LUFS, mit konstant +5,2 dB für COAST
  beziehungsweise +4,7 dB für HIGHLANDS. Keine AGC/Kompression.
- Monoenergie und DC geprüft. Grundton-/Oberton-/Suboktavbänder und
  Spektralschwerpunkt im 1–2-s-Sustainfenster je Register sind im Manifest.
  Die 1-s-Hann-Fenster und ±8-Hz-Bänder beschreiben Energie, keine allgemeine
  Tonhöhenmessung und keine Wahrnehmungsfreigabe.
- Unterschiedliche PCM-Hashes bei identischer Ansteuerung belegen nur
  unterschiedliche Ausgaben; Wärme und eigene musikalische Rolle bleiben Hörfragen.
- Aktuelles Product-Horn hat `driftInc=0`; keine wiederkehrende 0,9-Hz-Wah-Welle.
  Keine Suboktave oder fester 950-Hz-Formant wurde wieder eingeführt.
  Verbleibende 90-ms-Onset-Luft und SHAPE-Grenzen gehören getrennt zu SD05.

Quellen, Header, Prüftool und Renderer sind per SHA256 festgehalten. Neues
Werkzeug und Format-/Pegel-/Eventprüfungen bestehen lokal; Workflow und Diff
werden geprüft. Der unveränderte Firmwarekern ist über PR141-CI belegt.
Keine neue Geräte-, CPU-, Stack- oder Hörabnahme wird behauptet.

## Zuerst hören

`SD04_COAST_HIGHLANDS_AB_27s.wav` enthält sechs Viersekunden-Ausschnitte:

| Zeit | Quelle / Register |
|---|---|
| 0–4 s | COAST D3 |
| 4,6–8,6 s | HIGHLANDS D3 |
| 9,2–13,2 s | COAST D4 |
| 13,8–17,8 s | HIGHLANDS D4 |
| 18,4–22,4 s | COAST A4 |
| 23–27 s | HIGHLANDS A4 |

Je 0,6 s Pause, Key-up nach drei Sekunden. Jeder Ausschnitt erhält einen
festen dokumentierten Gain auf −26 LUFS und einen 100-ms-Schlussfade als
Ausschnittbearbeitung. Keine Behauptung eines natürlichen Release-Endes.
Die Einzeldateien erhalten die längeren Registerverläufe; RAW und LISTEN,
Ereignistraces, README und Messwerte liegen im Paket.

**KEEP:** HIGHLANDS trägt als eigener warmer Tonkörper mit anderer Artikulation,
ohne Signal-/Sirenen-/hohlen Röhrencharakter, bei D3/D4/A4 und in Mono.
**Bei Scheitern:** Dateiname und Zeitstelle, trockene Ursache benennen; höchstens
eine begründete neue Horn-Fassung vergleichen. Bleibt sie unpassend, REMOVE.
Zwei starke Welten sind zulässig. Keine Umbenennung oder zusätzliche Schicht.

SD04 bleibt bis zu diesem Nutzerurteil offen. SD05 hängt von KEEP oder einem
separat freigegebenen Ersatz ab; ohne diese Auswahl wird keine Horn-Revision
als Klangfreigabe gebaut. Der nächste Punkt ist SD05. Eine Produktüberarbeitung setzt die SD04-
Quellenentscheidung voraus; ein getrennter Diagnosevergleich des bestehenden
Kandidaten kann ohne diese Freigabe vorbereitet werden.
