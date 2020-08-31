#include "eq/cg_with_asm_annotation.h"
#include "eq/corr_graph.h"
#include "tfg/predicate_set.h"
#include "eq/unsafe_cond.h"
#include "support/debug.h"
#include "rewrite/assembly_handling.h"

namespace eqspace {

tuple<set<asm_insn_id_t>, map<asm_insn_id_t, string>, map<string, string>>
cg_with_asm_annotation::cg_obtain_marker_call_and_other_annotations(asm_insn_id_t start_insn_id) const
{
  //m_assembly_file.assembly_file_to_stream(cout);
  shared_ptr<eqcheck> const& eq = this->get_eq();
  tfg const& src_tfg = eq->get_src_tfg();
  tfg const& dst_tfg = eq->get_dst_tfg();
  map<nextpc_id_t, string> const& npc_map = src_tfg.get_nextpc_map();
  pair<map<dst_ulong, string>, map<nextpc_id_t, string>> annotations = this->cg_get_dst_pc_and_nextpc_annotations();
  map<dst_ulong, string> const& pc_annotations = annotations.first;
  map<nextpc_id_t, string> const& nextpc_annotations = annotations.second;
  vector<dst_ulong> const& dst_insn_pcs = eq->get_dst_insn_pcs();

  set<dst_ulong> marker_call_instructions = dst_tfg.tfg_dst_get_marker_call_instructions(dst_insn_pcs);

  set<dst_ulong> dst_insn_pcs_sorted;
  for (auto dst_insn_pc : dst_insn_pcs) {
    if (dst_insn_pc != PC_UNDEFINED) {
      dst_insn_pcs_sorted.insert(dst_insn_pc);
    }
  }

  set<asm_insn_id_t> marker_call_insn_ids;
  map<asm_insn_id_t, string> insn_id_annotations;
  size_t insn_id = start_insn_id;
  for (auto dst_insn_pc : dst_insn_pcs_sorted) {
    if (marker_call_instructions.count(dst_insn_pc)) {
      marker_call_insn_ids.insert(insn_id);
    }
    if (pc_annotations.count(dst_insn_pc)) {
      insn_id_annotations.insert(make_pair(insn_id, pc_annotations.at(dst_insn_pc)));
    }
    insn_id++;
  }
  map<string, string> exit_annotations;
  for (auto const& npc_annot : nextpc_annotations) {
    string exitname = (npc_map.count(npc_annot.first) == 0) ? "return" : npc_map.at(npc_annot.first);
    exit_annotations.insert(make_pair(exitname, npc_annot.second));
  }
  return make_tuple(marker_call_insn_ids, insn_id_annotations, exit_annotations);
}

string
cg_with_asm_annotation::get_asm_annotation_for_pcpair(pcpair const& pp) const
{
  stringstream ss;
  ss << string("# ") + pp.to_string() + ":";
  invariant_state_t<pcpair, corr_graph_node, corr_graph_edge, predicate> const& inv_state = this->get_invariant_state_at_pc(pp);
  inv_state.invariant_state_get_asm_annotation(ss);
  ss << "\n";
  return ss.str();
}

pair<map<dst_ulong, string>, map<nextpc_id_t, string>>
cg_with_asm_annotation::cg_get_dst_pc_and_nextpc_annotations() const
{
  shared_ptr<eqcheck> const& eq = this->get_eq();
  vector<dst_ulong> const& dst_insn_pcs = eq->get_dst_insn_pcs();
  tfg const& src_tfg = eq->get_src_tfg();
  map<dst_ulong, string> pc_annotations;
  map<nextpc_id_t, string> nextpc_annotations;
  for (auto const& pp : this->get_all_pcs()) {
    pc const& dst_pc = pp.get_second();
    char const* dst_pc_index = dst_pc.get_index();
    string annot = this->get_asm_annotation_for_pcpair(pp);
    if (dst_pc.is_label()) {
      int dst_insn_index = strtol(dst_pc_index, nullptr, 10);
      ASSERT(dst_insn_index >= 0 && dst_insn_index < dst_insn_pcs.size());
      pc_annotations.insert(make_pair(dst_insn_pcs.at(dst_insn_index), annot));
    } else if (dst_pc.is_exit()) {
      nextpc_id_t nextpc_id = strtol(dst_pc_index, nullptr, 10);
      //cout << __func__ << " " << __LINE__ << ": dst_pc_index = '" << dst_pc_index << "'\n";
      //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << "\n";
      nextpc_annotations.insert(make_pair(nextpc_id, annot));
    } else NOT_REACHED();
  }
  return make_pair(pc_annotations, nextpc_annotations);
}

}
