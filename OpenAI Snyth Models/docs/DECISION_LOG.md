# Decision log

## 2026-07-16 — Clean-origin rebuild

The package was rebuilt as an isolated original implementation after the
temporary development workspace was automatically cleaned. The restored
AMBIENT archive served only as a hardware/API context reference; no source from
other repository folders was copied into this directory.

## 2026-07-16 — Synthesis instead of samples

The commercial core uses procedural synthesis only. This removes sample
redistribution ambiguity, saves storage, allows continuous macro control, and
keeps every preview reproducible from reviewed source.

## 2026-07-16 — Shared memory architecture

One fixed context is reused across ten models. A shared spatial network and
short chorus provide family coherence and avoid the RAM cost of ten resident
engines.

## 2026-07-16 — Five foundational visual systems

Five reusable renderers cover the ten sounds through explicit default mapping.
This gives each family a visual identity while constraining code size, test
surface, and display CPU.

## 2026-07-16 — Thirteen clean motion translations

Ten user-supplied visuals and three public Pinterest previews were analyzed for
general motion and composition principles. Thirteen new renderers were written
from geometric primitives, with recognizable people, animals, hands, clothing,
icons, and exact reference layouts deliberately excluded. The modes reuse the
same fixed visual state; the expansion costs flash and offline preview storage,
not runtime framebuffer or state RAM.

Two color treatments are generated for each new mode so the review set tests
the geometry independently of a single palette choice.

## 2026-07-16 — Direct packed framebuffer rendering

Every visual clips and writes 4-bit pixels in place. No animation history frame
is retained; motion comes from compact particles, phases, and deterministic
geometry.

## 2026-07-16 — Colour through a 16-entry LUT

Colour was added by reinterpreting each existing 4-bit nibble as a palette
index. Twelve original palettes morph between quiet and alive anchors. The
driver needs a 32-byte RGB565 table, while framebuffer size, SPI transfer size,
visual state, and audio state remain unchanged.

Pillow is used only after the C renderer to package preview GIFs with one stable
global palette. It is neither linked into firmware nor used to invent display
geometry.

## 2026-07-16 — Saturated neon hierarchy

The initial palette set washed high tone indices toward pastel white and used
RGB interpolation between some opposing hues. The revision reserves index 14
for a strongly saturated colour body, index 15 for small specular highlights,
and morphs quiet/alive states through HSV hue space. This changes constants and
control-rate LUT math but adds no visual-state or framebuffer memory.

The preview encoder now builds its global GIF palette from equal-weight exact
C-renderer swatches instead of resized frame thumbnails. This prevents black
background area from suppressing rare bright colors.
