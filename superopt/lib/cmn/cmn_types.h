#include "cmn.h"

#if 0
static bool
ts_tab_entry_exists_cmn(insn_db_t<cmn_insn_t, ts_tab_entry_t> *tab/*, bool src, src_insn_t const *src_insn,
    dst_insn_t const *dst_insn*/, cmn_insn_t const *cmn_insn)
{
  long i;

  if (src) {
    i = src_iseq_hash_func_after_canon(src_insn, 1, tab->hash_size);
  } else {
    i = dst_iseq_hash_func_after_canon(dst_insn, 1, tab->hash_size);
  }
  for (auto iter = tab->hash[i].begin(); iter != tab->hash[i].end(); iter++) {
    ts_tab_entry_t *cur = *iter;
    if (src) {
      if (src_iseqs_equal(&cur->i.src_insn, src_insn, 1)) {
        return true;
      }
    } else {
      if (dst_iseqs_equal(&cur->i.dst_insn, dst_insn, 1)) {
        return true;
      }
    }
  }
  return false;
/*
  ts_tab_entry_t const *cur;

  for (cur = head; cur; cur = cur->next) {
    if (src) {
      if (src_iseqs_equal(&cur->i.src_insn, src_insn, 1)) {
        return true;
      }
    } else {
      if (dst_iseqs_equal(&cur->i.dst_insn, dst_insn, 1)) {
        return true;
      }
    }
  }
  return false;
*/
}
#endif

