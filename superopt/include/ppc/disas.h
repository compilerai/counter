#ifndef PPC_DISAS_H
#define PPC_DISAS_H

size_t
snprint_insn_ppc(char *buf, size_t buf_size, target_ulong nip,
    uint8_t *code, target_ulong size, int flags);

#endif /* ppc/disas.h */
