# PROJECT STATUS

**Updated: 2026-06-22 (r18.53 — tape character: hiss + warm saturation in master, ADR-0017 Phase 3)**

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

**Working branch:** `claude/read-start-here-YDlCd` → PR #37 merged to `main` 2026-06-22
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
| Tier A #1: velocity → filter cutoff | ✅ `src/pad.c` r18.43 — bright hits open up |
| Tier A #2: micro-humanisation (±0.5 cent / ±0.3% amp jitter) | ✅ `src/engine.c` r18.43 |
| Tier A #3: drone drift + breath (±2 cent walk, 0.04 Hz tremolo) | ✅ `src/drone.c` r18.43 |
| Tier A #4: texture body weight 0.35 → 0.10 (removes the Brumm) | ✅ `src/texture.c` r18.43 |
| Tier A #5: air band (+HP 3 kHz on white noise, 0.18×) | ✅ `src/texture.c` r18.43 |
| LIQUID modulated FDN reverb (default) | ✅ `src/reverb.c` r18.42 — Freeverb as `-DFAM_REVERB_MODE=0` fallback |
| `drone.c`, `bass.c`, `brain.c` | ✅ |
| 4-world sound spec (Tokyo / Coast / Drive / After Hours) | 🟠 only as `tools/render_worlds.c` |
| Universal wind generator (resonant BP, pink noise, gusts) | 🟠 inline in render_worlds.c |
| Per-world ambience layer in engine (ADR-0017 Phase 2 KOMPLETT) | ✅ r18.49-.52 — Wind universal + Rain Tokyo + Waves Coast + Vinyl After Hours. Drive bekommt Wind als „highway" (kein dedizierter Traffic-Generator, Wind ist character-genug). |
| Worlds-Modul (single source of truth) | ✅ r18.48 `src/worlds.c` + `worlds.h` (ADR-0017 Phase 1) — Lift aus menu.c, exakte RGB/Preset-Erhaltung per Test |
| Tape-hiss generator | 🟠 inline in render_dreamy_warm.c |
| Tape character (hiss + warm-tanh saturation) im Master | ✅ r18.53 `src/tape.c` (ADR-0017 Phase 3); always-on, Default = dreamy_warm-Referenz (hiss 0.005, drive 1.10) |
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
| **Display Akzent-Farbe pro World (Grau→RGB565-Tint)** | ✅ r18.44 `src/oled_color.c` — ADR-0015 Schritt 1; Default=Mono, pro World dezenter Cast; Host-Preview farbig |
| **Akzent-Crossfade beim World-Wechsel (erste UI-Animation)** | ✅ r18.45 — `oled_accent_tick/settle`, libm-frei; auf Pico-Bench testbar |
| **Bench `display_hw_test` aktuell zur World-UI (Subtitle + panel-agnostische Verdrahtung)** | ✅ r18.44/45 — 1.9″ Adafruit *und* Waveshare Silk; `display_hw_test.uf2` als CI-Artefakt |
| **Schaltbild-Walkthrough (A–Z PCB-Engineer-Tour)** | 🟡 r18.46 — `docs/hardware/SCHEMATIC_WALKTHROUGH.md`; Power + MCU + Audio voll, übrige 4 Sheets in Folge-PRs |
| **Power/Sleep-Architektur (ADR-0016, kein Switch)** | ⏳ ADR-0016 PROPOSED — `U9` TPS22918 Load-Switch + `SW_BOOT` Dual-Use |
| **Panel-Selector Firmware-Pfad (1.9″ ↔ 2.0″ via CMake-Flag)** | ✅ r18.47 — `oled.h` + Pico-Treiber + CMake; Default 1.9″ unverändert, 2.0″-Build kompiliert sauber. UI-Layout-Rebalance offen (70 px Dead-Space) |
| **Panel-Hardware-Pivot 1.9″ → 2.0″ (physisches Modul)** | ⏳ ADR-0015 — wartet auf User-Verifikation SKU/Pin-Order/Maße; BOM-Kandidat UNVERIFIED |
| **Voller RGB565-FB + DMA-Animationen** | ⏳ ADR-0015 D4 — nach Hardware-Pivot |

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

1. **Display-Pivot 1.9″ → 2.0″ + RGB565 + Animations-Architektur** ⏳ ADR-0015
   (r18.43, PROPOSED). User-Side: Modul-SKU + Pin-Order + Maße verifizieren.
   Firmware-Side: `oled_*` API von 4-bit Grau auf RGB565 portieren, FB nach
   AXI-SRAM, SPI-DMA + DMA2D in `lcd_st7789_h743.c` ausimplementieren
   (Step 13.3 TODO). Generator-Sheet erst nach Pin-Order-Bestätigung anfassen.
2. **Engine refactor — world model** ⏳ `worlds.c` + `ambience.c` (lift the
   inline generators from `tools/render_worlds.c` into real modules),
   `hiss.c`, warm-saturation module, rewrite `engine.c` to be world-driven,
   wire `menu_callbacks_t` in `src/hal_h743/main_h743.c`. Eigene ADR (0016)
   beim Start drafting.
3. **Per-world drums system:** extract `src/v2/beat.c` into `src/drums.c`,
   per-world pattern + tempo selection, wired to the menu Drums toggle.
   Retire the rest of `src/v2/` afterwards.
4. **Aliasing pre-filter** on noise sources when they move from tools/ into
   engine modules.
5. **PCB layout-blockers** (user-side KiCad work — ERC pass + TPS61089
   layout study).
6. **AI-Ready Schematic Standard compliance** (binding rule, see
   `docs/hardware/AI_READY_SCHEMATIC_STANDARD.md` §"compliance snapshot"):
   safe generator edits — rename active-low nets to `_N` suffix
   (`nSHDN`→`AMP_SHDN_N`), populate per-sheet title-block revision/date/author,
   add `TP_*` test pads. Plus the GUI-ERC pin-map + NC audit (= blocker B3).

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
