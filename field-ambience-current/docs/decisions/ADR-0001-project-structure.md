# ADR-0001: Repository Structure — Staged Refactor

**Status:** ACCEPTED (Phase 1 done; Phases 2–5 queued)
**Date:** 2026-06-11

## Context

The repo grew from a single research session into a multi-discipline product. All living material sits under `field-ambience-current/` — firmware, KiCad, docs, demos, legacy, all flat. A new contributor (industrial designer, hardware engineer, firmware engineer, or anyone trying to manufacture) cannot find what they need in under five minutes.

The brief asked for a full reorganization into discipline-based top-level folders (`product/`, `software/`, `firmware/`, `hardware/`, `mechanical/`, `manufacturing/`, `validation/`, `tools/`, `docs/`, `archive/`).

## Decision

Stage the refactor in five phases instead of one commit:

1. **Phase 1 (this commit)** — Additive only: real root `README`, `PROJECT_MAP`, `PROJECT_STATUS`, `REPO_STRUCTURE`, `CONTRIBUTING`, and four role-based onboarding docs. Improves navigation immediately; cannot break anything.
2. **Phase 2** — Safe doc/asset moves with no CI dependency (PITCH, mechanical_coordinates, webapp, .scd, reports, legacy, docs/archive).
3. **Phase 3** — Firmware split + synchronized CI update. Atomic, single PR.
4. **Phase 4** — KiCad reorganization with manual GUI verification on a real KiCad install.
5. **Phase 5** — Manufacturing scaffold, but only once a `.kicad_pcb` exists. Pre-creating empty `manufacturing/` folders would imply progress that isn't there.

## Consequences

**Positive:**
- Phase 1 lands a useful repo immediately with zero CI risk.
- Each later phase is independently reviewable and revertible.
- Firmware engineers don't get a broken CI on Monday morning because of a refactor friday.
- Hardware engineers' KiCad project keeps working until we have hands-on KiCad-9 GUI time to verify the rename.

**Negative:**
- Repo doesn't reach the "ideal" target structure in one shot.
- The `field-ambience-current/` folder name is still in many paths and feels like legacy naming.
- New contributors during the transition see a mixed structure.

**Mitigations:**
- `REPO_STRUCTURE.md` documents the staging plan transparently — readers know it's staged on purpose, not by neglect.
- `README.md` and `PROJECT_MAP.md` route role-based, so the awkward folder name doesn't slow anyone down at first read.

## Alternatives considered

- **One-shot big-bang refactor:** Rejected. The firmware-c CI workflow has 14 path references, pages.yml has 2; KiCad project files store sheet UUIDs against filesystem paths; doing all of this atomically in one commit would be unreviewable and likely break CI silently.
- **Don't refactor, just add navigation docs:** Rejected. The discipline-based separation is the right end state; deferring it forever keeps the repo confusing.

## Related ADRs

- ADR-0002 — RP2350 → STM32H743 migration (drove much of the current schematic churn)
- ADR-0003 — Manufacturing gate model (production-readiness criteria)
- ADR-0004 — MIDI design decision (open)
