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

## 2026-07-16 — Five visual systems, not ten

Five reusable renderers cover the ten sounds through explicit default mapping.
This gives each family a visual identity while constraining code size, test
surface, and display CPU.

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
