#include "cmn.h"
#include "support/src-defs.h"
#include "rewrite/usedef_tab_entry.h"
#include "rewrite/ts_tab_entry.h"
#include "rewrite/static_translate.h"

static void
cmn_print_usedef_mismatch(cmn_insn_t const *insn, regset_t const *regs1,
    regset_t const *regs2, char const *id)
{
  fprintf(stderr, "Warning: %s registers computed from boolean circuits do not match "
      "hand-computed %s registers.\n", id, id);
  fprintf(stderr, "insn: %s\n", cmn_iseq_to_string(insn, 1, as1, sizeof as1));
  fprintf(stderr, "%s (computed from boolean circuit): %s\n", id,
      regset_to_string(regs1, as1, sizeof as1));
  fprintf(stderr, "o%s (hand-computed): %s\n", id,
      regset_to_string(regs2, as1, sizeof as1));
}

#if 0
void
cmn_gen_sym_exec_reader_file(struct cmn_insn_list_elem_t *ls, FILE *ofp)
{
  struct cmn_insn_list_elem_t *ptr;
  //size_t num_insns;

  /*for (ptr = ls, num_insns = 0; ptr; ptr = ptr->next, num_insns++);

  fwrite(&num_insns, 1, sizeof(size_t), ofp);*/
  for (ptr = ls/*, num_insns = 0*/; ptr; ptr = ptr->next/*, num_insns++*/) {
    cmn_gen_sym_exec_reader_file_entry_for_insn(&ptr->insn, ofp);
  }
  //fprintf(ofp, "#define " CMN_M_DB_RO_NUM_ENTRIES_STR " %zd\n", num_insns);
}
#endif
//void
//cmn_get_usedef_using_transfer_function(cmn_insn_t const *insn,
//    regset_t *use, regset_t *def, memset_t *memuse,
//    memset_t *memdef);


void
cmn_gen_insn_usedef_db(struct cmn_insn_list_elem_t *ls, char const *sym_exec_file,
    char const *ofile, char const *existing_file)
{
  autostop_timer func_timer(__func__);
  regset_t use, def, ouse, odef;
  struct cmn_insn_list_elem_t *cur;
  memset_t memuse, memdef;
  int max_alloc = 102400;
  unsigned insn_count;
  char *buf;
  FILE *fp;

  sym_exec se(false);
  se.cmn_parse_sym_exec_db(g_ctx);

  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);
  fp = fopen(ofile, "w");
  ASSERT(fp);

  memset_init(&memuse);
  memset_init(&memdef);

  dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>> usedef_tab = cmn_usedef_db_read(existing_file);
  ASSERT(usedef_tab);
  for (cur = ls; cur; cur = cur->next) {
    //fputs("=insn\n", fp);
    //cout << __func__ << " " << __LINE__ << ": calling insn_db_get_all_matches()\n";
    vector<insn_db_match_t<cmn_insn_t, usedef_tab_entry_t>> matched =
          usedef_tab->insn_db_get_all_matches(*cur->insn, true);
    //cout << __func__ << " " << __LINE__ << ": matched.size() = " << matched.size() << endl;
    bool found_existing = false;
    for (const auto &match : matched) {
      if (cmn_insns_equal(&match.e.i, cur->insn)) {
        regset_copy(&use, &match.e.v.use);
        regset_copy(&def, &match.e.v.def);
        memset_copy(&memuse, &match.e.v.memuse);
        memset_copy(&memdef, &match.e.v.memdef);
        found_existing = true;
        break;
      }
    }

    fputs("\nENTRY:\n", fp);
    cmn_iseq_to_string(cur->insn, cur->num_insns, buf, max_alloc);
    //cout << __func__ << " " << __LINE__ << ": found_existing = " << found_existing << " for " << buf << endl;
    fputs(buf, fp);
    fflush(fp);

    stringstream ss;
    cmn_iseq_serialize(ss, cur->insn, cur->num_insns);
    fputs("--\n", fp);
    fputs(ss.str().c_str(), fp);
    fflush(fp);

    if (!found_existing) {
      se.cmn_get_usedef_using_transfer_function(&cur->insn[0], &use, &def, &memuse,
          &memdef);
    }
    regset_to_string(&use, buf, max_alloc);
    fputs(buf, fp);
    fputs("\n", fp);
    //fputs("=def\n", fp);
    fputs("--\n", fp);
    regset_to_string(&def, buf, max_alloc);
    fputs(buf, fp);
    fputs("\n--\n", fp);
    memset_to_string(&memuse, buf, max_alloc);
    fputs(buf, fp);
    fputs("\n", fp);
    //fputs("=def\n", fp);
    fputs("--\n", fp);
    memset_to_string(&memdef, buf, max_alloc);
    fputs(buf, fp);
    fputs("\n==\n", fp);
    fflush(fp);
  }

  memset_free(&memuse);
  memset_free(&memdef);
  fclose(fp);
  free(buf);
}

void
cmn_gen_sym_exec_db(struct cmn_insn_list_elem_t *ls, char const *file, char const *existing_file)
{
  //peep_entry_t *peep_entry;
  //peep_table_t *peeptab;
  struct cmn_insn_list_elem_t *cur;
  int max_alloc = 409600;
  unsigned insn_count;
  int nextpc_indir;
  char *buf, *str;
  FILE *fp;
  //long i;

  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);

  fp = fopen(file, "w");
  ASSERT(fp);
  fputs("=Base state\n", fp);
  str = cmn_get_fresh_state_str();
  fputs(str, fp);
  free(str);
  //peeptab = &ti->peep_table;
  insn_count = 0;
  for (cur = ls; cur; cur = cur->next) {
    if ((insn_count++ % 100) == 0) {
      PROGRESS("fbgen insn %d", insn_count);
    }
    fputs("=insn\n", fp);
    cmn_iseq_to_string(cur->insn, cur->num_insns, buf, max_alloc);
    fputs(buf, fp);
    if (cmn_insn_is_indirect_branch(&cur->insn[0])) {
      nextpc_indir = 0;
    } else {
      nextpc_indir = -1;
    }
    fflush(fp);
    stringstream ss;
    cmn_iseq_serialize(ss, cur->insn, cur->num_insns);
    fputs("--\n", fp);
    fputs(ss.str().c_str(), fp);

    str = cmn_get_transfer_state_str(&cur->insn[0], cur->num_nextpcs, nextpc_indir);
    fputs(str, fp);
    free(str);
  }
  fclose(fp);
  free(buf);
}

dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>
cmn_usedef_db_read(char const *filename)
{
  dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>> ret;
  if (filename[0] != '\0') {
    CPP_DBG_EXEC(INSN_MATCH, cout << __func__ << " " << __LINE__ << ":\n");
    ret = cmn_usedef_tab_read_from_file(filename);
  }
  if (!ret) {
    CPP_DBG_EXEC(INSN_MATCH, cout << __func__ << " " << __LINE__ << ":\n");
    ret = make_dshared<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>(DEFAULT_USEDEF_TAB_HASH_SIZE);
    ASSERT(ret);
  }
  return ret;
}

