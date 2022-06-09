#ifndef SRC_TAG_H
#define SRC_TAG_H

#include <stdbool.h>

typedef enum src_tag_t {
  tag_invalid = 0,
  tag_const = 1,
  tag_var,
  tag_imm_insn_pc,
  tag_imm_nextpc,
  //tag_imm_vregnum,
  tag_imm_vconst_num,
  tag_imm_symbol_num,
  tag_imm_exvregnum,
  tag_imm_symbol,
  tag_imm_local,
  tag_imm_dynsym,
  tag_reloc_string,
  tag_abs_inum, //used temporarily for i386_iseq_window_rename_nextpcs_to_src_pcrels
  tag_sboxed_abs_inum, //used in dst_insn_sandbox()
  tag_binpcrel_inum, //used in dst_insn_sandbox()
  tag_src_insn_pc,
  tag_input_exec_reloc_index,
} src_tag_t;

enum flag_code {
	CODE_32BIT,
	CODE_16BIT,
	CODE_64BIT
};

/*enum assembler_result {
  ASSEMBLER_SUCCEEDED,
  ASSEMBLER_WARNED
};*/

//extern enum assembler_result assembler_result; /* defined in gas/tc-i386.c */
extern enum flag_code flag_code; /* defined in gas/tc-i386.c */
extern bool regular_assembly_only; /* defined in gas/tc-i386.c */
char const *tag2str(src_tag_t tag);

#endif /* src_tag.h */
