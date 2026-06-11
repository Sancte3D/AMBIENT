# Industrial Designer — Onboarding

## What this product is

Field Ambience is a portable, hand-held ambient instrument: an audio device
you turn on, twist four knobs on, hold buttons on, and listen through built-in
speakers or a TRS jack. It generates evolving ambient music — no recording,
no sample triggering, no laptop.

Closest spiritual references: **OP-1 field, Stylophone Beat, Critter & Guitari Organelle**.
Different sound — generative pads + textures + slow harmonic movement,
not loops or samples.

## The interaction model in one paragraph

A 1.9″ 320×170 IPS display shows a **single big value** at any time —
whichever parameter you're touching. Four endless encoders along one edge:
Drive (reverb saturation), Brightness (audio tone — *not* the screen),
Display (menu navigation), Volume. Buttons below them switch modes (Drone /
Generate / Hold) and modifiers (Shift / Clear). The whole UI is monochrome
white-on-black; it speaks the same OP-1 / Teenage Engineering visual language.

## Where to look

- **Product brief**: [`PITCH.md`](../../PITCH.md) — what we're selling, who it's for
- **Try the menu now**: open [`firmware-c-next/tools/display_sim.html`](../../firmware-c-next/tools/display_sim.html) in any browser, scroll-wheel on a dial to "turn" it, click to push, hold Shift to enter Brightness mode
- **Listen to it**: [`demos/audio/`](../../../demos/audio/) at the repo root — `engine_performance_long.flac` is 8 minutes of the engine playing itself
- **Mechanical/panel sketch**: [`mechanical_coordinates.md`](../../mechanical_coordinates.md) — current X/Y/Z constraints (provisional, will update once enclosure CAD starts)

## Why each design choice is what it is

Read the ADRs: [`docs/decisions/`](../decisions/). Each documents one
non-obvious choice and what was rejected.

## What to leave alone

You can change anything in `PITCH.md`, `mechanical_coordinates.md`, and
write new ADRs. Don't edit:

- The schematic generator (`kicad/generate_kicad_project.py`)
- The schematic files (they're generated)
- The firmware C source
- The web simulator's `<script>` block (it mirrors the firmware — change
  firmware, simulator follows)

If you have a UX/mechanical change that affects firmware behavior, open an
issue and tag it `industrial-design` — a firmware engineer will translate.

## Quick-fire status check

- "Can I see what it would look like in a case?" — Not yet, no CAD.
  Mechanical coordinates are paper-and-pencil right now.
- "Can I hold a working unit?" — There's a breadboard bring-up (Pico 2
  + 1.9″ ST7789 + one encoder + one tactile button) that runs the real menu.
  Ask the firmware engineer for the latest UF2.
- "When is it orderable?" — Not soon. We're at production gate 1.5 of 9.
  See [`PROJECT_MAP.md`](../../../PROJECT_MAP.md).
