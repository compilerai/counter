#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/sp_version.h"
#include "expr/relevant_memlabels.h"

#include "gsupport/sprel_map.h"

#include "graph/graph_loc_id.h"
#include "graph/graph_with_precondition.h"
#include "graph/graph_with_aliasing.h"
#include "graph/locset.h"
#include "graph/graph_locs_map.h"

//#include "eq/parse_input_eq_file.h"

namespace eqspace {

using namespace std;


template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_path_sensitive_locs : public graph_with_aliasing<T_PC, T_N, T_E, T_PRED>
{
public:
  typedef pair<T_PC, T_PC> path_id_t;
  graph_with_path_sensitive_locs(string const &name, string const& fname, context* ctx) : graph_with_aliasing<T_PC,T_N,T_E,T_PRED>(name, fname, ctx)
  { }

  graph_with_path_sensitive_locs(graph_with_path_sensitive_locs const &other) : graph_with_aliasing<T_PC,T_N,T_E,T_PRED>(other),
                                                  m_orig_locid_to_cloned_locid_map(other.m_orig_locid_to_cloned_locid_map),
                                                  m_path_sensitive_avail_exprs(other.m_path_sensitive_avail_exprs),
                                                  m_path_sensitive_loc_definedness(other.m_path_sensitive_loc_definedness),
                                                  m_path_sensitive_var_definedness(other.m_path_sensitive_var_definedness),
                                                  m_path_sensitive_sprels(other.m_path_sensitive_sprels)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_path_sensitive_locs const& other)
  {
    graph_with_aliasing<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    ASSERT(m_orig_locid_to_cloned_locid_map.empty());
  }

  graph_with_path_sensitive_locs(istream& in, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);
  virtual ~graph_with_path_sensitive_locs() = default;

  set<string_ref> get_var_definedness_for_path_id(path_id_t pp) const 
  { 
    if (m_path_sensitive_var_definedness.count(pp))
      return m_path_sensitive_var_definedness.at(pp); 
    else
      return this->get_var_definedness_at_pc(pp.first);
  }
//  map<path_id_t, set<graph_loc_id_t>> const& get_path_sensitive_loc_definedness() const { 
//    return m_path_sensitive_loc_definedness; 
//  }
//  void set_path_sensitive_loc_definedness(map<path_id_t, set<graph_loc_id_t>> const& loc_definedness) { 
//    m_path_sensitive_loc_definedness = loc_definedness; 
//    this->populate_var_definedness();
//  }
//  void set_path_sensitive_var_definedness(map<path_id_t, set<string_ref>> const& var_definedness) { 
//    m_path_sensitive_var_definedness = var_definedness; 
//  }
  bool loc_is_defined_for_path_id(path_id_t const &pp, graph_loc_id_t loc_id) const { 
    if(m_path_sensitive_loc_definedness.count(pp))
      return m_path_sensitive_loc_definedness.at(pp).count(loc_id) != 0; 
    else 
      return this->loc_is_defined_at_pc(pp.first, loc_id);
  }
  map<graph_loc_id_t, graph_cp_location> get_locs_for_path_id(path_id_t const& pp) const
  {
    map<graph_loc_id_t, graph_cp_location> ret;
    if (m_path_sensitive_loc_definedness.count(pp)) {
      for (auto const& lid : m_path_sensitive_loc_definedness.at(pp))
      {
        ret.insert(make_pair(lid, this->get_loc(lid)));
      }
    } 
    else  ret =  this->get_locs_at_pc(pp.first);
    return ret;
  }
  map<graph_loc_id_t, graph_cp_location> get_ghost_locs_for_path_id(path_id_t const& pp) const
  {
    map<graph_loc_id_t, graph_cp_location> ret;

    for (auto const& loc : this->get_ghost_locs(pp.first)) {
      if (this->loc_is_defined_for_path_id(pp, loc.first))
        ret.insert(loc);
    }
    return ret;
  }
  set<expr_ref> get_ghost_loc_exprs_for_path_id(path_id_t const& p) const;
  map<graph_loc_id_t, graph_cp_location> graph_populate_locs_for_cloned_edge(dshared_ptr<T_E const> const& new_e1, T_PC const& orig_pc);
  void graph_populate_avail_exprs_and_sprels_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
  void graph_populate_loc_livenes_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
  void graph_populate_loc_definedness_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
  void graph_populate_lr_status_at_cloned_pc(dshared_ptr<T_E const> const& cloned_edge, T_PC const& orig_pc/*, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map*/);
  void populate_path_sensitive_var_definedness_for_path_id(path_id_t const& pp) const;
  graph_loc_id_t get_cloned_loc_for_input_loc_at_pc (T_PC const& cloned_pc, graph_loc_id_t orig_loc_id) const
  {
    ASSERT(cloned_pc.is_cloned_pc());
    graph_loc_id_t ret = orig_loc_id;
    if (m_orig_locid_to_cloned_locid_map.count(cloned_pc) && m_orig_locid_to_cloned_locid_map.at(cloned_pc).count(orig_loc_id)) {
      ret = m_orig_locid_to_cloned_locid_map.at(cloned_pc).at(orig_loc_id);
    }
    return ret;
  }

