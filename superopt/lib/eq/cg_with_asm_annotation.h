#pragma once

#include "support/log.h"
#include "support/mymemory.h"
#include "rewrite/assembly_handling.h"

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include "tfg/tfg.h"
#include "tfg/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_safety.h"
#include "eq/unsafe_cond.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {

using namespace std;

class cg_with_asm_annotation : public cg_with_safety
{
private:
public:
  cg_with_asm_annotation(corr_graph const &cg) : cg_with_safety(cg_with_inductive_preds(cg/*, cg.graph_with_guessing_get_invariants_map()*/))
  { }

  tuple<set<asm_insn_id_t>, map<asm_insn_id_t, string>, map<string, string>> cg_obtain_marker_call_and_other_annotations(asm_insn_id_t start_asm_insn_id) const;
protected:
private:
  pair<map<dst_ulong, string>, map<nextpc_id_t, string>> cg_get_dst_pc_and_nextpc_annotations() const;
  string get_asm_annotation_for_pcpair(pcpair const& pp) const;
};

}
