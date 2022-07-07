#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"


#include "graph/graph.h"
#include "graph/graph_with_regions.h"
#include "graph/graph_with_var_versions.h"
#include "graph/graph_with_guessing.h"
#include "graph/var_liveness_dfa.h"
#include "gsupport/reaching_defs.h"


namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_ssa_funcs;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class ssa_rename_map_t {
private:
  map<expr_ref, string> m_map;
public:
  ssa_rename_map_t() : m_map()
  { }
  ssa_rename_map_t(map<expr_ref, string> const& m) : m_map(m)
  { }
  ssa_rename_map_t(ssa_rename_map_t  const& other) : m_map(other.m_map)
  { }
  static ssa_rename_map_t top()
  {
    ssa_rename_map_t ret;
    return ret;
  }
  void add_mapping(expr_ref const& e, string const& s)
  {
    m_map[e] = s;
  }
  
  map<expr_ref, string> get_map() const { return m_map;}
  
  
  static std::function<ssa_rename_map_t(T_PC const &)>
  get_boundary_value(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g)
  {
    auto f = [g](T_PC const &p)
    {
      ASSERT(p.is_start());
      dshared_ptr<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const> gp = dynamic_pointer_cast<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const>(g);

      context* ctx = gp->get_context();
      ssa_rename_map_t retval;
      state init_state = gp->get_start_state().get_state();
      for (auto &kv : init_state.get_value_expr_map_ref()) {
        ASSERT(kv.second->is_var());
        expr_ref v_expr = ctx->get_input_expr_for_key(kv.first, kv.second->get_sort());
        retval.add_mapping(v_expr, v_expr->get_name()->get_str());
      }
      //expr_ref sp = get_sp_version_at_entry(ctx);
      //string name = sp->get_name()->get_str();
      //size_t pos = name.find(ESP_VERSION_REGISTER_NAME_PREFIX);
      //ASSERT(pos != string::npos);
      //string sp_base = name.substr(0, pos + strlen(ESP_VERSION_REGISTER_NAME_PREFIX)-1);
      //expr_ref sp_base_var = ctx->mk_var(sp_base, sp->get_sort());
      //retval.add_mapping(sp_base_var, name);

      return retval;
    };
    return f;
  }
};

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class ssa_reaching_defs_t {
private:
  // map from varname to map of incoming edge and the corresponding rdef
//  map<string_ref, map<shared_ptr<T_E const>, shared_ptr<T_E const>> > m_map;
  // map from varname to pair of rdef and the set of incoming edges for that same rdef
  map<string_ref, pair<shared_ptr<T_E const>, set<shared_ptr<T_E const>>> > m_map;

  //map from varname to set of incoming edges from which we have received rdefs for this varname
  //Only m_map wont suffice because during meet we kiil the exact incoming edges in case of multiple rdefs and add a phi rdef
  map<string_ref, set<shared_ptr<T_E const>>> m_phi_varset;
public:
  ssa_reaching_defs_t() : m_map(), m_phi_varset()
  { }
  ssa_reaching_defs_t(map<string_ref, pair<shared_ptr<T_E const>, set<shared_ptr<T_E const>>> > const& m) : m_map(m), m_phi_varset()
  { }
  ssa_reaching_defs_t(ssa_reaching_defs_t const& other) : m_map(other.m_map), m_phi_varset(other.m_phi_varset)
  { }
  static ssa_reaching_defs_t top()
  {
    ssa_reaching_defs_t ret;
    return ret;
  }
  void add_reaching_definition(string_ref const& v, shared_ptr<T_E const> const& incoming_edge, shared_ptr<T_E const> const& rdef)
  {
    set<shared_ptr<T_E const>> ie_set;
    if(m_map.count(v)){
      ASSERT(m_map.at(v).first == rdef);
      ie_set = m_map.at(v).second;
    }
    ie_set.insert(incoming_edge);
    m_map[v] = make_pair(rdef, ie_set);
  }
  
  map<string_ref, pair<shared_ptr<T_E const>, set<shared_ptr<T_E const>>> > get_map() const { return m_map;}
  
  void reaching_definitions_meet(ssa_reaching_defs_t const& other, shared_ptr<T_E const> const& dummy_edge) 
  {
    for (auto const& om : other.m_map) {
      pair<shared_ptr<T_E const>, set<shared_ptr<T_E const>>> other_rdefs = om.second;
      if (!m_map.count(om.first)) {
        m_map[om.first] = other_rdefs;
      } else {
        pair<shared_ptr<T_E const>, set<shared_ptr<T_E const>>> this_rdefs = m_map[om.first];
        shared_ptr<T_E const> rdef1 = this_rdefs.first;
        shared_ptr<T_E const> rdef2 = other_rdefs.first;
        if(rdef1 == rdef2) {
          set<shared_ptr<T_E const>> rdef_ie = this_rdefs.second;
          set_union(rdef_ie, other_rdefs.second);
          m_map[om.first] = make_pair(rdef1, rdef_ie);
//        } else if(rdef1->get_to_pc() == rdef2->get_to_pc()) {
//          out.kill_and_add_phi_edge(m.first, gp->get_dummy_edge_from_to_pc(e->get_to_pc()), gp->get_dummy_edge_from_to_pc(e->get_to_pc()));
      } else if(this_rdefs.second == other_rdefs.second) {
          m_map[om.first] = make_pair(rdef2, this_rdefs.second);
//        } else if(rdef1->get_to_pc() == rdef2->get_to_pc()) {
//          out.kill_and_add_phi_edge(m.first, gp->get_dummy_edge_from_to_pc(e->get_to_pc()), gp->get_dummy_edge_from_to_pc(e->get_to_pc()));
        } else {
          add_phi_mapping(om.first, this_rdefs.second);
          add_phi_mapping(om.first, other_rdefs.second);
          set<shared_ptr<T_E const>> phi_ie_set;
          phi_ie_set.insert(dummy_edge);
          m_map[om.first] = make_pair(dummy_edge, phi_ie_set);
//          out.kill_and_add_phi_edge(om.first, gp->get_dummy_edge_from_to_pc(e->get_to_pc()), gp->get_dummy_edge_from_to_pc(e->get_to_pc()));
        }
      }
    }
  }
//  void kill_and_add_phi_edge(string_ref const& s, shared_ptr<T_E const> const& e, shared_ptr<T_E const> const& rdef)
//  {
//    ASSERT(m_map.count(s));
//    ASSERT(m_map.at(s).size() == 2);
//    map<shared_ptr<T_E const>, shared_ptr<T_E const>> phi_rdef;
//    phi_rdef.insert(make_pair(e,rdef));
//    m_map[s] = phi_rdef;
//  }

  void add_phi_mapping(string_ref const& s, set<shared_ptr<T_E const>> const& ie_set) {
    set<shared_ptr<T_E const>> new_ie_set;
    if(m_phi_varset.count(s))
      new_ie_set = m_phi_varset.at(s);
    set_union(new_ie_set, ie_set);
    m_phi_varset[s] = new_ie_set;
  }

  map<string_ref, set<shared_ptr<T_E const>>> get_phi_varset() const {
    return m_phi_varset;
  }
  
  static std::function<ssa_reaching_defs_t(T_PC const &)>
  get_boundary_value(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g)
  {
    auto f = [g](T_PC const &p)
    {
      
      ASSERT(p.is_start());
      dshared_ptr<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const> gp = dynamic_pointer_cast<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const>(g);

      context* ctx = gp->get_context();
      // add input args
      ssa_reaching_defs_t retval;
      shared_ptr<T_E const> start_edge = gp->get_dummy_edge_from_to_pc(T_PC::start());
      //tfg_edge_ref start_edge = mk_tfg_edge(pc::start(), pc::start(), expr_true(ctx), state(), unordered_set<expr_ref>(), te_comment_t(false, 0,"start-edge"));
      state init_state = gp->get_start_state().get_state();
      for (auto &kv : init_state.get_value_expr_map_ref()) {
        retval.add_reaching_definition(kv.first, start_edge, start_edge);
      }
      graph_arg_regs_t const &arg_regs = gp->get_argument_regs();
      for (auto const& arg : arg_regs.get_map()) {
        //if (arg.second.get_val()->is_var() && (arg.second.get_val()->is_bv_sort() || arg.second.get_val()->is_bool_sort())) 
        if (arg.second.get_val()->is_var()) {
          //retval.insert(arg.second->get_name());
          //retval.add_reaching_definition(arg.second.get_val(), start_edge);
          string_ref vname = ctx->get_key_from_input_expr(arg.second.get_val());
          retval.add_reaching_definition(vname, start_edge, start_edge);
        }
      }

//      expr_ref mem = gp->get_memvar_version_at_pc(T_PC::start());
//      ASSERT(mem->is_var());
//      string_ref memname = ctx->get_key_from_input_expr(mem);
//      retval.add_reaching_definition(memname, start_edge, start_edge);
//
//      expr_ref mem_alloc = gp->get_memallocvar_version_at_pc(T_PC::start());
//      ASSERT(mem_alloc->is_var());
//      string_ref mem_allocname = ctx->get_key_from_input_expr(mem_alloc);
//      retval.add_reaching_definition(mem_allocname, start_edge, start_edge);

      return retval;
    };
    return f;
  }
};


