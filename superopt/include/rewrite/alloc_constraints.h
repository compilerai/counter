#ifndef STACK_SLOT_CONSTRAINTS_H
#define STACK_SLOT_CONSTRAINTS_H
#include <map>
#include <algorithm>
#include <set>
#include <string>
#include "support/debug.h"
#include "support/distinct_sets.h"
#include "support/types.h"
#include "valtag/reg_identifier.h"
#include "rewrite/alloc_map.h"
#include "rewrite/prog_point.h"
#include "rewrite/translation_instance.h"

//typedef std::pair<exreg_group_id_t, reg_identifier_t> exgroup_reg_id_t;
//typedef std::pair<prog_point_t, exgroup_reg_id_t> ssm_elem_t;
class alloc_elem_t {
private:
  prog_point_t m_pp;
  alloc_regname_t m_regname;
public:
  alloc_elem_t()  {} //needed by distinct_set<alloc_elem_t>
  alloc_elem_t(prog_point_t const &pp, alloc_regname_t const &regname) : m_pp(pp), m_regname(regname)
  {
  }

  prog_point_t const &get_prog_point() const { return m_pp; }
  alloc_regname_t const &get_regname() const { return m_regname; }
  std::string alloc_elem_to_string() const;
  bool operator==(alloc_elem_t const &other) const
  {
    return m_pp == other.m_pp && m_regname == other.m_regname;
  }
  bool operator!=(alloc_elem_t const &other) const
  {
    return !(*this == other);
  }
  bool operator<(alloc_elem_t const &other) const
  {
    if (!(m_pp == other.m_pp)) {
      return m_pp < other.m_pp;
    }
    return m_regname < other.m_regname;
  }
  static alloc_elem_t alloc_elem_from_string(string const &s)
  {
    size_t rparen = s.find(')');
    ASSERT(rparen != string::npos);
    ASSERT(rparen < s.size() - 1);
    ASSERT(s.at(rparen + 1) == ',');
    ASSERT(s.at(0) == '(');
    string pp_str = s.substr(1, rparen + 1);
    prog_point_t pp = prog_point_t::prog_point_from_string(pp_str);
    //cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
    //cout << __func__ << " " << __LINE__ << ": rparen = " << rparen << endl;
    size_t lparen = s.find('(', rparen + 1);
    ASSERT(lparen != string::npos);
    rparen = s.find(')', lparen + 1);
    ASSERT(rparen != string::npos);
    //cout << __func__ << " " << __LINE__ << ": lparen = " << lparen << endl;
    //cout << __func__ << " " << __LINE__ << ": rparen = " << rparen << endl;
    string alloc_regname_str = s.substr(lparen, rparen + 1 - lparen);
    //cout << __func__ << " " << __LINE__ << ": alloc_regname_str = " << alloc_regname_str << endl;
    alloc_regname_t alloc_regname = alloc_regname_t::alloc_regname_from_string(alloc_regname_str);
    return alloc_elem_t(pp, alloc_regname);
  }
};

//std::string ssm_elem_to_string(ssm_elem_t const &e);

class alloc_elem_eq_mapping_t
{
private:
  alloc_elem_t m_first, m_second;
  cost_t m_cost;
public:
  alloc_elem_eq_mapping_t(alloc_elem_t const &a, alloc_elem_t const &b, cost_t cost)  : m_first(a), m_second(b), m_cost(cost) {}
  alloc_elem_eq_mapping_t(pair<alloc_elem_t, alloc_elem_t> const &p)  : m_first(p.first), m_second(p.second), m_cost(COST_INFINITY) {}
  alloc_elem_t const &get_first() const { return m_first; }
  alloc_elem_t const &get_second() const { return m_second; }
  cost_t alloc_elem_eq_mapping_get_cost() const { return m_cost; }
  std::string alloc_elem_eq_mapping_to_string() const
  {
    std::stringstream ss;
    ss << m_first.alloc_elem_to_string() << "-->" << m_second.alloc_elem_to_string() << "[" << m_cost << "]";
    return ss.str();
  }
  static alloc_elem_eq_mapping_t alloc_elem_eq_mapping_from_stream(istream &iss, char eol)
  {
    string word;
    char ch;
    iss >> word;
    ASSERT(word == "offset[");
    string off_str;
    while ((iss >> ch) && ch != ']') {
      off_str.append(1, ch);
    }
    alloc_elem_t e1 = alloc_elem_t::alloc_elem_from_string(off_str);
    iss >> word;
    ASSERT(word == "==");
    iss >> word;
    ASSERT(word == "offset[");
    off_str = "";
    while ((iss >> ch) && ch != ']') {
      off_str.append(1, ch);
    }
    alloc_elem_t e2 = alloc_elem_t::alloc_elem_from_string(off_str);
    iss >> word;
    ASSERT(word == "cost");
    cost_t cost;
    iss >> cost;
    return alloc_elem_eq_mapping_t(e1, e2, cost);
  }

  bool operator<(alloc_elem_eq_mapping_t const &other) const
  {
    if (!(m_first == other.m_first)) {
      return m_first < other.m_first;
    }
    return m_second < other.m_second;
  }
  bool alloc_elem_eq_mapping_equals(alloc_elem_eq_mapping_t const &other) const
  {
    return m_first == other.m_first && m_second == other.m_second;
  }
};

#include "support/ugraph.h"

template<typename VALTYPE>
class alloc_constraints_t {
private:
  static int const MAX_NUM_RELAX_CANDIDATES_TO_CONSIDER = 32768;
  alloc_constraints_t() = delete;
  translation_instance_t const *m_ti;
  std::set<alloc_elem_t> m_elems;
  std::set<alloc_elem_eq_mapping_t> m_eq_mappings;
  std::set<std::pair<alloc_elem_t, alloc_elem_t>> m_negative_mappings;
  //std::set<std::pair<ssm_elem_t, VALTYPE>> m_constant_mappings;
  std::map<alloc_elem_t, std::set<VALTYPE>> m_constraints;
  transmap_loc_id_t m_loc_id;

  bool find_path_with_untenable_cycle(list<pair<alloc_elem_t, alloc_elem_t>> &shortest_eq_path, ugraph_t<alloc_elem_t> const &ugraph, ugraph_t<alloc_elem_t> const &ugraph_trans)
  {
    bool found_untenable_cycle = false;
    for (const auto &mne : m_negative_mappings) {
      const auto &a = mne.first;
      const auto &b = mne.second;
      list<pair<alloc_elem_t, alloc_elem_t>> path;
      if (ugraph_trans.edge_belongs(a, b)) {
        bool gotpath = ugraph.get_path(a, b, path);
        ASSERT(gotpath);
        if (!found_untenable_cycle) {
          found_untenable_cycle = true;
          shortest_eq_path = path;
        } else {
          if (path.size() < shortest_eq_path.size()) {
            shortest_eq_path = path;
          }
        }
      }
    }
    return found_untenable_cycle;
  }

  alloc_elem_eq_mapping_t const *
  find_eq_mapping(pair<alloc_elem_t, alloc_elem_t> const &p) const
  {
    for (const auto &e : m_eq_mappings) {
      if (   (e.get_first() == p.first && e.get_second() == p.second)
          || (e.get_second() == p.first && e.get_first() == p.second))  {
        return &e;
      }
    }
    return NULL;
  }

  list<alloc_elem_eq_mapping_t const *>
  convert_eq_path_to_eqm_path(list<pair<alloc_elem_t, alloc_elem_t>> const &eq_path) const
  {
    list<alloc_elem_eq_mapping_t const *> ret;
    for (const auto &p : eq_path) {
      alloc_elem_eq_mapping_t const *eqm = find_eq_mapping(p);
      if (!eqm) {
        cout << __func__ << " " << __LINE__ << ": this =\n" << this->alloc_constraints_to_string() << endl;
        cout << __func__ << " " << __LINE__ << ": p = " << p.first.alloc_elem_to_string() << "->" << p.second.alloc_elem_to_string() << endl;
      }
      ASSERT(eqm);
      ret.push_back(eqm);
    }
    return ret;
  }

