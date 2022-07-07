#pragma once

#include "support/utils.h"

#include "expr/context.h"
#include "expr/memlabel_obj.h"
#include "expr/mlvarname.h"

namespace eqspace {

class graph_memlabel_map_t {
private:
  map<mlvarname_t, memlabel_ref> m_map;

public:
  graph_memlabel_map_t()
  { }

  graph_memlabel_map_t(map<mlvarname_t, memlabel_ref> const& m) : m_map(m)
  { }

  map<mlvarname_t, memlabel_ref> const& mlmap_get_map() const
  {
    return m_map;
  }

  void mlmap_insert(mlvarname_t const& mlvarname, memlabel_ref const& ml)
  {
    bool inserted = m_map.insert(make_pair(mlvarname, ml)).second;
    if (!inserted) {
      ASSERT(m_map.at(mlvarname) == ml);
    }
  }

  memlabel_ref mlmap_get_ml_for_mlvar(mlvarname_t const& mlvar) const
  {
    return m_map.at(mlvar);
  }

  bool mlmap_has_mapping(mlvarname_t const& mlvar) const
  {
    return m_map.count(mlvar) != 0;
  }

  void mlmap_update_mapping(mlvarname_t const& mlvar, memlabel_ref const& mlr)
  {
    if (m_map.count(mlvar)) {
      m_map.at(mlvar) = mlr;
    } else {
      m_map.insert(make_pair(mlvar, mlr));
    }
  }

  void mlmap_clear_non_callee_memlabels(string const& graph_name)
  {
    function<bool (mlvarname_t const&, memlabel_ref&)> f = [&graph_name](mlvarname_t const& mlv, memlabel_ref& _) -> bool
    { return !mlvarname_refers_to_fcall(mlv, graph_name); };
    map_mutate_or_erase(m_map, f);
  }

  void mlmap_union(graph_memlabel_map_t const& other)
  {
    m_map.insert(other.m_map.begin(), other.m_map.end());
  }

  void mlmap_clear()
  {
    m_map.clear();
  }

  bool mlmap_is_empty() const
  {
    return m_map.empty();
  }

  void mlmap_coarsen_readonly_memlabels_to_rodata_memlabel()
  {
    for (auto& [mlvar,mlr] : m_map) {
      auto ml = mlr->get_ml();
      ml.memlabel_replace_readonly_memlabels_with_rodata_memlabel();
      mlr = mk_memlabel_ref(ml);
    }
  }

  void mlmap_set_all_mlvars_to_ml(memlabel_ref const& mlr)
  {
    for (auto& mm : m_map) {
      mm.second = mlr;
    }
  }
};

set<memlabel_ref> get_memlabel_set_from_mlvarnames(graph_memlabel_map_t const &memlabel_map, set<mlvarname_t> const &mlvars);

void memlabel_map_intersect(graph_memlabel_map_t &dst, graph_memlabel_map_t const &src);
void memlabel_map_get_relevant_memlabels(graph_memlabel_map_t const &m, vector<memlabel_ref> &out);

void expr_set_memlabels_to_top(expr_ref const& e, graph_memlabel_map_t& memlabel_map);
void graph_memlabel_map_union(graph_memlabel_map_t& dst, graph_memlabel_map_t const& src);

expr_ref expr_label_memlabels_using_memlabel_map(expr_ref const& e, graph_memlabel_map_t const& memlabel_map);

string read_memlabel_map(istream& in, graph_memlabel_map_t &memlabel_map);
ostream& memlabel_map_to_stream(ostream& os, graph_memlabel_map_t const &ml_map, string const& suffix_string = "");

}
