#ifndef JTABLE_GENCODE_H
#define JTABLE_GENCODE_H

#include <stdlib.h>

struct jtable_entry_t;
class fixed_reg_mapping_t;
class regset_t;

void jtable_init_gencode_functions(char *outfile, size_t outfile_size,
    struct jtable_entry_t const *jentries, long num_entries, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs);

#endif /* jtable_fixup_code.h */
