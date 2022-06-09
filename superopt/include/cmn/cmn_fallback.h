#include "cmn.h"
#if CMN == COMMON_SRC
static void
fallback_add_to_peeptab(translation_instance_t *ti,
    src_insn_t const *src_iseq, long src_iseq_len, dst_insn_t const *dst_iseq,
    long dst_iseq_len, regset_t const *live_outs, long num_nextpcs,
    bool mem_live_out, transmap_t *fb_in_tmap, transmap_t *fb_out_tmap/*,
    regset_t const &flexible_regs*/, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  transmap_t out_tmaps[num_nextpcs];
  //transmap_t fb_tmap;
  //bool mem_live_out;
  //regset_t touch;
  bool ret;
  long j;

  ASSERT(num_nextpcs);

  for (j = 0; j < num_nextpcs; j++) {
    transmap_copy(&out_tmaps[j], fb_out_tmap);
    //DBG(TMP, "%s\nlive_out[%ld]: %s\n",
    //    src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1), j,
    //    regset_to_string(&live_outs[j], as2, sizeof as2));
  }
  //mem_live_out = true;

  if (ti) {
    //DBG(TMP, "dst_iseq:\n%s\n",
    //    dst_iseq_to_string(dst_iseq, dst_iseq_len, as2, sizeof as2));
    peeptab_try_and_add(&ti->peep_table, src_iseq, src_iseq_len/*, flexible_regs*/, fb_in_tmap,
        out_tmaps, dst_iseq, dst_iseq_len, live_outs, num_nextpcs, mem_live_out,
        fixed_reg_mapping,
        //NULL, NULL, NULL, NULL, NULL, NULL,
        //NULL,
        /*PEEP_ENTRY_FLAGS_BASE_ONLY, */-1);
  }
}

void
src_fallback_translate(translation_instance_t *ti, struct src_insn_t const *src_iseq,
    long src_iseq_len,// struct regset_t const *live_outs,
    nextpc_t const *nextpcs,
    long num_nextpcs, regmap_t const *st_src_regmap, imm_vt_map_t const *st_imm_map,
    struct dst_insn_t *dst_iseq,
    long *dst_iseq_len, long dst_iseq_size, char const *src_filename)
{
  regmap_t src_regmap, dst_regmap;
  //regset_t flexible_regs;
  transmap_t fb_out_tmap, fb_in_tmap;
  dst_insn_t *insns;
  operand_t *op;
  long i, ret;

  ASSERT(src_iseq_len > 0);

  regmap_invert(&src_regmap, st_src_regmap);

  DBG(FB, "src_regmap:\n%s\n", regmap_to_string(&src_regmap, as1, sizeof as1));
  DBG(FB, "src_iseq:\n%s\n",
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));

  regset_t use, touch;
  memset_t memuse, memdef;
  src_iseq_compute_touch_var(&touch, &use, &memuse, &memdef, src_iseq, src_iseq_len);
  regset_t live_outs[num_nextpcs];
  for (int j = 0; j < num_nextpcs; j++) {
    regset_copy(&live_outs[j], &touch);
  }
  bool mem_live_out = memuse.memaccess.size() > 0 || memdef.memaccess.size() > 0;

  fixed_reg_mapping_t fixed_reg_mapping;

#if SRC_FALLBACK == FALLBACK_ID
  ASSERT(src_iseq_len < dst_iseq_size);
  *dst_iseq_len = src_iseq_len;
  for (i = 0; i < src_iseq_len; i++) {
    dst_insn_copy(&dst_iseq[i], &src_iseq[i]);
  }
  transmap_copy(&fb_in_tmap, transmap_fallback());
  transmap_copy(&fb_out_tmap, transmap_fallback());
#elif SRC_FALLBACK == FALLBACK_QEMU

  fallback_qemu_translate(ti, src_iseq, src_iseq_len, num_nextpcs,
      &src_regmap, st_imm_map, dst_iseq, dst_iseq_len, dst_iseq_size);
  transmap_copy(&fb_in_tmap, transmap_fallback());
  transmap_copy(&fb_out_tmap, transmap_fallback());
#elif SRC_FALLBACK == FALLBACK_ETFG
  fallback_etfg_translate(src_iseq, src_iseq_len, num_nextpcs, dst_iseq, dst_iseq_len, dst_iseq_size, src_regmap, &fb_out_tmap, live_outs, src_filename/*, flexible_regs*/, fixed_reg_mapping);
#endif
  transmap_intersect_with_regset(&fb_out_tmap, &touch);
  transmap_copy(&fb_in_tmap, &fb_out_tmap);
  transmap_intersect_with_regset(&fb_in_tmap, &use);

  *dst_iseq_len = dst_iseq_optimize_branching_code(dst_iseq, *dst_iseq_len,
      dst_iseq_size);

  //DBG(TMP, "dst_iseq:\n%s\n",
  //    dst_iseq_to_string(dst_iseq, *dst_iseq_len, as2, sizeof as2));
  fallback_add_to_peeptab(ti, src_iseq, src_iseq_len,
      dst_iseq, *dst_iseq_len, live_outs, num_nextpcs, mem_live_out, &fb_in_tmap, &fb_out_tmap/*, flexible_regs*/, fixed_reg_mapping);

  //regmap_init(&dst_regmap);

  //nextpc_map_t nextpc_map;
  //nextpc_map_set_identity_var(&nextpc_map, num_nextpcs);

  //dst_iseq_pick_reg_assignment(&dst_regmap, dst_iseq, *dst_iseq_len);
  /*dst_iseq_rename(dst_iseq, *dst_iseq_len, &dst_regmap, st_imm_map,
      &src_regmap, &nextpc_map, nextpcs, num_nextpcs);*/

  //DBG(FB, "dst_iseq:\n%s\n",
  //    dst_iseq_to_string(dst_iseq, *dst_iseq_len, as2, sizeof as2));
}

void
src_fallback_init(void)
{
#if SRC_FALLBACK == FALLBACK_QEMU
  fallback_qemu_init();
#endif
}
#else
#error "not-implemented"
#endif
