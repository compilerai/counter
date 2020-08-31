#include <map>
#include <iostream>
#include "equiv/jtable.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include "support/debug.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "i386/execute.h"
#include "support/src-defs.h"
#include "superopt/superopt.h"
#include "equiv/equiv.h"
#include "rewrite/transmap.h"
#include "support/c_utils.h"
#include "exec/state.h"
#include "superopt/typestate.h"
#include "superopt/src_env.h"
#include "equiv/jtable_gencode.h"
//#include "imm_map.h"
#include "rewrite/src-insn.h"
#include "support/timers.h"
//#include "rewrite/harvest.h"
#include "valtag/memslot_map.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"
#include "superopt/rand_states.h"
#include "i386/insn.h"
#include "superopt/fingerprintdb_live_out.h"

static char as1[409600];
static char as2[40960];
static char as3[40960];

static void
jtable_entries_print(FILE *fp, itable_t const *itable, jtable_entry_t const *jentries)
{
  static dst_insn_t dst_iseq[256];
  long i, dst_iseq_len;

  for (i = 0; i < itable->get_num_entries(); i++) {
    fprintf(fp, "JENTRY.%ld:\n", i);
    fprintf(fp, "%s",
        dst_insn_vec_to_string(itable->get_entry(i).get_iseq(), as1, sizeof as1));
    /*fprintf(fp, "--\n%s", regmap_to_string(&jtable->jentries[i].regmap, as1,
        sizeof as1));
    fprintf(fp, "--\n%s",
        dst_iseq_to_string(&jtable->jentries[i].cinsn, 1, as1, sizeof as1));

    dst_iseq_len = dst_iseq_disassemble(dst_iseq, jtable->jentries[i].bincode,
        jtable->jentries[i].binsize, NULL, NULL, NULL);*/
    ASSERT(jentries);
    for (size_t l = 0; l < jentries[i].itable_entry_iseq_len; l++) {
      ASSERT(jentries[i].exec_iseq[JTABLE_EXEC_ISEQ_DATA][l]);
      ASSERT(jentries[i].exec_iseq[JTABLE_EXEC_ISEQ_TYPE][l]);
      fprintf(fp, "-- iseq_type.%zd\n%s", l, dst_iseq_to_string(
          jentries[i].exec_iseq[JTABLE_EXEC_ISEQ_TYPE][l],
          jentries[i].exec_iseq_len[JTABLE_EXEC_ISEQ_TYPE][l], as1, sizeof as1));
      fprintf(fp, "-- temps.iseq_type.%zd\n%s\n", l,
          regset_to_string(&jentries[i].jtable_entry_temps[JTABLE_EXEC_ISEQ_TYPE][l], as1, sizeof as1));
      fprintf(fp, "-- iseq_data.%zd\n%s", l,
          dst_iseq_to_string(jentries[i].exec_iseq[JTABLE_EXEC_ISEQ_DATA][l],
              jentries[i].exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][l], as1, sizeof as1));
      fprintf(fp, "-- temps.iseq_data.%zd\n%s\n", l,
          regset_to_string(&jentries[i].jtable_entry_temps[JTABLE_EXEC_ISEQ_DATA][l], as1, sizeof as1));
    }
    fprintf(fp, "-- regcons\n%s",
        regcons_to_string(&jentries[i].regcons, as1, sizeof as1));
    fprintf(fp, "-- use\n%s\n", regset_to_string(&jentries[i].use, as1, sizeof as1));
    fprintf(fp, "-- def\n%s\n", regset_to_string(&jentries[i].def, as1, sizeof as1));
    fprintf(fp, "-- memslot_use\n%s\n", jentries[i].memslot_use.memslot_set_to_string().c_str());
    fprintf(fp, "-- memslot_def\n%s\n", jentries[i].memslot_def.memslot_set_to_string().c_str());
    fprintf(fp, "==\n");
    //fprintf(fp, "-- ctemps\n%s\n==\n",
        //regset_to_string(&jentries[i].ctemps, as1, sizeof as1));
    /*fprintf(fp, "--\n.byte%s==\n", hex_string(jtable->jentries[i].bincode,
            jtable->jentries[i].binsize, as1, sizeof as1));*/
  }
}

void
jtable_print(FILE *fp, jtable_t const *jtable)
{
  fprintf(fp, "JTABLE:\n");
  jtable_entries_print(fp, jtable->itab, jtable->jentries);
}

static size_t
jtable_save_regs(uint8_t *buf, size_t size)
{
  uint8_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  *ptr++ = 0x50; //push rax
  *ptr++ = 0x51; //push rcx
  *ptr++ = 0x52; //push rdx
  *ptr++ = 0x53; //push rbx
  *ptr++ = 0x55; //push rbp
  *ptr++ = 0x56; //push rsi
  *ptr++ = 0x57; //push rdi
  *ptr++ = 0x9c; //pushf

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc0;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc1;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc2;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc3;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc5;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc6;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0xc7;  *ptr++ = 0xc7;
  *(uint32_t *)ptr = 0;    ptr += 4;

  *ptr++ = 0x48;  *ptr++ = 0x89;  *ptr++ = 0x24; *ptr++ = 0x25;
  *(uint32_t *)ptr = SRC_ENV_SCRATCH(0);
  ptr += 4;

  return ptr - buf;
}

static size_t
jtable_restore_regs(uint8_t *buf, size_t size)
{
  uint8_t *ptr, *end;

  ptr = buf;
  end = buf + size;

/*
  *ptr++ = 0x58; //pop rax
  *ptr++ = 0x59; //pop rcx
  *ptr++ = 0x5a; //pop rdx
  *ptr++ = 0x5b; //pop rbx
  *ptr++ = 0x5d; //pop rbp
  *ptr++ = 0x5e; //pop rsi
  *ptr++ = 0x5f; //pop rdi
  *ptr++ = 0x9d; //popf
*/

  return ptr - buf;
}

static size_t
jtable_branch_to_next_exec_tcode(long nextpc_num, uint8_t *pcbuf, uint8_t *buf, size_t size)
{
  uint8_t *pcptr, *ptr, *end;

  ptr = buf;
  end = buf + size;
  pcptr = pcbuf;

  ASSERT(pcptr >= (uint8_t *)0 && pcptr <= (uint8_t *)0xffffffff);

  alloc_exec_nextpcs(nextpc_num + 1);

  *ptr++ = 0xe9;
  pcptr++;

  //ASSERT(exec_next_tcodes[nextpc_num].txinsn_get_tag() == tag_var);
  *(uint32_t *)ptr = (uint32_t)(exec_next_tcodes[nextpc_num] - (pcptr + 4));
  ptr += 4;
  pcptr += 4;
  ASSERT(ptr < end);

  return ptr - buf;
}

static void
jtable_gen_header_and_footer_code(uint8_t *header_bincode, size_t *header_binsize,
    uint8_t *footer_bincode[], size_t footer_binsize[], uint8_t *footer_pc[],
    int num_nextpcs, regmap_t const *regmap, fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    regset_t const &itable_dst_relevant_regs)
{
  size_t max_header_binsize, max_footer_binsize[num_nextpcs];
  uint8_t *pcptr, *ptr, *end;
  long i;

  max_header_binsize = *header_binsize;
  for (i = 0; i < num_nextpcs; i++) {
    max_footer_binsize[i] = footer_binsize[i];
  }

  transmap_t tmap_id = dst_transmap_default(dst_fixed_reg_mapping);
  transmap_intersect_with_regset(&tmap_id, &itable_dst_relevant_regs);
  transmap_rename_using_dst_regmap(&tmap_id, regmap, NULL);
  //cout << __func__ << " " << __LINE__ << ": tmap_id =\n" << transmap_to_string(&tmap_id, as1, sizeof as1) << endl;

  ptr = header_bincode;
  end = header_bincode + max_header_binsize;

  ptr += jtable_save_regs(ptr, end - ptr);
  ptr += transmap_translation_code(&transmap_env_dst(), &tmap_id, -1, fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), I386_AS_MODE_64,
      ptr, end - ptr);
  *header_binsize = ptr - header_bincode;

  for (i = 0; i < num_nextpcs; i++) {
    //transmap_copy(&tmap_id, dst_transmap_default());
    //transmap_rename_using_dst_regmap(&out_tmaps[i], &jtable->cur_regmap);
    if (!max_footer_binsize[i]) {
      break;
    }

    ptr = footer_bincode[i];
    end = footer_bincode[i] + max_footer_binsize[i];

    ptr += transmap_translation_code(&tmap_id, &transmap_env_dst(), -1,
        fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), I386_AS_MODE_64,
        ptr, end - ptr);
    ptr += jtable_restore_regs(ptr, end - ptr);
    pcptr = (ptr - footer_bincode[i]) + footer_pc[i];
    ptr += jtable_branch_to_next_exec_tcode(i, pcptr, ptr, end - ptr);

    footer_binsize[i] = ptr - footer_bincode[i];
  }
}

