# GitHub Source Catalog

This pack is an AMBIENT-specific synthesis. It does not vendor third-party code or copy unlicensed skill text. Use the locked revisions in `sources.lock.json` when consulting upstream material.

## Agent skills

| Source | Relevant material | License/use |
|---|---|---|
| [nextlevelbuilder/ui-ux-pro-max-skill](https://github.com/nextlevelbuilder/ui-ux-pro-max-skill) | UI/UX system search, color, typography, motion and anti-pattern data | MIT; adapted conceptually |
| [travisjneuman/.claude](https://github.com/travisjneuman/.claude) | `ui-animation`, `ui-research`, `graphic-design`, `embedded-iot` skills | MIT; adapted conceptually |
| [Jeffallan/claude-skills](https://github.com/Jeffallan/claude-skills) | `embedded-systems` skill with STM32, DMA, ISR and RTOS guidance | MIT; adapted conceptually |
| [rmyndharis/antigravity-skills](https://github.com/rmyndharis/antigravity-skills) | `arm-cortex-expert` and `ui-ux-designer` skills | MIT; adapted conceptually |
| [Milind220/Ki-Stack](https://github.com/Milind220/Ki-Stack) | Nine composable KiCad skills for orientation, live IPC, rendering, file surgery, verification, PCB, schematic, symbols and footprints | MIT; preferred KiCad workflow source |
| [Seeed-Studio/ai-skills](https://github.com/Seeed-Studio/ai-skills) | `schematic-analyzer` skill and evidence-first schematic reading strategy | MIT; adapted conceptually |
| [aklofas/kicad-happy](https://github.com/aklofas/kicad-happy) | KiCad, BOM, JLCPCB and LCSC skills | MIT; preferred sourcing/workflow source |
| [LeoKemp223/NextBoard](https://github.com/LeoKemp223/NextBoard) | Hardware-solution skill, verification gates, BOM and sourcing risk | MIT declared in README; concepts only |
| [github/awesome-copilot](https://github.com/github/awesome-copilot) | Embedded C agent and circuit/LCD references | MIT; concepts only |
| [anthropics/skills](https://github.com/anthropics/skills/tree/main/skills/frontend-design) | Distinctive visual direction and anti-template design judgment | Upstream-specific terms; link only |
| [uxuiprinciples/agent-skills](https://github.com/uxuiprinciples/agent-skills) | UX evaluation, interface audit, flow checks | No root license found at locked revision; link only |

## Embedded graphics and display implementations

| Source | Use |
|---|---|
| [lvgl/lvgl](https://github.com/lvgl/lvgl) | Embedded UI architecture, animations, invalidation, draw buffers and display drivers |
| [lvgl/lv_demos](https://github.com/lvgl/lv_demos) | Small reproducible UI and performance examples |
| [lvgl/lv_port_stm32h7b3i_disco](https://github.com/lvgl/lv_port_stm32h7b3i_disco) | STM32H7 display-port structure; do not copy board-specific assumptions |
| [ARM-software/Arm-2D](https://github.com/ARM-software/Arm-2D) | Cortex-M partial-framebuffer, dirty-region, tile, alpha-blend and acceleration patterns |
| [libdriver/st7789](https://github.com/libdriver/st7789) | ST7789 command behavior and portable driver reference |
| [STMicroelectronics/STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7) | Authoritative H7 HAL, DMA, cache, DMA2D and memory examples; observe per-component licenses |
| [STMicroelectronics/stm32h7xx-hal-driver](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) | Current H7 HAL driver implementation |
| [aptumfr/awesome-lvgl](https://github.com/aptumfr/awesome-lvgl) | Discovery index only; verify every downstream source independently |

## Selection rules

1. Prefer vendor datasheets, errata, reference manuals and official library repositories for electrical and register-level facts.
2. Prefer MIT or Apache-2.0 implementations for reusable patterns.
3. Treat generic UI skills as visual-thinking inputs, not as embedded performance authority.
4. Treat repository popularity as discovery evidence only.
5. Do not copy text or code from a source without checking the exact locked revision and license.
6. Record any newly used source in `sources.lock.json` with repository, commit, path, license and purpose.
