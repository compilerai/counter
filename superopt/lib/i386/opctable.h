#ifndef PEEP_OPCTABLE_H
#define PEEP_OPCTABLE_H
#include <stdio.h>
#include "support/types.h"

typedef int opc_t;
#define opc_inval 0

#define FGRPS_DPNUM(dp, rm)    
#define FLOAT_MEM_DPNUM(fp_index)

#ifdef __cplusplus
extern "C" {
#endif
void opctable_init(void);
char const *opctable_name(opc_t opc);
opc_t opctable_find(int dp_num, i386_as_mode_t mode, int rex, int sizeflag);
opc_t opc_nametable_find(char const *name);
void opctable_insert(char const *name, int dp_num, i386_as_mode_t mode, int rex, int sizeflag);
#ifdef __cplusplus
}
#endif

int opctable_size(void);
void opctable_print(FILE *fp);
size_t opctable_list_all(char *opcodes[], size_t opcodes_size);

#endif
