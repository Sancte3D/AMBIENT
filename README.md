# Field Ambience

A field-recorder-style ambient instrument. Hardware-software product in
active development — generative audio engine running on STM32H7, controlled
by an OP-1-inspired 320×170 IPS display + four endless encoders.

**Current state: prototype.** Audio engine runs on Pico 2 host tests and a
display bench, hardware migration to STM32H743 is in progress, no PCB layout
yet — **not orderable**.

## Five-minute tour

| What | Where | Status |
|---|---|---|
| **Product intent** (what we're building, for whom) | [`field-ambience-current/PITCH.md`](field-ambience-current/PITCH.md) | concept locked |
| **Sound engine** (host-portable C, the heart) | [`field-ambience-current/firmware-c-next/`](field-ambience-current/firmware-c-next/) | active, 12 host test suites green |
| **Web simulator** (try the menu in a browser) | [`field-ambience-current/firmware-c-next/tools/display_sim.html`](field-ambience-current/firmware-c-next/tools/display_sim.html) + GitHub Pages | live |
| **Bench bring-up** (Pico 2 + ST7789 + KY-040 → flashable UF2) | [`field-ambience-current/firmware-c-next/tools/display_hw_test.c`](field-ambience-current/firmware-c-next/tools/display_hw_test.c) | works on real hardware |
| **Hardware schematic** (8 sheets, generator-based) | [`field-ambience-current/kicad/`](field-ambience-current/kicad/) | STM32H743 migration done (r18.6), **no `.kicad_pcb` yet** |
| **What we know about the parts** (datasheets, verifications) | [`field-ambience-current/docs/component_reviews/`](field-ambience-current/docs/component_reviews/) | 2 reviews complete, ~20 open |
| **What's blocking manufacturing** | [`field-ambience-current/PCB_LAYOUT_STATUS.md`](field-ambience-current/PCB_LAYOUT_STATUS.md) | honest, updated every revision |
| **How decisions were made** | [`field-ambience-current/docs/decisions/`](field-ambience-current/docs/decisions/) | 20 ADRs (rejected ones marked SUPERSEDED) |
| **Render samples to listen to** | [`demos/audio/`](demos/audio/) | 3 FLAC: 8-min master tape, 5-min performance, 3-min GENERATE autoplay (r18.89 plucks/crackle/drive) |
| **Full project map / what to open first per role** | [`PROJECT_MAP.md`](PROJECT_MAP.md) | role-based |

## Who you are, what to read first

- **Industrial designer** → [`docs/onboarding/INDUSTRIAL_DESIGNER_START.md`](field-ambience-current/docs/onboarding/INDUSTRIAL_DESIGNER_START.md)
- **Hardware engineer** → [`docs/onboarding/HARDWARE_ENGINEER_START.md`](field-ambience-current/docs/onboarding/HARDWARE_ENGINEER_START.md)
- **Firmware engineer** → [`docs/onboarding/FIRMWARE_ENGINEER_START.md`](field-ambience-current/docs/onboarding/FIRMWARE_ENGINEER_START.md)
- **Anyone trying to order PCBs** → [`docs/onboarding/MANUFACTURING_START.md`](field-ambience-current/docs/onboarding/MANUFACTURING_START.md)
- **New contributor, general** → [`START_HERE.md`](field-ambience-current/START_HERE.md)

## Folder logic (current repo state)

```
AMBIENT/
├── README.md                    ← this file
├── PROJECT_MAP.md               ← what lives where, by role
├── REPO_STRUCTURE.md            ← roadmap for the bigger refactor
├── CONTRIBUTING.md
├── demos/audio/                 ← FLAC renders to listen to
├── mechanical/coordinates/      ← X/Y/Z placement spec (r18.13 ex-field-ambience-current/)
├── software/
│   ├── webapp/                  ← in-browser engine reference (field_ambience_webapp.html)
│   └── supercollider_reference/ ← canonical SC engine (field_ambience_v29o.scd)
├── archive/
│   ├── legacy_pre_native/       ← pre-native bridge.py, MicroPython firmware
│   ├── old_specs/               ← pre-Step-6 pitch + roadmap
│   └── PCB_TODO_historical.md   ← pre-H743 hardware issue log (history only)
└── field-ambience-current/      ← active design + code + docs
    ├── START_HERE.md
    ├── PCB_LAYOUT_STATUS.md     ← honest manufacturing state
    ├── CHANGELOG.md             ← what changed and when
    ├── NATIVE_PORT_PLAN.md      ← Pico → STM32H7 migration plan
    ├── field_ambience_pcb_SPEC_v0.7.md  ← live hardware spec
    ├── firmware-c/              ← FROZEN host-test snapshot (Pico era)
    ├── firmware-c-next/         ← ACTIVE firmware development
    ├── kicad/                   ← schematic + custom footprints
    │   ├── legacy_pico2/        ← old Pico-era sheets, archived
    │   └── libraries/           ← kiswitch (vendored) + field_ambience.pretty
    ├── docs/                    ← decisions / hardware checklists / component reviews
    └── scripts/                 ← KiCad ERC/footprint helpers
```

> **Note on folder names:** Most living material is still under
> `field-ambience-current/`. A larger repo refactor is planned (firmware
> separation, CI-path migration) — it's intentionally staged so the live
> CI doesn't break. See [REPO_STRUCTURE.md](REPO_STRUCTURE.md) for the
> roadmap.

## Help / contributing

[`CONTRIBUTING.md`](CONTRIBUTING.md) covers commit conventions, how to run
the host tests, and what to check before pushing changes to the schematic
generator.

## License

TBD before public release. Until then: all rights reserved by the project
authors. Open for review and pull requests within the repo, but please
don't redistribute the source elsewhere without asking first.