static void
jtable_gen_save_restore_code(uint8_t *save_code, size_t *save_codesize,
    uint8_t *restore_code, size_t *restore_codesize, regmap_t const *regmap,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  size_t max_save_codesize, max_restore_codesize;
  transmap_t tmap_no_regs;

  max_save_codesize = *save_codesize;
  max_restore_codesize = *restore_codesize;

  //transmap_id_dst(&tmap_id);
  transmap_t tmap_id = dst_transmap_default(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  transmap_rename_using_dst_regmap(&tmap_id, regmap, NULL);
  //transmap_fill_no_regs_using_env_addr(&tmap_no_regs, TYPE_ENV_VALUE_ADDR);
  //transmap_init_env(&tmap_no_regs, TYPE_ENV_VALUE_ADDR);
  transmap_init_env_dst(&tmap_no_regs, SRC_ENV_ADDR);

  *save_codesize = transmap_translation_code(&tmap_id, &tmap_no_regs, -1,
      fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), I386_AS_MODE_64, save_code, max_save_codesize);
  ASSERT(*save_codesize < max_save_codesize);

  *restore_codesize = transmap_translation_code(&tmap_no_regs, &tmap_id, -1,
      fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), I386_AS_MODE_64, restore_code, max_restore_codesize);
  ASSERT(*restore_codesize < max_restore_codesize);
}

static unsigned
cached_bincode_elem_hash(struct myhash_elem const *e, void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_elem_t const *ce;

  ASSERT(e);
  ce = chash_entry(e, struct cached_bincode_elem_t const, h_elem);
#if ARCH_DST == ARCH_I386
  return regmap_hash_mod_group(&ce->regmap, I386_EXREG_GROUP_SEGS);
#else
  return regmap_hash(&ce->regmap);
#endif
}

static bool
cached_bincode_elem_hash_equal(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_elem_t const *ca, *cb;

  ASSERT(a);
  ASSERT(b);
  ca = chash_entry(a, struct cached_bincode_elem_t const, h_elem);
  cb = chash_entry(b, struct cached_bincode_elem_t const, h_elem);

#if ARCH_DST == ARCH_I386
  return regmaps_equal_mod_group(&ca->regmap, &cb->regmap, I386_EXREG_GROUP_SEGS);
#else
  return regmaps_equal(&ca->regmap, &cb->regmap);
#endif
}

static unsigned
cached_bincode_arr_elem_hash(struct myhash_elem const *e, void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_arr_elem_t const *ce;

  ASSERT(e);
  ce = chash_entry(e, struct cached_bincode_arr_elem_t const, h_elem);
#if ARCH_DST == ARCH_I386
  return regmap_hash_mod_group(&ce->regmap, I386_EXREG_GROUP_SEGS);
#else
  return regmap_hash(&ce->regmap);
#endif
}

static bool
cached_bincode_arr_elem_hash_equal(struct myhash_elem const *a, struct myhash_elem const *b,
    void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_arr_elem_t const *ca, *cb;

  ASSERT(a);
  ASSERT(b);
  ca = chash_entry(a, struct cached_bincode_arr_elem_t const, h_elem);
  cb = chash_entry(b, struct cached_bincode_arr_elem_t const, h_elem);

#if ARCH_DST == ARCH_I386
  return regmaps_equal_mod_group(&ca->regmap, &cb->regmap, I386_EXREG_GROUP_SEGS);
#else
  return regmaps_equal(&ca->regmap, &cb->regmap);
#endif
}

