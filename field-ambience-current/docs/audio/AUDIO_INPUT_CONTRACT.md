# Audio input contract — 2026-10-06

## KERNURTEIL

Ungültige Floats konnten SHAPE, Mixer und zwischengespeicherte FX-Ziele
vergiften oder auf Defaults zurücksetzen. Der Audiopfad verwirft jetzt NaN
und ±Inf an seinen Parametergrenzen und bewahrt den letzten gültigen Zielwert.
Das betrifft Klangzustand und musikalische Zuverlässigkeit, nicht nur Crashschutz.

## FUNDAMENTAL FALSCH

Eine abgewiesene Eingabe darf keinen anderen Klangzustand erzeugen. Außerdem
darf ein erfolgreicher besitzender Pluck-Start keine 20-Hz-Note eintragen,
während sein Delay in Wirklichkeit auf 60 Hz begrenzt wird.

## NOCH NICHT SELBSTVERSTÄNDLICH

Die alten manuellen/Oneshot-Pfade und sämtliche Produktparameterbereiche werden
noch vereinheitlicht. Dieses Paket schließt numerische Eingaben und den
besitzenden Quellenbereich, keine gesamte Palette oder Klangfreigabe.

## LOCKED

Gültige SHAPE-/Gain-/FX-Eingaben behalten ihre bisherigen Kennlinien.
Ungültige Noten werden vor Besitz/Hook/Historie abgewiesen. Bestehende
klingende Stimmen und deren Release werden dabei nicht berührt.

## REMOVE / MERGE / REDESIGN

- Sämtliche 24 floatbasierten `engine_set_*`-Parametergrenzen bewahren bei
  NaN/Inf den Zustand, einschließlich Native-Macro-Weiterleitung.
- Acht direkte Master-FX-Setter sowie SHAPE, Body, Nature und Texture
  verhalten sich ebenso. Kein zwischengespeicherter NaN wird weiter gepusht.
- Besitzt ein Pluck einen Source-Owner, gilt **60–8.000 Hz**, außerhalb wird
  abgewiesen; Legacy-`pluck_note` bleibt ausdrücklich ein Kompatibilitätsweg.
- Allgemeiner manueller Engine-Start begrenzt gültige Frequenzen auf 20–8.000 Hz.
  Weltzulassung prüft den Pluck-Floor vor einer Humanisierungsentscheidung.
- Source-API-Bereich und das spätere engere musikalische World-Register sind
  getrennte Verträge; 8.000 Hz ist keine Ambient-Presetempfehlung.

## BESTE VERSION

Eine gültige Kontrolle bewegt den Ton musikalisch. Eine ungültige Kontrolle
ändert nichts. Planung, tatsächlicher Quellenstart und gehörte Historie
beschreiben dieselbe Frequenz. Verbleibende Bereiche und Besitzer werden im
[Sounddesign-Masterplan](AMBIENT_SOUND_DESIGN_TODO.md), SD07/SD08/SD32, geschlossen.

## TEST AM GERÄT

Normale und korrupte Control-Pakete bei laufenden Releases einspeisen. Keine
Pitch-/Pegeländerung oder Audiounterbrechung; DWT/Stack im endgültigen Build
prüfen. Ohne Hardware bleibt diese Abnahme offen.

## Softwareprüfung

`test_reverb_engine.c` vergleicht zwei tatsächliche 2-s-PCM-Durchläufe durch
Quelle, Mixer und Masterraum: gültige Referenz gegen identische Eingaben mit
NaN/±Inf-Injektion in 36 direkte Parametergrenzen und dem Native-Param-Adapter.
Klang ist nicht still; invalid-control PCM bleibt bitidentisch zur Referenz.
`test_pluck_tone.c` prüft untere/obere Grenzverletzung und nichtfinite Frequenzen:
kein Start, kein belegter Slot; akzeptierte obere Source-Eingaben bleiben
endlich und nicht still. Diese Diagnosen exportieren keine Hördateien.
