#ifndef _QEMU_DISAS_H
#define _QEMU_DISAS_H

//#include <cpu-defs.h>
/*
size_t snprint_insn_i386(char *buf, size_t buf_size, void *code, unsigned long size,
    uint32_t start_addr);
size_t snprint_insn_ppc(char *buf, size_t buf_size, src_ulong pc, uint8_t *code,
    src_ulong size, int flags);
    */

/* Look up symbol for debugging purpose.  Returns "" if unknown. */
//const char *lookup_symbol(src_ulong orig_addr);

/* Return the start addr of symbol sym. Return -1 if unknown symbol */
//long int symbol_start(char const *symbol);

/* Return the stop addr of symbol sym. Return -1 if unknown symbol */
//long int symbol_stop(char const *symbol);

/* Filled in by elfload.c.  Simplistic, but will do for now. */
/*
extern struct syminfo {
    unsigned int disas_num_syms;
    void *disas_symtab;
    const char *disas_strtab;
    struct syminfo *next;
} *syminfos;
*/

#endif /* _QEMU_DISAS_H */
