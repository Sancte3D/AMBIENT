# Build-tool licenses

The firmware implementation in this package remains original C code and has no
runtime dependency on an external graphics library. Preview generation uses the
following separately installed development tool:

## Pillow

- Purpose: convert C-rendered RGB frames into stable-palette animated GIFs,
  build labelled contact sheets, and measure user-provided motion references
  without importing those references into firmware or generated assets.
- Distribution: Pillow itself is not copied into this repository or linked into
  device firmware.
- License: MIT-CMU.
- Copyright: Pillow contributors; PIL copyright holders as listed in Pillow's
  `LICENSE` file.
- Upstream license: <https://github.com/python-pillow/Pillow/blob/main/LICENSE>
- Format documentation:
  <https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html#gif>

The encoder uses documented `save_all`, `append_images`, duration, loop,
disposal, palette, and optimization options. Generated GIFs contain only this
project's original framebuffer geometry and palette data; no upstream example
artwork or source asset is embedded.
