#ifndef SYM_EXEC_DB_H
#define SYM_EXEC_DB_H

void consts_gen_sym_exec_reader_file(FILE *ofp);
void src_gen_sym_exec_reader_file(struct src_insn_list_elem_t *ls, FILE *ofp);
void dst_gen_sym_exec_reader_file(struct dst_insn_list_elem_t *ls, FILE *ofp);

void src_gen_insn_usedef_db(struct src_insn_list_elem_t *ls, char const *sym_filename,
    char const *out_filename, char const *existing_file);
void dst_gen_insn_usedef_db(struct dst_insn_list_elem_t *ls, char const *sym_filename,
    char const *out_filename, char const *existing_file);

void src_gen_insn_types_db(struct src_insn_list_elem_t *ls, char const *sym_filename,
    char const *out_filename, char const *existing_file);
void dst_gen_insn_types_db(struct dst_insn_list_elem_t *ls, char const *sym_filename,
    char const *out_filename, char const *existing_file);

#endif /* sym_exec_db.h */