template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_ssa_funcs : public graph_with_guessing<T_PC, T_N, T_E, T_PRED>
{
public:

  graph_with_ssa_funcs(string const &name, string const& fname, context* ctx) : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(name, fname, ctx)
  {
//    this->populate_dominator_and_postdominator_relations();
//    this->populate_varname_modification_edges();
//    this->populate_varname_default_expr_map(); 
//    this->populate_var_liveness();
//    this->populate_vars_reaching_definitions();
  }

  graph_with_ssa_funcs(graph_with_ssa_funcs const &other) : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(other),
                                                              m_varname_modification_edges_map(other.m_varname_modification_edges_map)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_ssa_funcs(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx) : graph_with_guessing<T_PC, T_N, T_E, T_PRED>(in, name, read_edge_fn, ctx)
  {
//    this->populate_dominator_and_postdominator_relations();
//    this->populate_varname_modification_edges();
//    this->populate_varname_default_expr_map(); 
//    this->populate_var_liveness();
//    this->populate_vars_reaching_definitions();
  }


  bool varname_has_modification_edges(string_ref const& v) const
  {
    return m_varname_modification_edges_map.count(v);
  }

  list<dshared_ptr<tfg_edge const>> const& get_modification_edges_for_varname(string_ref const& varname) const
  {
    if(!m_varname_modification_edges_map.count(varname))
      cout << __func__ << " " << __LINE__ << ": varname = " << varname->get_str() << endl;

    ASSERT(m_varname_modification_edges_map.count(varname));
    return m_varname_modification_edges_map.at(varname);
  }

  map<string_ref, list<dshared_ptr<T_E const>>> const& get_varname_modification_edges_map() const
  {
    return m_varname_modification_edges_map;
  }

  map<string_ref, list<dshared_ptr<T_E const>>> recompte_and_get_varname_modification_edges_map() const
  {
    map<string_ref, list<dshared_ptr<T_E const>>> ret_varname_modification_edges_map;
    compute_varname_modification_edges(ret_varname_modification_edges_map);
    return ret_varname_modification_edges_map;
  }

  virtual shared_ptr<T_E const> get_dummy_edge_from_to_pc(T_PC const& p) const = 0;

  map<T_PC, ssa_reaching_defs_t<T_PC, T_N, T_E, T_PRED>> compute_vars_reaching_definitions() const;
  map<T_PC, map<expr_ref, string>> compute_ssa_vars_renaming_map() const;
  expr_ref get_default_input_expr_for_varname(string_ref const& v) const;
  void populate_var_liveness();
  dshared_ptr<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const> graph_with_ssa_funcs_shared_from_this() const
  {
    dshared_ptr<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const> ret = dynamic_pointer_cast<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED> const>(this->shared_from_this());
    ASSERT(ret);
    return ret;
  }
    

//  virtual dshared_ptr<graph_with_ssa_funcs<T_PC, T_N, T_E, T_PRED>> construct_ssa_graph(map<graph_loc_id_t, graph_cp_location> const &llvm_locs = {}) const = 0;
  void populate_varname_modification_edges()
  {
    compute_varname_modification_edges(m_varname_modification_edges_map);
  }
  
  bool is_empty_var_liveness_map_at_pc(T_PC const& p) const 
  { 
    return !m_var_liveness_map.count(p); 
  }

  set<string_ref> const& get_var_liveness_map_at_pc(T_PC const& p) const 
  { 
    ASSERT(m_var_liveness_map.count(p));
    return m_var_liveness_map.at(p);
  }

  
private:
  void compute_varname_modification_edges(map<string_ref, list<dshared_ptr<T_E const>>> &varname_modification_edges_map) const
  {
    varname_modification_edges_map.clear();
    context* ctx = this->get_context();
    //context* ctx = this->get_context();
    shared_ptr<T_E const> start_edge = this->get_dummy_edge_from_to_pc(T_PC::start());
    //tfg_edge_ref start_edge = mk_tfg_edge(pc::start(), pc::start(), expr_true(ctx), state(), unordered_set<expr_ref>(), te_comment_t(false, 0,"start-edge"));
    // add input args
    graph_arg_regs_t const &arg_regs = this->get_argument_regs();
    for (auto const& arg : arg_regs.get_map()) {
      //if (arg.second.get_val()->is_var() && (arg.second.get_val()->is_bv_sort() || arg.second.get_val()->is_bool_sort())) 
      if (arg.second.get_val()->is_var()) {
        //retval.insert(arg.second->get_name());
        //retval.add_reaching_definition(arg.second.get_val(), start_edge);
        string_ref vname = ctx->get_key_from_input_expr(arg.second.get_val());
        varname_modification_edges_map[vname].push_back(start_edge);
      }
    }

    for (auto const& ve : this->get_start_state().get_state().get_value_expr_map_ref()) {
      //cout << _FNLN_ << ": adding modification edges for " << ve.first->get_str() << endl;
      varname_modification_edges_map[ve.first].push_back(start_edge);
    }
    for (auto const& e : this->get_edges()) {
      state const& to_state = e->get_to_state();
      for (auto const& ve : to_state.get_value_expr_map_ref()) {
        //cout << _FNLN_ << ": adding modification edges for " << ve.first->get_str() << endl;
        varname_modification_edges_map[ve.first].push_back(e);
      }
    }
  }

  map<string_ref, list<dshared_ptr<T_E const>>> m_varname_modification_edges_map;
  map<T_PC, set<string_ref>> m_var_liveness_map;
};

}