  void alloc_constraints_break_untenable_cycles()
  {
    set<alloc_elem_t> eqnodes;
    set<pair<alloc_elem_t, alloc_elem_t>> eq_edges;
    for (const auto &eqm : m_eq_mappings) {
      eqnodes.insert(eqm.get_first());
      eqnodes.insert(eqm.get_second());
      eq_edges.insert(make_pair(eqm.get_first(), eqm.get_second()));
    }
    //alloc_constraints_t<VALTYPE> ret = *this;
    ugraph_t<alloc_elem_t> ugraph(eqnodes, eq_edges);
    ugraph_t<alloc_elem_t> ugraph_trans = ugraph.ugraph_transitive_closure();
    bool found_untenable_cycle;
    do {
      found_untenable_cycle = false;
      list<pair<alloc_elem_t, alloc_elem_t>> shortest_eq_path;
      found_untenable_cycle = find_path_with_untenable_cycle(shortest_eq_path, ugraph, ugraph_trans);
  
      if (found_untenable_cycle) {
        //cout << __func__ << " " << __LINE__ << ": shortest_eq_path.begin() = " << shortest_eq_path.begin()->first.alloc_elem_to_string() << endl;
        //cout << __func__ << " " << __LINE__ << ": shortest_eq_path.rbegin/end() = " << shortest_eq_path.rbegin()->second.alloc_elem_to_string() << endl;
        list<alloc_elem_eq_mapping_t const *> shortest_eqm_path = convert_eq_path_to_eqm_path(shortest_eq_path);;
        CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << "found untenable cycle; shortest-eq-path: " << path_to_string(shortest_eqm_path) << endl);
        alloc_elem_eq_mapping_t const *mincost_edge = path_find_eq_edge_to_remove(shortest_eqm_path);
        CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": removing edge to break untenable cycle: " << mincost_edge->alloc_elem_eq_mapping_to_string() << endl);
        this->m_eq_mappings.erase(*mincost_edge);
        //pair<alloc_elem_t, alloc_elem_t> swapped_mincost_edge = make_pair(mincost_edge.second, mincost_edge.first);
        //this->m_eq_mappings.erase(alloc_elem_eq_mapping_t(swapped_mincost_edge));
        ugraph.delete_edge(mincost_edge->get_first(), mincost_edge->get_second());
        ugraph_trans = ugraph.ugraph_transitive_closure();
      }
  
