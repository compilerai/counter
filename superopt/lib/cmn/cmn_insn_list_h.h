#include "cmn.h"
#include "valtag/regmap.h"
#include "valtag/imm_vt_map.h"
#include "valtag/nextpc.h"

typedef struct cmn_insn_list_elem_t {
  struct cmn_insn_t insn[2];
  nextpc_t nextpcs[2];
  long num_nextpcs, num_insns;
  regmap_t st_regmap;
  imm_vt_map_t st_imm_map;

  string function_name;
  long insn_num_in_function;

  struct cmn_insn_list_elem_t *next; 
} cmn_insn_list_elem_t;
