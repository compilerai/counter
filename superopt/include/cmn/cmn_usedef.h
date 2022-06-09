#include "cmn.h"

size_t
cmn_insn_get_size_for_exreg(cmn_insn_t const *insn, exreg_group_id_t group, reg_identifier_t const &r)
{
#if (CMN == COMMON_SRC) && (ARCH_SRC == ARCH_ETFG)
  return etfg_insn_get_size_for_exreg(insn, group, r);
#endif
#if CMN == COMMON_SRC
  return SRC_EXREG_LEN(group);
#elif CMN == COMMON_DST
  return DST_EXREG_LEN(group);
#else
#error "CMN is neither COMMON_SRC nor COMMON_DST"
#endif
}

void
cmn_regset_truncate_masks_based_on_reg_sizes_in_insn(regset_t *regs, cmn_insn_t const *cmn_insn)
{
  for (auto &g : regs->excregs) {
    for (auto &r : g.second) {
      size_t sz = cmn_insn_get_size_for_exreg(cmn_insn, g.first, r.first);
      r.second &= MAKE_MASK(sz);
    }
  }
}

void
cmn_insn_get_usedef(cmn_insn_t const *cmn_insn, regset_t *use, regset_t *def,
    memset_t *memuse, memset_t *memdef)
{
  autostop_timer func_timer(__func__);
  if (cmn_insn_is_nop(cmn_insn)) {
    regset_clear(use);
    regset_clear(def);
    memset_clear(memuse);
    memset_clear(memdef);
    return;
  }

  ASSERT(usedef_tab_cmn);
  vector<insn_db_match_t<cmn_insn_t, usedef_tab_entry_t>> matched =
        usedef_tab_cmn->insn_db_get_all_matches(*cmn_insn, true);
        //cmn_insn_find_usedef_tab_entry(cmn_insn, &inv_regmap, &inv_imm_vt_map, &nextpc_map);
  if (matched.size() == 0) {
    printf("Could not find usedef_tab entry for '%s'.\n", cmn_iseq_to_string(cmn_insn, 1, as1, sizeof as1));
    fprintf(stderr, "Could not find usedef_tab entry for '%s'.\n", cmn_iseq_to_string(cmn_insn, 1, as1, sizeof as1));
  }
  ASSERT(matched.size() > 0);
  bool regset_inited = false;
  for (const auto &m : matched) {
    regset_t iuse, idef;
    memset_t imemuse, imemdef;
    //autostop_timer func2_timer(string(__func__) + ".2");
    insn_db_list_elem_t<cmn_insn_t, usedef_tab_entry_t> const &entry = m.e;
    regmap_t const &inv_regmap = m.src_iseq_match_renamings.get_regmap();
    imm_vt_map_t const &inv_imm_vt_map = m.src_iseq_match_renamings.get_imm_vt_map();
    nextpc_map_t const &nextpc_map = m.src_iseq_match_renamings.get_nextpc_map();


    usedef_tab_entry_t const &e = entry.v;
    DBG2(SRC_USEDEF, "cmn_insn =\n%s\n", cmn_iseq_to_string(cmn_insn, 1, as1, sizeof as1));
    DBG2(SRC_USEDEF, "e->use = %s\n", regset_to_string(&e.use, as1, sizeof as1));
    DBG2(SRC_USEDEF, "e->def = %s\n", regset_to_string(&e.def, as1, sizeof as1));
    DBG2(SRC_USEDEF, "inv_regmap =\n%s\n", regmap_to_string(&inv_regmap, as1, sizeof as1));
    regset_rename(&iuse, &e.use, &inv_regmap);
    regset_rename(&idef, &e.def, &inv_regmap);
    DBG2(SRC_USEDEF, "iuse = %s\n", regset_to_string(&iuse, as1, sizeof as1));
    DBG2(SRC_USEDEF, "idef = %s\n", regset_to_string(&idef, as1, sizeof as1));

    cmn_regset_truncate_masks_based_on_reg_sizes_in_insn(&iuse, cmn_insn);
    cmn_regset_truncate_masks_based_on_reg_sizes_in_insn(&idef, cmn_insn);

    DBG2(SRC_USEDEF, "e->memuse: %s\n", memset_to_string(&e.memuse, as1, sizeof as1));
    DBG2(SRC_USEDEF, "e->memdef: %s\n", memset_to_string(&e.memdef, as1, sizeof as1));

    DBG2(SRC_USEDEF, "inv_regmap:\n%s\n", regmap_to_string(&inv_regmap, as1, sizeof as1));
    DBG2(SRC_USEDEF, "inv_imm_vt_map:\n%s\n",
        imm_vt_map_to_string(&inv_imm_vt_map, as1, sizeof as1));

    /*{
      static bool printed_warning = false;
      if (!printed_warning) {
        printf("Warning: memset_rename has been commented out!\n");
        printed_warning = true;
      }
    }*/
    memset_rename(&imemuse, &e.memuse, &inv_regmap, &inv_imm_vt_map);
    memset_rename(&imemdef, &e.memdef, &inv_regmap, &inv_imm_vt_map);

    DBG2(SRC_USEDEF, "memuse: %s\n", memset_to_string(memuse, as1, sizeof as1));
    DBG2(SRC_USEDEF, "memdef: %s\n", memset_to_string(memdef, as1, sizeof as1));

    if (!regset_inited) {
      regset_copy(use, &iuse);
      regset_copy(def, &idef);
      memset_copy(memuse, &imemuse);
      memset_copy(memdef, &imemdef);
      regset_inited = true;
    } else {
      regset_intersect(use, &iuse);
      regset_intersect(def, &idef);
      memset_intersect(memuse, &imemuse);
      memset_intersect(memdef, &imemdef);
    }
  }
}

