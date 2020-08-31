#include "cmn.h"
#include "rewrite/insn_line_map.h"


//struct locals_info_t;

struct cmn_insn_t;
struct vconstants_t;
struct typestate_t;
struct transmap_t;
struct regmap_t;
struct insn_line_map_t;
class rand_states_t;


bool cmn_iseq_matches_with_symbols(struct cmn_insn_t const *iseq, long iseq_len,
    struct imm_vt_map_t const *imm_vt_map);
