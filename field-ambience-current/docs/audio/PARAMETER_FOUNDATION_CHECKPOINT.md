# Parameterverwaltung und Loop-Archiv, 28.10.2026

Fortsetzung: Die hier ausdrücklich offenen Feinparameter und die große
Geräte-Speicheranbindung sind nun im separaten
[Effekt-/Session-Kandidaten](SOUND_SESSION_CHECKPOINT.md) implementiert.
Dieser Text bleibt der historische erste Abschnitt von PR191.

Auftrag: technisch ausbauen, Encoderbelegung ausdrücklich erst am Ende.
Basis PR190, da9acce4f1e50b4afa7921c4a678bcfe16e25640. Michalis bewertet die
neuen World-Klänge als besser; dies ist keine pauschale Effekt-/Gerätefreigabe.

Erster Implementierungsabschnitt: typisierter gemeinsamer Effektzustand mit
Main-only Get/Apply-API, vollständiger Ablehnung nichtendlicher Werte und
begrenzten Zielwerten. Die bestehende Bulk-API benutzt denselben Zustand.
Audio übernimmt fünf Makros, Freeze und Raummodell als zusammengehörigen
Snapshot. Bei unterbrochener Veröffentlichung behält es den letzten ganzen
Zustand, ohne Spins oder IRQ-Sperren. Auch die Raumwahl verwendet diesen
Snapshot. Clear löscht weiterhin History, ohne Parameterwerte zu verlieren.

Loops lassen sich unabhängig vom Speicherbackend als GLP1 exportieren und
importieren: explizites Little Endian, Version, Länge, Ereignisanzahl und
FNV-Prüfsumme. Abgebrochene Aufnahmen werden nicht exportiert. Import prüft
das komplette Archiv einschließlich Zeitreihenfolge, Cell/Register/Velocity
und Grenzen, bevor vorhandene Wiedergabebesitzer losgelassen werden. Erfolg
lädt eine gestoppte Aufnahme und startet keine Noten. Fehler lassen die
bisherige Aufnahme und Wiedergabe unverändert. FNV erkennt Beschädigung und
ist ausdrücklich keine kryptographische Authentifizierung.

Die Encoder-/Menübelegung und World-DSP bleiben unverändert. Dies ist noch
keine fertige Geräte-Persistenz: GLP1 benötigt maximal 1040 Bytes, während
das bestehende SCN7-/Journal-Payload nur 512 Bytes zulässt. Eine Integration
muss deshalb ein eigenes ausreichend großes, ausfallsicheres Speicherformat
mit Migration erhalten. Den alten Scene-Puffer einfach zu vergrößern würde
die vorhandenen Flash-/Journal-Annahmen verletzen. Neue Zeit-/Feedback-/
Grain-/Phaser-Feinparameter und deren komplette Speichereinbindung bleiben
weitere technische Abschnitte, nicht behauptete fertige Funktionen.

test_parameter_foundation prüft tatsächliche Engine-Parameter, Ablehnung von
NaN ohne Teiländerung, Clamping, unterbrochene FX-Veröffentlichung, Archive-
Roundtrip und sämtliche Ein-Byte-Beschädigungen sowie abgeschnittene Daten
bei laufender Wiedergabe. Der bestehende vollständige Sound-Review führt
diese Prüfung zusätzlich native und ASan/UBSan aus. Die bestehenden Tests
für PCM-Partitionen, FX/Freeze/Clear, Eigentümer und lange WOODLAND-Töne
bleiben erforderlich. Keine neuen Hördateien, kein Merge oder Flashen.