static void
jtable_gen_header_and_footer_codes(jtable_t *jtable)
{
  struct cached_bincode_arr_elem_t *fe;
  struct cached_bincode_elem_t *he;
  regmap_t regmap_id_dst;
  struct myhash_elem *ret;
  regmap_t iter;
  bool next;
  int i, j;
  long n;

  myhash_init(&jtable->cached_codes_for_regmaps.header, cached_bincode_elem_hash,
      cached_bincode_elem_hash_equal, NULL);
  myhash_init(&jtable->cached_codes_for_regmaps.footer, cached_bincode_arr_elem_hash,
      cached_bincode_arr_elem_hash_equal, NULL);

  regmap_identity_for_dst(&regmap_id_dst, jtable->jtable_get_fixed_reg_mapping());
  regset_t itable_dst_relevant_regs = jtable->itab->itable_get_dst_relevant_regs();

  for (next = regmap_group_permutation_enumerate_first(&iter, &regmap_id_dst,
           I386_EXREG_GROUP_GPRS, jtable->jtable_get_fixed_reg_mapping()), n = 0;
       next;
       next = regmap_group_permutation_enumerate_next(&iter, &regmap_id_dst,
           I386_EXREG_GROUP_GPRS, jtable->jtable_get_fixed_reg_mapping()), n++) {
    //he = (struct cached_bincode_elem_t *)malloc(sizeof(struct cached_bincode_elem_t));
    he = new cached_bincode_elem_t;
    ASSERT(he);
    //fe = (struct cached_bincode_arr_elem_t *)malloc(sizeof(struct cached_bincode_arr_elem_t));
    fe = new cached_bincode_arr_elem_t;
    ASSERT(fe);

    regmap_copy(&he->regmap, &iter);
    he->bincode = (uint8_t *)malloc(MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
    ASSERT(he->bincode);
    he->binsize = MAX_SAVE_RESTORE_REGS_BINSIZE;

    regmap_copy(&fe->regmap, &iter);
    ASSERT(NELEM(fe->bincode) == NELEM(fe->binsize));
    ASSERT(jtable->itab->get_num_nextpcs() <= NELEM(fe->bincode));
    memset(fe->bincode, 0, sizeof fe->bincode);
    for (i = 0; i < jtable->itab->get_num_nextpcs(); i++) {
      fe->bincode[i] = (uint8_t *)malloc(MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
      ASSERT(fe->bincode[i]);
      fe->binsize[i] = MAX_SAVE_RESTORE_REGS_BINSIZE;
    }

    jtable_gen_header_and_footer_code(he->bincode, &he->binsize, fe->bincode, fe->binsize,
        jtable->footer_bincode, jtable->itab->get_num_nextpcs(), &iter, jtable->jtable_get_fixed_reg_mapping(),
        itable_dst_relevant_regs);

    ASSERT(he->binsize <= MAX_SAVE_RESTORE_REGS_BINSIZE);
    he->bincode = (uint8_t *)realloc(he->bincode, he->binsize * sizeof(uint8_t));
    ASSERT(he->bincode);

    for (i = 0; i < jtable->itab->get_num_nextpcs(); i++) {
      ASSERT(fe->binsize[i] <= MAX_SAVE_RESTORE_REGS_BINSIZE);
      fe->bincode[i] = (uint8_t *)realloc(fe->bincode[i],
          fe->binsize[i] * sizeof(uint8_t));
      ASSERT(fe->bincode[i]);
    }
    ret = myhash_insert(&jtable->cached_codes_for_regmaps.header, &he->h_elem);
    ASSERT(!ret);
    ret = myhash_insert(&jtable->cached_codes_for_regmaps.footer, &fe->h_elem);
    ASSERT(!ret);
  }
}

static void
jtable_gen_save_restore_codes(jtable_t *jtable)
{
  struct cached_bincode_elem_t *se, *re;
  regmap_t iter, regmap_id_dst;
  struct myhash_elem *ret;
  bool next;
  long n;

  myhash_init(&jtable->cached_codes_for_regmaps.save_regs, cached_bincode_elem_hash,
      cached_bincode_elem_hash_equal, NULL);
  myhash_init(&jtable->cached_codes_for_regmaps.restore_regs, cached_bincode_elem_hash,
      cached_bincode_elem_hash_equal, NULL);

  regmap_identity_for_dst(&regmap_id_dst, jtable->jtable_get_fixed_reg_mapping());

  for (next = regmap_group_permutation_enumerate_first(&iter, &regmap_id_dst,
           I386_EXREG_GROUP_GPRS, jtable->jtable_get_fixed_reg_mapping()), n = 0;
       next;
       next = regmap_group_permutation_enumerate_next(&iter, &regmap_id_dst,
           I386_EXREG_GROUP_GPRS, jtable->jtable_get_fixed_reg_mapping()), n++) {
    //cout << __func__ << " " << __LINE__ << ": iter =\n" << regmap_to_string(&iter, as1, 1024) << endl;
    //se = (struct cached_bincode_elem_t *)malloc(sizeof(struct cached_bincode_elem_t));
    se = new cached_bincode_elem_t;
    ASSERT(se);
    //re = (struct cached_bincode_elem_t *)malloc(sizeof(struct cached_bincode_elem_t));
    re = new cached_bincode_elem_t;
    ASSERT(re);

    regmap_copy(&se->regmap, &iter);
    se->bincode = (uint8_t *)malloc(MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
    ASSERT(se->bincode);
    se->binsize = MAX_SAVE_RESTORE_REGS_BINSIZE;

    regmap_copy(&re->regmap, &iter);
    re->bincode = (uint8_t *)malloc(MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
    ASSERT(re->bincode);
    re->binsize = MAX_SAVE_RESTORE_REGS_BINSIZE;

    jtable_gen_save_restore_code(se->bincode, &se->binsize, re->bincode, &re->binsize,
        &iter, jtable->jtable_get_fixed_reg_mapping());
    ASSERT(se->binsize <= MAX_SAVE_RESTORE_REGS_BINSIZE);
    ASSERT(re->binsize <= MAX_SAVE_RESTORE_REGS_BINSIZE);

    se->bincode = (uint8_t *)realloc(se->bincode, se->binsize * sizeof(uint8_t));
    ASSERT(!se->binsize || se->bincode);
    re->bincode = (uint8_t *)realloc(re->bincode, re->binsize * sizeof(uint8_t));
    ASSERT(!re->binsize || re->bincode);
    DBG3(JTABLE, "adding regmap (hash %u) to save_regs:\n%s\n", regmap_hash(&se->regmap),
        regmap_to_string(&se->regmap, as1, sizeof as1));
    ret = myhash_insert(&jtable->cached_codes_for_regmaps.save_regs, &se->h_elem);
    ASSERT(!ret);
    ret = myhash_insert(&jtable->cached_codes_for_regmaps.restore_regs, &re->h_elem);
    ASSERT(!ret);
  }
}

static void
jtable_get_header_and_footer_code(jtable_t *jtable)
{
  struct cached_bincode_arr_elem_t const *cea;
  struct cached_bincode_arr_elem_t aneedle;
  struct cached_bincode_elem_t const *ce;
  struct cached_bincode_elem_t needle;
  struct myhash_elem *found;
  int i;

  stopwatch_run(&jtable_get_header_and_footer_code_timer);

  regmap_copy(&needle.regmap, &jtable->cur_regmap);
  regmap_copy(&aneedle.regmap, &jtable->cur_regmap);

  found = myhash_find(&jtable->cached_codes_for_regmaps.header, &needle.h_elem);
  ASSERT(found);
  ce = chash_entry(found, struct cached_bincode_elem_t const, h_elem);

  ASSERT(ce->binsize <= MAX_HEADER_BINSIZE);
  memcpy(jtable->header_bincode, ce->bincode, ce->binsize);
  jtable->header_binsize = ce->binsize;

  found = myhash_find(&jtable->cached_codes_for_regmaps.footer, &aneedle.h_elem);
  ASSERT(found);
  cea = chash_entry(found, struct cached_bincode_arr_elem_t const, h_elem);

  for (i = 0; i < jtable->itab->get_num_nextpcs(); i++) {
    ASSERT(cea->binsize[i] <= MAX_FOOTER_BINSIZE);
    memcpy(jtable->footer_bincode[i], cea->bincode[i], cea->binsize[i]);
    jtable->footer_binsize[i] = cea->binsize[i];
  }
  stopwatch_stop(&jtable_get_header_and_footer_code_timer);
}

static void
jtable_get_save_restore_code(jtable_t *jtable)
{
  struct cached_bincode_elem_t const *ce;
  struct cached_bincode_elem_t needle;
  struct myhash_elem *found;
  uint8_t *ptr, *end;

  stopwatch_run(&jtable_get_save_restore_code_timer);

  regmap_copy(&needle.regmap, &jtable->cur_regmap);
  found = myhash_find(&jtable->cached_codes_for_regmaps.save_regs, &needle.h_elem);
  if (!found) {
    cout << __func__ << " " << __LINE__ << ": could not find cached code for regmap:\n" << regmap_to_string(&jtable->cur_regmap, as1, sizeof as1) << endl;
  }
  ASSERT(found);
  ce = chash_entry(found, struct cached_bincode_elem_t const, h_elem);

  ASSERT(ce->binsize < MAX_SAVE_RESTORE_REGS_BINSIZE);
  memcpy(jtable->save_regs_bincode, ce->bincode, ce->binsize);
  jtable->save_regs_binsize = ce->binsize;

  found = myhash_find(&jtable->cached_codes_for_regmaps.restore_regs, &needle.h_elem);
  ASSERT(found);
  ce = chash_entry(found, struct cached_bincode_elem_t const, h_elem);

  ASSERT(ce->binsize < MAX_SAVE_RESTORE_REGS_BINSIZE);
  memcpy(jtable->restore_regs_bincode, ce->bincode, ce->binsize);
  jtable->restore_regs_binsize = ce->binsize;

  ptr = jtable->footer_bincode[NEXTPC_TYPE_ERROR];
  end = jtable->footer_bincode[NEXTPC_TYPE_ERROR] + MAX_FOOTER_BINSIZE;

  // first restore regs saved by type-checking code.
  memcpy(ptr, jtable->restore_regs_bincode, jtable->restore_regs_binsize);
  ptr += jtable->restore_regs_binsize;

  // then restore regs saved by header code.
  ptr += jtable_restore_regs(ptr, end - ptr);
  ptr += jtable_branch_to_next_exec_tcode(NEXTPC_TYPE_ERROR, ptr, ptr, end - ptr);
  ASSERT(ptr < end);

  jtable->footer_binsize[NEXTPC_TYPE_ERROR] =
      ptr - jtable->footer_bincode[NEXTPC_TYPE_ERROR];

  stopwatch_stop(&jtable_get_save_restore_code_timer);
}

static void
jtable_init_arrays(jtable_t *jtable, size_t itable_num_entries,
    char const *jtable_cur_regmap_filename, size_t num_states)
{
  long i, j, d;

  if (!jtable_cur_regmap_filename) {
    regmap_identity_for_dst(&jtable->cur_regmap, jtable->jtable_get_fixed_reg_mapping());
  } else {
    dst_regmap_from_file(&jtable->cur_regmap, jtable_cur_regmap_filename);
  }
  DBG2(JTABLE, "jtable->cur_regmap:\n%s\n",
      regmap_to_string(&jtable->cur_regmap, as1, sizeof as1));

  jtable->header_bincode = (uint8_t *)malloc32(MAX_HEADER_BINSIZE * sizeof(uint8_t));
  ASSERT(jtable->header_bincode);

  memset(jtable->footer_bincode, 0, sizeof jtable->footer_bincode);

  for (i = 0; i < ENUM_MAX_LEN; i++) {
    jtable->footer_bincode[i] = (uint8_t *)malloc32(MAX_FOOTER_BINSIZE *
        sizeof(uint8_t));
    ASSERT(jtable->footer_bincode[i]);
    DBG4(JTABLE, "jtable->footer_bincode[%ld] = %p.\n", i, jtable->footer_bincode[i]);
  }

  jtable->footer_bincode[NEXTPC_TYPE_ERROR] = (uint8_t *)malloc32(MAX_FOOTER_BINSIZE *
      sizeof(uint8_t));
  ASSERT(jtable->footer_bincode[NEXTPC_TYPE_ERROR]);
  DBG4(JTABLE, "jtable->footer_bincode[%ld] = %p.\n", (long)NEXTPC_TYPE_ERROR, jtable->footer_bincode[NEXTPC_TYPE_ERROR]);

  jtable->save_regs_bincode = (uint8_t *)malloc32(
      MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
  ASSERT(jtable->save_regs_bincode);

  jtable->restore_regs_bincode = (uint8_t *)malloc32(
      MAX_SAVE_RESTORE_REGS_BINSIZE * sizeof(uint8_t));
  ASSERT(jtable->restore_regs_bincode);

  jtable_gen_save_restore_codes(jtable);
  jtable_gen_header_and_footer_codes(jtable);

  jtable_get_save_restore_code(jtable);
  jtable_get_header_and_footer_code(jtable);

  //jtable->jentries = (jtable_entry_t *)malloc(itable_num_entries *
  //    sizeof(jtable_entry_t));
  jtable->jentries = new jtable_entry_t[itable_num_entries];
  ASSERT(jtable->jentries);

  jtable->num_states = num_states;
  for (i = 0; i < ENUM_MAX_LEN; i++) {
    jtable->insn_bincode[i] = new uint8_t *[num_states];
    jtable->insn_binsize[i] = new size_t[num_states];
    for (size_t s = 0; s < num_states; s++) {
      jtable->insn_bincode[i][s] = (uint8_t *)malloc32(MAX_INSN_BINSIZE *
          sizeof(uint8_t));
      ASSERT(jtable->insn_bincode[i][s]);
    }
  }
}

static void
jtable_set_gencode(jtable_t *jtable, char const *_template)
{
  void *lib_handle;
  lib_handle = dlopen(_template, RTLD_NOW|RTLD_GLOBAL);
  if (lib_handle == NULL) {
    printf("failed loading lib %s.\nerror: %s\nexiting\n", _template, dlerror());
    exit(EXIT_FAILURE);
  }
  DBG(JTABLE, "library '%s' loaded successfully.\n", _template);

  dlerror(); //clear any existing error. see 'man dlopen'

  DBG4(JTABLE, "%s", "dlerror() called to clear any existing error.\n");

  jtable->gencode = (size_t (*)(int, uint8_t*, size_t, map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &, map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &, const int (*)[MAX_NUM_EXREGS(0)], const int*, const int32_t*, const int32_t*, long int, long const *, long, memslot_map_t const *, const long int*, long int, const uint8_t* const*, int32_t (*)(const valtag_t*, int32_t, const valtag_t*, long int, const nextpc_t*, int, map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const *)/*, int, size_t*/))dlsym(lib_handle, "jtable_gencode_function");

  DBG4(JTABLE, "jtable->gencode=%p\n", jtable->gencode);

  char const *error;
  if ((error = dlerror()) != NULL) {
    printf("%s\n", error);
    exit(EXIT_FAILURE);
  }

  DBG4(JTABLE, "%s", "checked dlerror. no error!\n");

  ASSERT(jtable->gencode);

  jtable->gencode_lib_handle = lib_handle;
  //dlclose(_template);
}

static void
jtable_free_exec_iseqs(jtable_t *jtable)
{
  long i;

  for (i = 0; i < jtable->itab->get_num_entries(); i++) {
    int e;
    for (e = 0; e < JTABLE_NUM_EXEC_ISEQS; e++) {
      for (size_t l = 0; l < ITABLE_ENTRY_ISEQ_MAX_LEN; l++) {
        if (jtable->jentries[i].exec_iseq[e][l]) {
          //free(jtable->jentries[i].exec_iseq[e]);
          delete [] jtable->jentries[i].exec_iseq[e][l];
        }
        jtable->jentries[i].exec_iseq[e][l] = NULL;
      }
    }
  }
}

static void
jtable_entries_read(jtable_t *jtable, char const *jtab_filename)
{
  long i, prev_i, max_alloc;
  char *buf;
  FILE *fp;
  int ret;

  fp = fopen(jtab_filename, "r");
  ASSERT(fp);

  max_alloc = 409600;

  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);

  ret = fscanf(fp, "JTABLE:\n");
  ASSERT(ret == 0);

  prev_i = -1;
  while (!feof(fp)) {
    ret = fscanf(fp, "JENTRY.%ld:\n", &i);
    ASSERT(ret == 1);
    ASSERT(i == prev_i + 1);
    prev_i = i;
    ASSERT(i < jtable->itab->get_num_entries());

    fscanf_entry_section(fp, buf, max_alloc); //insn
    fscanf_entry_section(fp, buf, max_alloc); //exec iseq data
    fscanf_entry_section(fp, buf, max_alloc); //exec iseq type

    fscanf_entry_section(fp, buf, max_alloc); //regcons
    regcons_from_string(&jtable->jentries[i].regcons, buf);

    fscanf_entry_section(fp, buf, max_alloc); //use
    regset_from_string(&jtable->jentries[i].use, buf);

    fscanf_entry_section(fp, buf, max_alloc); //def
    regset_from_string(&jtable->jentries[i].def, buf);

    fscanf_entry_section(fp, buf, max_alloc); //temps
    NOT_IMPLEMENTED();
    //regset_from_string(&jtable->jentries[i].jtable_entry_temps, buf);

    //fscanf_entry_section(fp, buf, max_alloc); //ctemps
    //regset_from_string(&jtable->jentries[i].ctemps, buf);

    fscanf(fp, "\n");
  }
  free(buf);
  fclose(fp);
}

void
jtable_read(jtable_t *jtable, itable_t const *itable, char const *jtab_filename,
    char const *jtable_cur_regmap_filename, size_t num_states)
{
  char gencode_so_filename[strlen(jtab_filename) + 128];
  char gencode_c_filename[strlen(jtab_filename) + 128];
  char gencode_filename[strlen(jtab_filename) + 128];

  stopwatch_run(&jtable_init_timer);

  jtable->itab = itable;

  jtable_init_arrays(jtable, itable->get_num_entries(), jtable_cur_regmap_filename, num_states);

  snprintf(gencode_filename, sizeof gencode_filename, "%s.gencode", jtab_filename);
  snprintf(gencode_so_filename, sizeof gencode_so_filename, "%s.so", gencode_filename);
  snprintf(gencode_c_filename, sizeof gencode_c_filename, "%s.c", gencode_filename);

  jtable_entries_read(jtable, jtab_filename);

  if (!file_exists(gencode_so_filename)) {
    if (!file_exists(gencode_c_filename)) {
      NOT_IMPLEMENTED();
    }
    NOT_IMPLEMENTED();
  }
  jtable_set_gencode(jtable, gencode_so_filename);
  stopwatch_stop(&jtable_init_timer);
}

void
jtable_init(jtable_t *jtable, itable_t const *itable,
    char const *jtable_cur_regmap_filename, size_t num_states)
{
  autostop_timer func_timer(__func__);
#define MAX_EXEC_INSNS 10240
  //static dst_insn_t dst_exec_insns[MAX_EXEC_INSNS];
  long i, j, num_dst_exec_insns;
  jtable_entry_t *jentry;
  regset_t touch;
  bool ret;

  stopwatch_run(&jtable_init_timer);
  fixed_reg_mapping_t const &itable_fixed_reg_mapping = itable->get_fixed_reg_mapping();
  regset_t itable_dst_relevant_regs = itable->itable_get_dst_relevant_regs();

  jtable->itab = itable;

  jtable_init_arrays(jtable, itable->get_num_entries(), jtable_cur_regmap_filename, num_states);

  ASSERT(jtable->itab);
  for (i = 0; i < itable->get_num_entries(); i++) {
    memset_t memuse, memdef;
    int e;

    jentry = &jtable->jentries[i];

    dst_insn_vec_get_usedef(jtable->itab->get_entry(i).get_iseq(), &jentry->use,
        &jentry->def, &memuse, &memdef);

    regset_subtract_fixed_regs(&jentry->use, itable_fixed_reg_mapping);
    regset_subtract_fixed_regs(&jentry->def, itable_fixed_reg_mapping);

    jentry->memslot_use.memslot_set_from_memset(&memuse);
    jentry->memslot_def.memslot_set_from_memset(&memdef);

    regset_copy(&touch, &jentry->use);
    regset_union(&touch, &jentry->def);

    size_t itab_entry_iseq_len = jtable->itab->get_entry(i).get_iseq().size();

    for (e = 0; e < JTABLE_NUM_EXEC_ISEQS; e++) {
      //jentry->exec_iseq[e] = (dst_insn_t *)malloc(MAX_EXEC_INSNS * sizeof(dst_insn_t));
      for (size_t l = 0; l < itab_entry_iseq_len; l++) {
        jentry->exec_iseq[e][l] = new dst_insn_t[MAX_EXEC_INSNS];
        ASSERT(jentry->exec_iseq[e][l]);
      }
      for (size_t l = itab_entry_iseq_len; l < ITABLE_ENTRY_ISEQ_MAX_LEN; l++) {
        jentry->exec_iseq[e][l] = NULL;
      }
    }

    for (size_t l = 0; l < itab_entry_iseq_len; l++) {
      dst_insn_copy(jentry->exec_iseq[JTABLE_EXEC_ISEQ_DATA][l],
          &jtable->itab->get_entry(i).get_iseq_ft().at(l));
      CPP_DBG_EXEC(JTABLE, cout << __func__ << " " << __LINE__ << ": sandboxing\n" << dst_iseq_to_string(jentry->exec_iseq[JTABLE_EXEC_ISEQ_DATA][l], 1, as1, sizeof as1) << endl);

      //long *insn_num_map = new long[itab_entry_iseq_len];
      //ASSERT(insn_num_map);
      jentry->exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][l] = dst_iseq_sandbox_without_converting_sboxed_abs_inums_to_pcrel_inums(
          jentry->exec_iseq[JTABLE_EXEC_ISEQ_DATA][l], 1, MAX_EXEC_INSNS, -1,
          /*&jentry->ctemps, */itable_fixed_reg_mapping, l, NULL);
      ASSERT(jentry->exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][l] < MAX_EXEC_INSNS);
      //ASSERT(regset_contains(&jentry->ctemps, &dst_reserved_regs));

      CPP_DBG_EXEC(JTABLE, cout << __func__ << " " << __LINE__ << ": sandboxed\n" << dst_iseq_to_string(jentry->exec_iseq[JTABLE_EXEC_ISEQ_DATA][l], jentry->exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][l], as1, sizeof as1) << endl);


      //delete [] insn_num_map;

      jentry->exec_iseq_len[JTABLE_EXEC_ISEQ_TYPE][l] = gen_typechecking_code(
          jentry->exec_iseq[JTABLE_EXEC_ISEQ_TYPE][l], MAX_EXEC_INSNS,
          &jtable->itab->get_entry(i).get_iseq_ft().at(l), jtable->itab->get_num_regs_used(),
          fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end_var_to_var(),
          jtable->itab->get_typesystem().get_use_types());
    }
    jentry->itable_entry_iseq_len = itab_entry_iseq_len;

    for (e = 0; e < JTABLE_NUM_EXEC_ISEQS; e++) {
      for (size_t l = 0; l < itab_entry_iseq_len; l++) {
        regset_t iuse, idef;

        regset_clear(&jentry->jtable_entry_temps[e][l]);
        jentry->exec_iseq_len[e][l] = dst_iseq_make_position_independent(
            jentry->exec_iseq[e][l],
            jentry->exec_iseq_len[e][l], MAX_EXEC_INSNS, SRC_ENV_SCRATCH(8));
        ASSERT(jtable->itab->get_num_nextpcs() > 0);
        //cout << __func__ << " " << __LINE__ << ": jentry->exec_iseq[" << e << "] = " << dst_iseq_to_string(jentry->exec_iseq[e], jentry->exec_iseq_len[e], as1, sizeof as1) << endl;
        dst_iseq_rename_nextpc_consts(jentry->exec_iseq[e][l], jentry->exec_iseq_len[e][l],
            (uint8_t const **)jtable->footer_bincode, jtable->itab->get_num_nextpcs());
        ASSERT(jentry->exec_iseq_len[e][l] <= MAX_EXEC_INSNS);
        //cout << __func__ << " " << __LINE__ << ": jentry->exec_iseq[" << e << "] = " << dst_iseq_to_string(jentry->exec_iseq[e], jentry->exec_iseq_len[e], as1, sizeof as1) << endl;

        //jentry->exec_iseq[e] = (dst_insn_t *)realloc(jentry->exec_iseq[e],
        //    jentry->exec_iseq_len[e] * sizeof(dst_insn_t));
        //ASSERT(!jentry->exec_iseq_len[e] || jentry->exec_iseq[e]);

        dst64_iseq_get_usedef_regs(jentry->exec_iseq[e][l], jentry->exec_iseq_len[e][l], &iuse,
            &idef);
        regset_union(&jentry->jtable_entry_temps[e][l], &iuse);
        regset_union(&jentry->jtable_entry_temps[e][l], &idef);


        regset_t itab_iuse, itab_idef;
        dst_insn_get_usedef_regs(&jtable->itab->get_entry(i).get_iseq().at(l), &itab_iuse,
            &itab_idef);
        regset_union(&itab_iuse, &itab_idef);

        regset_diff(&jentry->jtable_entry_temps[e][l], &itab_iuse);
        regset_subtract_fixed_regs(&jentry->jtable_entry_temps[e][l], itable_fixed_reg_mapping);
        DBG4(JTABLE, "jtable %ld jentry->temps[%d][%d]: %s\n", i, (int)e, (int)l,
            regset_to_string(&jentry->jtable_entry_temps[e][l], as1, sizeof as1));
      }
    }
    DBG4(JTABLE, "jtable %ld touch: %s\n", i,
        regset_to_string(&touch, as1, sizeof as1));
    //DBG4(JTABLE, "jtable %ld jentry->ctemps: %s\n", i,
    //    regset_to_string(&jentry->ctemps, as1, sizeof as1));
    //regset_diff(&jentry->temps, &jentry->ctemps);

    //DBG4(JTABLE, "jtable %ld jentry->temps: %s\n", i,
    //    regset_to_string(&jentry->temps, as1, sizeof as1));

    dst_insn_vec_to_string(jtable->itab->get_entry(i).get_iseq_ft(), as1, sizeof as1);
    dst_infer_regcons_from_assembly(as1, &jentry->regcons);
    regcons_update_using_fixed_reg_mapping(&jentry->regcons, itable_fixed_reg_mapping);
    DBG4(JTABLE, "jtable %ld as1:\n %s\nregcons:\n%s\n", i, as1,
        regcons_to_string(&jentry->regcons, as2, sizeof as2));
  }

  char _template[1024], gencode_file[1024];
  //snprintf(_template, sizeof _template, ABUILD_DIR "/superopt-files/jtab.%s", itable->get_id_string().c_str());
  snprintf(_template, sizeof _template, ABUILD_DIR "/superopt-files/jtab.%ld", (long)getpid());
  FILE *fp = fopen(_template, "w");
  ASSERT(fp);
  jtable_print(fp, jtable);
  fclose(fp);

  snprintf(gencode_file, sizeof gencode_file, "%s.gencode", _template);
  jtable_init_gencode_functions(gencode_file, sizeof gencode_file, jtable->jentries,
      jtable->itab->get_num_entries(), itable_fixed_reg_mapping, itable_dst_relevant_regs);

  jtable_set_gencode(jtable, gencode_file);

  DBG_EXEC(JTABLE, jtable_print(stdout, jtable););

  jtable_free_exec_iseqs(jtable);

  stopwatch_stop(&jtable_init_timer);
}

static bool
regcons_rename_using_sequence(regcons_t *dst, regcons_t const *src,
    long const exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN]
                                 [MAX_REGS_PER_ITABLE_ENTRY],
    long const exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN], long s,
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  int i, j, r;

  regcons_set_dst(dst, fixed_reg_mapping);

  for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < exreg_sequence_len[i][s]; j++) {
      r = exreg_sequence[i][s][j];
      ASSERT(r >= 0 && r < I386_NUM_EXREGS(i));
      //regcons_entry_intersect(&dst->regcons_extable[i][r], &src->regcons_extable[i][j]);
      //regcons_entry_intersect(regcons_get_entry(dst, i, r), regcons_get_entry(src, i, j));
      if (!regcons_entry_intersect(dst, src, i, reg_identifier_t(r), reg_identifier_t(j))) {
        return false;
      }
    }
  }
  return true;
}

