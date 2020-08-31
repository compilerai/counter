#include <stdlib.h>
#include "config-host.h"
#include "support/debug.h"
#include "gas/universal.h"
#include "gas/common/common.h"
#include "gas/common/as.h"
#include "gas/common/safe-ctype.h"
#include "common/common.h"
#include "common/as.h"
#include "common/safe-ctype.h"
#include "common/subsegs.h"
#include "common/dwarf2dbg.h"
#include "common/dw2gencfi.h"
#include "common/frags.h"
#include "gas/tc-ppc.h"
#include "gas/tc-i386.h"


struct expressionS;

int gas_assemble_mode = ARCH_I386;

int
universal_parse_name (const char *name, struct expressionS *exp, char *nextcharP)
{
#if (ARCH_SRC == ARCH_PPC || ARCH_DST == ARCH_PPC)
  if (gas_assemble_mode == ARCH_PPC) {
    return ppc_parse_name(name, exp);
  } else
#endif
#if (ARCH_SRC == ARCH_I386 || ARCH_DST == ARCH_I386)
  if (gas_assemble_mode == ARCH_I386) {
    return i386_parse_name(name, exp, nextcharP);
  }
#endif
  NOT_REACHED();
}