        /*while (   ugraph_trans.edge_belongs(a, b)
               && ugraph.get_path(a, b, path)) { //the second check is required because we are changing ugraph in the body of this loop but we are not accordingly changing ugraph_trans. Using ugraph_trans gives us significant speedup
          CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": found untenable cycle between " << ssm_elem_to_string(a) << " and " << ssm_elem_to_string(b) << endl);
          CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << "eq-path: " << path_to_string(path) << endl);
          pair<ssm_elem_t, ssm_elem_t> const &mincost_edge = path_find_mincost_edge(path);
          CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": removing edge to break untenable cycle: " << ssm_elem_to_string(mincost_edge.first) << " --> " << ssm_elem_to_string(mincost_edge.second) << endl);
          ret.m_eq_mappings.erase(mincost_edge);
          pair<ssm_elem_t, ssm_elem_t> swapped_mincost_edge = make_pair(mincost_edge.second, mincost_edge.first);
          ret.m_eq_mappings.erase(swapped_mincost_edge);
          ugraph.delete_edge(mincost_edge.first, mincost_edge.second);
        }*/
    } while (found_untenable_cycle);
    //return ret;
  }

  static void update_canonical_elems(distinct_set_t<alloc_elem_t> &canonical_elem, alloc_elem_t const &first, alloc_elem_t const &second)
  {
    canonical_elem.distinct_set_union(first, second);
    //bool change = false;
    //if (!canonical_elem.count(first)) {
    //  canonical_elem.insert(make_pair(first, first));
    //  change = true;
    //}
    //if (!canonical_elem.count(second)) {
    //  canonical_elem.insert(make_pair(second, second));
    //  change = true;
    //}
    //if (canonical_elem.at(second) < canonical_elem.at(first)) {
    //  canonical_elem.erase(first);
    //  canonical_elem.insert(make_pair(first, canonical_elem.at(second)));
    //  change = true;
    //}
    //if (canonical_elem.at(first) < canonical_elem.at(second)) {
    //  canonical_elem.erase(second);
    //  canonical_elem.insert(make_pair(second, canonical_elem.at(first)));
    //  change = true;
    //}
    //return change;
  }

  static void init_canonical_elem(distinct_set_t<alloc_elem_t> &canonical_elem, alloc_constraints_t const &alloc_constraints)
  {
    canonical_elem.clear();
    for (const auto &elem : alloc_constraints.m_elems) {
      canonical_elem.distinct_set_make(elem);
      //cout << __func__ << " " << __LINE__ << ": canonical[" << elem.alloc_elem_to_string() << "] = " << canonical_elem.distinct_set_find(elem).alloc_elem_to_string() << endl;;
    }
  }

  bool merge_nodes_and_color_graph_for_graph_without_untenable_cycles(std::map<prog_point_t, alloc_map_t<VALTYPE>> &ssm, vector<alloc_elem_t> &uncolorable_set, distinct_set_t<alloc_elem_t> &canonical_elem, map<alloc_elem_t, set<VALTYPE>> &canon_constraints) const
  {
    canon_constraints.clear();
    init_canonical_elem(canonical_elem, *this);
    //bool change = true;
    //while (change) {
    //  change = false;
    //  for (const auto &me : m_eq_mappings) {
    //    change = update_canonical_elems(canonical_elem, me.get_first(), me.get_second()) || change;
    //  }
    //}
    for (const auto &me : m_eq_mappings) {
      ASSERT(m_elems.count(me.get_first()));
      ASSERT(m_elems.count(me.get_second()));
      //cout << __func__ << " " << __LINE__ << ": updating canonical_elem through eq mapping " << me.alloc_elem_eq_mapping_to_string() << endl;
      update_canonical_elems(canonical_elem, me.get_first(), me.get_second());

      //for (const auto &e : m_elems) {
      //  cout << __func__ << " " << __LINE__ << ": canonical[" << e.alloc_elem_to_string() << "] = ";
      //  cout << canonical_elem.distinct_set_find(e).alloc_elem_to_string() << endl;
      //}
    }
    //collapse_connected_eq_components into neq_edges using canonical nodes
    std::set<std::pair<alloc_elem_t, alloc_elem_t>> neq_edges;
    for (const auto &mne : m_negative_mappings) {
      //if (!canonical_elem.count(mne.first)) {
      //  canonical_elem.insert(make_pair(mne.first, mne.first));
      //}
      //if (!canonical_elem.count(mne.second)) {
      //  canonical_elem.insert(make_pair(mne.second, mne.second));
      //}
      ASSERT(m_elems.count(mne.first));
      ASSERT(m_elems.count(mne.second));
      auto cmne_first = canonical_elem.distinct_set_find(mne.first);
      auto cmne_second = canonical_elem.distinct_set_find(mne.second);
      if (cmne_first == cmne_second) {
        cout << __func__ << " " << __LINE__ << ": loc_id = " << (int)m_loc_id << ": error : cmne_first == cmne_second\n";
        cout << "mne.first = " << mne.first.alloc_elem_to_string() << endl;
        cout << "mne.second = " << mne.second.alloc_elem_to_string() << endl;
        cout << "cmne_first = " << cmne_first.alloc_elem_to_string() << endl;
        cout << "cmne_second = " << cmne_second.alloc_elem_to_string() << endl;
      }
      ASSERT(!(cmne_first == cmne_second)); //because we have already broken untenable cycles
      neq_edges.insert(make_pair(cmne_first, cmne_second));
    }
    //for (const auto &e : m_elems) {
    //  if (!canonical_elem.count(e)) {
    //    canonical_elem.insert(make_pair(e, e));
    //  }
    //}
  
    std::set<alloc_elem_t> nodes;
    for (const auto &e : m_elems) {
      auto ce = canonical_elem.distinct_set_find(e);
      nodes.insert(ce);
      if (m_constraints.count(e)) {
        if (!canon_constraints.count(ce)) { //no constraints exist for canonical_elem yet
          canon_constraints.insert(make_pair(ce, m_constraints.at(e)));
        } else {
          set_intersect(canon_constraints.at(ce), m_constraints.at(e));
          ASSERT(canon_constraints.at(ce).size() != 0); //because we have already broken untenable constraints
        }
      }
    }
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS,
        cout << __func__ << " " << __LINE__ << ": loc " << (int)m_loc_id << ":\n" << this->alloc_constraints_to_string() << endl;
        for (const auto &e : m_elems) {
          auto ce = canonical_elem.distinct_set_find(e);
          cout << "loc " << (int)m_loc_id << ": canonical[(" << e.alloc_elem_to_string() << ", " << e.get_regname().alloc_regname_to_string() << ")] = (" << ce.alloc_elem_to_string() << ", " << ce.get_regname().alloc_regname_to_string() << ")\n";
        }
        for (auto ne : neq_edges) {
          cout << "loc " << (int)m_loc_id << ": (" << ne.first.alloc_elem_to_string() << ", " << ne.first.get_regname().alloc_regname_to_string() << ") != (" << ne.second.alloc_elem_to_string() << ", " << ne.second.get_regname().alloc_regname_to_string() << ")\n";
        }
    );
  
    std::map<alloc_elem_t, VALTYPE> color_map;
    if (!graph_color_for_edges(nodes, neq_edges, canon_constraints, m_loc_id, color_map, uncolorable_set)) {
      return false;
    }
    //std::map<int, VALTYPE> alloc_offset_for_color;
    //std::map<VALTYPE, int> color_for_alloc_offset;
    //for (const auto &constant_mapping : m_constant_mappings) {
    //  //src_ulong pc = constant_mapping.first.first;
    //  //exreg_group_id_t g = constant_mapping.first.second.first;
    //  //reg_identifier_t const &r = constant_mapping.first.second.second;
    //  //ssm[pc].ssm_add_entry(g, r, constant_mapping.second);
    //  //cout << __func__ << " " << __LINE__ << ": constant_mapping.first = (" << constant_mapping.first.first << ", " << constant_mapping.first.second.first << ", " << constant_mapping.first.second.second.reg_identifier_to_string() << ")\n";
    //  auto ce = canonical_elem.at(constant_mapping.first);
    //  int c = color_map.at(ce);
    //  if (!alloc_offset_for_color.count(c)) {
    //    CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": adding mapping from color " << c << " to " << constant_mapping.second << endl);
    //    ASSERT(!color_for_alloc_offset.count(constant_mapping.second));
    //    alloc_offset_for_color.insert(make_pair(c, constant_mapping.second));
    //    color_for_alloc_offset.insert(make_pair(constant_mapping.second, c));
    //  }
    //}
    //size_t cur_alloc_offset = 0;
    for (const auto &e : m_elems) {
      auto ce = canonical_elem.distinct_set_find(e);
      prog_point_t const &pc = e.get_prog_point();
      //exreg_group_id_t g = ce.first.get_regname().first;
      //reg_identifier_t const &r = ce.first.get_regname().second;
      if (!ssm[pc].ssm_entry_exists(e.get_regname()/*g, r*/)) {
        ASSERT(color_map.count(ce));
        VALTYPE const &color = color_map.at(ce);
        //if (!alloc_offset_for_color.count(color)) {
        //  //alloc_offset = cur_alloc_offset;
        //  //alloc_offset_for_color.insert(make_pair(color, stack_offset_t(false, cur_alloc_offset * STACK_SLOT_SIZE)));
        //  VALTYPE v = VALTYPE::get_default_object(cur_alloc_offset);
        //  while (color_for_alloc_offset.count(v)) { //find a value that is not yet used
        //    cur_alloc_offset++;
        //    v = VALTYPE::get_default_object(cur_alloc_offset);
        //  }
        //  CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": adding mapping from color " << color << " to " << v << endl);
        //  ASSERT(!color_for_alloc_offset.count(v));
        //  alloc_offset_for_color.insert(make_pair(color, v));
        //  color_for_alloc_offset.insert(make_pair(v, color));
        //  cur_alloc_offset++;
        //}
        //ssm[pc].ssm_add_entry(g, r, alloc_offset_for_color.at(color));
        //ssm[pc].ssm_add_entry(g, r, color);
        ssm[pc].ssm_add_entry(e.get_regname(), color);
      }
    }
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS,
        for (auto ps : ssm) {
          std::cout << "ssm[" << ps.first.to_string() << "] =\n" << ps.second.to_string() << endl;
        }
    );
    return true;
  }

  alloc_constraints_t<VALTYPE>
  add_more_mappings_based_on_singular_constraints() const
  {
    map<VALTYPE, set<alloc_elem_t>> eq_constant_mappings;
    for (const auto &constant_mapping : m_constraints) {
      if (constant_mapping.second.size() == 1) {
        eq_constant_mappings[*constant_mapping.second.begin()].insert(constant_mapping.first);
      }
    }
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS,
      cout << __func__ << " " << __LINE__ << ": Printing eq_constant_mapping:\n";
      for (const auto &e : eq_constant_mappings) {
        cout << e.first << ":";
        for (const auto &s : e.second) {
          cout << " " << s.alloc_elem_to_string();
        }
        cout << endl;
      }
    );

    auto ret = *this;
    //next encode the fact that all alloc_elems that share the same constant mapping must have equal mappings
    for (auto v : eq_constant_mappings) {
      for (auto iter = v.second.begin(); iter != v.second.end(); iter++) {
        auto next = iter;
        next++;
        if (next != v.second.end()) {
          //change = update_canonical_elems(canonical_elem, *iter, *next) || change;
          ret.ssc_add_mapping(*iter, *next, COST_INFINITY);
        }
      }
    }

    //next encode the fact that all alloc_elems whose constraints do not intersect must have neq mappings
    for (const auto &c1 : m_constraints) {
      for (const auto &c2 : m_constraints) {
        set<VALTYPE> s = c1.second;
        set_intersect(s, c2.second);
        if (s.size() == 0) {
          ret.add_negative_mapping(c1.first, c2.first);
        }
      }
    }
    return ret;
  }

  static set<alloc_elem_t> get_alloc_elems_with_canon_elem(alloc_elem_t const &n, distinct_set_t<alloc_elem_t> const &canonical_elem, set<alloc_elem_t> const &elems)
  {
    set<alloc_elem_t> ret;
    //for (const auto &c : canonical_elem) {
    //  if (c.second == n) {
    //    ret.insert(c.first);
    //  }
    //}
    for (const auto &e : elems) {
      if (canonical_elem.distinct_set_find(e) == n) {
        ret.insert(e);
      }
    }
    return ret;
  }

  alloc_elem_eq_mapping_t const &get_mincost_eq_mapping_to_be_relaxed(set<alloc_elem_t> const &nset) const
  {
    alloc_elem_eq_mapping_t const *ret = NULL;
    for (const auto &eqm : m_eq_mappings) {
      if (   !set_belongs(nset, eqm.get_first())
          || !set_belongs(nset, eqm.get_second())
          || eqm.alloc_elem_eq_mapping_get_cost() == COST_INFINITY) {
        continue;
      }
      if (!ret || ret->alloc_elem_eq_mapping_get_cost() > eqm.alloc_elem_eq_mapping_get_cost()) {
        ret = &eqm;
      }
    }
    if (!ret) {
      cout << __func__ << " " << __LINE__ << ": nset =\n";
      for (const auto &a : nset) {
	cout << " " << a.alloc_elem_to_string();
      }
      cout << endl;
    }
    ASSERT(ret);
    return *ret;
  }

  set<alloc_elem_eq_mapping_t>
  get_eq_mappings_that_can_be_relaxed_among_nodes(set<alloc_elem_t> const &nodes) const
  {
    set<alloc_elem_eq_mapping_t> ret;
    for (const auto &e : m_eq_mappings) {
      if (   set_belongs(nodes, e.get_first())
	        && set_belongs(nodes, e.get_second())
	        && e.alloc_elem_eq_mapping_get_cost() != COST_INFINITY) {
        ret.insert(e);
      }
    }
    return ret;
  }

  size_t get_max_partition_size_of_uncolorable_set_partitions_if_eq_edge_is_relaxed(alloc_elem_eq_mapping_t const &eq_edge_relax_candidate, set<alloc_elem_t> const &uncolorable_set_elems) const
  {
    distinct_set_t<alloc_elem_t> canonical_elem;
    bool change = true;
    alloc_elem_t const &relax_e1 = eq_edge_relax_candidate.get_first();
    alloc_elem_t const &relax_e2 = eq_edge_relax_candidate.get_second();
    init_canonical_elem(canonical_elem, *this);
    //while (change) {
    //  change = false;
    //  for (const auto &eqm : m_eq_mappings) {
    //    if (eqm.alloc_elem_eq_mapping_equals(eq_edge_relax_candidate)) {
    //      continue;
    //    }
    //    change = update_canonical_elems(canonical_elem, eqm.get_first(), eqm.get_second()) || change;
    //  }
    //}
    for (const auto &eqm : m_eq_mappings) {
      if (eqm.alloc_elem_eq_mapping_equals(eq_edge_relax_candidate)) {
        continue;
      }
      update_canonical_elems(canonical_elem, eqm.get_first(), eqm.get_second());
    }
    CPP_DBG_EXEC3(STACK_SLOT_CONSTRAINTS,
      cout << __func__ << " " << __LINE__ << ": Printing canonical elems:\n";
      for (const auto &e : m_elems) {
        auto ce = canonical_elem.distinct_set_find(e);
        cout << "canonical_elem[" << e.alloc_elem_to_string() << "] = " << ce.alloc_elem_to_string() << endl;
      }
    );
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << "relax_e1 = " << relax_e1.alloc_elem_to_string() << endl);
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << "relax_e2 = " << relax_e2.alloc_elem_to_string() << endl);
    //alloc_elem_t const &canon_elem1 = canonical_elem.count(relax_e1) ? canonical_elem.at(relax_e1) : relax_e1;
    //alloc_elem_t const &canon_elem2 = canonical_elem.count(relax_e2) ? canonical_elem.at(relax_e2) : relax_e2;
    alloc_elem_t const &canon_elem1 = canonical_elem.distinct_set_find(relax_e1);
    alloc_elem_t const &canon_elem2 = canonical_elem.distinct_set_find(relax_e2);
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << "canon_e1 = " << canon_elem1.alloc_elem_to_string() << endl);
    CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << "canon_e2 = " << canon_elem2.alloc_elem_to_string() << endl);
    set<alloc_elem_t> partition1; //canon_elems that have negative mappings with canon_elem1 (and are part of uncolorable set)
    set<alloc_elem_t> partition2; //canon_elems that have negative mappings with canon_elem2 (and are part of uncolorable set)
    for (const auto &nem : m_negative_mappings) {
      alloc_elem_t const &a = nem.first;
      alloc_elem_t const &b = nem.second;
      if (!uncolorable_set_elems.count(a) || !uncolorable_set_elems.count(b)) {
        continue;
      }
      //alloc_elem_t const &canon_a = canonical_elem.count(a) ? canonical_elem.at(a) : a;
      //alloc_elem_t const &canon_b = canonical_elem.count(b) ? canonical_elem.at(b) : b;
      alloc_elem_t const &canon_a = canonical_elem.distinct_set_find(a);
      alloc_elem_t const &canon_b = canonical_elem.distinct_set_find(b);
      CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": negative_mapping.a = " << a.alloc_elem_to_string() << endl);
      CPP_DBG_EXEC2(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": negative_mapping.b = " << b.alloc_elem_to_string() << endl);
      if (canon_a == canon_elem1) {
        partition1.insert(canon_b);
      } else if (canon_b == canon_elem1) {
        partition1.insert(canon_a);
      }
      if (canon_a == canon_elem2) {
        partition2.insert(canon_b);
      } else if (canon_b == canon_elem2) {
        partition2.insert(canon_a);
      }
    }
    return max(partition1.size(), partition2.size());
  }

  class relax_candidates_vec_cmp_on_flow_information_t
  {
  private:
    //input_exec_t const *m_in;
    map<alloc_elem_t, set<VALTYPE>> const &m_canon_constraints;
    distinct_set_t<alloc_elem_t> const &m_canon_elem;
  public:
    relax_candidates_vec_cmp_on_flow_information_t(map<alloc_elem_t, set<VALTYPE>> const &canon_constraints, distinct_set_t<alloc_elem_t> const &canon_elem) : m_canon_constraints(canon_constraints), m_canon_elem(canon_elem)  {}
    bool operator()(alloc_elem_eq_mapping_t const &a, alloc_elem_eq_mapping_t const &b) const
    {
      alloc_elem_t const &af = a.get_first();
      alloc_elem_t const &bf = b.get_first();
      //alloc_elem_t const &caf = m_canon_elem.count(af) ? m_canon_elem.at(af) : af;
      //alloc_elem_t const &cbf = m_canon_elem.count(bf) ? m_canon_elem.at(bf) : bf;
      alloc_elem_t const &caf = m_canon_elem.distinct_set_find(af);
      alloc_elem_t const &cbf = m_canon_elem.distinct_set_find(bf);
      if (m_canon_constraints.at(caf).size() != m_canon_constraints.at(cbf).size()) {
        return m_canon_constraints.at(caf).size() > m_canon_constraints.at(cbf).size();
      }
      return a.alloc_elem_eq_mapping_get_cost() < b.alloc_elem_eq_mapping_get_cost();
    }
  };

  vector<alloc_elem_eq_mapping_t> truncate_relax_candidates_based_on_cost_and_constraints(set<alloc_elem_eq_mapping_t> const &candidates, map<alloc_elem_t, set<VALTYPE>> const &canon_constraints, distinct_set_t<alloc_elem_t> const &canon_elem) const
  {
    vector<alloc_elem_eq_mapping_t> candidates_vec(candidates.begin(), candidates.end());
    relax_candidates_vec_cmp_on_flow_information_t relax_candidates_vec_cmp_on_flow_information(canon_constraints, canon_elem);
    std::sort(candidates_vec.begin(), candidates_vec.end(), relax_candidates_vec_cmp_on_flow_information);
    if (candidates_vec.size() > MAX_NUM_RELAX_CANDIDATES_TO_CONSIDER) {
      candidates_vec.erase(candidates_vec.begin() + MAX_NUM_RELAX_CANDIDATES_TO_CONSIDER, candidates_vec.end());
    }
    return candidates_vec;
  }

  alloc_elem_eq_mapping_t identify_eq_mapping_to_relax_for_uncolorable_set(vector<alloc_elem_t> const &uncolorable_set, distinct_set_t<alloc_elem_t> const &canonical_elem, map<alloc_elem_t, set<VALTYPE>> const &canon_constraints) const
  {
    set<alloc_elem_eq_mapping_t> candidates;

    for (const auto &n : uncolorable_set) {
      set<alloc_elem_t> n_set = get_alloc_elems_with_canon_elem(n, canonical_elem, m_elems);
      set<alloc_elem_eq_mapping_t> ecandidates = get_eq_mappings_that_can_be_relaxed_among_nodes(n_set);
      set_union(candidates, ecandidates);
      //if (candidates.size() > 0) {
      //  break; //stop when you find the first conflicting node which contains relaxable edges; this is an optimization that reduces the translation time; needs further investigation
      //}
    }

    if (candidates.size() == 0) {
      DBG_SET(STACK_SLOT_CONSTRAINTS, 1);
    }
    CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS,
      cout << __func__ << " " << __LINE__ << ": loc " << (int)m_loc_id << ": Printing uncolorable set of canonical elems:\n";
      //cout << n.alloc_elem_to_string() << ":";
      //for (const auto &nn : n_set) {
      //  cout << " " << nn.alloc_elem_to_string();
      //}
      //cout << ":";
      //for (const auto &v : canon_constraints.at(n)) {
      //  cout << " " << v;
      //}
      //cout << endl;
      for (const auto &nn : uncolorable_set) {
        cout << nn.alloc_elem_to_string() << ":";
        set<alloc_elem_t> nn_set = get_alloc_elems_with_canon_elem(nn, canonical_elem, m_elems);
        for (const auto &nnn : nn_set) {
          cout << " " << nnn.alloc_elem_to_string();
        }
        cout << ":";
        for (const auto &v : canon_constraints.at(nn)) {
          cout << " " << v;
        }
        cout << endl;
      }
      cout << "candidates.size() = " << candidates.size() << endl;
    );
    ASSERT(candidates.size() > 0);
    set<alloc_elem_t> uncolorable_set_elems;
    for (const auto &nn : uncolorable_set) {
      set<alloc_elem_t> nn_set = get_alloc_elems_with_canon_elem(nn, canonical_elem, m_elems);
      set_union(uncolorable_set_elems, nn_set);
    }
    vector<alloc_elem_eq_mapping_t> trunc_candidates = truncate_relax_candidates_based_on_cost_and_constraints(candidates, canon_constraints, canonical_elem);
    //size_t uncolorable_set_size = uncolorable_set.begin()->second.size();
    size_t min_partition_size/* = uncolorable_set_size*/;
    alloc_elem_eq_mapping_t const *min_candidate = NULL;
    cost_t min_partition_cost;
    for (const auto &candidate : trunc_candidates) {
      CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": relax candidate: " << candidate.alloc_elem_eq_mapping_to_string() << endl);
      size_t partition_size = get_max_partition_size_of_uncolorable_set_partitions_if_eq_edge_is_relaxed(candidate, uncolorable_set_elems);
      ASSERT(partition_size);
      CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": relax candidate: " << candidate.alloc_elem_eq_mapping_to_string() << ": partition_size = " << partition_size << endl);
      cost_t partition_cost = candidate.alloc_elem_eq_mapping_get_cost();
      if (   !min_candidate
          || partition_size < min_partition_size
          || (partition_size == min_partition_size && partition_cost < min_partition_cost)) {
        min_partition_size = partition_size;
        min_candidate = &candidate;
        min_partition_cost = partition_cost;
      }
    }
    //ASSERT(min_partition_size >= uncolorable_set_size/2);
    ASSERT(min_candidate);

    return *min_candidate;
  }

  //void alloc_constraints_solve_for_constraints_without_untenable_cycles(std::map<prog_point_t, alloc_map_t<VALTYPE>> &ssm)
  //{
  //}
  class eq_edge_sort_on_edge_erase_cost_t //at some point, we need to incorporate information like flow, register pressure, etc.
  {
  private:
    map<alloc_elem_t, set<VALTYPE>> const &m_constraints;
    list<pair<alloc_elem_t, alloc_elem_t>> const &m_path;

    static bool edge_represents_singleton_constraint_pair(pair<alloc_elem_t, alloc_elem_t> const &edge, map<alloc_elem_t, set<VALTYPE>> const &constraints)
    {
      return    constraints.count(edge.first)
             && constraints.at(edge.first).size() == 1
             && constraints.count(edge.second)
             && constraints.at(edge.second).size() == 1;
    }
    static long path_distance_to_edge_referring_to_different_regs(pair<alloc_elem_t, alloc_elem_t> const &edge, list<pair<alloc_elem_t, alloc_elem_t>> const &path)
    {
      long index_seen_edge = LONG_MIN;
      long i = 0;
      for (const auto &e : path) {
        if (e == edge) {
          index_seen_edge = i;
        }
        i++;
      }
      ASSERT(index_seen_edge != LONG_MIN);
      long min_distance = LONG_MAX;
      i = 0;
      for (const auto &e : path) {
        if (alloc_elems_refer_to_different_regs(e.first, e.second)) {
          long distance;
          if (index_seen_edge < i) {
            distance = i - index_seen_edge;
          } else {
            distance = i - index_seen_edge + path.size();
          }
          if (distance < min_distance) {
            min_distance = distance;
          }
        }
        i++;
      }
      ASSERT(min_distance != LONG_MAX);
      //long ret = index_seen_edge_referring_to_different_regs - index_seen_edge; //we want the edge which is the closest predecessor of the edge referring to different regs
      //cout << __func__ << " " << __LINE__ << ": returning " << min_distance << " for " << edge.first.alloc_elem_to_string() << "-->" << edge.second.alloc_elem_to_string() << " (index_seen_edge " << index_seen_edge << ")" << endl;
      return min_distance; //ret;
    }
    //static bool edge_is_fallthrough(pair<alloc_elem_t, alloc_elem_t> const &e)
    //{
    //  return e.second.get_prog_point().get_pc() == e.first.get_prog_point().get_pc() + 1;
    //}

  public:
    eq_edge_sort_on_edge_erase_cost_t(map<alloc_elem_t, set<VALTYPE>> const &constraints, list<pair<alloc_elem_t, alloc_elem_t>> const &path) : m_constraints(constraints), m_path(path) {}
    bool operator()(pair<alloc_elem_t, alloc_elem_t> const &a, pair<alloc_elem_t, alloc_elem_t> const &b)
    {
      //edges where both nodes have singleton constraints are meaningless to erase; they should have a high cost
      if (edge_represents_singleton_constraint_pair(a, m_constraints) && !edge_represents_singleton_constraint_pair(b, m_constraints)) {
        return false;
      }
      if (!edge_represents_singleton_constraint_pair(a, m_constraints) && edge_represents_singleton_constraint_pair(b, m_constraints)) {
        return true;
      }
      //compute shortest distance of both a and b to an edge where alloc_elems refer to different regs, and order based on that
      long adist = path_distance_to_edge_referring_to_different_regs(a, m_path);
      long bdist = path_distance_to_edge_referring_to_different_regs(b, m_path);
      return adist < bdist;
    }
  };

  static list<pair<alloc_elem_t, alloc_elem_t>>
  path_eliminate_edges_across_same_pc(list<pair<alloc_elem_t, alloc_elem_t>> const &path)
  {
    list<pair<alloc_elem_t, alloc_elem_t>> ret;
    for (const auto &e : path) {
      if (!e.first.get_prog_point().prog_point_has_same_pc_with(e.second.get_prog_point())) {
        ret.push_back(e);
      }
    }
    return ret;
  }

  //bool regname_belongs_to_pc_def(alloc_regname_t const &regname, src_ulong pc) const
  //{
  //  if (regname.alloc_regname_is_dst_temporary()) {
  //    return false;
  //  }
  //  src_insn_t insn;
  //  bool fetch = src_insn_fetch(&insn, m_ti->in, pc, NULL);
  //  ASSERT(fetch);
  //  regset_t use, def;
  //  src_insn_get_usedef_regs(&insn, &use, &def);
  //  return regset_belongs_ex(&def, regname.alloc_regname_get_group(), regname.alloc_regname_get_reg_id());
  //}

  bool edge_is_non_phi(alloc_elem_eq_mapping_t const &e) const
  {
#if ARCH_SRC == ARCH_ETFG
    prog_point_t const &from_pp = e.get_first().get_prog_point();
    prog_point_t const &to_pp = e.get_second().get_prog_point();
    src_ulong from_pc = from_pp.get_pc();
    src_ulong to_pc = to_pp.get_pc();
    bool ret = m_ti ? !etfg_input_exec_edge_represents_phi_edge(m_ti->in, from_pc, to_pc) : false;
    //cout << __func__ << " " << __LINE__ << ": e = " << e.alloc_elem_eq_mapping_to_string() << ", ret = " << ret << endl;
    return ret;
#else
    return true;
#endif
  }

  static alloc_elem_eq_mapping_t const *path_find_mincost_edge(list<alloc_elem_eq_mapping_t const *> const &path)
  {
    ASSERT(path.size() > 0);
    auto ret = *path.begin();
    cost_t mincost = ret->alloc_elem_eq_mapping_get_cost();
    for (auto iter : path) {
      if (iter->alloc_elem_eq_mapping_get_cost() < mincost) {
        mincost = iter->alloc_elem_eq_mapping_get_cost();
        ret = iter;
      }
    }
    return ret;
  }

  static transmap_t const *
  ti_get_peep_transmap_for_program_point(translation_instance_t const *ti, prog_point_t const &pp)
  {
    src_ulong pc = pp.get_pc();
    prog_point_t::inout_t inout = pp.get_inout();
    int outnum = pp.get_outnum();
    input_exec_t const *in = ti->in;
    long idx = pc2index(in, pc);
    ASSERT(idx >= 0 && idx < in->num_si);
    transmap_t const *tmap;
    if (inout == prog_point_t::IN) {
      tmap = &ti->peep_in_tmap[idx];
    } else {
      tmap = &ti->peep_out_tmaps[idx][outnum];
    }
    return tmap;
  }
  static bool
  ti_regname_is_live_at_pp(translation_instance_t const *ti, prog_point_t const &pp, alloc_regname_t const &r)
  {
    if (r.alloc_regname_is_dst_temporary()) {
      return false;
    }
    if (!ti) {
      return true;
    }
    exreg_group_id_t group = r.alloc_regname_get_group();
    reg_identifier_t const &reg = r.alloc_regname_get_reg_id();
    transmap_t const *tmap = ti_get_peep_transmap_for_program_point(ti, pp);
    return tmap->extable.count(group) && tmap->extable.at(group).count(reg);
  }

  static size_t
  ti_get_num_allocated_regs_at_pp(translation_instance_t const *ti, prog_point_t const &pp, exreg_group_id_t dst_group)
  {
    if (!ti) {
      return 0;
    }
    transmap_t const *tmap = ti_get_peep_transmap_for_program_point(ti, pp);
    return tmap->transmap_count_dst_regs_for_group(dst_group);
  }

  static bool alloc_elem_eq_mapping_represents_phi_edge(input_exec_t const *in, alloc_elem_eq_mapping_t const *eqm)
  {
#if ARCH_SRC == ARCH_ETFG
    src_ulong from_pc = eqm->get_first().get_prog_point().get_pc();
    src_ulong to_pc = eqm->get_second().get_prog_point().get_pc();
    return etfg_input_exec_edge_represents_phi_edge(in, from_pc, to_pc);
#else
    return false;
#endif
  }

  static bool alloc_elem_eq_mappings_have_common_prog_point(alloc_elem_eq_mapping_t const *a, alloc_elem_eq_mapping_t const *b)
  {
    return    a->get_first().get_prog_point() == b->get_second().get_prog_point()
           || a->get_second().get_prog_point() == b->get_second().get_prog_point()
           || a->get_first().get_prog_point() == b->get_first().get_prog_point()
           || a->get_second().get_prog_point() == b->get_first().get_prog_point()
    ;
  }

  static prog_point_t const &alloc_elem_eq_mappings_get_common_prog_point(alloc_elem_eq_mapping_t const *a, alloc_elem_eq_mapping_t const *b)
  {
    if (   a->get_first().get_prog_point() == b->get_second().get_prog_point()
        || a->get_first().get_prog_point() == b->get_first().get_prog_point()) {
      return a->get_first().get_prog_point();
    }
    if (   a->get_second().get_prog_point() == b->get_second().get_prog_point()
        || a->get_second().get_prog_point() == b->get_first().get_prog_point()) {
      return a->get_second().get_prog_point();
    }
    NOT_REACHED();
  }

  static bool alloc_elem_eq_mapping_has_same_pc(alloc_elem_eq_mapping_t const *a)
  {
    return a->get_first().get_prog_point().get_pc() == a->get_second().get_prog_point().get_pc();
  }

  static bool
  path_is_forward_direction(list<alloc_elem_eq_mapping_t const *> const &path)
  {
    if (path.size() <= 1) {
      return true;
    }
    auto iter = path.begin();
    auto e1 = *iter;
    iter++;
    auto e2 = *iter;
    if (!alloc_elem_eq_mappings_have_common_prog_point(e1, e2)) {
      return true;
    }
    prog_point_t const &pp = alloc_elem_eq_mappings_get_common_prog_point(e1, e2);
    if (alloc_elem_eq_mapping_has_same_pc(e1)) {
      return pp.get_inout() == prog_point_t::OUT;
    } else {
      if (!alloc_elem_eq_mapping_has_same_pc(e2)) {
        return true; //probably the cycle is due to regcons; just return true
      } else {
        return pp.get_inout() == prog_point_t::IN;
      }
    }
  }

  alloc_elem_eq_mapping_t const *path_find_eq_edge_to_remove(list<alloc_elem_eq_mapping_t const *> const &path) const
  {
    ASSERT(path.size() > 0);
    alloc_elem_eq_mapping_t const *first_eqm = *path.begin();
    alloc_elem_eq_mapping_t const *last_eqm = *path.rbegin();
    if (alloc_elem_eq_mappings_have_common_prog_point(first_eqm, last_eqm)) {
      alloc_regname_t const &reg1 = first_eqm->get_first().get_regname();
      alloc_regname_t const &reg2 = last_eqm->get_second().get_regname();
      //cout << __func__ << " "  << __LINE__ << ": regname = " << regname.alloc_regname_to_string() << endl;
      auto ti = m_ti;
      std::function<vector<cost_t> (alloc_elem_eq_mapping_t const *)> get_break_cost =
          [&reg1, &reg2, ti, &path](alloc_elem_eq_mapping_t const *eqm)
      {
        vector<cost_t> ret;
        //ret.push_back(eqm->get_cost()); //prefer edges which have a lower flow cost //XXX: flow not computed precisely currently
        ret.push_back(eqm->alloc_elem_eq_mapping_get_cost() == COST_INFINITY); //prefer edges which have cost != INFINITY
        bool at_least_one_reg_is_dead_at_pc =
                                       !ti_regname_is_live_at_pp(ti, eqm->get_first().get_prog_point(), reg1)
                                    || !ti_regname_is_live_at_pp(ti, eqm->get_first().get_prog_point(), reg2);
        ret.push_back(!at_least_one_reg_is_dead_at_pc); //prefer edges which have at least one reg dead (because such edges are likely to resolve conflicts at multiple pcs between reg1 and reg2)
        ret.push_back(ti ? alloc_elem_eq_mapping_represents_phi_edge(ti->in, eqm) : false); //prefer non-phi edges
	ret.push_back(  ti_get_num_allocated_regs_at_pp(ti, eqm->get_first().get_prog_point(), reg1.alloc_regname_get_group())
	              + ti_get_num_allocated_regs_at_pp(ti, eqm->get_second().get_prog_point(), reg1.alloc_regname_get_group()));
        if (path_is_forward_direction(path)) {
          ret.push_back(list_get_index(path, eqm)); //prefer edges closer to the conflicting node.
        } else {
          ret.push_back(path.size() - list_get_index(path, eqm)); //prefer edges closer to the conflicting node.
        }
        //cout << __func__ << " " << __LINE__ << ": eqm = " << eqm->alloc_elem_eq_mapping_to_string() << ": cost vector:";
        //for (const auto &c : ret) {
        //  cout << " " << c;
        //}
        //cout << endl;
        return ret;
      };
      alloc_elem_eq_mapping_t const *m = list_find_min<alloc_elem_eq_mapping_t const *, vector<cost_t>>(path, get_break_cost);
      if (m->alloc_elem_eq_mapping_get_cost() == COST_INFINITY) {
        cout << __func__ << " " << __LINE__ << ": all edges in untenable cycle are COST_INFINITY: " << path_to_string(path) << endl;
        cout << __func__ << " " << __LINE__ << ": this =\n" << this->alloc_constraints_to_string() << endl;
      }
      ASSERT(m->alloc_elem_eq_mapping_get_cost() != COST_INFINITY);
      return m;
    }
    alloc_elem_eq_mapping_t const *mincost_edge = path_find_mincost_edge(path);
    if (mincost_edge->alloc_elem_eq_mapping_get_cost() == COST_INFINITY) {
      cout << __func__ << " " << __LINE__ << ": all edges in untenable cycle are COST_INFINITY: " << path_to_string(path) << endl;
      cout << __func__ << " " << __LINE__ << ": this =\n" << this->alloc_constraints_to_string() << endl;
    }
    ASSERT(mincost_edge->alloc_elem_eq_mapping_get_cost() != COST_INFINITY);
    return mincost_edge;
    //auto prev = path.back();
    //for (auto cur = path.begin(); cur != path.end(); cur++) {
    //  if (alloc_elems_refer_to_different_regs(cur->first, cur->second)) {
    //    //return prev;
    //    return *cur;
    //  }
    //  prev = *cur;
    //}
    //NOT_REACHED();
  }

  static bool alloc_elems_refer_to_different_regs(alloc_elem_t const &a, alloc_elem_t const &b)
  {
    //return    a.get_regname().first != b.get_regname().first
    //       || a.get_regname().second != b.get_regname().second;

    bool ret = !(a.get_regname().equals_mod_phi_modifier(b.get_regname()));
    //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << a.alloc_elem_to_string() << " <-> " << b.alloc_elem_to_string() << endl;
    return ret;
  }

  static string path_to_string(list<alloc_elem_eq_mapping_t const *> const &path)
  {
    stringstream ss;
    ASSERT(path.size() > 0);
    //ss << (*path.begin())->get_first().alloc_elem_to_string();
    for (const auto &e : path) {
      //ss << " -> " << e->get_second().alloc_elem_to_string() << "[" << e->get_cost() << "]";
      ss << e->alloc_elem_eq_mapping_to_string() << " -> ";
    }
    ss << endl;
    return ss.str();
  }

  static string edges_to_string(list<pair<alloc_elem_t, alloc_elem_t>> const &path)
  {
    stringstream ss;
    ASSERT(path.size() > 0);
    for (const auto &e : path) {
      ss << e.first.alloc_elem_to_string();
      ss << " -> " << e.second.alloc_elem_to_string() << "; ";
    }
    ss << endl;
    return ss.str();
  }

  static void alloc_map_mappings_union(std::map<prog_point_t, alloc_map_t<VALTYPE>> &dst, std::map<prog_point_t, alloc_map_t<VALTYPE>> const &src)
  {
    for (const auto &m : src) {
      dst[m.first].alloc_map_union(m.second);
    }
  }

  alloc_constraints_t<VALTYPE>
  get_subgraph_belonging_to_connected_component(distinct_set_t<alloc_elem_t> const &connected_components, alloc_elem_t const &root) const
  {
    std::set<alloc_elem_t> elems;
    std::set<alloc_elem_eq_mapping_t> eq_mappings;
    std::set<std::pair<alloc_elem_t, alloc_elem_t>> negative_mappings;
    std::map<alloc_elem_t, std::set<VALTYPE>> constraints;
    for (const auto &e : m_elems) {
      if (connected_components.distinct_set_find(e) == root) {
        elems.insert(e);
      }
    }
    for (const auto &eqm : m_eq_mappings) {
      if (connected_components.distinct_set_find(eqm.get_first()) == root) {
        ASSERT(connected_components.distinct_set_find(eqm.get_second()) == root);
        eq_mappings.insert(eqm);
      }
    }
    for (const auto &nm : m_negative_mappings) {
      if (connected_components.distinct_set_find(nm.first) == root) {
        ASSERT(connected_components.distinct_set_find(nm.second) == root);
        negative_mappings.insert(nm);
      }
    }
    for (const auto &c : m_constraints) {
      if (connected_components.distinct_set_find(c.first) == root) {
        constraints.insert(c);
      }
    }
    return alloc_constraints_t<VALTYPE>(m_ti, m_loc_id, eq_mappings, negative_mappings, constraints);
  }

  vector<alloc_constraints_t<VALTYPE>> identify_connected_components() const
  {
    vector<alloc_constraints_t<VALTYPE>> ret;
    distinct_set_t<alloc_elem_t> connected_components;
    for (const auto &e : m_elems) {
      connected_components.distinct_set_make(e);
    }
    for (const auto &eqm : m_eq_mappings) {
      connected_components.distinct_set_union(eqm.get_first(), eqm.get_second());
    }
    for (const auto &nm : m_negative_mappings) {
      connected_components.distinct_set_union(nm.first, nm.second);
    }
    set<alloc_elem_t> roots;
    for (const auto &e : m_elems) {
      roots.insert(connected_components.distinct_set_find(e));
    }
    //cout << __func__ << " " << __LINE__ << ": roots.size() = " << roots.size() << endl;
    for (const auto &r : roots) {
      alloc_constraints_t<VALTYPE> acv = get_subgraph_belonging_to_connected_component(connected_components, r);
      ret.push_back(acv);
    }
    return ret;
  }

