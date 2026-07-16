# Eighteen display directions

All concepts render directly into the existing 320 × 170 packed luminance
buffer. They use six already-available audio descriptors—RMS, bass, mid, high,
centroid, and beat phase—so no FFT is required inside the display renderer.

| Concept | Sound mapping | Interaction feeling | Cost profile |
|---|---|---|---|
| RESONANT GARDEN | bass → height; RMS → stem light; high → pollen | alive, gentle, organic | 15 stems + 96 small particles |
| ORBITAL LOOM | bands → orbit dimensions; high → nodes | cosmic instrument / harmonic machine | seven 64-segment rings |
| SPECTRAL CANYON | bass → relief; mid → depth; high → detail | immersive landscape | 13 low-resolution ridges |
| RAIN MEMORY | high → drops; bass → ring size; RMS → glow | sound leaves visible memory | 96 bounded drops + line bands |
| DREAM TOPOGRAPHY | bands → contour deformation; high → beacons | strange, meditative map | 17 contours, no particles required |

Thirteen additional directions reuse the same state and framebuffer:

| Concept | Sound mapping | Interaction feeling | Cost profile |
|---|---|---|---|
| FOCUS RAIL | RMS → focus brightness; time → row | explicit, calm instrument UI | five rows + geometric icons |
| PRISM VEINS | bands → vertical reach and bend | neon spectral organism | 46 bounded streaks |
| CRYSTAL CHOIR | high → shimmer; bass → reach | bright crystalline field | 64 mirrored needles |
| RADIANT GATE | RMS → rays; mid → aperture | non-figurative energy portal | up to 44 rays + seven ellipses |
| LUMEN RIBBON | bass → undulation; high → fibres | slow living signal | 25 procedural segments |
| GLYPH RELAY | RMS/high → trace and head | legible path reveal | 16-point authored glyph |
| PARTICLE CURRENT | bands → spiral width/aspect | cosmic flowing matter | existing 96-point capacity |
| SIGNAL CHAMBER | high → rain speed; RMS → grid | deep wireframe room | perspective grid + 96 signals |
| RESONANCE ORB | bands → shell deformation | tactile harmonic body | six 96-segment contours |
| GLITCH HALO | bands → cell field deformation | digital but elegant | 84 outlined cells |
| TWIN PULSE | bass/mid → amplitude and lobes | bilateral breathing waveform | six mirrored contours |
| CHROMA FALL | bass/mid → shell depth/motion | liquid light architecture | eleven nested shells |
| SOFTBURST | high → ray count/length | colorful transient bloom | up to 54 rounded rays |

## Recommended product arrangement

Use one visual per sound family but allow the user to override it:

| Sound models | Default visual |
|---|---|
| ACID RAIN, BAMBOO CIRCUIT | RAIN MEMORY |
| FM GLASS, GLASS ORBIT | ORBITAL LOOM |
| CHORUS MIST, NACRE HORIZON | RESONANT GARDEN |
| ION STORM, HOLLOW CHOIR | SPECTRAL CANYON |
| TIDEGLASS, LUMEN SWARM | DREAM TOPOGRAPHY |

That gives each sound a recognizable home without creating ten separate UI
systems. A 250–400 ms luminance crossfade can hide visual switches; if RAM does
not allow two frames, crossfade by reducing the outgoing renderer's tone range
for eight frames, switch, then raise the incoming range for eight frames.

## Readability rules

- Keep the top 14 pixels available for the existing status strip.
- Freeze particle spawning while menus are open, but keep low-rate background
  motion so the device still feels alive.
- Quantize audio features at the UI refresh rate and apply attack/release
  smoothing before this module.
- Cap the renderer at 24–30 fps. Ambient movement benefits from restraint and
  the saved CPU belongs to audio.
- Do not encode critical state only through animation. Model name, battery, and
  menu focus remain explicit text/icons.

## Colour without a larger framebuffer

The new palette layer treats each existing 4-bit pixel as a colour index rather
than only a grey value. A 16-entry RGB565 LUT costs 32 bytes and can be rebuilt
at UI rate. This creates multicolour gradients and slow hue breathing while the
framebuffer remains exactly 27,200 bytes.

Twelve palettes are included: NACRE DAWN, TIDAL PRISM, EMBER MOSS, ION VIOLET,
LUNAR PEACH, ARCTIC BLOOM, ACID PETAL, COPPER RAIN, DEEP CORAL, GHOST ORCHID,
SOLAR INK, and BIOLUME. Each has quiet and alive anchor sets; the palette phase
morphs between them without altering geometry or audio state. The morph now
travels through HSV hue space rather than averaging opposing RGB channels, so
it retains chroma throughout the animation. Index 14 is the saturated neon
anchor; index 15 is used only for restrained near-white sparkle cores.

For the existing driver, the integration is a small generalization of its
16-entry grey-to-RGB565 LUT: call `ambient_palette_build_rgb565`, then index the
result with the framebuffer nibble during row conversion. No framebuffer
format conversion is needed.

## Preview files

Generated base previews are stored in `ui/previews` as one animated GIF and one
PNG per foundational concept. `ui/previews/color` adds fifteen 96-frame colour
GIFs and matching stills. `ui/previews/motion-studies` adds 26 more 96-frame
GIFs—two palette directions for each of the thirteen new systems. The labelled
overviews are `ui/previews/color-contact-sheet.png`,
`ui/previews/motion-primary-contact-sheet.png`, and
`ui/previews/motion-contact-sheet.png`.

Geometry and RGB values come from the C framebuffer and palette code. Pillow is
used only to collect the renderer's exact colour swatches, quantize them into a
stable GIF palette, and package the animation; it does not design or paint the
frames. Swatches are weighted independently of pixel area so thin neon lines
cannot be averaged into the black background.
