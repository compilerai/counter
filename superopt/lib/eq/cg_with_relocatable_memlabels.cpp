#include "eq/cg_with_relocatable_memlabels.h"

namespace eqspace {


//bool
//cg_with_relocatable_memlabels::invariants_val_t::xfer_on_incoming_edge(shared_ptr<corr_graph_edge const> const& e, invariants_val_t const& in_unused, corr_graph const& cg)
//{
//  //cout << __func__ << ':' << __LINE__ << ": entry\n";
//  bool some_pred_made_weaker = update_proof_status_for_preds_over_edge(e, m_preds, cg);
//  if (some_pred_made_weaker) {
//    for (auto itr = m_preds.begin(); itr != m_preds.end(); ) {
//      if (itr->get_proof_status() == predicate::not_provable) {
//        cout << __func__ << ':' << __LINE__ << ": removed not provable pred: " << itr->to_string(true) << endl;
//        itr = m_preds.erase(itr);
//      } else { ++itr; }
//    }
//  }
//  return some_pred_made_weaker;
//}
//
//bool
//cg_with_relocatable_memlabels::invariants_val_t::update_proof_status_for_preds_over_edge(shared_ptr<corr_graph_edge const> const& e, unordered_set<predicate> const& preds, corr_graph const& cg) const
//{
//  bool some_guess_made_weaker = false;
//  pcpair const &from_pc = e->get_from_pc();
//  for (auto const& pred : preds) {
//    bool guess_made_weaker;
//    proof_status_t proof_status = cg.graph_with_proofs::decide_hoare_triple({}, from_pc, mk_edge_composition<pcpair,corr_graph_edge>(e), pred, guess_made_weaker); //we do not need to pass from_pc_preds here because they will anyway be used from within decide_hoare_triple
//    if (proof_status == proof_status_timeout) {
//      NOT_IMPLEMENTED();
//    }
//    some_guess_made_weaker = some_guess_made_weaker || pred.get_proof_status() == predicate::not_provable || guess_made_weaker;
//  }
//  return some_guess_made_weaker;
//}
//
//class update_proof_status_for_invariants_dfa_t : public data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, cg_with_relocatable_memlabels::invariants_val_t, dfa_dir_t::forward>
//{
//private:
//  corr_graph const& m_cg;
//public:
//  update_proof_status_for_invariants_dfa_t(corr_graph const& cg, map<pcpair, cg_with_relocatable_memlabels::invariants_val_t> &invariants_map)
//    : data_flow_analysis<pcpair, corr_graph_node, corr_graph_edge, cg_with_relocatable_memlabels::invariants_val_t, dfa_dir_t::forward>(&cg, invariants_map),
//      m_cg(cg)
//  { }
//
//  virtual bool xfer_and_meet(shared_ptr<corr_graph_edge const> const& e, cg_with_relocatable_memlabels::invariants_val_t const& in, cg_with_relocatable_memlabels::invariants_val_t& out) override
//  {
//    return out.xfer_on_incoming_edge(e, in, m_cg);
//  }
//};
//
//void
//cg_with_relocatable_memlabels::update_invariants_proof_status()
//{
//  update_proof_status_for_invariants_dfa_t update_proof_status_dfa(*this, m_invariants_map);
//  update_proof_status_dfa.solve();
//}

bool
cg_with_relocatable_memlabels::check_equivalence_proof()
{
  //return true;
  autostop_timer ft(__func__);
  MSG("Forcing use of abstract memlabel assertions\n");
  this->use_abstract_memlabel_assertion();
  bool ret = this->recompute_invariants_at_all_pcs();
  if (!ret) {
    MSG("Equivalence check failed on product CFG with relocatable memlabels\n");
    return false;
  }
  if (this->cg_with_inductive_preds::check_equivalence_proof()) {
    MSG("Equivalence check passed on product CFG with relocatable memlabels\n");
    return true;
  } else {
    MSG("Equivalence check failed on product CFG with relocatable memlabels\n");
    return false;
  }
}

//map<expr_id_t,pair<expr_ref,expr_ref>>
//cg_with_relocatable_memlabels::reset_memlabel_range_submap() const
//{
//  NOT_IMPLEMENTED();
//  //map<expr_id_t,pair<expr_ref,expr_ref>> ret = this->get_context()->get_memlabel_range_submap();
//  //this->get_context()->set_memlabel_range_submap({});
//  //return ret;
//}
//
//void
//cg_with_relocatable_memlabels::set_memlabel_range_submap(map<expr_id_t,pair<expr_ref,expr_ref>> const& memlabel_range_submap) const
//{
//  NOT_IMPLEMENTED();
//  //this->get_context()->set_memlabel_range_submap(memlabel_range_submap);
//}

}