void
cmn_ts_tab_read_from_file(insn_db_t<cmn_insn_t, ts_tab_entry_t> *ts_tab, char const *file)
{
  long max_alloc, iseq_size;
  //ts_tab_entry_t *new_entry;
  char *section_buffer;
  cmn_insn_t *cmn_iseq;
  //dst_insn_t *dst_iseq;
  int linenum = 0;
  char line[2048];
  size_t fs;
  FILE *fp;

  //ts_tab->hash_size = DEFAULT_TS_TAB_HASH_SIZE;
  //ts_tab->hash = new ts_entry_list_t[ts_tab->hash_size];
  //ts_tab->head = NULL;
  //ASSERT(ts_tab->hash);
  fp = fopen(file, "r");
  if (!fp) {
    cout << __func__ << " " << __LINE__ << ": could not open file for reading: " << file << endl;
    return;
  }
  ASSERT(fp);

  iseq_size = 4096;
  max_alloc = 4096;

  section_buffer = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(section_buffer);

  cmn_iseq = new cmn_insn_t[iseq_size];
  ASSERT(cmn_iseq);

  //ts_tab_entry_t *cur = NULL;
  long ts_tab_num_entries = 0;
  char delim;
  while (   (fs = fscanf(fp, "%[^\n;]%c", line, &delim)) != EOF
         && (delim == ';' || delim == '\n')) {
    char const *remaining = NULL;
    linenum++;
    if (fs == 0) {
      fread(line, 1, 1, fp);
      ASSERT(line[0] == '\n');
      continue;
    }
    if (line[0] == '#') {
      continue;
    } else if (   !strstart(line, "ENTRY:", &remaining)
               && !strstart(line, "NEW_ENTRY:", &remaining)) {
      err ("Parse error in linenum %d: %s\n", linenum, line);
      ASSERT (0);
    }
    ASSERT(remaining);

    ts_tab_entry_transfer_t *trans;
    ts_tab_entry_cons_t *cons;
    long iseq_len;
    int sa;

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    linenum += sa;

    if (cmn_insn_t::uses_serialization()) {
      //ignore the string representation of iseq, and use the serialized representation instead
      sa = fscanf_entry_section(fp, section_buffer, max_alloc);
      ASSERT(sa != EOF);
      string section = string(section_buffer) + "--\n";
      linenum += sa;
      istringstream iss(section);
      iseq_len = cmn_iseq_deserialize(iss, cmn_iseq, iseq_size);
    } else {
      iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, iseq_size, section_buffer, I386_AS_MODE_32);
      DBG(TYPESTATE, "section_buffer:\n%s\ncmn_iseq:\n%s\n", section_buffer,
            cmn_iseq_to_string(cmn_iseq, iseq_len, as1, sizeof as1));
      //iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, iseq_size, section_buffer, I386_AS_MODE_32);
        //printf("%s() %d: iseq_len = %d\n", __func__, __LINE__, (int)iseq_len);
#if (CMN==COMMON_SRC && ARCH_SRC == ARCH_I386) || (CMN == COMMON_DST && ARCH_DST == ARCH_I386)
      if (iseq_len == 0) {
        iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, iseq_size, section_buffer, I386_AS_MODE_64);
        ASSERT(iseq_len == 1 || iseq_len == 2);
      }
#endif
    }

    /*if (iseq_len != 1) {
      cout << __func__ << " " << __LINE__ << ": iseq_len = " << iseq_len << " for\n" << section_buffer << endl;
    }*/
    //ASSERT(iseq_len == 1);

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    linenum += sa;
    cons = parse_ts_entry_constraints(section_buffer);

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    linenum += sa;
    trans = parse_ts_entry_transfers(section_buffer);
    DBG_EXEC3(TYPESTATE,
      ts_tab_entry_transfer_t const *te;
      printf("section_buffer:\n%s\n", section_buffer);
      printf("Printing transfers:\n");
      for (te = trans; te; te = te->next) {
        ts_tab_entry_transfer_print(stdout, te);
      }
    );
    trans = trans_add_must_happen_after_edges(trans);
    DBG_EXEC(TYPESTATE, print_transfers_with_edges(trans, "after breaking cycles"););

    /*if (ts_tab_entry_exists_cmn(ts_tab, &cmn_iseq[0])) {
      continue;
    }*/

    //new_entry = (ts_tab_entry_t *)malloc(sizeof(ts_tab_entry_t));
    ts_tab_entry_t new_entry;// = new ts_tab_entry_t;
    //ASSERT(new_entry);
    /*if (src) {
      new_entry->src = true;
      src_insn_copy(&new_entry->i.src_insn, &src_iseq[0]);
    } else {
      new_entry->src = false;
      dst_insn_copy(&new_entry->i.dst_insn, &dst_iseq[0]);
    }*/
    new_entry.cons = cons;
    new_entry.trans = trans;
    new_entry.tc_iseq_len = tc_iseq_generate(&new_entry.tc_iseq, new_entry.cons,
        new_entry.trans);
    //new_entry->next = NULL;

    /*if (!cur) {
      ASSERT(!ts_tab->head);
      ts_tab->head = new_entry;
    } else {
      ASSERT(ts_tab->head);
      cur->next = new_entry;
    }
    cur = new_entry;*/
    //ts_tab_hash_add(ts_tab, new_entry);
    ts_tab_cmn.insn_db_add(cmn_iseq[0], new_entry);
    ts_tab_num_entries++;
  }
  DBG(TYPESTATE, "ts_tab_num_entries = %ld.\n", ts_tab_num_entries);

  free(section_buffer);
    //free(src_iseq);
  delete [] cmn_iseq;
  fclose(fp);

  //ts_tab_print(stdout, ts_tab);
}

static void
cmn_ts_tab_init(insn_db_t<cmn_insn_t, ts_tab_entry_t> *ts_tab/*, bool src*/)
{
  char const *file = SUPEROPTDBS_DIR cmn_prefix ".insn.types.preprocessed";
  cmn_ts_tab_read_from_file(ts_tab, file);
}

static ts_tab_entry_t const *
cmn_insn_find_ts_tab_entry(insn_db_t<cmn_insn_t, ts_tab_entry_t> &types_db, cmn_insn_t const *insn, regmap_t *inv_src_regmap, imm_vt_map_t *inv_imm_vt_map, nextpc_map_t *nextpc_map)
{
  vector<insn_db_match_t<cmn_insn_t, ts_tab_entry_t>> matched = types_db.insn_db_get_all_matches(*insn, true);
  if (matched.size() == 0) {
    return NULL;
  }
  const auto &m = matched.at(0);
  regmap_copy(inv_src_regmap, &m.src_iseq_match_renamings.get_regmap());
  imm_vt_map_copy(inv_imm_vt_map, &m.src_iseq_match_renamings.get_imm_vt_map());
  nextpc_map_copy(nextpc_map, &m.src_iseq_match_renamings.get_nextpc_map());
  return &m.e.v;
}