  map<graph_loc_id_t, graph_loc_id_t> get_orig_locid_to_cloned_locid_map_at_pc( T_PC const& cloned_pc) const
  {
    map<graph_loc_id_t, graph_loc_id_t> ret;
    if(m_orig_locid_to_cloned_locid_map.count(cloned_pc))
      ret = m_orig_locid_to_cloned_locid_map.at(cloned_pc);
    return ret;
  }
  void set_orig_locid_to_new_locid_for_pc(T_PC const& cloned_pc, map<graph_loc_id_t, graph_loc_id_t> const& old_locid_to_new_locid_map)
  {
    ASSERT(!m_orig_locid_to_cloned_locid_map.count(cloned_pc));
    m_orig_locid_to_cloned_locid_map.insert(make_pair(cloned_pc, old_locid_to_new_locid_map));
  }
  
  avail_exprs_t const &get_avail_exprs_for_path_id(path_id_t const &pp) const
  {
    if(this->m_path_sensitive_avail_exprs.count(pp))
      return this->m_path_sensitive_avail_exprs.at(pp);
    else
      return this->get_avail_exprs/*_at_pc*/(/*pp.first*/);
  }

  expr_ref expr_simplify_for_path_id(expr_ref const& e, path_id_t const& pp) const;
  bool loc_has_sprel_mapping_for_path_id(path_id_t const &pp, graph_loc_id_t loc_id) const 
  { 
    if(m_path_sensitive_sprels.count(pp))
      return m_path_sensitive_sprels.at(pp).has_mapping_for_loc(loc_id);
    else
     return this->loc_has_sprel_mapping/*_at_pc*/(/*pp.first, */loc_id); 
  }
  sprel_map_t const &get_sprel_map_for_path_id(path_id_t const &pp) const { 
    if(m_path_sensitive_sprels.count(pp))
      return m_path_sensitive_sprels.at(pp);
    else
      return this->get_sprel_map(/*pp.first*/);
  }

//  void set_sprel_map_for_path_id(path_id_t const &pp, sprel_map_t const& s) const
//  {
//    ASSERT(!m_path_sensitive_sprels.count(pp));
//    m_path_sensitive_sprels.insert(make_pair(pp, s));
//  }
  
  sprel_map_pair_t get_sprel_map_pair_for_path_id(path_id_t const &pp) const
  {
    sprel_map_t const &sprel_map = get_sprel_map_for_path_id(pp);
    return sprel_map_pair_t(sprel_map);
  }
 
//  void avail_exprs_insert_for_path_id(path_id_t const& pp, avail_exprs_t const& avail_exprs)
//  {
//    ASSERT(!m_path_sensitive_avail_exprs.count(pp));
//    this->m_path_sensitive_avail_exprs.insert(make_pair(pp, avail_exprs));
//  }
  virtual void graph_to_stream(ostream& ss, string const& prefix="") const override;
  set<expr_ref> get_spreled_loc_exprs_for_path_id(path_id_t const& p) const;
  void populate_path_sensitive_structuces_for_path_id(path_id_t const& pp, map<graph_loc_id_t, graph_loc_id_t> orig_locid_to_cloned_locid_map_for_input_path_id) const
  {
    if (!orig_locid_to_cloned_locid_map_for_input_path_id.size())
      return;
    /*if(this->has_avail_exprs_at_pc(pp.first)) */{
      remove_cv_t<remove_reference_t<decltype((this->get_avail_exprs().get_map()))>> new_avail_exprs_map;
      for (auto const& [locid,av_val] : this->get_avail_exprs().get_map()) {
        if (orig_locid_to_cloned_locid_map_for_input_path_id.count(locid)) {
          auto cloned_locid = orig_locid_to_cloned_locid_map_for_input_path_id.at(locid);
          if (cloned_locid != GRAPH_LOC_ID_INVALID) {
            new_avail_exprs_map.emplace(cloned_locid,av_val);
          }
        } else {
          new_avail_exprs_map.emplace(locid,av_val);
        }
      }
      m_path_sensitive_avail_exprs[pp] = avail_exprs_t(new_avail_exprs_map);
      if (m_path_sensitive_sprels.count(pp))
        m_path_sensitive_sprels.erase(pp);
      m_path_sensitive_sprels.insert(make_pair(pp, this->get_sprel_map_from_avail_exprs(this->get_avail_exprs_for_path_id(pp), this->get_locs(), this->get_locid2expr_map())));
    }
    if (this->get_loc_definedness().count(pp.first)) {
      set<graph_loc_id_t> new_loc_definedness;
      for (auto const& m : this->get_loc_definedness().at(pp.first))
      {
        if (orig_locid_to_cloned_locid_map_for_input_path_id.count(m)) {
          auto cloned_locid = orig_locid_to_cloned_locid_map_for_input_path_id.at(m);
          if (cloned_locid != GRAPH_LOC_ID_INVALID) {
            new_loc_definedness.insert(cloned_locid);
          }
        } else {
          new_loc_definedness.insert(m);
        }
      }
      m_path_sensitive_loc_definedness[pp] = new_loc_definedness;
      this->populate_path_sensitive_var_definedness_for_path_id(pp);
    }
  }

private:

  map<T_PC, map<graph_loc_id_t, graph_loc_id_t>> m_orig_locid_to_cloned_locid_map;   //for each cloned PC, the map from orig locid to the new cloned locid to be used for renaming
  mutable map<path_id_t, avail_exprs_t> m_path_sensitive_avail_exprs;      //for each PC, the available expressions at each loc-id
  mutable map<path_id_t, set<graph_loc_id_t>> m_path_sensitive_loc_definedness;  //for each PC, the set of defined locs
  mutable map<path_id_t, set<string_ref>> m_path_sensitive_var_definedness;  //for each PC, the set of defined locs
  mutable map<path_id_t, sprel_map_t> m_path_sensitive_sprels;
};

}
