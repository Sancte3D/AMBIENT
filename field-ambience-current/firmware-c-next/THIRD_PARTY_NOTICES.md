# Third-party notices

Open-source code used in this firmware (policy: see `CLAUDE.md`,
"Open-source DSP"). Each ported file keeps its original license header.

## Mutable Instruments eurorack — MIT

- Upstream: https://github.com/pichenettes/eurorack (commit
  `f4cfa7697095a28d292bff758beb51deb49b5a86`)
- Ported to C in `include/mi_fx.h`:
  - `rings/dsp/fx/reverb.h` — Griesinger/Dattorro reverb with modulated
    long delays (the hall of the instrument Worlds FJORD, MEADOW, GLACIER)
  - `rings/dsp/fx/ensemble.h` — string-machine ensemble chorus
  - `rings/dsp/fx/fx_engine.h` — the shared 16-bit delay-memory scheme
- Only the STM32 projects (MIT) are used; the AVR projects of the same
  repository are GPL-3.0 and are not used.

```
Copyright 2014 Emilie Gillet.

Author: Emilie Gillet (emilie.o.gillet@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
