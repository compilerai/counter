#include <stdlib.h>
#include <assert.h>
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "ppc/insn.h"
#include "valtag/regset.h"
#include "valtag/memset.h"
#include "rewrite/usedef_tab_entry.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"

static char as1[4096000];
#include "rewrite/usedef_tab_entry.h"
static dshared_ptr<insn_db_t<src_insn_t, usedef_tab_entry_t>> usedef_tab_src = dshared_ptr<insn_db_t<src_insn_t, usedef_tab_entry_t>>::dshared_nullptr();
static dshared_ptr<insn_db_t<dst_insn_t, usedef_tab_entry_t>> usedef_tab_dst = dshared_ptr<insn_db_t<dst_insn_t, usedef_tab_entry_t>>::dshared_nullptr();




#define CMN COMMON_SRC
#include "cmn/cmn_usedef.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_usedef.h"
#undef CMN

void
usedef_init(void)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << ": entry\n";
  usedef_tab_src = src_usedef_tab_init();
#if ARCH_SRC == ARCH_DST
  usedef_tab_dst = usedef_tab_src;
#else
  usedef_tab_dst = dst_usedef_tab_init();
#endif
  //cout << __func__ << " " << __LINE__ << ": exit\n";
}

