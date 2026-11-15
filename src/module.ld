ENTRY(_start)
PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    dynamic PT_DYNAMIC FLAGS(6);
}
SECTIONS {
    . = 0;
    .text : { KEEP(*(.text.start)) *(.text .text.*) } :text
    . = ALIGN(0x1000);
    .rodata : {
        LONG(0); LONG(7); BYTE(104); BYTE(121); BYTE(114); BYTE(117);
        BYTE(108); BYTE(101); BYTE(0); BYTE(0);
        *(.rodata .rodata.*)
    } :rodata
    .dynsym : { *(.dynsym) } :rodata
    .dynstr : { *(.dynstr) } :rodata
    .hash : { *(.hash) } :rodata
    .rela.dyn : { *(.rela.dyn) } :rodata
    . = ALIGN(0x1000);
    .data : { *(.data .data.*) *(.got .got.*) } :data
    .dynamic : { *(.dynamic) } :data :dynamic
    .bss (NOLOAD) : {
        . = ALIGN(16);
        HIDDEN(__bss_start = .);
        *(.bss .bss.*) *(COMMON)
        . = ALIGN(16);
        HIDDEN(__bss_end = .);
    } :data
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.interp) }
}
