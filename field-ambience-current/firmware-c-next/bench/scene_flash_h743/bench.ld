/* Standalone storage bench: every executable/load byte is in Bank1.
 * Bank2 sectors6/7 are the test journal, never an ELF load segment. */
ENTRY(Reset_Handler)
MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}
_estack = ORIGIN(DTCM) + LENGTH(DTCM);
_Min_Stack_Size = 16K;
__bench_journal_start = 0x081C0000;
__bench_journal_end = 0x08200000;
SECTIONS {
    .isr_vector : { KEEP(*(.isr_vector)) . = ALIGN(4); } > FLASH
    .text : {
        *(.text .text*) *(.glue_7 .glue_7t) *(.eh_frame)
        KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4);
    } > FLASH
    .rodata : { *(.rodata .rodata*) . = ALIGN(4); } > FLASH
    .ARM.extab (READONLY) : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx (READONLY) : {
        __exidx_start = .; *(.ARM.exidx* .gnu.linkonce.armexidx.*) __exidx_end = .;
    } > FLASH
    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH
    .data : {
        . = ALIGN(4); _sdata = .; *(.data .data*) . = ALIGN(4); _edata = .;
    } > DTCM AT > FLASH
    _sidata = LOADADDR(.data);
    __bench_load_end = LOADADDR(.data) + SIZEOF(.data);
    .bss (NOLOAD) : {
        . = ALIGN(32); _sbss = .; KEEP(*(.bench_mailbox))
        *(.bss .bss* COMMON) . = ALIGN(8); _ebss = .;
    } > DTCM
    _end = .; PROVIDE(end = .);
    .ARM.attributes 0 : { *(.ARM.attributes) }
    /DISCARD/ : { *(.note.GNU-stack) }
}
ASSERT(ADDR(.isr_vector) == 0x08000000, "bench vectors must start in Bank1")
ASSERT(__bench_load_end <= 0x08100000, "bench executable/load bytes exceed Bank1")
ASSERT(_ebss + _Min_Stack_Size <= _estack, "bench DTCM stack reserve exhausted")
