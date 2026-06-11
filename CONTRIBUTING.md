# Contributing

## Quick orientation

Read [`PROJECT_MAP.md`](PROJECT_MAP.md) first — it tells you which folder
your change belongs in, by role.

## Commit conventions

This repo uses scope-prefixed conventional-style messages. Look at the
recent log for examples — `git log --oneline -20`. Typical scopes:

- `pcb(rXX.Y):` — schematic / generator / footprint / BOM changes
- `firmware(step-X):` — active firmware development
- `bench:` — display_hw_test bring-up on Pico 2 + ST7789
- `spec(vX.Y):` — hardware spec changes
- `docs:` — README / ADR / status-doc changes
- `review(...):` — component review additions
- `fix(...):`, `feat(...):` — code-only changes

Body explains **why**, not just what. The diff already shows what changed.

## Before pushing

1. **Firmware change?** Run host tests:
   ```bash
   cd field-ambience-current/firmware-c-next/test
   ./run_tests.sh
   ```
   All suites must be green.
2. **Generator change?**
   ```bash
   cd field-ambience-current/kicad
   python3 generate_kicad_project.py
   ```
   Then inspect the resulting `.kicad_sch` diff carefully — the generator
   is the source of truth for the schematic.
3. **Documentation that claims something works?** Verify it actually works.
   Don't write "manufacturing-ready" if no `.kicad_pcb` exists.

## Pull requests

- Subscribe Claude to PR activity if you want CI failures and review
  comments auto-investigated (see [`PR-activity setup`](#) in your
  Claude Code session).
- Squash merges are the default. Body of the squash commit gets the full
  history of what happened in the branch.
- Don't push to `main` directly. PR via `claude/...` or topic branch.

## ADRs (Architecture Decision Records)

Big decisions get an ADR in `field-ambience-current/docs/decisions/`.
Format: Context, Decision, Consequences, Status, Date. See ADR-0004
(MIDI design decision) as the latest example. New ADRs auto-numbered
in commit order.

## What not to do

- Don't delete `firmware-c/` — it's the frozen listening-test snapshot.
  Use `firmware-c-next/`.
- Don't edit `.kicad_sch` files directly — they're generated.
  Edit `generate_kicad_project.py`.
- Don't mark things `READY` if blockers exist in `PCB_LAYOUT_STATUS.md`.
- Don't move files cross-folder without checking CI path filters in
  `.github/workflows/`. The firmware-c workflow is path-sensitive.
- Don't add datasheets without also adding a `component_review/<ref>.md`
  if the part is going into production (small passives are fine without).