static void
regset_rename_using_sequence(regset_t *dst, regset_t const *src,
    long const exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN]
        [MAX_REGS_PER_ITABLE_ENTRY],
    long const exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN], long s)
{
  int i, j, r;

  regset_clear(dst);

  for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < exreg_sequence_len[i][s]; j++) {
      r = exreg_sequence[i][s][j];
      ASSERT(r >= 0 && r < I386_NUM_EXREGS(i));
      regset_mark_ex_mask(dst, i, r, regset_get_elem(src, i, j));
    }
  }
}

static void
memslot_set_rename_using_sequence(memslot_set_t *dst, memslot_set_t const *src,
    long const memloc_sequence[MAX_MEMLOCS_PER_ITABLE_ENTRY],
    long const memloc_sequence_len)
{
  dst->memslot_set_clear();

  for (int j = 0; j < memloc_sequence_len; j++) {
    if (!src->memslot_set_belongs(j)) {
      continue;
    }
    int r = memloc_sequence[j];
    dst->memslot_set_add_mapping(r, src->memslot_set_get_elem(j));
  }
}

static void
regmap_construct_using_sequence_and_regmap(regmap_t *dst,
    long const exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN]
                                 [MAX_REGS_PER_ITABLE_ENTRY],
    long const exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN],
    regset_t const *temps,
    long s, regmap_t const *src)
{
  bool used[MAX_NUM_REGS_EXREGS];
  int i, j, t, r;

  regmap_init(dst);

  for (i = 0; i < I386_NUM_EXREG_GROUPS; i++) {
    //total = exreg_sequence_len[i][s] + num_temps[i];
    //ASSERT(total <= I386_NUM_EXREGS(i));
    memset(used, 0, MAX_NUM_REGS_EXREGS * sizeof(bool));
    for (j = 0; j < exreg_sequence_len[i][s]; j++) {
      r = exreg_sequence[i][s][j];
      used[r] = true;
      regmap_set_elem(dst, i, j, regmap_get_elem(src, i, r));
    }
    if (i != I386_EXREG_GROUP_GPRS) {
      continue; //only worry about gpr temporaries
    }
    for (t = 0; t < I386_NUM_EXREGS(i); t++) {
      if ((regset_get_elem(temps, i, t) & MAKE_MASK(DWORD_LEN)) != MAKE_MASK(DWORD_LEN)) {
        continue;
      }
      //'t' is an available temporary
      //(determined by checking if temps->excregs[i][t] is non-null)
      //we only check the 32 least significant bits of the reg to determine if the temp
      //is available .i.e, if the last 32 bits are not available, temp is not available
      for (r = 0; r < I386_NUM_EXREGS(i); r++) {
        if (!used[r]) {
          break;
        }
      }
      ASSERT(r < I386_NUM_EXREGS(i));
      used[r] = true;
      ASSERT(regmap_get_elem(dst, i, t).reg_identifier_is_unassigned());
      //dst->regmap_extable[i][t] = src->regmap_extable[i][r];
      regmap_set_elem(dst, i, t, regmap_get_elem(src, i, r));
    }
  }
}

