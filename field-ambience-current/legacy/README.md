# `legacy/` — Pre-Step-6 Architektur (archiviert)

Diese Dateien gehören zur ursprünglichen Field-Ambience-Architektur, die mit
Step 6 des Native-Ports (siehe `../NATIVE_PORT_PLAN.md`) **abgelöst** wurde.
Sie sind hier archiviert statt gelöscht, damit der Hergang nachvollziehbar
bleibt und ältere Doku-Querverweise nicht ins Leere zeigen.

## Was das war (alte Architektur)

```
Browser HTML-Panel  ←─ WebSocket :8765 ─→  Python Bridge  ←─ OSC :57120/21 ─→  SuperCollider
                                                                                    ↓
                                                                              Pi Zero 2 W
                                                                                    ↓
                                                                               Pico (UI)
                                                                                    ↓
                                                                              PCM5102A DAC
                                                                                    ↓
                                                                               PAM8403 Amp
```

Die Audio-Engine lief in **SuperCollider** auf einem **Raspberry Pi Zero 2 W**;
der **Pico** war nur für UI (OLED + Encoder + Tasten) zuständig und sprach
UART mit dem Pi.

## Was das ist (heute, ab Step 6)

Audio-Engine + UI laufen **nativ in C auf dem RP2350 (Pico 2)**. Pi Zero,
SuperCollider, Python-Bridge und Browser-Panel sind alle ersatzlos
weggefallen. Aktuelle Quellen:

- `../firmware-c/` — die native Firmware (Steps 1–11 + 12a fertig)
- `../NATIVE_PORT_PLAN.md` — wie der Port lief
- `../field_ambience_pcb_SPEC_v0.6.md` — aktuelles Hardware-Design
- `../field_ambience_webapp.html` — **bleibt aktiv** als Klang-Referenz für
  den Port; ist *kein* Legacy-Artefakt (nicht hier drin).
- `../field_ambience_v29o.scd` — **bleibt aktiv** als kanonische SC-Referenz.

## Inhalt dieses Ordners

| Datei | Was |
|---|---|
| `firmware-mpy/` | MicroPython-Firmware des Pico (UI-only, sprach UART mit dem Pi). NATIVE_PORT_PLAN sah genau diese Archivierung vor: „Nach Step 12 ist die alte MicroPython-Firmware obsolet — wird entweder gelöscht oder als `firmware-mpy-legacy/` archiviert." |
| `bridge.py` | Python-Bridge Browser-WS ↔ SC-OSC. Lief auf dem Pi. |
| `panel.html` | Browser-Steuerpanel, sprach WS mit der Bridge. |
| `skill.md` | Skill-Beschreibung der alten Architektur. Inhaltlich überholt. |
| `v29p_test.scd` | Kleine SC-Test-Iteration. |
| `layout.png` | Geräte-Renderbild aus der alten Doku. |

## Wenn du das wirklich nochmal brauchst

Die Dateien sind unverändert eingefroren und über `git log -- legacy/<datei>`
mit voller Historie. Für eine vollständige Wiederherstellung der alten
Laufumgebung bräuchtest du:

- ein Raspberry Pi Zero 2 W mit SuperCollider 3.13+
- Python 3.11 + `python-osc` + `websockets`
- den letzten v0.5-Schaltplan (vor Step 6) — siehe `../CHANGELOG.md`, Eintrag
  „v0.5 → v0.6 transition" und davor

Realistisch: das hier ist Geschichte. Wer das Gerät heute baut, nutzt nur
`firmware-c/`.
