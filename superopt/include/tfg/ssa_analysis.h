# include"tfg/tfg.h"
# include"graph/graph_with_ssa_funcs.h"

namespace eqspace {

using namespace std;

class ssa_construction_analysis
{
public:
  ssa_construction_analysis(tfg const& t, dshared_ptr<tfg const> const& src_tfg) : m_tfg(t.tfg_ssa_copy()) 
  {
    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": Input TFG for SSA:\n" << t.graph_to_string() << endl); 
//    tfg_rename_vars_to_non_ssa();
//    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": TFG After non-ssa renaming:\n" << m_tfg->graph_to_string() << endl); 
//    collapse_phi_edges();
//    m_tfg->compute_regions();   
//    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": TFG After collpase phi edges:\n" << m_tfg->graph_to_string() << endl); 
    if (!m_tfg->get_context()->get_config().disable_ssa_cloning) {
      m_tfg->tfg_conservatively_add_all_phi_edges();
    }
    m_tfg->compute_regions();   
    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": After splitting all edges to have phi edges: TFG:\n" << m_tfg->graph_to_string() << endl);
    ssa_phi_placement_analysis();
    if(m_required_phi_mappings.size()) {
      add_phi_edges();
      m_tfg->compute_regions();   
      DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": After add Phi edges: TFG:\n" << m_tfg->graph_to_string() << endl); 
    }
    ssa_rename_vars();
    m_tfg->compute_regions();   
    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": After var renaming: TFG:\n" << m_tfg->graph_to_string() << endl);
//    m_tfg->tfg_run_copy_propagation();
//    m_tfg->compute_regions();   
//    m_tfg->populate_var_liveness();
//    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": After copy propagation: TFG:\n" << m_tfg->graph_to_string() << endl);
//    m_tfg->tfg_remove_dead_vars();
//    m_tfg->compute_regions();   
//    m_tfg->collapse_nop_edges();
//    m_tfg->compute_regions();
//    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": After remove dead vars: TFG:\n" << m_tfg->graph_to_string() << endl);
    if(src_tfg) {
      // this is ssa for DST tfg
      m_tfg->set_locs_map_start_loc_id(DST_TFG_SSA_START_LOC_ID);
    } else {
      // this is ssa for SRC tfg
      m_tfg->set_locs_map_start_loc_id(SRC_TFG_SSA_START_LOC_ID);
    }
    m_tfg->set_graph_as_ssa();
    m_tfg->tfg_populate_structs_after_ssa_construction(src_tfg);
    DYN_DEBUG(ssa_transform, cout << __func__ << " " << __LINE__ << ": Final SSA TFG:\n" << m_tfg->graph_to_string() << endl);
//    } else {
//      ASSERT(!m_required_phi_mappings.size());
//      m_tfg = dshared_ptr<tfg>::dshared_nullptr();
//    }
  }

  dshared_ptr<tfg> get_ssa_graph() const
  {
    return m_tfg;
  }

private:

  void collapse_phi_pc(pc const& p) {
    context* ctx = m_tfg->get_context();
    list<dshared_ptr<tfg_edge const>> incoming, outgoing;
    m_tfg->get_edges_outgoing(p, outgoing);
    m_tfg->get_edges_incoming(p, incoming);
    ASSERT(incoming.size() == 1 && outgoing.size() == 1);
    auto edge1 = incoming.front();
    auto edge2 = outgoing.front();
    state collapsed_edge_state; 
    for (auto &kv : edge1->get_to_state().get_value_expr_map_ref()) {
      collapsed_edge_state.set_expr_in_map(kv.first, kv.second);
    }
    for (auto &kv : edge2->get_to_state().get_value_expr_map_ref()) {
      // Ignore if it is an identity mapping 
      if(ctx->get_input_expr_for_key(kv.first, kv.second->get_sort()) == kv.second)
        continue;
      collapsed_edge_state.set_expr_in_map(kv.first, kv.second);
    }
    ASSERT(edge2->get_cond() == expr_true(ctx));
    auto collapsed_edge = mk_tfg_edge(edge1->get_from_pc(), edge2->get_to_pc(), edge1->get_cond(), collapsed_edge_state, edge2->get_assumes(), edge1->get_te_comment());
    m_tfg->remove_edge(edge1);
    m_tfg->remove_edge(edge2);
    m_tfg->graph_with_proofs<pc, tfg_node, tfg_edge, predicate>::add_edge(collapsed_edge);
  }

