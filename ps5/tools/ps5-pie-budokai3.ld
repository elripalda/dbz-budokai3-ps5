/*
 * ps5-native-app-boilerplate - Intermediate PIE linker layout (SvR 2007 copy:
 * a fixed gap for the process parameters; see [svr07] below).
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Keeps code, immutable data, RELRO, and writable data in distinct address
 * ranges. The native converter replaces LLVM's generic dynamic metadata.
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

PHDRS {
    text PT_LOAD FLAGS (0x5);
    ro PT_LOAD FLAGS (0x4);
    relro PT_LOAD FLAGS (0x6);
    data PT_LOAD FLAGS (0x6);
    dynamic PT_DYNAMIC FLAGS (0x6);
}

SECTIONS {
    .text 0 : ALIGN(CONSTANT(MAXPAGESIZE)) {
        QUAD(0xcccccccccccccccc)
        QUAD(0xcccccccccccccccc)
        *(.text .text.*)
        *(.plt .plt.*)
    } : text

    .eh_frame_hdr : ALIGN(CONSTANT(MAXPAGESIZE)) {
        KEEP(*(.eh_frame_hdr))
    } : ro
    .eh_frame : { KEEP(*(.eh_frame)) } : ro
    .gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) } : ro
    .rodata : { *(.rodata .rodata.*) } : ro

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .data.rel.ro : {
        *(.data.rel.ro .data.rel.ro.*)
    } : relro
    .got : { *(.got) } : relro
    .got.plt : { *(.got.plt) } : relro
    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } : relro
    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
    } : relro
    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } : relro

    /* Kept only long enough for the converter to read and rewrite them. */
    .dynsym : { *(.dynsym) } : relro
    .gnu.hash : { *(.gnu.hash) } : relro
    .dynstr : { *(.dynstr) } : relro
    .rela.dyn : { *(.rela.dyn) } : relro
    .rela.plt : { *(.rela.plt) } : relro
    .note.dlopen : { *(.note.dlopen) } : relro

    /* [svr07] Room for the PS5 process parameters the converter writes after
     * the RELRO content: without it, how close that content ends to a page
     * boundary decides whether the build packages. */
    . = . + 0x4000;
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .dynamic : {
        *(.dynamic)
    } : data : dynamic
    /* [budokai3] The converter writes the process parameters into the file right
     * after the RELRO content, and the file has no gap there (only the address
     * range does): when the RELRO content ends just short of a page boundary
     * they run past .dynamic into .data. Test 1 lost the start of the function
     * table that way. src/ps5/procparam_room.cpp puts writable room here. */
    .ps5_room : { KEEP(*(.ps5_room)) } : data
    .data : { *(.data .data.*) } : data
    .tdata : { *(.tdata .tdata.*) } : data
    .tbss : { *(.tbss .tbss.*) } : data
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } : data
}

/* [budokai3] the code's range, for src/ps5/thread_profile.cpp */
PROVIDE_HIDDEN(__budokai3_text_start = ADDR(.text));
PROVIDE_HIDDEN(__budokai3_text_end = ADDR(.text) + SIZEOF(.text));
PROVIDE(__eh_frame_start = ADDR(.eh_frame));
PROVIDE(__eh_frame_end = ADDR(.eh_frame) + SIZEOF(.eh_frame));
PROVIDE(__eh_frame_hdr_start = ADDR(.eh_frame_hdr));
PROVIDE(__eh_frame_hdr_end = ADDR(.eh_frame_hdr) + SIZEOF(.eh_frame_hdr));
