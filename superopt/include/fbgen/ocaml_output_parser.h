#pragma once

void gen_consts_db(char const *out_filename);
void src_gen_sym_exec_db(struct src_insn_list_elem_t *ls, char const *out_filename, char const *existing_file);
void dst_gen_sym_exec_db(struct dst_insn_list_elem_t *ls, char const *out_filename, char const *existing_file);

char *get_consts_struct_str(void);
char *src_get_fresh_state_str(void);
char *dst_get_fresh_state_str(void);