//  void collapse_phi_edges() {
//    set<pc> ssa_phi_pcs;
//    for(auto const& p : m_tfg->get_all_pcs()) {
//      if(p.is_ssa_phi_pc())
//        ssa_phi_pcs.insert(p);
//    }
//    for(auto const& p : ssa_phi_pcs) {
//      collapse_phi_pc(p);
//      m_tfg->remove_node(p);
//    }
//  }
  void ssa_phi_placement_analysis()
  {
    context* ctx = m_tfg->get_context();
    m_tfg->populate_varname_modification_edges();
    map<pc, ssa_reaching_defs_t<pc, tfg_node, tfg_edge, predicate>> vars_reaching_definitions_map = m_tfg->compute_vars_reaching_definitions();
    for(auto const& m : vars_reaching_definitions_map) {
      pc const& p = m.first;
      map<shared_ptr<tfg_edge const>, set<string_ref>> phi_mappings;
      for(auto const& v_eset : m.second.get_phi_varset()) {
        string_ref v = v_eset.first;
        if(!m_varname_default_expr_map.count(v)) {
          expr_ref v_def = m_tfg->get_default_input_expr_for_varname(v);
//          expr_ref non_ssa_def_expr = ctx->get_input_expr_for_key(non_ssa_key, v_def->get_sort());
          m_varname_default_expr_map.insert(make_pair(v, v_def));
        }
        for(auto const& e : v_eset.second) {
          phi_mappings[e].insert(v);
        }
      }
      if(phi_mappings.size())
        m_required_phi_mappings.insert(make_pair(p, phi_mappings));
    }
    
    DYN_DEBUG2(ssa_transform,
      for(auto const& pvset : m_required_phi_mappings) {
        cout << "PC: " << pvset.first.to_string() << endl;
        for(auto const& e_vset : pvset.second) {
          cout << "From edge: " << e_vset.first->to_string_concise() << endl;
          for(auto const& v : e_vset.second)
            cout << v->get_str() << ", ";
          cout << endl;
        }
      }
    );
  }


  void ssa_rename_vars();
  

//  void tfg_rename_vars_to_non_ssa()
//  {
//    context *ctx = m_tfg->get_context();
//    std::function<expr_ref (pc const &p, const string&, expr_ref)> f = [ctx, this](pc const &p, const string& key, expr_ref e) -> expr_ref { 
//      list<expr_ref> const& var_exprs = ctx->expr_get_vars(e);
//      map<expr_id_t, pair<expr_ref, expr_ref>> subst_map;
//      for(auto const& v: var_exprs) {
//        ASSERT(v->is_var());
//        string non_ssa_v = get_non_ssa_varname(v->get_name()->get_str());
//        expr_ref non_ssa_v_expr = ctx->mk_var(non_ssa_v, v->get_sort());
//        if(v != non_ssa_v_expr)
//          subst_map.insert(make_pair(v->get_id(), make_pair(v, non_ssa_v_expr))); // var_expr = input.varname
//      }
//      return ctx->expr_substitute(e, subst_map);
//    };
//    std::function<string (pc const &p, const string&, expr_ref)> f_keys = [this](pc const &p, const string& key, expr_ref e) -> string { 
//      // Input p should be from_pc for the edge
//      string non_ssa_key = get_non_ssa_varname(key);
//      return non_ssa_key;
//    };
//    m_tfg->tfg_visit_exprs_and_keys(f, f_keys);
//    m_tfg->compute_regions();
//  }

//  string get_ssa_renamed_varname(string const& varname, pc const& to_pc)
//  {
//    ASSERT(varname == get_non_ssa_varname(varname));
//
//    stringstream ss;
//    ss << varname << "." << to_pc.to_string();
//    return ss.str();
//  }

//  static bool is_ssa_renamed_var(string const& varname, pc const& to_pc)
//  {
//    return string_has_suffix(varname, to_pc.to_string());
//  }