public:
  alloc_constraints_t(translation_instance_t const *ti, transmap_loc_id_t loc_id) : m_ti(ti), m_loc_id(loc_id) { ASSERT(ti); }
  alloc_constraints_t(translation_instance_t const *ti, transmap_loc_id_t loc_id, std::set<alloc_elem_eq_mapping_t> const &eq_mappings, std::set<std::pair<alloc_elem_t, alloc_elem_t>> const &negative_mappings, std::map<alloc_elem_t, std::set<VALTYPE>> const &constraints) : m_ti(ti), m_eq_mappings(eq_mappings), m_negative_mappings(negative_mappings), m_constraints(constraints), m_loc_id(loc_id)
  {
    for (const auto &eqm : m_eq_mappings) {
      m_elems.insert(eqm.get_first());
      m_elems.insert(eqm.get_second());
    }
    for (const auto &nm : m_negative_mappings) {
      m_elems.insert(nm.first);
      m_elems.insert(nm.second);
    }
    for (const auto &c : m_constraints) {
      m_elems.insert(c.first);
    }
  }
  void add_elem(alloc_elem_t const &a)
  {
    m_elems.insert(a);
  }
  void ssc_add_mapping(alloc_elem_t const &a, alloc_elem_t const &b, cost_t cost)
  {
    if (!m_elems.count(a)) {
      m_elems.insert(a);
    }
    if (!m_elems.count(b)) {
      m_elems.insert(b);
    }
    if (a < b) {
      m_eq_mappings.insert(alloc_elem_eq_mapping_t(a, b, cost));
    } else {
      m_eq_mappings.insert(alloc_elem_eq_mapping_t(b, a, cost));
    }
  }
  void add_constraint(alloc_elem_t const &a, set<VALTYPE> const &off)
  {
    if (!m_elems.count(a)) {
      m_elems.insert(a);
    }
    m_constraints.insert(make_pair(a, off));
  }
  void add_negative_mapping(alloc_elem_t const &a, alloc_elem_t const &b)
  {
    if (!m_elems.count(a)) {
      m_elems.insert(a);
    }
    if (!m_elems.count(b)) {
      m_elems.insert(b);
    }

    if (a < b) {
      m_negative_mappings.insert(std::make_pair(a, b));
    } else {
      m_negative_mappings.insert(std::make_pair(b, a));
    }
  }
  static bool
  graph_color_for_edges(std::set<alloc_elem_t> const &nodes, std::set<std::pair<alloc_elem_t, alloc_elem_t>> const &neq_edges, std::map<alloc_elem_t, std::set<VALTYPE>> const &constraints, transmap_loc_id_t loc_id, std::map<alloc_elem_t, VALTYPE> &color_map, vector<alloc_elem_t> &uncolorable_set)
  {
#if ARCH_SRC == ARCH_DST
    if (loc_id != TMAP_LOC_SYMBOL) {
      //hack: just name all registers identically!
      //std::map<alloc_elem_t, VALTYPE> ret;
      for (const auto &n : nodes) {
	VALTYPE v = VALTYPE::get_default_object(n.get_regname().alloc_regname_get_reg_id().reg_identifier_get_id());
        color_map.insert(make_pair(n, v));
      }
      return true;
    }
#endif
    ugraph_t<alloc_elem_t> ugraph(nodes, neq_edges);
    long max_num_vals;
    if (loc_id == TMAP_LOC_SYMBOL) {
      max_num_vals = -1;
    } else {
      exreg_group_id_t dst_group = loc_id - TMAP_LOC_EXREG(0);
      max_num_vals = DST_NUM_EXREGS(dst_group);
    }
    CPP_DBG_EXEC(GRAPH_COLORING, cout << __func__ << " " << __LINE__ << ": calling ugraph_find_min_color_assignment for LOC_ID = " << (int)loc_id << endl);
    return ugraph.ugraph_find_min_color_assignment<VALTYPE>(constraints, color_map, uncolorable_set, max_num_vals);
  }



  std::string alloc_constraints_to_string() const
  {
    stringstream ss;
    ss << "eq-mappings: " << m_eq_mappings.size() << "\n";
    for (const auto &e : m_eq_mappings) {
      ss << "loc " << (int)m_loc_id << ": offset[ " << e.get_first().alloc_elem_to_string() << " ] == offset[ " << e.get_second().alloc_elem_to_string() << " ] cost " << e.alloc_elem_eq_mapping_get_cost() << "\n";
    }
    ss << "constraints: " << m_constraints.size() << "\n";
    for (const auto &e : m_constraints) {
      ss << "loc " << (int)m_loc_id << ": offset[ " << e.first.alloc_elem_to_string() << " ] = ";
      for (const auto &v : e.second) {
        ss << v << "; ";
      }
      ss << endl;
    }
    ss << "negative-mappings: " << m_negative_mappings.size() << "\n";
    for (const auto &e : m_negative_mappings) {
      ss << "loc " << (int)m_loc_id << ": offset[ " << e.first.alloc_elem_to_string() << " ] != offset[ " << e.second.alloc_elem_to_string() << " ]\n";
    }
    return ss.str();
  }


  static alloc_constraints_t alloc_constraints_from_string(string const &str)
  {
    istringstream iss(str);
    transmap_loc_id_t loc_id = 0;
    string word;
    size_t n;
    char ch;
    iss >> word;
    ASSERT(word == "eq-mappings:");
    iss >> n;
    std::set<alloc_elem_eq_mapping_t> eq_mappings;
    //m_eq_mappings.clear();
    for (size_t i = 0; i < n; i++) {
      int loc_id_int;
      iss >> word >> loc_id_int >> ch;
      ASSERT(word == "loc");
      ASSERT(loc_id == 0 || loc_id == loc_id_int);
      loc_id = loc_id_int;
      //cout << "loc_id = " << loc_id << endl;
      //cout << "ch = " << ch << endl;
      ASSERT(ch == ':');
      alloc_elem_eq_mapping_t eqm = alloc_elem_eq_mapping_t::alloc_elem_eq_mapping_from_stream(iss, '\n');
      eq_mappings.insert(eqm);
    }
    iss >> word;
    ASSERT(word == "constraints:");
    iss >> n;
    //m_constraints.clear();
    std::map<alloc_elem_t, std::set<VALTYPE>> constraints;
    for (size_t i = 0; i < n; i++) {
      int loc_id_int;
      iss >> word >> loc_id_int >> ch;
      ASSERT(word == "loc");
      ASSERT(loc_id == 0 || loc_id == loc_id_int);
      loc_id = loc_id_int;
      ASSERT(ch == ':');
      iss >> word;
      ASSERT(word == "offset[");
      string off_str;
      while ((iss >> ch) && ch != ']') {
        off_str.append(1, ch);
      }
      alloc_elem_t e1 = alloc_elem_t::alloc_elem_from_string(off_str);
      iss >> ch;
      ASSERT(ch == '=');
      set<VALTYPE> vset;
      do {
        string val_str;
        bool found_newline; 
        found_newline = false;
        while ((iss.get(ch)) && (ch != ';')) {
          //cout << "ch = " << ch << ".\n";
          if (ch == '\n') {
            found_newline = true;
            break;
          }
          val_str.append(1, ch);
        }
        //cout << "ch = " << ch << ".\n";
        if (found_newline) {
          //cout << __func__ << " " << __LINE__ << ": found newline\n";
          break;
        }
        //cout << __func__ << " " << __LINE__ << ": val_str = " << val_str << endl;
        VALTYPE v = VALTYPE::from_string(val_str);
        vset.insert(v);
      } while (1);
      constraints.insert(make_pair(e1, vset));
    }
    iss >> word;
    ASSERT(word == "negative-mappings:");
    iss >> n;
    //m_negative_mappings.clear();
    std::set<std::pair<alloc_elem_t, alloc_elem_t>> negative_mappings;
    for (size_t i = 0; i < n; i++) {
      int loc_id_int;
      iss >> word >> loc_id_int >> ch;
      ASSERT(word == "loc");
      ASSERT(loc_id == 0 || loc_id == loc_id_int);
      loc_id = loc_id_int;
      ASSERT(ch == ':');
      iss >> word;
      ASSERT(word == "offset[");
      string off_str;
      while ((iss >> ch) && ch != ']') {
        off_str.append(1, ch);
      }
      alloc_elem_t e1 = alloc_elem_t::alloc_elem_from_string(off_str);
      iss >> word;
      ASSERT(word == "!=");

      iss >> word;
      ASSERT(word == "offset[");
      off_str = "";
      while ((iss >> ch) && ch != ']') {
        off_str.append(1, ch);
      }
      alloc_elem_t e2 = alloc_elem_t::alloc_elem_from_string(off_str);
      negative_mappings.insert(make_pair(e1, e2));
    }
    return alloc_constraints_t(NULL, loc_id, eq_mappings, negative_mappings, constraints);
  }

  void solve_for_connected_component(std::map<prog_point_t, alloc_map_t<VALTYPE>> &c_ssm) const
  {
    alloc_constraints_t<VALTYPE> const &connected_component = *this;
    vector<alloc_elem_t> uncolorable_set;
    distinct_set_t<alloc_elem_t> canonical_elem;
    std::map<alloc_elem_t, set<VALTYPE>> canon_constraints;
    std::list<alloc_elem_eq_mapping_t> relaxed_eq_mappings;
    size_t num_constraints_relaxed = 0;
    size_t num_constraints_unrelaxed = 0;
    alloc_constraints_t<VALTYPE> new_ssc = connected_component.add_more_mappings_based_on_singular_constraints();
    new_ssc.alloc_constraints_break_untenable_cycles();
    while (!new_ssc.merge_nodes_and_color_graph_for_graph_without_untenable_cycles(c_ssm, uncolorable_set, canonical_elem, canon_constraints)) {
      alloc_elem_eq_mapping_t to_relax = new_ssc.identify_eq_mapping_to_relax_for_uncolorable_set(uncolorable_set, canonical_elem, canon_constraints);
      CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": removing eq constraint to facilitate graph coloring: " << to_relax.get_first().alloc_elem_to_string() << " --> " << to_relax.get_second().alloc_elem_to_string() << endl);
      relaxed_eq_mappings.push_front(to_relax);
      new_ssc.m_eq_mappings.erase(to_relax);
      num_constraints_relaxed++;
    }
    //now run the optimistic coloring logic by trying to add the relaxed constraints back
    list<alloc_elem_eq_mapping_t> constraints_relaxed_net;
    if (relaxed_eq_mappings.size() > 0) {
      auto iter = relaxed_eq_mappings.begin();
      for (iter++; iter != relaxed_eq_mappings.end(); iter++) { //ignore the first mapping because adding that back is definitely going to make the graph uncolorable
        CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": trying to add back eq constraint (optimistic coloring): " << iter->get_first().alloc_elem_to_string() << " --> " << iter->get_second().alloc_elem_to_string() << endl);
        new_ssc.ssc_add_mapping(iter->get_first(), iter->get_second(), iter->alloc_elem_eq_mapping_get_cost());
        map<prog_point_t, alloc_map_t<VALTYPE>> c_ssm2;
        if (new_ssc.merge_nodes_and_color_graph_for_graph_without_untenable_cycles(c_ssm2, uncolorable_set, canonical_elem, canon_constraints)) {
          CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": adding back eq constraint (optimistic coloring): was able to color after adding the constraint back: " << iter->get_first().alloc_elem_to_string() << " --> " << iter->get_second().alloc_elem_to_string() << endl);
          c_ssm = c_ssm2;
          num_constraints_unrelaxed++;
        } else {
          new_ssc.m_eq_mappings.erase(*iter); //erase if not colorable after adding.
          constraints_relaxed_net.push_back(*iter);
        }
      }
    }
    CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS,
      cout << __func__ << " " << __LINE__ << ": num_constraints_relaxed = " << num_constraints_relaxed << ", num_constraints_unrelaxed = " << num_constraints_unrelaxed << ", num_constraints_relaxed_net = " << (num_constraints_relaxed - num_constraints_unrelaxed) << ", relaxed constraints net:\n";
      for (const auto &eqm : constraints_relaxed_net) {
        cout << eqm.alloc_elem_eq_mapping_to_string() << endl;
      }
      cout << endl;
    );
  }

  void solve(std::map<prog_point_t, alloc_map_t<VALTYPE>> &ssm) const
  {
    CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": solving alloc constraints =\n" << this->alloc_constraints_to_string() << endl);
    //alloc_constraints_t<VALTYPE> new_ssc = this->alloc_constraints_break_untenable_cycles();
    //new_ssc.alloc_constraints_solve_for_constraints_without_untenable_cycles(ssm);
    vector<alloc_constraints_t<VALTYPE>> connected_components = this->identify_connected_components();

    //cout << __func__ << " " << __LINE__ << ": num connected components = " << connected_components.size() << endl;
    for (const auto &connected_component : connected_components) {
      map<prog_point_t, alloc_map_t<VALTYPE>> c_ssm;
      connected_component.solve_for_connected_component(c_ssm);
      alloc_map_mappings_union(ssm, c_ssm);
    }
  }

};

#endif
