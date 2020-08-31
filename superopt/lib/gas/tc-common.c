#include <support/debug.h>
#include "gas/tc-common.h"
#include "support/src_tag.h"
//#include "imm_map.h"
//#include "gas/gas_common.h"
#include "common/common.h"
#include "common/as.h"
#include "common/safe-ctype.h"
#include "common/subsegs.h"
#include "common/dwarf2dbg.h"
#include "common/dw2gencfi.h"
#include "common/frags.h"
#include "gas/universal.h"

/* Pre-defined "_GLOBAL_OFFSET_TABLE_".  */
symbolS *GOT_symbol;



symbolS *
md_undefined_symbol (char *name)
{
/*
  if (name[0] == GLOBAL_OFFSET_TABLE_NAME[0]
      && name[1] == GLOBAL_OFFSET_TABLE_NAME[1]
      && name[2] == GLOBAL_OFFSET_TABLE_NAME[2]
      && strcmp (name, GLOBAL_OFFSET_TABLE_NAME) == 0)
    {
      if (!GOT_symbol)
	{
	  if (symbol_find (name))
	    as_bad (_("GOT already in symbol table"));
	  GOT_symbol = symbol_new (name, undefined_section,
				   (valueT) 0, &zero_address_frag);
	};
      return GOT_symbol;
    }
*/
  return 0;
}

char *
md_atof (int type, char *litP, int *sizeP)
{
  /* This outputs the LITTLENUMs in REVERSE order;
     in accord with the bigendian 386.  */
  return ieee_md_atof (type, litP, sizeP, FALSE);
}

/* Chars that can be used to separate mant from exp in floating point
   nums.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant
   As in 0f12.456
   or    0d1.2345e12.  */
const char FLT_CHARS[] = "fFdDxX";


