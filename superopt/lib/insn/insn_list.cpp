#include <map>
#include "support/debug.h"
#include "support/src-defs.h"
#include "support/types.h"
#include "etfg/etfg_insn.h"
#include "rewrite/translation_instance.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "valtag/regmap.h"
#include "insn/cfg.h"
#include "insn/insn_list.h"
#include "valtag/fixed_reg_mapping.h"
#include "gsupport/graph_symbol.h"

static char as1[40960];

#define CMN COMMON_SRC
#include "cmn/cmn_insn_list.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_insn_list.h"
#undef CMN