static void
vconst_map_construct_using_sequence_and_imm_map(int32_t *dst_map,
    int32_t const *sequence, long sequence_len, imm_vt_map_t const *imm_map)
{
  long i, s;

  for (i = 0; i < sequence_len; i++) {
    s = sequence[i];
    ASSERT(s >= 0 && s < imm_map->num_imms_used);
    ASSERT(imm_map->table[s].tag == tag_const);
    dst_map[i] = imm_map->table[s].val;
  }
}

/*static size_t
jtable_entry_construct_executable_binary(uint8_t *buf, size_t size,
    jtable_t const *jtable, long entrynum,
    int const regmap_extable[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)],
    int32_t const vconst_map[MAX_VCONSTS_PER_ITABLE_ENTRY], long vconst_map_len,
    long const nextpc_sequence[MAX_NEXTPCS_PER_ITABLE_ENTRY], long nextpc_sequence_len,
    uint8_t const * const footer_bincode[ENUM_MAX_LEN])
{
  uint8_t *ptr, *end;

  ASSERT(entrynum >= 0 && entrynum < jtable->itab->num_entries);
  ptr = buf;
  end = buf + size;

  ptr += (*jtable->gencode)(entrynum, ptr, end - ptr, regmap_extable, vconst_sequence,
      vconst_map_len, nextpc_sequence, nextpc_sequence_len, footer_bincode);

  return ptr - buf;
}*/

