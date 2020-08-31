#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include "support/debug.h"
#include "ppc/regs.h"
#include "valtag/regset.h"
#include "valtag/regmap.h"
//#include "ppc/regmap.h"
#include "support/src_tag.h"
#include "support/c_utils.h"

static char as1[4096], as2[4096];

regset_t volatile_regs;

#define ppc_regset_normalize_prefix(regset, prefix) do { \
  int i; \
  /*
  dbg ("before normalizing: " #prefix "bitmap=%" PRIx64 \
      ", " #prefix "fine_flags=%x. regset=%s\n", \
      regset->prefix##bitmap, regset->prefix##fine_flags, \
      ppc_regset_to_string(regset, as1, sizeof as1));  */ \
  for (i = 0; i < 8; i++) { \
    if (((regset->prefix##fine_flags >> (4*i)) & 0xf) == 0) { \
      regset->prefix##bitmap &= ~(0x1ULL << PPC_REG_CRFN(i)); \
    }\
    if ((regset->prefix##bitmap & (1ULL << PPC_REG_CRFN(i))) == 0) { \
      regset->prefix##fine_flags &= ~(0xf << (i*4)); \
    } \
  } \
} while(0)

#define ppc_regset_normalize(regset) do { \
  ppc_regset_normalize_prefix(regset, ); \
  ppc_regset_normalize_prefix(regset, v); \
} while (0)


/*src_regmap_t const *
ppc_v2c_regmap(void)
{
  static src_regmap_t __ppc_v2c_regmap;
  static bool init = false;
  int i, j;

  if (init) {
    return &__ppc_v2c_regmap;
  }
  for (i = 0; i < SRC_NUM_REGS - MAX_NUM_TEMPORARIES - 1; i++) {
    __ppc_v2c_regmap.table[i] = i + 1;
  }
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      __ppc_v2c_regmap.extable[i][j] = j;
    }
  }
  for (i = 0; i < MAX_NUM_TEMPORARIES; i++) {
    __ppc_v2c_regmap.temporaries[i] = SRC_NUM_REGS - MAX_NUM_TEMPORARIES - 1 + i;
  }
  init = true;
  return &__ppc_v2c_regmap;
}
*/

/*int
get_ppc_fine_flags_from_name(char *regname)
{
  int crfno, numchars, regno;

  regno = ppc_get_regnum_from_vname(regname);
  if (regno == -1) {
    regno = ppc_get_regnum_from_name(regname);
  }
  ASSERT(regno >= PPC_REG_CRFN(0) && regno < PPC_REG_CRFN(8));

  char *ptr = regname + numchars;

  if (*ptr != '[') {
    return 0xf;
  }
  int fine_flags = 0;
  ptr++;

  if (*ptr=='l') {
    fine_flags |= (1<<3);
    ptr++;
  }
  if (*ptr=='g') {
    fine_flags |= (1<<2);
    ptr++;
  }
  if (*ptr=='e') {
    fine_flags |= (1<<1);
    ptr++;
  }
  if (*ptr=='o') {
    fine_flags |= 1;
    ptr++;
  }
  ASSERT (*ptr == ']');

  return fine_flags;
}*/


char *
ppc_regname(int regnum, char *buf, size_t buf_size)
{
  ASSERT (regnum >= 0);
  if (regnum < 32) {
    snprintf (buf, buf_size, "r%d", regnum);
    return buf;
  }
  /*if (regnum >= PPC_REG_CRFN(0) && regnum < PPC_REG_CRFN(8)) {
    snprintf (buf, buf_size, "crf%d", regnum - PPC_REG_CRFN(0));
    return buf;
  }
  switch (regnum) {
    case PPC_REG_LR:
      snprintf (buf, buf_size, "lr");
      return buf;
    case PPC_REG_CTR:
      snprintf (buf, buf_size, "ctr");
      return buf;
    case PPC_REG_XER_SO:
      snprintf (buf, buf_size, "xer_so");
      return buf;
    case PPC_REG_XER_OV:
      snprintf (buf, buf_size, "xer_ov");
      return buf;
    case PPC_REG_XER_CA:
      snprintf (buf, buf_size, "xer_ca");
      return buf;
    case PPC_REG_XER_BC:
      snprintf (buf, buf_size, "xer_bc");
      return buf;
    case PPC_REG_XER_CMP:
      snprintf (buf, buf_size, "xer_cmp");
      return buf;
  }
  */
  ASSERT(0);
}

