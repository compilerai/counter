void
consts_read_sym_exec_reader_file(FILE *ifp)
{
  g_ctx->read_sym_exec_file_expr_table(ifp);
  se.read_sym_exec_file_consts(ifp);
  se.read_sym_exec_file_base_state(ifp);
}

void
cmn_read_sym_exec_file_entry_for_insn(cmn_insn_t const *insn, FILE *ifp)
{
  cmn_insn_t *insn_local;
  tfg_ro_t *tfg_ro;

  //insn_local = (cmn_insn_t *)malloc(sizeof(cmn_insn_t));
  insn_local = new cmn_insn_t;
  ASSERT(insn_local);
  cmn_insn_read_sym_exec_file(insn_local, ifp);
  ASSERT(cmn_insns_equal(insn, insn_local));
  //free(insn_local);
  delete insn_local;

  tfg_ro = (tfg_ro_t *)malloc(sizeof(tfg_ro_t));
  ASSERT(tfg_ro);
  se.read_sym_exec_file_tfg(tfg_ro, ifp);
  free(tfg_ro);

  //NOT_IMPLEMENTED(); //add to src_m_tfg and dst_m_tfg
}

void
cmn_read_sym_exec_reader_file(struct cmn_insn_list_elem_t *ls, FILE *ifp)
{
  struct cmn_insn_list_elem_t *ptr;

  for (ptr = ls; ptr; ptr = ptr->next) {
    cmn_read_sym_exec_file_entry_for_insn(&ptr->insn, ifp);
  }
}