/* *next_ptr holds the index of the ipos->sequence[] array element that was last
   changed. This function modifies itable->cur_regcons for next..ipos->sequence_len - 1
   appropriately. If any of
   itable->cur_regcons[i] (index i | i >= next && i < ipos->sequence_len)
   is found to be unsatisfiable, 'i' is returned. If no such 'i' exissts, -1 is
   returned. */

static size_t
jtable_set_state_last_executed_insn(int insn_index, uint8_t *buf, size_t size)
{
  uint8_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  // mov $insn_index, TYPE_ENV_INITIAL_ADDR + offsetof(typestate_t, last_executed_insn)
  *ptr++ = 0xc7;
  *ptr++ = 0x04;
  *ptr++ = 0x25;

  //*(uint32_t *)ptr = TYPE_ENV_INITIAL_ADDR + offsetof(typestate_t, last_executed_insn);
  *(uint32_t *)ptr = SRC_ENV_ADDR + offsetof(state_t, last_executed_insn);
  ptr += 4;

  *(uint32_t *)ptr = insn_index;
  ptr += 4;
  
  return ptr - buf;
}

size_t
jtable_get_executable_binary(uint8_t *buf, size_t size, jtable_t const *jtable,
    struct itable_position_t const *ipos, int fp_state_num)//, bool use_types
{
  uint8_t *ptr, *end;
  int i;

  stopwatch_run(&jtable_get_executable_binary_timer);

  ptr = buf;
  end = buf + size;

  DBG4(JTABLE, "header_bincode = %s\n", hex_string(jtable->header_bincode, jtable->header_binsize, as1, sizeof as1));
  memcpy(ptr, jtable->header_bincode, jtable->header_binsize);
  ptr += jtable->header_binsize;
  ASSERT(ptr < end);

  for (i = 0; i < ipos->sequence_len; i++) {
    ptr += jtable_set_state_last_executed_insn(i, ptr, end - ptr);
    /*if (jtable->itab->get_use_types() != USE_TYPES_NONE) {
      DBG4(JTABLE, "save_regs_bincode = %s\n", hex_string(jtable->save_regs_bincode, jtable->save_regs_binsize, as1, sizeof as1));
      memcpy(ptr, jtable->save_regs_bincode, jtable->save_regs_binsize);
      ptr += jtable->save_regs_binsize;

      DBG4(JTABLE, "insn_bincode[TYPE][%d][%d] = %s\n", i, (int)fp_state_num, hex_string(jtable->insn_bincode[JTABLE_EXEC_ISEQ_TYPE][i][fp_state_num], jtable->insn_binsize[JTABLE_EXEC_ISEQ_TYPE][i][fp_state_num], as1, sizeof as1));
      memcpy(ptr, jtable->insn_bincode[JTABLE_EXEC_ISEQ_TYPE][i][fp_state_num],
          jtable->insn_binsize[JTABLE_EXEC_ISEQ_TYPE][i][fp_state_num]);
      ptr += jtable->insn_binsize[JTABLE_EXEC_ISEQ_TYPE][i][fp_state_num];

      DBG4(JTABLE, "restore_regs_bincode = %s\n", hex_string(jtable->restore_regs_bincode, jtable->restore_regs_binsize, as1, sizeof as1));
      memcpy(ptr, jtable->restore_regs_bincode, jtable->restore_regs_binsize);
      ptr += jtable->restore_regs_binsize;
    }
    DBG4(JTABLE, "insn_bincode[DATA][%d][%d] = %s\n", i, (int)fp_state_num, hex_string(jtable->insn_bincode[JTABLE_EXEC_ISEQ_DATA][i][fp_state_num], jtable->insn_binsize[JTABLE_EXEC_ISEQ_DATA][i][fp_state_num], as1, sizeof as1));
    memcpy(ptr, jtable->insn_bincode[JTABLE_EXEC_ISEQ_DATA][i][fp_state_num],
        jtable->insn_binsize[JTABLE_EXEC_ISEQ_DATA][i][fp_state_num]);
    ptr += jtable->insn_binsize[JTABLE_EXEC_ISEQ_DATA][i][fp_state_num];*/
    /*for (size_t l = 0; l < jtable->itable_entry_iseq_len; l++) {
      if (jtable->itab->get_use_types() != USE_TYPES_NONE) {
        memcpy(ptr, jtable->save_regs_bincode, jtable->save_regs_binsize);
        ptr += jtable->save_regs_binsize;

        memcpy(ptr, jtable->insn_bincode[JTABLE_EXEC_ISEQ_TYPE][i][l][fp_state_num],
            jtable->insn_binsize[JTABLE_EXEC_ISEQ_TYPE][i][l][fp_state_num]);
        ptr += jtable->insn_binsize[JTABLE_EXEC_ISEQ_TYPE][i][l][fp_state_num];

        memcpy(ptr, jtable->restore_regs_bincode, jtable->restore_regs_binsize);
        ptr += jtable->restore_regs_binsize;
      }
      memcpy(ptr, jtable->insn_bincode[JTABLE_EXEC_ISEQ_DATA][i][l][fp_state_num],
          jtable->insn_binsize[JTABLE_EXEC_ISEQ_DATA][i][l][fp_state_num]);
      ptr += jtable->insn_binsize[JTABLE_EXEC_ISEQ_DATA][i][l][fp_state_num];
    }*/
    memcpy(ptr, jtable->insn_bincode/*[JTABLE_EXEC_ISEQ_DATA]*/[i][fp_state_num],
        jtable->insn_binsize/*[JTABLE_EXEC_ISEQ_DATA]*/[i][fp_state_num]);
    ptr += jtable->insn_binsize/*[JTABLE_EXEC_ISEQ_DATA]*/[i][fp_state_num];
  }

