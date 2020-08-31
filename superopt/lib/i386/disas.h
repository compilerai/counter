#ifndef PEEP_I386_DIS_H
#define PEEP_I386_DIS_H
#include <stdlib.h>
#include <stdbool.h>
#include <bfd.h>
#include "support/types.h"

struct i386_insn_t;
#ifdef __cplusplus
extern "C"
#endif
void opc_init(void);
int disas_insn_i386(unsigned char const *code, unsigned long pc,
    struct i386_insn_t *insn, i386_as_mode_t mode);


/*int sprint_insn_i386(bfd_vma pc, char *buf, size_t buf_size);
size_t sprint_iseq_i386(void *code, size_t size, unsigned long start_addr,
    char *buf, size_t buf_size);*/

#endif
