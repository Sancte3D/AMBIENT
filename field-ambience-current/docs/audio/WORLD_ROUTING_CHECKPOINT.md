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

## Follow-up unit: Pluck ownership and soft stop

The full pool previously overwrote a live delay line; Clear did not release
Pluck at all. Pluck now has a source-owned admission API returning success /
failure. Duplicate owner and full pool decline without changing excitation RNG
or another voice. Legacy one-shots also decline a full pool. This intentionally
reduces dense retriggers instead of truncating an audible tail.

Owned manual String notes release by source; Clear and core handover release
both owned and unowned plucks. A 20 ms linear output ramp applies equally to
dry and send. Release slots remain occupied until their ramp finishes. A
repeated stop cannot extend it. The audio IRQ sees an active voice only after
its excitation buffer is fully initialized. Invalid/zero/nonfinite inputs are
rejected; damping ignores nonfinite values.

The owned API is not yet a new WOODLAND grammar. The existing World catalog
has no Pluck melody descriptor; future generated source admission must return
success before committing harmony bookkeeping, counters and hook events.
The manual pad accompaniment remains and is not misrepresented as removed.

Regression evidence: full-pool rejection is waveform-identical to no attempted
third onset; stop matches the unstopped waveform times the defined ramp;
wrong-source release has no effect; repeated release preserves the 882-sample
stop deadline; slots cannot free early; Clear stops both manual String and
unowned sparkle through the real engine. Idle direct/send output is zero.
Hot-path lint now explicitly includes Pluck, which was missing from its list.

Host object static data: 8,288 -> 8,300 bytes (+12); voice pool 8,272 -> 8,288.
This is object-level host accounting, not an ARM linker-map result. No new
sample/delay buffer, allocation or per-sample transcendental call was added.
The full H743 map, peak CPU and physical listening gates remain open.

Verification after the Pluck unit: full host suite exit 0; dedicated ownership
regression 0 failures; engine integration and all 489,609 effect checks pass.
Hot-path lint: 26 modules, 0 forbidden calls. No listening verdict inferred.

## 2026-10-06: admitted sources, then musical state

`engine_try_world_note_on` validates input and bounded World-family occupancy,
then asks the actual source to start. Only success updates held pitch, tail
horizon and note-on hook. Source rejection restores the micro-humanization RNG
and emits no phantom MIDI. The scheduler commits last tone, note/phrase counts,
replay count and gentle-return completion only for an accepted onset. A failed
start preserves the motif cursor and waits 2 seconds before another attempt.
Harmony selection remains the caller's job; the generator still passes its
existing pitch-world/register/collision chain before calling admission.

Bowed/Horn/Choir have a strict idle-slot API separate from legacy manual
handovers. Releases and queued onsets occupy slots. Guembri compatibility
one-shots also have a strict non-stealing start; their natural decay and old
timbre are retained, not promoted to a new core World. Pluck uses its existing
owned admission. Across these families, generated admission stops at three
real DSP slots. This gate includes World-family manual tails too. It does not
count legacy pad/bass/Eno/Ember/drone/effect tails and does not restrict all
legacy entry points; removal of accompaniment and the global budget remain
explicit next work. `engine_active_voices` is still the legacy pad statistic.

Verification: 76 strict-source checks (full pool, pending reservations,
waveform-identical rejected onsets, maximum Shape release); 7,982 generator
checks including mixed-family saturation and no phantom MIDI/harmony/phrase
history; full host suite exit 0, 489,609 effect checks; hot-path lint 26 modules
and 0 forbidden calls. Long scheduler audits now render the actual World
sources for matching elapsed time, rather than advancing minutes of decisions
with milliseconds of envelope progression. No audio file over 30 s is exported.

Object-level static data is unchanged in engine/Bowed/Horn/Choir/Guembri;
no new pool, audio buffer, heap allocation or per-sample transcendental call.
This is host object accounting; target linker and device timing remain distinct.

The existing GitHub H743 job was found to cross-build successfully for the
previous commit aab3c5dc826238ecd1e430aace6561f29e4442f5 (run 37233599865).
Its link report: Flash 298,016 B; DTCM 119,440 B; D1 417,448 B; D2 258,100 B;
D3/ITCM 0 B. It did not explicitly request Release, so those values are a
previous-build baseline, not current optimized results or a CPU guarantee.
H743 CI now sets CMAKE_BUILD_TYPE=Release, prints the compiler flags and retains
.bin/.hex/.map artifacts for 14 days. New Release results are pending its run.
No on-device deadline or stack high-water result is inferred from linking.