  // add jump to footer_tcode[num_nextpcs - 1]
  DBG4(JTABLE, "Adding terminating branch: jtable->itab->num_nextpcs = %ld, "
      "jtable->footer_bincode[%ld] = %p.\n", jtable->itab->get_num_nextpcs(),
      jtable->itab->get_num_nextpcs() - 1,
      jtable->footer_bincode[jtable->itab->get_num_nextpcs() - 1]);

  ASSERT(ptr >= (uint8_t *)0 && ptr <= (uint8_t *)0xffffffff);
  ASSERT(end >= (uint8_t *)0 && end <= (uint8_t *)0xffffffff);
  ASSERT(jtable->itab->get_num_nextpcs() >= 1);

  *ptr++ = 0xe9;
  *(uint32_t *)ptr = (uint32_t)(jtable->footer_bincode[
      jtable->itab->get_num_nextpcs() - 1] - (ptr + 4));
  ptr += 4;

  ASSERT(ptr < end);

  DBG4(JTABLE, "returning true (len %ld):\n%s\n", ptr - buf,
      hex_string(buf, ptr - buf, as1, sizeof as1));

  stopwatch_stop(&jtable_get_executable_binary_timer);

  return ptr - buf;
}

static void
regset_truncate_using_num_regs_used_array(regset_t *regset, size_t const *num_regs_used,
    int num_groups)
{
  int i, j;

  for (i = 0; i < num_groups; i++) {
    ASSERT(num_regs_used[i] <= MAX_NUM_EXREGS(i));
    for (j = num_regs_used[i]; j < MAX_NUM_EXREGS(i); j++) {
      regset->excregs[i][j] = 0;
    }
  }
}

