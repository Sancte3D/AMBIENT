# PROJECT STATUS

**Updated: 2026-06-21 (r18.39)**

**Purpose:** persistent orientation document so the assistant (and the user)
can pick up the project without re-scanning the full history each session.
Source of truth for "what is done, what is in progress, what is open" —
overrides anything an old commit message says. Keep concise; update at the
end of every session that changes a shipping state.

**Conventions:**

- ✅ ready / merged-mergeable
- 🟡 in branch, working, not merged yet
- 🟠 audition-only / demo (renders, no engine integration)
- 🔴 broken / blocked
- ⏳ planned / next-up

---

## 1. Where we are right now

**Working branch:** `claude/read-start-here-YDlCd` → PR #35 (Sancte3D/AMBIENT)
**Latest user direction:**
- Sound stays synthetic (no field-recording samples). Stage is "fast gut genug".
- No web simulator — Pico 2 is the bench, not a browser
- Merge what's ready, keep an overview, write your own status doc

---

## 2. Project identity (what the device IS)

A handheld ambient instrument that loads **curated WORLDS** — sound presets
the user thinks in pictures (night city, sunset coast, night highway, jazz
bar) rather than synth-engine parameters. Cells trigger notes inside the
world; modifier buttons shape the playing; encoders nudge a few global
macros over the world preset.

**Audio target:** PS2-era / dreamy / warm-pop / chillout. NOT meditative,
NOT yoga app, NOT dark horror, NOT crystal-castles-aggressive. The
user-accepted reference render is `tools/render_dreamy_warm.c`.

**Hardware target product:** STM32H743 (`src/hal_h743/`). The Pico 2 is a
BENCH tool for LCD + encoder bring-up (`tools/display_hw_test.c`), NOT a
product build.

---

## 3. Status by layer

### Sound DSP (audio chain)

| Item | State |
|---|---|
| V1 warm-chorus pad ("100x better" sound) | ✅ `src/pad.c`, reverted from soften'd profiles in r18.37 |
| `texture.c` body / breath | ✅ exists; body rumble is the "brumm" — disabled in demos, engine still uses old default |
| `reverb.c`, `drone.c`, `bass.c`, `brain.c` | ✅ |
| 4-world sound spec (Tokyo / Coast / Drive / After Hours) | 🟠 only as `tools/render_worlds.c` |
| Universal wind generator (resonant BP, pink noise, gusts) | 🟠 inline in render_worlds.c |
| Per-world ambience (rain / waves / traffic / vinyl) | 🟠 inline in render_worlds.c, generators believable, not in engine |
| Tape-hiss generator | 🟠 inline in render_dreamy_warm.c |
| Warm-tanh master saturation | 🟠 inline in render_dreamy_warm.c |
| World preset application from engine | ⏳ next refactor step |
| Per-world drums system (menu toggle exists) | ⏳ `beat.c` lives in `src/v2/` and can be lifted out |
| Aliasing pre-filter on noise sources | ⏳ noted "macht prinzipiell Sinn", not done |

**Audition tools (do not ship, useful for A/B):**
- `tools/render_dreamy_warm.c` — accepted reference 60 s
- `tools/render_worlds.c` — all 4 worlds, has `measure` mode for level audits
- `tools/render_oled.c` — host-side menu preview PGMs

### Engine V2 (`src/v2/`)

| Item | State |
|---|---|
| Whole v2 engine (worlds, harmony field, particle, motion, beauty guard) | 🟠 dead-end direction the user rejected ("Horror", "Crystal Castles") |
| `src/v2/beat.c` (kick/snare/hat + bitcrush + drive) | 🟠 keep — will be lifted out for the per-world drums system |
| All other v2 modules | ⏳ likely removed when engine refactor lands; keeping for now |

### Menu / Display UI

| Item | State |
|---|---|
| `menu.c` / `menu.h` on WORLD model (World / Space / Tone / Atmos / Drums) | ✅ r18.38 |
| Renderer + tween engine in `display_hw_test.c` | ✅ unchanged, drives new menu |
| World subtitle ("night . rain" etc) under big value | ✅ |
| World preset loads on world change (Space/Tone/Atmos snap) | ✅ |
| `tools/render_oled.c` host preview | ✅ updated to world model |
| `tools/display_sim.html` JS port | ✅ updated to world model (sim still committed, just no auto-deploy) |
| Pico 2 bench build (`display_hw_test.uf2`) | ✅ CI RP2350 build green |
| Engine ↔ menu callbacks wired in HAL | ⏳ `menu_callbacks_t` exists, no HAL `main_*` currently wires it |

