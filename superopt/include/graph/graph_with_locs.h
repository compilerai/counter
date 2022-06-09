#pragma once

#include <map>
#include <list>
#include <string>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/sp_version.h"
#include "expr/relevant_memlabels.h"

#include "gsupport/sprel_map.h"
#include "gsupport/memlabel_assertions.h"

#include "graph/graph_loc_id.h"
#include "graph/graph_with_precondition.h"
#include "graph/locset.h"
#include "graph/graph_locs_map.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class alias_val_t;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_locs : public graph_with_precondition<T_PC, T_N, T_E, T_PRED>
{
public:
  graph_with_locs(string const &name, string const& fname, context* ctx) : graph_with_precondition<T_PC,T_N,T_E,T_PRED>(name, fname, ctx), m_graph_locs_map(make_dshared<graph_locs_map_t>(map<graph_loc_id_t, graph_cp_location>()))
  { }

  graph_with_locs(graph_with_locs const &other) : graph_with_precondition<T_PC,T_N,T_E,T_PRED>(other),
                                                  m_start_loc_id(other.m_start_loc_id),
                                                  m_avail_exprs(other.m_avail_exprs),
                                                  m_graph_locs_map(other.m_graph_locs_map),
//                                                  m_orig_locid_to_cloned_locid_map(other.m_orig_locid_to_cloned_locid_map),
                                                  //m_locs(other.m_locs),
                                                  //m_locid2expr_map(other.m_locid2expr_map),
                                                  //m_exprid2locid_map(other.m_exprid2locid_map),
                                                  m_loc_liveness(other.m_loc_liveness),
                                                  m_loc_definedness(other.m_loc_definedness),
                                                  m_var_definedness(other.m_var_definedness),
                                                  m_bavlocs(other.m_bavlocs),
                                                  m_sprels(other.m_sprels),
                                                  m_relevant_memlabels(other.m_relevant_memlabels)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_locs const& other)
  {
    graph_with_precondition<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    m_relevant_memlabels = other.m_relevant_memlabels;
    m_start_loc_id = other.m_start_loc_id;
//    ASSERT(m_orig_locid_to_cloned_locid_map.empty());
  }

  graph_with_locs(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);
  virtual ~graph_with_locs() = default;

  //void print_graph_locs() const
  //{
  //  cout << "Printing graph locs (" << get_num_graph_locs() << " total)\n";
  //  cout << this->graph_locs_to_string(*m_locs);
  //  cout << "done printing locs" << endl;
  //}

  //expr_ref const& graph_loc_get_value_for_node(graph_loc_id_t loc_index) const { return m_locid2expr_map.at(loc_index); }
  expr_ref const& graph_loc_get_value_for_node(graph_loc_id_t loc_index) const { return m_graph_locs_map->graph_locs_map_get_expr_at_loc_id(loc_index); }

  void set_locs(map<graph_loc_id_t, graph_cp_location> const &locs);

  dshared_ptr<graph_locs_map_t const> const& get_graph_locs_map() const { return m_graph_locs_map; }
  virtual map<graph_loc_id_t, graph_cp_location> get_locs() const { return m_graph_locs_map->graph_locs_map_get_locs(); }
  virtual map<graph_loc_id_t, graph_cp_location> get_locs_at_pc(T_PC const& p) const { 
    map<graph_loc_id_t, graph_cp_location> ret_locs;
    for(auto const& l: this->get_locs()) {
      if(this->loc_is_defined_at_pc(p, l.first))
        ret_locs.insert(l);
    }
    return ret_locs;
  }

  map<graph_loc_id_t, graph_cp_location> get_locs_with_memvars_at_pc(T_PC const& p) const {
    map<graph_loc_id_t, graph_cp_location> ret_locs;
    expr_ref memvar = this->get_memvar_version_at_pc(p);
    expr_ref mem_allocvar = this->get_memallocvar_version_at_pc(p);
    for(auto const& l: this->get_locs()) {
      if((l.second.is_memslot() || l.second.is_memmask()) && (l.second.m_memvar != memvar || l.second.m_mem_allocvar != mem_allocvar))
        continue;
      ret_locs.insert(l);
    }
    return ret_locs;
  }

  //map<graph_loc_id_t, graph_cp_location>& get_locs_ptr() { return *m_locs; }
  //graph_cp_location const &get_loc(graph_loc_id_t loc_id) const { return m_locs->at(loc_id); }
  bool has_loc(graph_loc_id_t loc_id) const { ASSERT(loc_id != GRAPH_LOC_ID_INVALID); return this->get_locs().count(loc_id);}
  virtual graph_cp_location const &get_loc(graph_loc_id_t loc_id) const { ASSERT(this->get_locs().count(loc_id)); return m_graph_locs_map->graph_locs_map_get_locs().at(loc_id); }

  void get_all_loc_ids(set<graph_loc_id_t> &out) const
  {
    for (auto const& loc : this->get_locs()) {
      out.insert(loc.first);
    }
  }

  expr_ref get_loc_expr(graph_loc_id_t loc_id) const
  {
    graph_cp_location l;

    map<graph_loc_id_t, graph_cp_location> const &graph_locs = this->get_locs();
    ASSERT(graph_locs.count(loc_id) > 0);
    l = graph_locs.at(loc_id);
    return l.graph_cp_location_get_value();
  }

  //static graph_loc_id_t get_max_loc_id(map<graph_loc_id_t, graph_cp_location> const &old_locs, map<graph_loc_id_t, graph_cp_location> const &new_locs);

  //graph_loc_id_t get_loc_id_for_newloc(graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  //graph_loc_id_t graph_add_new_loc(map<graph_loc_id_t, graph_cp_location> &locs_map, graph_cp_location const &newloc, map<graph_loc_id_t, graph_cp_location> const &old_locs, graph_loc_id_t suggested_loc_id = -1) const;

  //void graph_locs_add_location_memslot(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, expr_ref>& locid2expr_map, map<expr_id_t, graph_loc_id_t>& exprid2locid_map, expr_ref const &memvar, expr_ref const& mem_allocvar, expr_ref addr, int nbytes, bool bigendian, memlabel_t const& ml) const;

  void graph_locs_add_location_llvmvar(map<graph_loc_id_t, graph_cp_location> &new_locs, string const& varname, sort_ref const &sort, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  void graph_locs_add_location_reg(map<graph_loc_id_t, graph_cp_location> &new_locs, string const& name, sort_ref const &sort, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  void graph_locs_add_all_llvmvars(map<graph_loc_id_t, graph_cp_location> &new_locs/*, map<graph_loc_id_t, graph_cp_location> const &old_locs*/) const;
  void graph_locs_add_llvmvars_for_edge(dshared_ptr<T_E const> const& edge, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  void graph_locs_add_local_vars_for_edge(dshared_ptr<T_E const> const& edge, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;
  void graph_locs_add_all_local_vars(map<graph_loc_id_t, graph_cp_location> &new_locs) const;

  void graph_locs_add_rounding_modes_for_edge(dshared_ptr<T_E const> const& edge, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;
  void graph_locs_add_all_rounding_modes(map<graph_loc_id_t, graph_cp_location> &new_locs) const;

  void graph_locs_add_bools_bitvectors_and_floats_for_edge(dshared_ptr<T_E const> const& edge, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;
  void graph_locs_add_all_bools_bitvectors_and_floats(map<graph_loc_id_t, graph_cp_location> &new_locs/*, map<graph_loc_id_t, graph_cp_location> const &old_locs*/) const;
  void graph_locs_add_memmasks_for_pc(T_PC const& p, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  graph_cp_location graph_mk_memslot_loc(expr_ref const& memvar/*string const &memname*/,expr_ref const& mem_allocvar, memlabel_ref const& ml, expr_ref addr, size_t nbytes, bool bigendian) const;

  void graph_locs_add_all_exvregs(map<graph_loc_id_t, graph_cp_location> &new_locs/*, map<graph_loc_id_t, graph_cp_location> const &old_locs*/) const;
  void graph_locs_add_exvregs_for_edge(dshared_ptr<T_E const> const& edge, map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;

  void graph_locs_add_location_memmasked(map<graph_loc_id_t, graph_cp_location> &new_locs, expr_ref const& memvar/*string const &memname*/, expr_ref const& mem_allocvar, memlabel_ref const& ml, expr_ref addr, size_t memsize, map<graph_loc_id_t, graph_cp_location> const &old_locs) const;


  string graph_locs_to_string(map<graph_loc_id_t, graph_cp_location> const &graph_locs) const;
  void graph_add_location_slots_and_memmasked_using_llvm_locs(map<graph_loc_id_t, graph_cp_location> &new_locs, map<graph_loc_id_t, graph_cp_location> const &llvm_locs) const;

  void graph_locs_remove_duplicates_mod_comments_for_pc(map<graph_loc_id_t, graph_cp_location> const &graph_locs, map<graph_loc_id_t, bool> &is_redundant) const;

  void graph_locs_remove_duplicates_mod_comments(map<graph_loc_id_t, graph_cp_location> &locs_map) const;

  //void set_locid2expr_maps(map<graph_loc_id_t, expr_ref> const& locid2expr_map, map<expr_id_t, graph_loc_id_t> const& exprid2locid_map)
  //{
  //  m_locid2expr_map = locid2expr_map;
  //  m_exprid2locid_map = exprid2locid_map;
  //}

  //void populate_locid2expr_map();

  void populate_loc_liveness();
  virtual void populate_loc_and_var_definedness(/*dshared_ptr<T_E const> const& e*/) = 0;
  virtual void populate_branch_affecting_locs() = 0;

  protected:

  //bool has_avail_exprs_at_pc(T_PC const &p) const
  //{
  //  return this->m_avail_exprs.count(p) != 0;
  //}

  //avail_exprs_t const &get_avail_exprs/*_at_pc*/(/*T_PC const &p*/) const
  //{
  //  //ASSERT(this->m_avail_exprs.count(p));
  //  //return this->m_avail_exprs.at(p);
  //  return this->m_avail_exprs;
  //}

  avail_exprs_t const &get_avail_exprs() const
  {
    return this->m_avail_exprs;
  }
  expr_ref expr_simplify_using_aliasing_info(expr_ref const& e/*, T_PC const& p*/) const;

  bool loc_has_sprel_mapping/*_at_pc*/(/*T_PC const &p, */graph_loc_id_t loc_id) const { return m_sprels/*.at(p)*/.has_mapping_for_loc(loc_id); }
  map<graph_loc_id_t, graph_cp_location> get_ghost_locs(T_PC const& p) const
  {
    map<graph_loc_id_t, graph_cp_location> ret;
    context *ctx = this->get_context();

    for (auto const& loc : this->get_locs()) {
      if (loc.second.graph_cp_location_represents_ghost_var(ctx) && this->loc_is_defined_at_pc(p,loc.first))
        ret.insert(loc);
    }
    return ret;
  }
  virtual set<string_ref> get_var_definedness_at_pc(T_PC const& p) const { ASSERT(m_var_definedness.count(p)); return m_var_definedness.at(p); }
  map<T_PC, set<graph_loc_id_t>> const &get_loc_definedness() const { return m_loc_definedness; }
  void graph_init_sprels() const
  {
    this->m_sprels = sprel_map_t(/*this->get_locid2expr_map(), this->get_consts_struct()*/);
    //for (const auto &p : this->get_all_pcs()) {
    //  if (!this->m_sprels.count(p)) {
    //    this->m_sprels.insert(make_pair(p, sprel_map_t(this->get_locid2expr_map(), this->get_consts_struct())));
    //  }
    //}
  }

  //map<T_PC, sprel_map_t> const &get_sprels() const { return m_sprels; }

  sprel_map_t const &get_sprel_map(/*T_PC const &p*/) const { return m_sprels/*.at(p)*/; }

  void set_sprel_map/*_at_pc*/(/*T_PC const &p, */sprel_map_t const& s)
  {
    //ASSERT(!m_sprels.count(p));
    //m_sprels.insert(make_pair(p, s));
    m_sprels = s;
  }
  void add_sprel_mapping(/*T_PC const &p, */graph_loc_id_t loc_id, expr_ref const &e)
  {
    //if (!m_sprels.count(p)) {
    //  m_sprels.insert(make_pair(p, sprel_map_t(this->get_locid2expr_map(), this->get_consts_struct())));
    //}
    m_sprels/*.at(p)*/.sprel_map_add_constant_mapping(loc_id, e, this->get_locid2expr_map());
  }
  set<expr_ref> get_ghost_loc_exprs(T_PC const& p) const;
//  set<expr_ref> get_spreled_loc_exprs_at_pc(T_PC const& p) const;

  void set_avail_exprs(/*T_PC const& p, */avail_exprs_t const& avail_exprs)
  {
    //ASSERT(!m_avail_exprs.count(p));
    //this->m_avail_exprs.insert(make_pair(p, avail_exprs));
    this->m_avail_exprs = avail_exprs;
  }

  void set_remaining_sprel_mappings_to_bottom(/*T_PC const &p*/)
  {
    //if (!m_sprels.count(p)) {
    //  m_sprels.insert(make_pair(p, sprel_map_t(this->get_locid2expr_map(), this->get_consts_struct())));
    //}
    m_sprels/*.at(p)*/.sprel_map_set_remaining_mappings_to_bottom(this->get_locs());
  }

  virtual void graph_to_stream(ostream& ss, string const& prefix="") const override;
  string read_locs(istream& in, string line);

  void set_loc_liveness(map<T_PC, set<graph_loc_id_t>> const& loc_liveness) { m_loc_liveness = loc_liveness; }
  //void set_var_definedness_at_pc(T_PC const& p, set<string_ref> const& var_definedness) {
  //  m_var_definedness.at(p) = var_definedness;
  //}
  void set_var_definedness(map<T_PC, set<string_ref>> const& var_definedness) {
    m_var_definedness = var_definedness;
  }

  void set_loc_definedness_and_populate_var_definedness(map<T_PC, set<graph_loc_id_t>> const& loc_definedness) {
    m_loc_definedness = loc_definedness;
    this->populate_var_definedness();
  }
  void set_bavlocs(map<T_PC, set<graph_loc_id_t>> const& bavlocs) { m_bavlocs = bavlocs; }
  //virtual bool populate_auxilliary_structures_dependent_on_locs() = 0;
  //virtual void graph_init_memlabels_to_top(bool update_callee_memlabels) = 0;
  void print_loc_avail_expr(/*map<T_PC,*/avail_exprs_t/*>*/ const& avail_exprs, string const& prefix = "") const;

  void graph_locs_to_stream(ostream& ss) const;
  graph_loc_id_t get_locid_for_varname(string_ref const& varname) const { return m_graph_locs_map->graph_locs_map_get_locid_for_varname(varname); }
  graph_loc_id_t get_locs_map_start_loc_id() const
  {
    return m_start_loc_id;
  }
  //bool graph_prune_avail_exprs_using_definedness()
  //{
  //  bool changed = false;
  //  /*map<T_PC, */avail_exprs_t/*>*/ new_avail_exprs;
  //  /*for (auto const& m : m_avail_exprs) */{
  //    //T_PC const& p = m.first;
  //    map<graph_loc_id_t, avail_exprs_val_t> pruned_avail_exprs;
  //    for (auto const& l : m_avail_exprs.avail_exprs_get_loc_map()) {
  //      if (this->loc_is_defined_at_pc(p, l.first))
  //          pruned_avail_exprs.insert(l);
  //      else
  //        changed = true;
  //    }
  //    //new_avail_exprs.insert(make_pair(p, avail_exprs_t(pruned_avail_exprs)));
  //    new_avail_exprs = avail_exprs_t(pruned_avail_exprs);
  //  }
  //  m_avail_exprs = new_avail_exprs;
  //  return changed;
  //}


  public:

  bool loc_is_defined_at_pc(T_PC const &p, graph_loc_id_t loc_id) const { return m_loc_definedness.at(p).count(loc_id) != 0; }

  /* for CG we may want union of both src and dst */
  virtual sprel_map_pair_t get_sprel_map_pair(/*T_PC const &p*/) const
  {
    sprel_map_t const &sprel_map = get_sprel_map(/*p*/);
    return sprel_map_pair_t(sprel_map);
  }

  set<graph_loc_id_t> const& get_branch_affecting_locs_at_pc(T_PC const& pp) const
  {
    if (!m_bavlocs.count(pp)) {
      static set<graph_loc_id_t> empty;
      return empty;
    }
    return m_bavlocs.at(pp);
  }

//  expr_ref graph_loc_get_value(T_PC const &p, state const &s, graph_loc_id_t loc_index) const;

  size_t get_num_graph_locs() const { return this->get_locs().size(); }

  static bool sprels_are_different(sprel_map_t const &sprels1, sprel_map_t const &sprels2);

  virtual sprel_map_t compute_sprel_relations() const { NOT_REACHED(); }

  void print_sp_version_relations(/*map<T_PC, */sprel_map_t/*>*/ const &sprel_map, consts_struct_t const &cs) const
  {
    cout << "Printing sp relations" << endl;
    /*for (auto iter = sprel_map.cbegin(); iter != sprel_map.cend(); iter++) */{
      //T_PC p = iter->first;
      stringstream ss;
      string s;
      //ss << "pc " << p.to_string();
      //s = ss.str();
      sprel_map_t const &sprel = sprel_map; //iter->second;
      string sprel_status_string;
      sprel_status_string = sprel.to_string(this->get_locs()/*.at(p)*/, cs, s);
      if (sprel_status_string != "") {
        cout << sprel_status_string << endl;
      }
    }
    cout << "done Printing sp relations" << endl;
  }

  //virtual void populate_avail_exprs()
  //{
  //  m_avail_exprs = this->compute_loc_avail_exprs();
  //}

  bool propagate_sprels();

  // populate gen and kill sets
  //virtual void populate_gen_and_kill_sets_for_edge(shared_ptr<T_E const> const& e, map<graph_loc_id_t, graph_cp_location> const& locs, alias_val_t<T_PC, T_N, T_E, T_PRED> const& alias_val, map<graph_loc_id_t, expr_ref>& gen_set, set<graph_loc_id_t>& killed_locs) const
  //{ NOT_REACHED(); }
  // use existing available exprs to simplify the exprs
  void substitute_gen_set_exprs_using_avail_exprs(map<graph_loc_id_t, expr_ref> &gen_set/*, map<expr_ref, set<graph_loc_id_t>> &expr2deps_cache*/, avail_exprs_t const& this_avail_exprs) const;



  //void init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs);

  set<graph_loc_id_t> const &get_live_locids(T_PC const &p) const
  {
    static set<graph_loc_id_t> empty_set = set<graph_loc_id_t>();
    if (m_loc_liveness.count(p) == 0) {
      return empty_set;
    }
    return m_loc_liveness.at(p);
  }

  map<graph_loc_id_t, graph_cp_location> get_live_locs(T_PC const &p) const
  {
    map<graph_loc_id_t, graph_cp_location> ret;
    if (m_loc_liveness.count(p) == 0) {
      return ret;
    }

    for (auto const& loc : this->get_locs()) {
      if (loc_is_live_at_pc(p, loc.first))
        ret.insert(loc);
    }
    return ret;
  }

  void add_live_locs(T_PC const &p, set<graph_loc_id_t> const &locs, graph_loc_id_t start_loc_offset = 0)
  {
    for (auto l : locs) {
      graph_loc_id_t el =  l + start_loc_offset;
      if (this->get_locs().count(el)) {
        m_loc_liveness[p].insert(el);
      }
    }
  }

  void add_live_loc(T_PC const &p, graph_loc_id_t loc)
  {
    m_loc_liveness[p].insert(loc);
  }

  void print_loc_liveness() const
  {
    cout << __func__ << " " << __LINE__ << ": Printing loc liveness:\n";
    for (const auto &pls : m_loc_liveness) {
      T_PC const &p = pls.first;
      cout << p.to_string() << ": " << loc_set_to_string(pls.second) << endl;
    }
  }

  //void print_loc_definedness() const
  //{
  //  cout << __func__ << " " << __LINE__ << ": TFG:\n" << this->graph_to_string(/*true*/) << endl;
  //  cout << __func__ << " " << __LINE__ << ": Printing loc definedness:\n";
  //  for (const auto &pls : m_loc_definedness) {
  //    T_PC const &p = pls.first;
  //    cout << p.to_string() << ": " << loc_set_to_string(pls.second) << endl;
  //  }
  //}

  static string loc_set_to_expr_string(graph_with_locs<T_PC,T_N,T_E,T_PRED> const& g, set<graph_loc_id_t> const &loc_set)
  {
    stringstream ss;
    bool first = true;
    for (auto loc_id : loc_set) {
      if (!first) {
        ss << ", ";
      }
      ss << expr_string(g.graph_loc_get_value_for_node(loc_id));
      first = false;
    }
    return ss.str();
  }

  void print_branch_affecting_locs() const
  {
    cout << __func__ << " " << __LINE__ << ": Printing branch affecting vars:\n";
    for (const auto &pls : m_bavlocs) {
      T_PC const &p = pls.first;
      cout << p.to_string() << ": " << loc_set_to_string(pls.second) << "[ " << loc_set_to_expr_string(*this, pls.second) << " ]" << endl;
    }
  }

  void clear_loc_liveness() { m_loc_liveness.clear(); }
  map<T_PC, set<graph_loc_id_t>> const &get_loc_liveness() const { return m_loc_liveness; }

  bool loc_is_live_at_pc(T_PC const &p, graph_loc_id_t loc_id) const { return m_loc_liveness.at(p).count(loc_id) != 0; }

//  void add_defined_locs(T_PC const &p, set<graph_loc_id_t> const &locs)
//  {
//    for (auto l : locs) {
//      m_loc_definedness[p].insert(l);
//    }
//  }

  void add_bav_locs(T_PC const &p, set<graph_loc_id_t> const &locs)
  {
    for (auto l : locs) {
      m_bavlocs[p].insert(l);
    }
  }



  map<graph_loc_id_t, expr_ref> const &get_locid2expr_map() const { return m_graph_locs_map->graph_locs_map_get_locid2expr_map(); }
  //map<graph_loc_id_t, expr_ref>& get_locid2expr_map_ref() { return m_graph_locs_map->graph_locs_map_get_locid2expr_map_ref(); }

//  map<graph_loc_id_t, graph_cp_location> graph_populate_locs_for_cloned_edge(dshared_ptr<T_E const> const& new_e1, T_PC const& orig_pc);
//  void graph_populate_avail_exprs_and_sprels_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
//  void graph_populate_loc_livenes_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
//  void graph_populate_loc_definedness_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);

  graph_loc_id_t get_loc_id_for_masked_mem_expr(expr_ref const &e) const
  {
    for (auto const& loc : this->get_locs()) {
      if (loc.second.m_type != GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
        continue;
      }
      //expr_ref le = this->get_loc_expr(loc.first);
      expr_ref le = this->graph_loc_get_value_for_node(loc.first);
      //cout << __func__ << " " << __LINE__ << ": le = " << expr_string(le) << endl;
      if (e == le) {
        return loc.first;
      }
    }
    cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    NOT_REACHED();
  }

  set<graph_loc_id_t> get_argument_regs_locids() const
  {
    set<graph_loc_id_t> ret;
    set<expr_ref> arg_reg_exprs;
    for (auto const& pse : this->get_argument_regs().get_map()) {
      arg_reg_exprs.insert(pse.second.get_val());
      arg_reg_exprs.insert(pse.second.get_addr());
    }
    for (auto const& loc : this->get_locs()) {
      expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
      if (arg_reg_exprs.count(loc_expr)) {
        ret.insert(loc.first);
      }
    }
    return ret;
  }

  set<graph_loc_id_t> get_return_regs_locids() const
  {
    set<graph_loc_id_t> ret;
    set<expr_ref> ret_reg_exprs;
    for (auto const& p_pse : this->get_return_regs()) {
      for (auto const& pse : p_pse.second) {
        if (pse.second->get_operation_kind() == expr::OP_SELECT) {
          // XXX special handling of select-on-symbol
          // add the memmask instead of select since loc is formed for memmask only
          ret_reg_exprs.insert(pse.second->get_args().at(OP_SELECT_ARGNUM_MEM));
        }
        ret_reg_exprs.insert(pse.second);
      }
    }
    for (auto const& loc : this->get_locs()) {
      expr_ref loc_expr = this->graph_loc_get_value_for_node(loc.first);
      if (ret_reg_exprs.count(loc_expr)) {
        ret.insert(loc.first);
      }
    }
    return ret;
  }

  virtual set<graph_loc_id_t> graph_get_live_locids_at_boundary_pc(T_PC const& p) const
  {
    // init_loc_liveness_to_retlive
    ASSERT(p.is_exit());
    set<graph_loc_id_t> ret;
    map<string_ref, expr_ref> const& ret_regs = this->get_return_regs_at_pc(p);
    for (auto const& [_,reg_expr] : ret_regs) {
      set<graph_loc_id_t> read = this->get_locs_potentially_read_in_expr(reg_expr);
      DYN_DEBUG2(compute_liveness, cout << "loc expr: " << expr_string(reg_expr) << ".  locids potentially read:";
                                   for (auto const& locid : read) cout << ' ' << locid; cout << endl);
      set_union(ret, read);
    }
    return ret;
  }

  virtual set<graph_loc_id_t> graph_get_defined_locids_at_entry(T_PC const& entry_pc) const
  {
    context* ctx = this->get_context();
    // add input args
    set<graph_loc_id_t> ret = this->get_argument_regs_locids();
    expr_ref memvar = this->get_memvar_version_at_pc(entry_pc);
    expr_ref mem_allocvar = this->get_memallocvar_version_at_pc(entry_pc);
    map<graph_loc_id_t, graph_cp_location> const &locs = this->get_locs();
    for (auto const& l : locs) {
      if (  (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT
             || l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED)
          && l.second.m_memvar == memvar && l.second.m_mem_allocvar == mem_allocvar) {
        auto const& ml = l.second.m_memlabel->get_ml();
        if (   !ml.memlabel_is_stack()
            && (   !ml.memlabel_is_local()
                || (   ml.memlabel_get_local_id().allocstack_get_last_fname() == this->get_function_name()
                    && local_id_refers_to_arg(ml.memlabel_get_local_id().allocstack_get_last_allocsite(), this->get_argument_regs(), this->get_locals_map())))) {
          // only locals corresponding to args are defined at entry
          ret.insert(l.first);
        }
      }
    }
    // add start state regs
    // In assembly TFG, there can be read only registers. These registers should be defined at the pCs where they are used
    // Using start state for adding definedness for all such registers
    for(auto const& m : this->get_start_state().get_state().get_value_expr_map_ref()) {
      auto const& name =  m.first;
      expr_ref var_expr = ctx->get_input_expr_for_key(name, m.second->get_sort());
      graph_loc_id_t l = this->graph_expr_to_locid(var_expr);
      if(l != GRAPH_LOC_ID_INVALID) {
        ret.insert(l);
      }
    }
    // add regs live at exit minus the ret-reg
    //set_union(ret, gp->get_return_regs_locids());
    //graph_loc_id_t ret_reg_locid = gp->get_ret_reg_locid();
    //if (ret_reg_locid != -1)
    //  ret.erase(ret_reg_locid);
    return ret;
  }

  graph_loc_id_t get_ret_reg_locid() const
  {
    for (auto const& loc : this->get_locs()) {
      if (   loc.second.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM
          && loc.second.to_string().find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
        return loc.first;
      }
    }
    return -1;
  }

  static void expand_locset_to_include_all_memmasks(set<graph_loc_id_t> &locset, map<graph_loc_id_t, graph_cp_location> const& locs)
  {
    for (const auto &loc : locs) {
      if (loc.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
        locset.insert(loc.first);
      }
    }
  }

  static void relevant_memlabels_add(vector<memlabel_ref> &relevant_memlabels, memlabel_ref const &ml_input)
  {
    bool ml_input_exists = false;
    for (auto ml : relevant_memlabels) {
      if (ml == ml_input) {
        ml_input_exists = true;
      }
    }
    if (!ml_input_exists) {
      relevant_memlabels.push_back(ml_input);
    }
  }

  static void expand_locset_to_include_slots_for_memmask(set<graph_loc_id_t> &locset, map<graph_loc_id_t, graph_cp_location> const& locs);

  set<graph_loc_id_t> get_locs_potentially_read_in_expr(expr_ref const &e) const;

  static bool expr_indicates_definite_write(graph_loc_id_t loc_id, expr_ref const &new_expr, map<graph_loc_id_t, graph_cp_location> const& locs)
  {
    if (locs.at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
      return false;
    }
    if (locs.at(loc_id).m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      expr_find_op expr_stores_visitor(new_expr, expr::OP_STORE);
      expr_vector expr_stores = expr_stores_visitor.get_matched_expr();
      expr_find_op expr_uninit_stores_visitor(new_expr, expr::OP_STORE_UNINIT);
      expr_vector expr_uninit_stores = expr_uninit_stores_visitor.get_matched_expr();
      return expr_stores.empty() && expr_uninit_stores.empty(); // can make this even more precise by looking at target addr of store; if the target addr is different from slot's addr then return true
      //return false; //can make this more precise by looking at new_expr; e.g., if new_expr does not contain a store, we can return true
    }
    return true;
  }


  static map<graph_loc_id_t, expr_ref> compute_locs_definitely_written_on_edge(map<graph_loc_id_t, expr_ref> const& potentially_written, map<graph_loc_id_t, graph_cp_location> const& locs)
  {
    stopwatch_run(&compute_locs_definitely_written_on_edge_timer);
    //autostop_timer func(__func__);
    //this function only needs to be over-approximate. i.e., the returned locs must be definitely written-to;
    //but perhaps some more locs (that are not returned) are also written to
    //map<graph_loc_id_t, expr_ref> all = this->get_locs_potentially_written_on_edge(e);
    map<graph_loc_id_t, expr_ref> ret;
    for (const auto &le : potentially_written) {
      if (expr_indicates_definite_write(le.first, le.second, locs)) {
        ret.insert(le);
      }
    }
    stopwatch_stop(&compute_locs_definitely_written_on_edge_timer);
    return ret;
  }


  //set<graph_loc_id_t> graph_get_locs_potentially_belonging_to_memlabel(memlabel_t const &ml)
  //{
  //  set<graph_loc_id_t> ret;
  //  for (const auto &l : *this->m_locs) {
  //    if (   (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED
  //            && (ml.memlabel_is_top() || memlabel_contains(&ml, &l.second.m_memlabel->get_ml())))
  //        || (   l.second.m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT
  //            && (ml.memlabel_is_top() || memlabel_contains(&ml, &l.second.m_memlabel->get_ml())))) {
  //      ret.insert(l.first);
  //    }
  //  }
  //  return ret;
  //}

  map<expr_id_t, graph_loc_id_t> const& graph_get_exprid2locid_map() const {
    return m_graph_locs_map->graph_locs_map_get_exprid2locid_map();
  }
  //map<expr_id_t, graph_loc_id_t>& get_exprid2locid_map_ref() {
  //  return m_graph_locs_map->graph_locs_map_get_exprid2locid_map_ref();
  //}

  virtual graph_loc_id_t graph_expr_to_locid(expr_ref const &e) const
  {
    if (this->graph_get_exprid2locid_map().count(e->get_id())) {
      return this->graph_get_exprid2locid_map().at(e->get_id());
    } else {
      return GRAPH_LOC_ID_INVALID;
    }
  }

  virtual set<expr_ref> get_live_vars(T_PC const& pp) const
  {
    set<expr_ref> ret;
    for (auto const& loc : this->get_live_locs(pp)) {
      if (loc.second.m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
        ret.insert(this->get_loc_expr(loc.first));
      }
    }
    return ret;
  }

  set<expr_ref> get_live_loc_exprs_at_pc(T_PC const& p) const;
  //set<expr_ref> get_live_loc_exprs_having_sprel_map_at_pc(T_PC const &p) const;
  //map<graph_loc_id_t, graph_cp_location> get_live_locs_across_pcs(T_PC const& from_pc, T_PC const& to_pc) const;
  set<graph_loc_id_t> get_locs_potentially_read_in_expr_using_locs_map(expr_ref const &e, map<graph_loc_id_t, graph_cp_location> const &locs, map<expr_id_t, graph_loc_id_t> const& exprid2locid_map) const;
  relevant_memlabels_t const& graph_get_relevant_memlabels() const { ASSERT(m_relevant_memlabels); return *m_relevant_memlabels; }
  void graph_set_relevant_memlabels(relevant_memlabels_t const& rm) const { m_relevant_memlabels = make_dshared<relevant_memlabels_t>(rm); }

  memlabel_t                   graph_get_memlabel_all_except_locals_and_stack_and_args() const { ASSERT(m_relevant_memlabels); return m_relevant_memlabels->get_memlabel_all_except_locals_and_stack_and_args(); }
  memlabel_t                   get_memlabel_all() const        { return m_relevant_memlabels->get_memlabel_all(); }
  sprel_map_t get_sprel_map_from_avail_exprs(avail_exprs_t const& avail_exprs, map<graph_loc_id_t, graph_cp_location> const& locs, map<graph_loc_id_t, expr_ref> const& locid2expr_map) const;

  void populate_var_definedness();
  void set_locs_map_start_loc_id(graph_loc_id_t const& start_loc_id)
  {
    m_start_loc_id = start_loc_id;
  }

//  graph_loc_id_t get_cloned_loc_for_input_loc_at_pc (T_PC const& cloned_pc, graph_loc_id_t orig_loc_id) const
//  {
//    ASSERT(cloned_pc.is_cloned_pc());
//    graph_loc_id_t ret = orig_loc_id;
//    if (m_orig_locid_to_cloned_locid_map.count(cloned_pc) && m_orig_locid_to_cloned_locid_map.at(cloned_pc).count(orig_loc_id)) {
//      ret = m_orig_locid_to_cloned_locid_map.at(cloned_pc).at(orig_loc_id);
//    }
//    return ret;
//  }
//
//  map<graph_loc_id_t, graph_loc_id_t> get_orig_locid_to_cloned_locid_map_at_pc( T_PC const& cloned_pc) const
//  {
//    ASSERT(cloned_pc.is_cloned_pc());
//    map<graph_loc_id_t, graph_loc_id_t> ret;
//    if(m_orig_locid_to_cloned_locid_map.count(cloned_pc))
//      ret = m_orig_locid_to_cloned_locid_map.at(cloned_pc);
//    return ret;
//  }
  void clear_loc_structures() const
  {
    //this->m_avail_exprs.clear();
    this->m_avail_exprs = avail_exprs_t();
    this->m_loc_liveness.clear();
    this->m_loc_definedness.clear();
    this->m_var_definedness.clear();
    this->m_bavlocs.clear();
    //this->m_sprels.clear();
    this->graph_init_sprels();
  }


private:
  void set_locs_within_constructor(map<graph_loc_id_t, graph_cp_location> const &locs);
  //map<T_PC, avail_exprs_t> compute_loc_avail_exprs() const;
  void collect_memory_accesses(set<T_PC> const &nodes_changed, map<T_PC, set<eqspace::state::mem_access>> &state_mem_acc_map) const;
  //void refresh_graph_locs(map<graph_loc_id_t, graph_cp_location> const &llvm_locs);

  string read_loc(istream& in, graph_cp_location &loc) const;
  set<T_PC> get_nodes_where_live_locs_added(map<T_PC, set<graph_loc_id_t>> const &orig, map<T_PC, set<graph_loc_id_t>> const &cur);
  static bool loc_is_atomic(graph_cp_location const &loc);
  void eliminate_unused_loc_in_start_state_and_all_edges(graph_cp_location const &loc);
  void graph_locs_visit_exprs(std::function<expr_ref (T_PC const &, const string&, expr_ref)> f, map<graph_loc_id_t, graph_cp_location> &locs_map);
  //template<typename T_MAP_VALUETYPE>
  //static void edgeloc_map_eliminate_unused_loc(map<pair<edge_id_t<T_PC>, graph_loc_id_t>, T_MAP_VALUETYPE> &elmap, graph_loc_id_t loc_id);
  static bool expr_is_possible_loc(expr_ref e);
  map<graph_loc_id_t, sprel_status_t> get_sprel_status_mapping_from_avail_exprs(avail_exprs_t const &avail_exprs, map<graph_loc_id_t, graph_cp_location> const& locs, set<graph_loc_id_t> const& arg_locids) const;
//  void set_orig_locid_to_new_locid_for_pc(T_PC const& cloned_pc, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map)
//  {
//    ASSERT(!m_orig_locid_to_cloned_locid_map.count(cloned_pc));
//    m_orig_locid_to_cloned_locid_map.insert(make_pair(cloned_pc, old_locid_to_new_locid_map));
//  }
//protected:
  //map<expr_id_t, pair<expr_ref, expr_ref>> avail_exprs_get_submap(avail_exprs_t const& avail_exprs) const;
  //dshared_ptr<map<graph_loc_id_t, graph_cp_location> const> m_locs;
  //map<graph_loc_id_t, expr_ref>  m_locid2expr_map;
  //map<expr_id_t, graph_loc_id_t> m_exprid2locid_map;

private:

  graph_loc_id_t m_start_loc_id;
  mutable avail_exprs_t m_avail_exprs;      //for each PC, the available expressions at each loc-id
  dshared_ptr<graph_locs_map_t const> m_graph_locs_map;
//  map<T_PC, map<graph_loc_id_t, graph_loc_id_t>> m_orig_locid_to_cloned_locid_map;   //for each cloned PC, the map from orig locid to the new cloned locid to be used for renaming
  mutable map<T_PC, set<graph_loc_id_t>> m_loc_liveness;     //for each PC, the set of live locs
  mutable map<T_PC, set<graph_loc_id_t>> m_loc_definedness;  //for each PC, the set of defined locs
  mutable map<T_PC, set<string_ref>> m_var_definedness;  //for each PC, the set of defined locs
  mutable map<T_PC, set<graph_loc_id_t>> m_bavlocs;          //for each PC, the set of branch affecting (variables) locs

  mutable sprel_map_t m_sprels;

  mutable dshared_ptr<relevant_memlabels_t const> m_relevant_memlabels;
};

}
