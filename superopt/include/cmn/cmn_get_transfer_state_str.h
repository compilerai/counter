#include "cmn.h"

static int
cmn_insn_arg(cmn_insn_t const *insn, value args, int off)
{
  CAMLparam1(args);
  int64_t arr[MAX_ARGSLEN];
  int i, len, start = off;
  len = cmn_insn_to_int_array(insn, arr, MAX_ARGSLEN);
  for (i = 0; i < len; i++) {
    off += caml_store_int64(args, off, arr[i]);
  }
  CAMLreturnT(int, off - start);
}

char *
cmn_get_fresh_state_str()
{
#if (CMN == COMMON_SRC && ARCH_SRC == ARCH_ETFG)
  return etfg_insn_get_fresh_state_str();
#else
//  CAMLparam0();
  value args, str;
  static value const *mlfunc = NULL;
  int i, ncur, alen;
  char* ret;
  if (!mlfunc) {
    mlfunc = caml_named_value(__func__);
    ASSERT(mlfunc);
  }
  args = caml_alloc(0, 0);
  
  str = caml_callback(*mlfunc, args);
  ret = strdup(String_val(str));
  return ret;
#endif
}

char *
cmn_get_transfer_state_str(struct cmn_insn_t const *insn,
    int num_nextpcs, int nextpc_indir)
{
#if (CMN == COMMON_SRC && ARCH_SRC == ARCH_ETFG)
  return etfg_insn_get_transfer_state_str(insn, num_nextpcs, nextpc_indir);
#else
//  CAMLparam0();
  value args, str;
  static value const *mlfunc = NULL;
  int i, ncur, alen;
  if (!mlfunc) {
    mlfunc = caml_named_value(__func__);
    ASSERT(mlfunc);
  }
  
  DBG(FBGEN, "getting transfer state str for %s\n", cmn_iseq_to_string(insn, 1, as1, sizeof as1));
  args = caml_alloc(MAX_ARGSLEN, 0);
  ncur = 0;
  i = 0;
  //DBG(CAMLARGS, "%d: writing iseq_len (%ld)\n", ncur, iseq_len);
  //ncur += caml_store_int64(args, ncur, iseq_len);
  //ASSERT(ncur < MAX_ARGSLEN);
  //for (i = 0; i < iseq_len; i++) {
    DBG(CAMLARGS, "%d: writing insn %d\n", ncur, i);
    ncur += cmn_insn_arg(insn, args, ncur);
    ASSERT(ncur < MAX_ARGSLEN);
    DBG(CAMLARGS, "%d: After writing insn %d\n", ncur, i);
  //}
  DBG(CAMLARGS, "%d: writing num_nextpcs %d\n", ncur, num_nextpcs);
  ncur += caml_store_int64(args, ncur, num_nextpcs);
  ASSERT(ncur < MAX_ARGSLEN);
  DBG(CAMLARGS, "%d: writing nextpc_indir %d\n", ncur, nextpc_indir);
  ncur += caml_store_int64(args, ncur, nextpc_indir);
  ASSERT(ncur < MAX_ARGSLEN);
  
  FFLUSH(stdout);
  FFLUSH(stderr);

  str = caml_callback(*mlfunc, args);
  char *ret = strdup(String_val(str));
  //printf("%s() %d: ret = %s.\n", __func__, __LINE__, ret);
  return ret;
#endif
}
