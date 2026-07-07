# CLAUDE.md — working rules for this repo

Auto-loaded into every session. Keep concise; this is binding guidance, not docs.

## Orientation

- **Read `PROJECT_STATUS.md` first** — it is the current-state source of truth
  (what's done / in progress / open). Trust it over old commit messages.
- The PCB **schematic is generated** by `field-ambience-current/kicad/generate_kicad_project.py`.
  Never hand-edit `.kicad_sch` — change the generator and re-run.
- Firmware lives in `field-ambience-current/firmware-c-next/`. Host tests:
  `./test/run_tests.sh`. Pico-2 bench build target = `pico` (display_hw_test).

## BINDING RULE — Realtime Audio

Any change reachable from `engine_render()` (the DMA-IRQ audio hot path)
follows **`field-ambience-current/docs/hardware/REALTIME_AUDIO_RULES.md`**:
no heap/blocking/unbounded calls in the hot path (the `test/lint_hotpath.sh`
gate enforces this and hard-fails the suite), ramp every live parameter (no
sample discontinuities), keep FTZ + DMA cache-clean intact, never stop/restart
the audio pump, and keep hot buffers internal — **never** in QSPI-PSRAM.

## BINDING RULE — AI-Ready PCB Schematic Standard

When creating, reviewing, or modifying ANY PCB schematic (i.e. editing the
generator), follow **`field-ambience-current/docs/hardware/AI_READY_SCHEMATIC_STANDARD.md`**.

**Don't over-detail — nail the buildable core.** MUST-HAVE for every part/net:
(1) correct ref-designator + descriptive name, (2) value, (3) exact footprint,
(4) connectivity (what connects where, which pin). Full metadata + test points
+ title-block extras = where it matters (ICs, connectors, power), not on every
jellybean passive.

The non-negotiables (apply with that priority in mind):

1. **Treat the schematic as machine-readable design data**, not just a drawing.
   Every component, net, sheet, symbol, footprint, block must be explicit,
   consistently named, and traceable.
2. **Net naming:** descriptive, no spaces/slashes, consistent caps. Active-low
   = **`_N` suffix** (`AMP_SHDN_N`, not `nSHDN`). Diff pairs = `_P`/`_N`.
3. **Complete metadata per component:** MPN, manufacturer, supplier PN (LCSC/
   DigiKey/Mouser), package, footprint, value, ratings, tolerance, lifecycle,
   datasheet/source.
4. **Verify symbol↔footprint directly** against the datasheet (pin numbers,
   pad numbers, pin 1, polarity, exact package variant, exact LCSC part). Never
   assume a footprint is safe because it exists.
5. Standard unique ref-designators · labels not spaghetti (signal L→R, power
   T→B) · title blocks with revision/date/author · explicit no-connects ·
   explicit power design (rails, regulators, datasheet caps, decoupling,
   protection, ground domains) · test/debug access (`TP_*`, `SWDIO`/`SWCLK`).

**When reviewing a schematic**, don't just check if it "looks correct." For
every issue report: what's wrong · why it matters · what could fail (layout /
JLC assembly / firmware / debugging) · how to fix · severity (critical /
important / minor).

**DO NOT GUESS.** If a footprint, pinout, datasheet, package, polarity,
connector orientation, or supplier part cannot be verified, mark it exactly:
`UNVERIFIED — NEEDS HUMAN CHECK`. Never pretend a footprint is safe unless
symbol, datasheet, package, and footprint have been matched directly.

## Git / PR

- Develop on branch `claude/read-start-here-YDlCd`.
- Commit + push only when asked or when finishing a unit of work; always open a
  PR for a pushed branch.
- After a sound/UI/PCB change: run `./test/run_tests.sh` and keep it green.