//  string get_non_ssa_varname(string const& varname) const {
//    for(auto const& p : m_tfg->get_all_pcs()) {
//      if(is_ssa_renamed_var(varname, p)) {
//        ASSERT(varname.size() > (p.to_string().size()+1));
//        return varname.substr(0, varname.size() - p.to_string().size()-1);
//      }
//    }
//    return varname;
//  }
/*
  map<expr_id_t, pair<expr_ref, expr_ref>>
  get_substitution_map_for_pc(context* ctx, list<expr_ref> const& var_exprs, map<expr_ref, string> const& pc_var_id_map)
  {
    DYN_DEBUG3(ssa_transform, 
        cout << "pc_var_id_map is: " << endl;
        for(auto const& ev : pc_var_id_map) {
          cout << expr_string(ev.first) << " => " << ev.second << endl;
        }
    );
        
    map<expr_id_t, pair<expr_ref, expr_ref>> ret_subst_map;
    for(auto const& v: var_exprs) {
      ASSERT(v->is_var());
      string non_ssa_k = get_non_ssa_varname(v->get_name()->get_str());
      expr_ref non_ssa_k_expr = ctx->mk_var(non_ssa_k, v->get_sort());
      DYN_DEBUG2(ssa_transform, 
          cout << "var: " << expr_string(v) << ", non_ssa_k_expr: " << expr_string(non_ssa_k_expr) << endl;
      );
      if(!pc_var_id_map.count(non_ssa_k_expr))
        continue;
      string new_vname = pc_var_id_map.at(non_ssa_k_expr);
      expr_ref new_v = ctx->mk_var(new_vname, v->get_sort());
      if(v != new_v)
        ret_subst_map.insert(make_pair(v->get_id(), make_pair(v, new_v))); // var_expr = input.varname
    }
    return ret_subst_map;
  }
  
  void update_ssa_ids_for_edge(dshared_ptr<tfg_edge const> const &e)
  {
    context *ctx = m_tfg->get_context();
    pc const& from_pc = e->get_from_pc();
    pc const& to_pc = e->get_to_pc();
    state const& st = e->get_to_state();
  
    ASSERT(m_pc_var_id_map.count(from_pc));
    DYN_DEBUG2(ssa_transform, 
        cout << _FNLN_ <<" pc_var_id_map at " << from_pc.to_string() << endl;
        for(auto const& ev : m_pc_var_id_map.at(from_pc)) {
          cout << expr_string(ev.first) << " => " << ev.second << endl;
        }
    );
        
    map<expr_ref, string> to_pc_var_id_map;
    if(m_pc_var_id_map.count(to_pc))
      to_pc_var_id_map = m_pc_var_id_map.at(to_pc);
    DYN_DEBUG2(ssa_transform, 
        cout << _FNLN_ <<" Initial pc_var_id_map at " << to_pc.to_string() << endl;
        for(auto const& ev : to_pc_var_id_map) {
          cout << expr_string(ev.first) << " => " << ev.second << endl;
        }
    );
        
    for (auto &kv : st.get_value_expr_map_ref()) {
      string k = kv.first->get_str(); 
      string non_ssa_k = get_non_ssa_varname(k);
      expr_ref non_ssa_k_expr = ctx->get_input_expr_for_key(mk_string_ref(non_ssa_k), kv.second->get_sort());
      if(!to_pc_var_id_map.count(non_ssa_k_expr)) {
        to_pc_var_id_map[non_ssa_k_expr] = get_ssa_renamed_varname(non_ssa_k_expr->get_name()->get_str(), to_pc);
      }
    }
    ASSERT(m_pc_var_id_map.count(from_pc));
    for(auto const& ev: m_pc_var_id_map.at(from_pc)) {
      expr_ref const& var_expr = ev.first;
      ASSERT(var_expr->is_var());
      if(!to_pc_var_id_map.count(var_expr))
        to_pc_var_id_map[var_expr] = ev.second;
    }
    DYN_DEBUG2(ssa_transform, 
        cout << _FNLN_ <<" After edge " << e->to_string_concise() << " pc_var_id_map at " << to_pc.to_string() << endl;
        for(auto const& ev : to_pc_var_id_map) {
          cout << expr_string(ev.first) << " => " << ev.second << endl;
        }
    );
    m_pc_var_id_map[to_pc] = to_pc_var_id_map;
  }

  map<string, string> compute_state_keys_rename_map(context* ctx, dshared_ptr<tfg_edge const> const &e, map<pc, map<expr_ref, string>> const& pc_var_id_map)
  {
    pc const& from_pc = e->get_from_pc();
    pc const& to_pc = e->get_to_pc();
    state const& st = e->get_to_state();
    ASSERT(pc_var_id_map.count(to_pc));
    map<expr_ref, string> to_pc_var_id_map = pc_var_id_map.at(to_pc);
    map<string, string> ret_state_keys_rename_map;
    for (auto &kv : st.get_value_expr_map_ref()) {
      string k = kv.first->get_str(); 
      string non_ssa_k = get_non_ssa_varname(k);
      expr_ref non_ssa_k_expr = ctx->get_input_expr_for_key(mk_string_ref(non_ssa_k), kv.second->get_sort());
      ASSERT(to_pc_var_id_map.count(non_ssa_k_expr));
      string k_rename = to_pc_var_id_map.at(non_ssa_k_expr);
      ASSERT(string_has_prefix(k_rename, G_INPUT_KEYWORD "."));
      string key_rename = k_rename.substr(string(G_INPUT_KEYWORD ".").length());
      if(k != key_rename)
        ret_state_keys_rename_map[k] = key_rename;
    }
    return ret_state_keys_rename_map;
  }

  bool rename_edge_vars(dshared_ptr<tfg_edge const> const &e)
  {
    // in the to_state RHS, econd and assumes, add ssa_ver_id reaching the from pc
    // for v in  LHS of to_state, inc its max_ver_id and use v.max_ver_id 
    context *ctx = m_tfg->get_context();
    pc const& from_pc = e->get_from_pc();
    pc const& to_pc = e->get_to_pc();
    update_ssa_ids_for_edge(e);
    map<string, string> state_keys_rename_map = compute_state_keys_rename_map(ctx, e, m_pc_var_id_map);
    ASSERT(m_pc_var_id_map.count(from_pc));
    auto const& from_pc_map = m_pc_var_id_map.at(from_pc);
    std::function<expr_ref (expr_ref const &)> fe = [this, ctx, &from_pc_map](expr_ref const &e)
    {
      list<expr_ref> const& var_exprs = ctx->expr_get_vars(e);
      map<expr_id_t, pair<expr_ref, expr_ref>> subst_map = this->get_substitution_map_for_pc(ctx, var_exprs, from_pc_map);
      return ctx->expr_substitute(e, subst_map);
    };
    auto e_after_state_subst = e->visit_exprs(fe);
    state new_st = e_after_state_subst->get_to_state();
    new_st.state_rename_keys(state_keys_rename_map);
    auto e_after_keys_renaming = e_after_state_subst->tfg_edge_set_state(new_st);
    m_tfg->replace_edge(e, e_after_keys_renaming);
    return (state_keys_rename_map.size());
  }

  bool ssa_rename_vars_rec(pc const& p, set<pc> &visited_pcs)
  {
    bool changed = false;
    if(visited_pcs.count(p))
      return changed;
    context *ctx = m_tfg->get_context();
    list<dshared_ptr<tfg_edge const>> outgoing;
    m_tfg->get_edges_outgoing(p, outgoing);
    for(auto const& e: outgoing) {
      pc to_pc = e->get_to_pc();
      bool this_edge_changed = rename_edge_vars(e);
      if(!this_edge_changed) visited_pcs.insert(p);
      changed = this_edge_changed || changed;
      changed = ssa_rename_vars_rec(to_pc, visited_pcs) || changed;
    }
    auto return_regs = m_tfg->get_return_regs();
    if (return_regs.count(p)) {
      ASSERT(m_pc_var_id_map.count(p));
      auto const& pc_map = m_pc_var_id_map.at(p);
      for (auto &pr : return_regs.at(p)) {
        expr_ref const &e = pr.second;
        list<expr_ref> const& var_exprs = ctx->expr_get_vars(e);
        map<expr_id_t, pair<expr_ref, expr_ref>> subst_map = get_substitution_map_for_pc(ctx, var_exprs, pc_map);
        pr.second = ctx->expr_substitute(e, subst_map);
      }
      m_tfg->set_return_regs(return_regs);
    }
    return changed;
  }


  bool ssa_rename_vars()
  {
    context *ctx = m_tfg->get_context();
    //map<expr_ref, unsigned> var_max_ver_id;
    //map<pc, map<expr_ref, unsigned>> pc_var_id_map;
    set<pc> visited_pcs;
    pc start_pc = m_tfg->get_entry_pc();
    state init_state = m_tfg->get_start_state().get_state();
    map<expr_ref, string> start_pc_var_id_map;
    for (auto &kv : init_state.get_value_expr_map_ref()) {
      ASSERT(kv.second->is_var());
      expr_ref non_ssa_v_expr = ctx->mk_var(get_non_ssa_varname(kv.second->get_name()->get_str()), kv.second->get_sort());
      start_pc_var_id_map[non_ssa_v_expr] = kv.second->get_name()->get_str();
    }
    m_pc_var_id_map[start_pc] = start_pc_var_id_map;
    return ssa_rename_vars_rec(start_pc, visited_pcs);
  }
  */

  void add_phi_mapping_between_pcs_for_input_varset(pc const& from_pc, pc const& to_pc, set<string_ref> const& vset)
  {
    if (!vset.size())
      return;
    context* ctx = m_tfg->get_context();
    dshared_ptr<tfg_edge const> old_e = m_tfg->get_edge(from_pc, to_pc); 
    ASSERT(old_e);
    state state_with_phi_mappings = old_e->get_to_state(); 
    for (auto const& v : vset) {
      bool var_exists_in_state = false;
      string root_v = get_root_varname_for_ssa_renamed_var<pc>(v->get_str());
      for (auto const& kv : state_with_phi_mappings.get_value_expr_map_ref()) {
        string root_key = get_root_varname_for_ssa_renamed_var<pc>(kv.first->get_str());
        if (root_key == root_v) {
          var_exists_in_state = true;
          break;
        }
      }
      if (var_exists_in_state)
        continue;
      assert(m_varname_default_expr_map.count(v));
      expr_ref def_var_expr = m_varname_default_expr_map.at(v);
      state_with_phi_mappings.set_expr_in_map(v, def_var_expr);
    }
//    if(from_pc.is_ssa_phi_pc()) {
//      auto phi_edge = mk_tfg_edge(from_pc, to_pc, old_e->get_cond(), state_with_phi_mappings, old_e->get_assumes(), old_e->get_te_comment());
//      m_tfg->remove_edge(old_e);
//      m_tfg->graph_with_proofs<pc, tfg_node, tfg_edge, predicate>::add_edge(phi_edge);
//    } else {
//      unsigned i = 0;
//      pc intermediate_pc;
//      do {
//        intermediate_pc = pc(pc::insn_label, to_pc.get_index(), to_pc.get_subindex(), PC_SUBSUBINDEX_SSA_PHINUM(i++));
//      } while (m_tfg->get_all_pcs().count(intermediate_pc));
//      dshared_ptr<tfg_node> intermediate_node = make_dshared<tfg_node>(intermediate_pc);
//      m_tfg->add_node(intermediate_node);
//      auto updated_edge = mk_tfg_edge(from_pc, intermediate_pc, old_e->get_cond(), old_e->get_to_state(), old_e->get_assumes(), old_e->get_te_comment());
//      auto new_edge = mk_tfg_edge(intermediate_pc, to_pc, expr_true(ctx), state_with_phi_mappings, {}, te_comment_t(true, -1, "phivar.assignment.edge"));
//      m_tfg->remove_edge(old_e);
//      m_tfg->graph_with_proofs<pc, tfg_node, tfg_edge, predicate>::add_edge(updated_edge);
//      m_tfg->graph_with_proofs<pc, tfg_node, tfg_edge, predicate>::add_edge(new_edge);
//    }
    auto updated_edge = mk_tfg_edge(from_pc, to_pc, old_e->get_cond(), state_with_phi_mappings, old_e->get_assumes(), old_e->get_te_comment());
    m_tfg->remove_edge(old_e);
    m_tfg->graph_with_proofs<pc, tfg_node, tfg_edge, predicate>::add_edge(updated_edge);
    
  }
  void add_phi_edges()
  {
//    set<pc> visited_pcs;
    for(auto const& pvset : m_required_phi_mappings) {
//      string_ref const& v = vps.first;
      auto const& to_pc = pvset.first;      
      list<dshared_ptr<tfg_edge const>> incoming; 
      m_tfg->get_edges_incoming(to_pc, incoming);
//      ASSERT(incoming.size() > 1);
      for(auto const& ie : incoming) {
        pc from_pc = ie->get_from_pc();
        set<string_ref> phi_vset;
        for(auto const& e_vset: pvset.second) {
          if(e_vset.first->get_from_pc() == from_pc) {
            phi_vset = e_vset.second;
            break;
          }
        }
        add_phi_mapping_between_pcs_for_input_varset(from_pc, to_pc, phi_vset);
      }
    }
  }

//  map<expr_ref, unsigned> m_var_max_ver_id;
  map<pc, map<shared_ptr<tfg_edge const>, set<string_ref>>> m_required_phi_mappings;
  map<pc, map<expr_ref, string>> m_pc_var_id_map;
  map<string_ref, expr_ref> m_varname_default_expr_map;
  dshared_ptr<tfg> m_tfg;
 
};

}
