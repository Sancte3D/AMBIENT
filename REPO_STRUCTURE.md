# Repository Structure — Current State & Refactor Roadmap

## Current state (r18.6, 2026-06-11)

The repo has grown organically. Most living material is under
`field-ambience-current/`. That folder name worked when there was a
predecessor — there isn't anymore. It's the active code now.

```
AMBIENT/
├── README.md
├── PROJECT_MAP.md
├── REPO_STRUCTURE.md          (this file)
├── CONTRIBUTING.md
├── .github/workflows/         CI: firmware-c.yml, pages.yml
├── demos/audio/               render samples (FLAC)
└── field-ambience-current/
    ├── (status & spec docs at top level — see PROJECT_MAP.md)
    ├── firmware-c/            FROZEN listening-test snapshot
    ├── firmware-c-next/       ACTIVE firmware (Pico bench + host tests)
    ├── kicad/                 schematic generator + 8 sheets + libs
    │   ├── legacy_pico2/      archived old sheets
    │   ├── libraries/         vendored kiswitch + project-local FPs
    │   └── datasheets/        active + datasheets/legacy/ for replaced
    ├── docs/                  ADRs, hardware checklists, component_reviews,
    │                          archive of pre-Step-6 specs, onboarding
    ├── reports/               validation reports
    ├── scripts/               KiCad helpers (ERC, footprint check)
    └── legacy/                pre-native bridge.py, MicroPython firmware
```

This structure is **honest** about what's active vs. archived. It is **not
yet** organized by product-development discipline (product / software /
firmware / hardware / mechanical / manufacturing).

## Refactor roadmap

The full reorganization proposed in the brief moves everything into
discipline-based top-level folders. That's the right end state.

Two reasons it's staged instead of done in one commit:

1. **CI breaks if moves and CI updates aren't atomic.** The firmware-c
   workflow has 14 path references; pages.yml has 2. A single typo in
   either file silently disables the bench-UF2 build. Doing the firmware
   refactor + CI update as a single, reviewable change is safer than
   bundling it with documentation moves.
2. **KiCad project moves need verification in the KiCad GUI** (the
   `.kicad_pro` and per-sheet UUIDs follow file paths). Since `kicad-cli`
   isn't available in this environment, that verification needs to happen
   manually on a real KiCad install before the move lands in `main`.

### Phase 1 (DONE, this commit)

- Real root `README.md` (was: 1 line)
- `PROJECT_MAP.md` (role-based navigation)
- `REPO_STRUCTURE.md` (this file)
- `CONTRIBUTING.md` (commit / test / push conventions)
- Onboarding docs for industrial designer / hardware / firmware / manufacturing
- `PCB_LAYOUT_STATUS.md` made the single truth-source for manufacturing status

### Phase 2 (planned — safe doc/asset moves, no CI risk)

- `field-ambience-current/PITCH.md` → `product/brief/PITCH.md`
- `field-ambience-current/mechanical_coordinates.md` → `mechanical/coordinates/`
- `field-ambience-current/field_ambience_webapp.html` → `software/webapp/`
- `field-ambience-current/field_ambience_v29o.scd` → `software/supercollider_reference/`
- `field-ambience-current/reports/` → `validation/reports/`
- `field-ambience-current/legacy/` → `archive/legacy_pre_native/`
- `field-ambience-current/docs/archive/` → `archive/old_specs/`

### Phase 3 (planned — firmware split, atomic with CI update)

- `field-ambience-current/firmware-c/` → `firmware/frozen_snapshots/rp2350_hoertest_step11_12a/`
- `field-ambience-current/firmware-c-next/` → `firmware/active/` (still RP2350-targeted, eventual STM32 work alongside)
- `.github/workflows/firmware-c.yml` paths updated synchronously
- `.github/workflows/pages.yml` paths updated synchronously
- All cross-references in CHANGELOG / SPEC / READMEs updated by `grep` pass

### Phase 4 (planned — KiCad reorganization, manual KiCad-GUI verification)

- `field-ambience-current/kicad/` → `hardware/kicad/`
- Internal split into `project/`, `schematics/`, `pcb/`, `libraries/`, `datasheets/`, etc.
- `generate_kicad_project.py` output paths updated
- Smoke-test in KiCad 9 on a real install: ERC must still pass before commit

### Phase 5 (planned — manufacturing scaffold)

Only meaningful **once a `.kicad_pcb` exists**. Pre-creating empty `manufacturing/` folders would imply progress that isn't there. Honest state stays in `PCB_LAYOUT_STATUS.md` until then.

## Why staged is the right call

The brief asked for a "real refactor, not a planning document." Phase 1
delivers real, immediate navigation improvement without any risk. Phases 2-5
are queued as separate PRs precisely because each can break something
specific (path-sensitive CI, KiCad project integrity, BOM links). Bundling
them would make the refactor unreviewable.

Status of phases is tracked here — each phase merges as its own PR.
