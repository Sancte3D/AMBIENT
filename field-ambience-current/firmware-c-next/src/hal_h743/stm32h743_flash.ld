/*
 * stm32h743_flash.ld — Field Ambience STM32H743VIT6 linker script
 * (r18.86, Step 13.3).
 *
 * Memory-placement policy (ADR-0015 D3):
 *   - Stack lives at the top of DTCM (fastest, zero-wait; the CPU-only
 *     render scratch on the stack benefits, and DMA never touches stacks).
 *   - .data/.bss/heap live in AXI SRAM (D1, 0x24000000, 512 KB) — DMA1/DMA2
 *     CANNOT reach DTCM, and the SAI audio ping-pong buffer is a plain .bss
 *     object, so the default segment must be DMA-reachable. Engine buffers
 *     (~200 KB reverb/echo/blur/pads) also fit here comfortably.
 *   - ITCM + SRAM1..4 are left unmapped-by-default (future: hot render loop
 *     into ITCM, DMA2D scratch into D2) — add sections when needed.
 *
 * Symbols consumed by vendor/CMSIS/Device/ST/STM32H7xx/Source/
 * startup_stm32h743xx.s: _estack, _sidata, _sdata, _edata, _sbss, _ebss.
 */

ENTRY(Reset_Handler)

MEMORY
{
  /* r19.22: the last 128 KB sector (bank 2 sector 7, 0x081E0000) is reserved
   * for persistent Scenes storage — see hal_h743/scenes_flash_h743.c
   * (SCENES_FLASH_ADDR). Shrinking FLASH by that sector keeps the linker from
   * ever placing code/data into it (firmware is ~203 KB today, far below, but
   * this guards against future growth silently colliding with the store). */
  FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 1920K   /* 2048K - 128K scenes */
  DTCMRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1  (xrw) : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2  (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3  (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
}

/* Stack at the top of DTCM; heap minimum asserted below. */
_estack        = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
_Min_Heap_Size  = 0x1000;   /* 4 KB  — newlib-nano internal use only     */
_Min_Stack_Size = 0x4000;   /* 16 KB — render path is static, menu light */

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  /* Initialised data → AXI SRAM (D1), load image in FLASH. */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM_D1 AT> FLASH

  /* --- Large-DSP-buffer placement (r18.86) -------------------------------
   * The full engine's zero-init state (~630 KB) exceeds the 512 KB AXI
   * SRAM, so the biggest CPU-ONLY buffers are steered to other domains by
   * OBJECT FILE — the shared DSP sources stay MCU-agnostic (no section
   * attributes in src/). Constraint (ADR-0015 D3): only DMA-touched
   * memory must avoid DTCM; all of these are touched exclusively by the
   * CPU inside engine_render(), never by DMA.
   *
   * NB: startup_stm32h743xx.s zeroes ONLY _sbss.._ebss — main() zeroes
   * these two regions FIRST THING via the _s/_e symbols below. */

  /* pad.c voice state (~100 KB) → DTCM: hottest per-sample state, and DTCM
   * is the fastest RAM on the chip (0-wait TCM port, not via AXI). Shares
   * DTCM with the stack; the ._stack_guard below still asserts the fit. */
  .dtcm_bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sdtcm_bss = .;
    *pad.c.obj(.bss .bss* COMMON)
    . = ALIGN(4);
    _edtcm_bss = .;
  } >DTCMRAM

  /* r19.41: the master-effects adapter (storage 4 KB + hot arena 240 KB +
   * interleave scratch 4 KB ≈ 248 KB) → D2 SRAM1-3 (288 KB). This is the
   * region the legacy echo (~207 KB) + blur (~69 KB) delay lines occupied;
   * those modules are host-test-only now (out of the target link). CPU-only
   * access from engine_render — never DMA — so D2 is safe per ADR-0015 D3.
   * Confirm placement + margin in the .map after any change here. */
  .d2_bss (NOLOAD) :
  {
    . = ALIGN(32);
    _sd2_bss = .;
    *fx_master.c.obj(.bss .bss* COMMON)
    . = ALIGN(4);
    _ed2_bss = .;
  } >RAM_D2

  /* Zero-init data → AXI SRAM (D1). The SAI ping-pong audio buffer lives
   * here (32-byte aligned via attribute) so DMA1 can read it. */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM_D1

  /* Heap grows after .bss in D1 (newlib-nano; the firmware itself never
   * mallocs on the audio path). Stack is separately in DTCM (_estack). */
  ._user_heap :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
  } >RAM_D1

  /* Assert the DTCM stack allowance fits (it trivially does — nothing else
   * is placed in DTCM — but keep the guard for future DTCM sections). */
  ._stack_guard :
  {
    . = ALIGN(8);
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
