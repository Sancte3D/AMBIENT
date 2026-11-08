.syntax unified
.cpu cortex-m7
.thumb

/* Fixed, inspectable faulting instruction. The caller checks faulted before
 * publishing bytes; a skipped load returns the harmless address in r0. */
.section .text.scenes_flash_h743_guarded_read,"ax",%progbits
.balign 2
.global scenes_flash_h743_guarded_read
.type scenes_flash_h743_guarded_read,%function
.thumb_func
scenes_flash_h743_guarded_read:
.global scenes_flash_h743_guarded_load
scenes_flash_h743_guarded_load:
    ldr r0, [r0]
.global scenes_flash_h743_guarded_resume
scenes_flash_h743_guarded_resume:
    bx lr
.size scenes_flash_h743_guarded_read, .-scenes_flash_h743_guarded_read

.section .text.scenes_flash_h743_async_busfault_entry,"ax",%progbits
.balign 2
.global scenes_flash_h743_async_busfault_entry
.type scenes_flash_h743_async_busfault_entry,%function
.thumb_func
scenes_flash_h743_async_busfault_entry:
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp
    mov r1, lr
    b scenes_flash_h743_busfault_dispatch
.size scenes_flash_h743_async_busfault_entry, .-scenes_flash_h743_async_busfault_entry
.section .note.GNU-stack,"",%progbits
