# World routing and hardware checkpoint — 2026-10-04

## Completed unit

User authorizes source archiving and renewed code/sound development.
Six unchanged legacy engines moved to `firmware-c-next/src/v2/Synths_Archive`.
CMake, test runner, hot-path lint and offline render commands follow the move.
Legacy scene IDs and symbols are preserved. These engines remain linked and
available through compatibility routing: this is not yet runtime removal,
a menu redesign, or a RAM saving.

Generated MEL_SRC now dispatches the selected World voice directly after
pitch humanization, hook and ownership bookkeeping. It does not allocate a
pad or refresh the accompaniment bass. The scheduler no longer strikes a
second World voice using an unhumanized frequency. Normal note-off still
releases the same source. Generated Bowed/Horn/Choir amplitude floors were
removed: softer scheduled returns now produce softer source input instead
of snapping back to the minimum loudness. Existing maximum gains remain.

Regression: selected Horn starts, hidden pad count stays zero, owned release
finishes. The current five-world scheduler remains; new three-world grammars
are not implemented by this change. Bed/Eno pad layers still exist separately.
No physical listening, acoustic quality or calming effect is certified.

## Chip and storage — verified source constraints

Product target: STM32H743VIT6. Linker regions: Flash 1920 KiB (128 KiB reserved
for scenes), DTCM 128 KiB, D1 512 KiB, D2 288 KiB, D3 64 KiB, ITCM 64 KiB.
Heap minimum 4 KiB, stack minimum 16 KiB. Banks are not interchangeable;
audio DMA cannot be placed in DTCM. This patch adds no audio arrays, pools,
heap allocation or sample storage. Moving sources does not shrink their data.

BOM_CURRENT U9 and ADR-0022 specify APS6404L-3SQN-SN, 64 Mbit / 8 MB PSRAM.
PSRAM is volatile working memory, not persistent sample storage. Initialization,
cache behavior and real hardware reliability are not proved by the BOM.
Realtime rules keep hot delay/reverb/DMA buffers internal.

Fresh ARM linker usage could not be measured here: neither CMake nor
arm-none-eabi-gcc is installed. No current Flash/RAM occupancy or CPU-fit
claim is made. Required gate: Release H743 cross-build, linker-map per-bank
usage, stack worst case, then real-device DWT peak_load < 0.60 and zero misses.
Old resource numbers must not be presented as current measurements.

No SD socket/interface is specified in the inspected current BOM/pin map.
That does not make SD impossible for the MCU: ST provides STM32H743 SDMMC
examples. PC11 is already QSPI chip select; available GPIOs alone do not prove
an accessible SD route. A retrofit needs actual board/layout/test-pad access,
bus and power design, mechanics and firmware. No PCB/CAD files were found in
this checkout, so a solder-only retrofit cannot be approved. A planned PCB
revision can include storage if justified; current synthesis must not depend
on an unimplemented SD path. Reference:
https://github.com/STMicroelectronics/STM32CubeH7/tree/master/Projects/STM32H743I-EVAL/Examples/SD

## Next units, in order

1. Eliminate implicit Bed/Eno accompaniment from the new World contract while
   preserving explicit harmony ownership and transition releases.
2. Source occupancy: count actual DSP sources plus releases; give Pluck a
   controllable owner/soft stop instead of round-robin truncation.
3. Replace manual catalog routing with the same World palette; migrate old
   scenes explicitly, then remove compatibility engines from product links.
4. Dry Bowed/Pluck/Horn tonal review, gains/envelopes and bounded <=30 s A/B.
5. Common room/modal-body review, then World-specific grammars and macro ranges.
6. Fresh H743 map and on-device timing remain required acceptance gates.

## Verification

Full host suite completed with exit 0 (including 7,961 generative scheduler
checks and 489,609 effect checks), plus source/hot-path regression gates.
Effect arena reported 214,489 / 245,760 bytes, state 672 / 4,096 bytes.
This is one subsystem's allocator accounting, not whole-chip RAM usage.
No audible-quality or ARM performance result is inferred from these checks.
