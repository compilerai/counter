#ifndef TS_TAB_ENTRY_H
#define TS_TAB_ENTRY_H

typedef struct ts_tab_entry_t {
  //bool src;
  //struct /*union*/ { src_insn_t src_insn; dst_insn_t dst_insn; } i;
  ts_tab_entry_cons_t *cons;
  ts_tab_entry_transfer_t *trans;
  dst_insn_t *tc_iseq; //typecheck iseq
  long tc_iseq_len;
  //struct ts_tab_entry_t *next;
} ts_tab_entry_t;

void src_ts_tab_read_from_file(insn_db_t<src_insn_t, ts_tab_entry_t> *ts_tab, char const *file);
void dst_ts_tab_read_from_file(insn_db_t<dst_insn_t, ts_tab_entry_t> *ts_tab, char const *file);


#endif
