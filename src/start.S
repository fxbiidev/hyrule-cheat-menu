.section .text.start,"ax",%progbits
.global _start
.hidden _start
_start:
    ret
    .word mod0 - _start

.section .rodata.mod0,"a",%progbits
.balign 4
mod0:
    .ascii "MOD0"
    .word _DYNAMIC - mod0
    .word __bss_start - mod0
    .word __bss_end - mod0
    .word 0, 0
    .word module_object - mod0

.section .bss.rtld,"aw",%nobits
.balign 16
module_object:
    .space 0x400
