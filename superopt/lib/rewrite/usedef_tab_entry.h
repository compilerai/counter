#ifndef USEDEF_TAB_ENTRY_H
#define USEDEF_TAB_ENTRY_H
#include "valtag/regset.h"
#include "valtag/memset.h"
#include "rewrite/insn_db.h"

typedef struct usedef_tab_entry_t {
  //bool src;
  //struct /*union */ { src_insn_t src_insn; dst_insn_t dst_insn; } i; //should have been a union but the constituent structs can be non-trivial and so union is not allowed
  regset_t use, def;
  memset_t memuse, memdef;
  //struct usedef_tab_entry_t *next;
} usedef_tab_entry_t;

shared_ptr<insn_db_t<src_insn_t, usedef_tab_entry_t>>
src_usedef_tab_read_from_file(char const *file);

shared_ptr<insn_db_t<dst_insn_t, usedef_tab_entry_t>>
dst_usedef_tab_read_from_file(char const *file);
#endif