void
cmn_iseq_get_usedef(struct cmn_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def, struct memset_t *memuse,
    struct memset_t *memdef)
{
  autostop_timer func_timer(__func__);
  regset_clear(use);
  regset_clear(def);
  memset_clear(memuse);
  memset_clear(memdef);

  int i;
  for (i = n_insns - 1; i >= 0; i--) {
    regset_t iuse, idef;
    memset_t imemuse, imemdef;

    memset_init(&imemuse);
    memset_init(&imemdef);

    cmn_insn_get_usedef(&insns[i], &iuse, &idef, &imemuse, &imemdef);
    //cout << __func__ << " " << __LINE__ << ": insns[" << i << "] = " << cmn_iseq_to_string(&insns[i], 1, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": iuse = " << regset_to_string(&iuse, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": idef = " << regset_to_string(&idef, as1, sizeof as1) << endl;

    regset_diff(use, &idef);
    regset_union(def, &idef);
    regset_union(use, &iuse);

    //cout << __func__ << " " << __LINE__ << ": use = " << regset_to_string(use, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": def = " << regset_to_string(def, as1, sizeof as1) << endl;

    //memset_diff(&memuse, &imemdef);
    memset_union(memdef, &imemdef);
    memset_union(memuse, &imemuse);

    memset_free(&imemuse);
    memset_free(&imemdef);
  }
}

void
cmn_insn_get_usedef_regs(cmn_insn_t const *insn, regset_t *use,
    regset_t *def)
{
  memset_t memuse, memdef;
  memset_init(&memuse);
  memset_init(&memdef);

  cmn_insn_get_usedef(insn, use, def, &memuse, &memdef);

  memset_free(&memuse);
  memset_free(&memdef);
}

void
cmn_iseq_get_usedef_regs(struct cmn_insn_t const *insns, int n_insns,
    struct regset_t *use, struct regset_t *def)
{
  autostop_timer func_timer(__func__);
  memset_t memuse, memdef;
  memset_init(&memuse);
  memset_init(&memdef);

  cmn_iseq_get_usedef(insns, n_insns, use, def, &memuse, &memdef);

  memset_free(&memuse);
  memset_free(&memdef);

}

dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>
cmn_usedef_tab_read_from_file(char const *file)
{
  autostop_timer func_timer(__func__);
  long max_alloc, iseq_size;
  char *section_buffer;
  cmn_insn_t *cmn_iseq;
  int linenum = 0;
  size_t fs;
  FILE *fp;

  //usedef_tab->hash_size = DEFAULT_USEDEF_TAB_HASH_SIZE;
  //usedef_tab->hash = (usedef_tab_entry_t **)malloc(usedef_tab->hash_size
  //    * sizeof(usedef_tab_entry_t *));
  //usedef_tab->hash = new usedef_entry_list_t[usedef_tab->hash_size];
  //ASSERT(usedef_tab->hash);
  //memset(usedef_tab->hash, 0, usedef_tab->hash_size * sizeof(usedef_tab_entry_t *));

  //printf("opening file %s\n", file);
  fp = fopen(file, "r");
  if (!fp) {
    printf("Could not open file %s: %s\n", file, strerror(errno));
    return dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>::dshared_nullptr();
  }
  ASSERT(fp);

  char *line = new char[2048];
  ASSERT(line);
  dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>> usedef_tab = make_dshared<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>(DEFAULT_USEDEF_TAB_HASH_SIZE);
  ASSERT(usedef_tab);

  iseq_size = 4096;
  max_alloc = 4096;

  section_buffer = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(section_buffer);

  //src_iseq = (src_insn_t *)malloc(iseq_size * sizeof(src_insn_t));
  cmn_iseq = new cmn_insn_t[iseq_size];
  ASSERT(cmn_iseq);

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

    //regset_t use, def;
    long iseq_len;
    int sa;

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    linenum += sa;
    string assembly_section = section_buffer;

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    linenum += sa;
    string serialized_section = string(section_buffer) + "--\n";

    if (cmn_insn_t::uses_serialization()) {
      //ignore the string representation of iseq, and use the serialized representation instead
      istringstream iss(serialized_section);
      iseq_len = cmn_iseq_deserialize(iss, cmn_iseq, iseq_size);
    } else {
      iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, iseq_size, assembly_section.c_str(), I386_AS_MODE_CMN);
        //printf("%s() %d: iseq_len = %d\n", __func__, __LINE__, (int)iseq_len);
#if (CMN==COMMON_SRC && ARCH_SRC == ARCH_I386) || (CMN == COMMON_DST && ARCH_DST == ARCH_I386)
      if (iseq_len == 0) { //for i386, make another attempt with I386_AS_MODE_64
        iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, iseq_size, assembly_section.c_str(), I386_AS_MODE_64);
        ASSERT(iseq_len == 1 || iseq_len == 2);
      }
#endif
    }
    if (iseq_len != 1 && iseq_len != 2) {
      printf("assembly_section: %s\n", assembly_section.c_str());
      printf("serialized_section: %s\n", serialized_section.c_str());
      printf("iseq_len = %d\n", (int)iseq_len);
    }
    ASSERT(iseq_len == 1 || iseq_len == 2);

    //new_entry = (usedef_tab_entry_t *)malloc(sizeof(usedef_tab_entry_t));
    //new_entry = new usedef_tab_entry_t;
    //ASSERT(new_entry);
    usedef_tab_entry_t new_entry;

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    //cout << __func__ << " " << __LINE__ << ": section_buffer = " << section_buffer << endl;
    linenum += sa;
    regset_from_string(&new_entry.use, section_buffer);

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    //cout << __func__ << " " << __LINE__ << ": section_buffer = " << section_buffer << endl;
    linenum += sa;
    regset_from_string(&new_entry.def, section_buffer);

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    //cout << __func__ << " " << __LINE__ << ": section_buffer = " << section_buffer << endl;
    linenum += sa;
    memset_from_string(&new_entry.memuse, section_buffer);

    sa = fscanf_entry_section(fp, section_buffer, max_alloc);
    ASSERT(sa != EOF);
    //cout << __func__ << " " << __LINE__ << ": section_buffer = " << section_buffer << endl;
    linenum += sa;
    memset_from_string(&new_entry.memdef, section_buffer);

    //new_entry->src = true;
    //src_insn_copy(&new_entry->i.src_insn, &src_iseq[0]);
    CPP_DBG_EXEC(INSN_MATCH, cout << __func__ << " " << __LINE__ << ": cmn_iseq[0] = " << cmn_iseq_to_string(&cmn_iseq[0], 1, as1, sizeof as1) << endl);
    usedef_tab->insn_db_add(cmn_iseq[0], new_entry);
    //regset_copy(&new_entry->use, &use);
    //regset_copy(&new_entry->def, &def);
    //usedef_tab_hash_add(usedef_tab, new_entry);
  }

  ASSERT(fs == EOF);
  //CPP_DBG_EXEC(INSN_MATCH, cout << __func__ << " " << __LINE__ << ": done\n");

  fclose(fp);

  delete [] cmn_iseq;
  delete [] line;
  return usedef_tab;
}

static dshared_ptr<insn_db_t<cmn_insn_t, usedef_tab_entry_t>>
cmn_usedef_tab_init(void)
{
  autostop_timer func_timer(__func__);
  char const *file = SUPEROPTDBS_DIR cmn_prefix ".insn.usedef.preprocessed";
  return cmn_usedef_tab_read_from_file(file);
}