### Cells / Input

| Item | State |
|---|---|
| Gateron LP Magnetic + DRV5056A4 Hall plan (ADR-0013) | ✅ |
| `cells.c` velocity state machine | ✅ |
| Pressure/aftertouch from `cells_position()` | ⏳ idea, dropped — function exists, no engine reads it |
| Modifier set Shift / Hold / Drone / Generate (auto-play) / Clear | ⏳ specified, not implemented in world engine |

### HAL

| Target | State |
|---|---|
| `src/hal_pico/` (Pico 2 SDK, bench-only) | ✅ display_hw_test builds + runs |
| `src/hal_h743/` (STM32H743, product) | 🟡 skeleton compiles syntactically; CubeH7 toolchain integration is the final step |
| `tools/display_hw_test.c` (deliverable for "test on Pico") | ✅ |

### PCB / BOM

| Item | State |
|---|---|
| `BOM_MASTER.md` | ✅ r18.37 — FP links clickable, Hall doc-drift fixed |
| `field-ambience-current/PCB_FOOTPRINT_RISK_AUDIT.md` (risk-based per user brief) | ✅ r18.37 |
| 6 custom KiCad footprints in `field_ambience.pretty/` | ✅ all actively referenced |
| 7 STEP models in `field_ambience.3dshapes/` | ✅ |
| BOM ↔ generator LCSC string-diff | ✅ 0 actual mismatches |
| Layout-blocker: ERC walkthrough of 9 ICs in KiCad | ⏳ user task (KiCad GUI) |
| Layout-blocker: TPS61089 datasheet §10 pre-study | ⏳ user task |
| Order-blocker: 1:1 print of 10 mech-critical parts vs enclosure | ⏳ user task |
| Order-blocker: Waveshare LCD pin-order verify on actual module | ⏳ user task |

### CI / Build

| Workflow | State |
|---|---|
| `firmware-c.yml` host unit tests | ✅ 83/83 + 46 display-bench |
| `firmware-c.yml` RP2350 build (display_hw_test) | ✅ |
| `pages.yml` (GitHub Pages deploy of display_sim.html) | ❌ REMOVED r18.39 per user — Pages was never enabled, deploy always failed |

---

## 4. What is mergeable to `main` right now

Everything since `main` is mergeable. Nothing in `claude/...` is broken or
experimental in a way that would make merging risky.

- All `tools/render_*.c` audition renderers (no firmware change, demos only)
- `BOM_MASTER.md` r18.36/37 cleanups
- `PCB_FOOTPRINT_RISK_AUDIT.md`
- Pad-profile revert to V1 warm-chorus (r18.37)
- World UI in `menu.c` / `menu.h` / `display_sim.html` / `render_oled.c` (r18.38)
- Test updates for world model
- 6 obsolete audition tools deleted in r18.37
- `pages.yml` deletion (r18.39)

The DSP/engine sound integration of the worlds is NOT in here yet — that's a
separate ⏳ chunk, lives only as audition tools.

---

## 5. What's next, in priority order

1. **Merge PR #35 to main** (after pages.yml removal turns CI green).
2. **Engine refactor — world model:** `worlds.c` + `ambience.c` (lift the
   inline generators from `tools/render_worlds.c` into real modules),
   `hiss.c`, warm-saturation module, rewrite `engine.c` to be world-driven,
   wire `menu_callbacks_t` in `src/hal_h743/main_h743.c`. ADR-0015 to draft
   first, then implement in small steps.
3. **Per-world drums system:** extract `src/v2/beat.c` into `src/drums.c`,
   per-world pattern + tempo selection, wired to the menu Drums toggle.
   Retire the rest of `src/v2/` afterwards.
4. **Aliasing pre-filter** on noise sources when they move from tools/ into
   engine modules.
5. **PCB layout-blockers** (user-side KiCad work — ERC pass + TPS61089
   layout study).

---

## 6. Update protocol for the assistant

When ending a session, do one pass through §3 and §5 and update lines that
changed. Don't rewrite the whole file. The point: anyone reading this knows
in 90 seconds what's true today, without trusting old commit-message claims.

**This file replaces** the old "Project Status" snapshot (r18.6, 2026-06-11)
which was 10 days stale. Other docs intentionally NOT replaced:

- `field-ambience-current/CHANGELOG.md` — full revision history
- `field-ambience-current/ROADMAP.md` — long-term plan
- `BOM_MASTER.md` — single source of truth for parts
- `field-ambience-current/PCB_FOOTPRINT_RISK_AUDIT.md` — risk-based PCB pass