long
jtable_construct_executable_binary(
    jtable_t *jtable, struct itable_position_t const *ipos,
    imm_vt_map_t const *imm_map,
    fingerprintdb_live_out_t const &fingerprintdb_live_out,
    long next, int fp_state_num)//, bool use_types
{
  memslot_set_t imemslot_use, imemslot_def;
  regset_t iuse, idef;
  regcons_t iregcons;
  long bingen_index;
  regmap_t iregmap;
  long i, s;

  stopwatch_run(&jtable_construct_executable_binary_timer);
  DBG4(JTABLE, "jtable->cur_regmap:\n%s\n",
      regmap_to_string(&jtable->cur_regmap, as1, sizeof as1));
  ASSERT(ipos->sequence_len == 0 || (next >= 0 && next < ipos->sequence_len));

  for (i = next; i < ipos->sequence_len; i++) {
    s = ipos->sequence[i];
    ASSERT(s >= 0 && s < jtable->itab->get_num_entries());

    if (i == 0) {
      regcons_set_dst(&jtable->cur_regcons[i], jtable->jtable_get_fixed_reg_mapping());
      regset_clear(&jtable->cur_use[i]);
      regset_clear(&jtable->cur_def[i]);
      jtable->cur_memslot_use[i].memslot_set_clear();
      jtable->cur_memslot_def[i].memslot_set_clear();
    } else {
      regcons_copy(&jtable->cur_regcons[i], &jtable->cur_regcons[i - 1]);
      regset_copy(&jtable->cur_use[i], &jtable->cur_use[i - 1]);
      regset_copy(&jtable->cur_def[i], &jtable->cur_def[i - 1]);
      jtable->cur_memslot_use[i].memslot_set_copy(jtable->cur_memslot_use[i - 1]);
      jtable->cur_memslot_def[i].memslot_set_copy(jtable->cur_memslot_def[i - 1]);
    }

    if (!regcons_rename_using_sequence(&iregcons, &jtable->jentries[s].regcons,
            ipos->exreg_sequence, ipos->exreg_sequence_len, i, jtable->jtable_get_fixed_reg_mapping())) {
      stopwatch_stop(&jtable_construct_executable_binary_timer);
      return i;
    }
    DBG4(JTABLE, "jtable->jentries[%ld].regcons =\n%s\n", s, regcons_to_string(&jtable->jentries[s].regcons, as1, sizeof as1));
    DBG4(JTABLE, "iregcons =\n%s\n", regcons_to_string(&iregcons, as1, sizeof as1));
    DBG4(JTABLE, "ipos =\n%s\n", itable_position_to_string(ipos, as1, sizeof as1));
    regcons_intersect(&jtable->cur_regcons[i], &iregcons);

    regset_rename_using_sequence(&iuse, &jtable->jentries[s].use, ipos->exreg_sequence,
        ipos->exreg_sequence_len, i);
    regset_rename_using_sequence(&idef, &jtable->jentries[s].def, ipos->exreg_sequence,
        ipos->exreg_sequence_len, i);

    memslot_set_rename_using_sequence(&imemslot_use, &jtable->jentries[s].memslot_use,
        ipos->memloc_sequence[i], ipos->memloc_sequence_len[i]);
    memslot_set_rename_using_sequence(&imemslot_def, &jtable->jentries[s].memslot_def,
        ipos->memloc_sequence[i], ipos->memloc_sequence_len[i]);

    regset_diff(&jtable->cur_use[i], &idef);
    regset_union(&jtable->cur_use[i], &iuse);
    //regset_copy(&jtable->cur_use[i], &iuse);

    regset_union(&jtable->cur_def[i], &idef);
    fingerprintdb_live_out.fingerprintdb_live_out_regset_truncate(&jtable->cur_def[i]);
    //cout << __func__ << " " << __LINE__ << ": idef = " << regset_to_string(&idef, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": jtable->cur_def[" << i << "] = " << regset_to_string(&jtable->cur_def[i], as1, sizeof as1) << endl;
    //regset_copy(&jtable->cur_def[i], &idef);

    jtable->cur_memslot_use[i].memslot_set_diff(imemslot_def);
    jtable->cur_memslot_use[i].memslot_set_union(imemslot_use);

    jtable->cur_memslot_def[i].memslot_set_union(imemslot_def);
    //jtable->cur_memslot_def[i].memslot_set_copy(imemslot_def);

    /*if (jtable->itab->get_use_types() != USE_TYPES_NONE) {
      regset_truncate_using_num_regs_used_array(&jtable->cur_def[i],
          jtable->itab->get_num_regs_used(), I386_NUM_EXREG_GROUPS);
    }*/
  }

  if (   ipos->sequence_len > 0
      && !regmap_satisfies_regcons(&jtable->cur_regmap,
             &jtable->cur_regcons[ipos->sequence_len - 1])) {

    DBG2(JTABLE, "regmap does not satisfy regcons:\nipos:\n%s\nregmap:\n%s"
        "regcons:\n%s\n", itable_position_to_string(ipos, as3, sizeof as3),
        regmap_to_string(&jtable->cur_regmap, as1, sizeof as1),
        regcons_to_string(&jtable->cur_regcons[ipos->sequence_len - 1], as2,
            sizeof as2));

    for (i = next; i < ipos->sequence_len; i++) {
      regmap_init(&iregmap);
      if (!regmap_assign_using_regcons(&iregmap, &jtable->cur_regcons[i], ARCH_I386,
              NULL, jtable->jtable_get_fixed_reg_mapping())) {
        stopwatch_stop(&jtable_construct_executable_binary_timer);
        return i;
      }
    }
    ASSERT(regmap_satisfies_regcons(&iregmap,
        &jtable->cur_regcons[ipos->sequence_len - 1]));
    bingen_index = 0;
    regmap_copy(&jtable->cur_regmap, &iregmap);
    DBG2(JTABLE, "ipos:\n%s\n", itable_position_to_string(ipos, as1, sizeof as1));
    DBG2(JTABLE, "cur_regmap:\n%s\n",
        regmap_to_string(&jtable->cur_regmap, as1, sizeof as1));

    jtable_get_save_restore_code(jtable);
    jtable_get_header_and_footer_code(jtable);
  } else {
    bingen_index = next;
  }

  for (i = bingen_index; i < ipos->sequence_len; i++) {
    s = ipos->sequence[i];

    int32_t vconst_map[ipos->vconst_sequence_len[i]];
    int exreg_sequence[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
    int exreg_sequence_len[MAX_NUM_EXREG_GROUPS];
    int g, j;
    ASSERT(({memset(exreg_sequence, -1, sizeof exreg_sequence); true;})); //just to initialize exreg_sequence to -1 to catch any errors due to uninitialized values early
    for (g = 0; g < I386_NUM_EXREG_GROUPS; g++) {
      exreg_sequence_len[g] = ipos->exreg_sequence_len[g][i];
      for (j = 0; j < exreg_sequence_len[g]; j++) {
        exreg_sequence[g][j] = ipos->exreg_sequence[g][i][j];
      }
    }

    ASSERT(s >= 0 && s < jtable->itab->get_num_entries());
    regmap_construct_using_sequence_and_regmap(&iregmap, ipos->exreg_sequence,
        ipos->exreg_sequence_len, &jtable->jentries[s].jtable_entry_temps[JTABLE_EXEC_ISEQ_DATA][i], i, &jtable->cur_regmap);
    DBG4(JTABLE, "jtable->cur_regmap =\n%s\n", regmap_to_string(&jtable->cur_regmap, as1, sizeof as1));
    DBG4(JTABLE, "iregmap = \n%s\n", regmap_to_string(&iregmap, as1, sizeof as1));

    vconst_map_construct_using_sequence_and_imm_map(vconst_map,
        ipos->vconst_sequence[i], ipos->vconst_sequence_len[i], imm_map);

    DBG4(JTABLE, "imm_map = \n%s\n", imm_vt_map_to_string(imm_map, as1, sizeof as1));
    CPP_DBG_EXEC4(JTABLE,
        cout << "ipos.vconst_sequence[" << i << "] =";
	for (size_t l = 0; l < ipos->vconst_sequence_len[i]; l++) {
	  cout << " " << ipos->vconst_sequence[i][l];
	}
	cout << "\n";
        cout << "vconst_map =";
	for (size_t l = 0; l < ipos->vconst_sequence_len[i]; l++) {
	  cout << " " << hex << vconst_map[l] << dec;
	}
	cout << "\n";
    );
    ASSERT(fp_state_num < jtable->num_states);
    jtable->insn_binsize/*[d]*/[i]/*[l]*/[fp_state_num] = (*jtable->gencode)(s,
        jtable->insn_bincode/*[d]*/[i]/*[l]*/[fp_state_num],
        MAX_INSN_BINSIZE,
        /*(int const (*)[MAX_NUM_EXREGS(0)])*/iregmap.regmap_extable,
        jtable->cur_regmap.regmap_extable,
        (int const (*)[MAX_NUM_EXREGS(0)])exreg_sequence,
        exreg_sequence_len,
        vconst_map,
        ipos->vconst_sequence[i],
        ipos->vconst_sequence_len[i],
        ipos->memloc_sequence[i],
        ipos->memloc_sequence_len[i],
        memslot_map_t::memslot_map_for_src_env_state(),
        ipos->nextpc_sequence[i], ipos->nextpc_sequence_len[i],
        (uint8_t const **)jtable->footer_bincode,
        &imm_vt_map_rename_vconstant_to_const/*,
        d, l*/);
  }
  stopwatch_stop(&jtable_construct_executable_binary_timer);
  return -1;
}

long
jtable_construct_executable_binary_for_all_states(
    jtable_t *jtable, struct itable_position_t const *ipos,
    rand_states_t const &rand_states,
    fingerprintdb_live_out_t const &fingerprintdb_live_out,
    long next)
{
  size_t num_states = rand_states.get_num_states();
  long regcons_unsat_index = -1;
  ASSERT(num_states >= 1);
  for (size_t f = 0; f < num_states; f++) {
    regcons_unsat_index = jtable_construct_executable_binary(jtable, ipos, &rand_states.rand_states_get_state(f).imm_vt_map, fingerprintdb_live_out, next, f);
    if (regcons_unsat_index != -1) {
      ASSERT(f == 0);
      return regcons_unsat_index;
    }
  }
  ASSERT(regcons_unsat_index == -1);
  return regcons_unsat_index;
}


static void
cached_bincode_elem_destroy(struct myhash_elem *e, void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_elem_t *ce;
  ASSERT(e);
  ce = chash_entry(e, struct cached_bincode_elem_t, h_elem);
  ASSERT(ce->bincode);
  free(ce->bincode);
  ce->bincode = NULL;
  //free(ce);
  delete ce;
}

static void
cached_bincode_arr_elem_destroy(struct myhash_elem *e, void *aux)
{
  ASSERT(!aux);
  struct cached_bincode_arr_elem_t *ce;
  int i;

  ASSERT(e);
  ce = chash_entry(e, struct cached_bincode_arr_elem_t, h_elem);
  ASSERT(ce->bincode);
  for (i = 0; i < NELEM(ce->bincode); i++) {
    if (ce->bincode[i]) {
      free(ce->bincode[i]);
      ce->bincode[i] = NULL;
    }
  }
  //free(ce);
  delete ce;
}

void
jtable_free(jtable_t *jtable)
{
  autostop_timer func_timer(__func__);
  long i;
  int d;

  if (jtable->jentries) {
    //free(jtable->jentries);
    delete [] jtable->jentries;
    jtable->jentries = NULL;
  }

  for (i = 0; i < ENUM_MAX_LEN; i++) {
    ASSERT(jtable->insn_bincode[i]);
    for (size_t s = 0; s < jtable->num_states; s++) {
      if (jtable->insn_bincode[i][s]) {
        free32(jtable->insn_bincode[i][s]);
        jtable->insn_bincode[i][s] = NULL;
      }
    }
    delete [] jtable->insn_bincode[i];
    delete [] jtable->insn_binsize[i];
  }

  if (jtable->header_bincode) {
    free32(jtable->header_bincode);
    jtable->header_bincode = NULL;
  }

  for (i = 0; i < NELEM(jtable->footer_bincode); i++) {
    if (jtable->footer_bincode[i]) {
      free32(jtable->footer_bincode[i]);
      jtable->footer_bincode[i] = NULL;
    }
  }

  if (jtable->save_regs_bincode) {
    free32(jtable->save_regs_bincode);
    jtable->save_regs_bincode = NULL;
  }

  if (jtable->restore_regs_bincode) {
    free32(jtable->restore_regs_bincode);
    jtable->restore_regs_bincode = NULL;
  }

  ASSERT(jtable->gencode_lib_handle);
  dlclose(jtable->gencode_lib_handle);
  jtable->gencode_lib_handle = NULL;

  myhash_destroy(&jtable->cached_codes_for_regmaps.save_regs, cached_bincode_elem_destroy);
  myhash_destroy(&jtable->cached_codes_for_regmaps.restore_regs, cached_bincode_elem_destroy);
  myhash_destroy(&jtable->cached_codes_for_regmaps.header, cached_bincode_elem_destroy);
  myhash_destroy(&jtable->cached_codes_for_regmaps.footer,
      cached_bincode_arr_elem_destroy);
}
