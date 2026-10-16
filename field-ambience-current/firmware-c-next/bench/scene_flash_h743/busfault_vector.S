.syntax unified
.cpu cortex-m7
.thumb
.section .text.BusFault_Handler,"ax",%progbits
.balign 2
.global BusFault_Handler
.type BusFault_Handler,%function
.thumb_func
BusFault_Handler:
    b scenes_flash_h743_async_busfault_entry
.size BusFault_Handler, .-BusFault_Handler
.section .note.GNU-stack,"",%progbits
