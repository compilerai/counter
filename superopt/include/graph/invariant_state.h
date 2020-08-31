#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "graph/point_set.h"
#include "graph/invariant_state_eqclass.h"
#include "graph/invariant_state_eqclass_bv.h"
#include "graph/invariant_state_eqclass_arr.h"
#include "graph/invariant_state_eqclass_ineq.h"
#include "graph/invariant_state_eqclass_ineq_loose.h"
#include "graph/invariant_state_eqclass_houdini.h"
#include "graph/eqclasses.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_t
{
private:
  eqclasses_ref m_expr_eqclasses;
  list<invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *> m_eqclasses;
  //graph_with_ce<T_PC, T_N, T_E, T_PRED> const *m_graph;
  bool m_is_top;
  bool m_is_stable; // not stable => bottom

  //caches
  //mutable bool m_need_to_recompute_preds; //cache does not help because we sometimes need to get the preds in the middle of eqclass xfer function
  //mutable unordered_set<T_PRED> m_preds;
public:
  invariant_state_t() : m_expr_eqclasses(nullptr), m_is_top(true), //default constructor creates TOP
                        m_is_stable(true)
  {
    stats_num_invariant_state_constructions++;
  }
  invariant_state_t(invariant_state_t const& other) : m_expr_eqclasses(other.m_expr_eqclasses), m_is_top(other.m_is_top), m_is_stable(other.m_is_stable)
  {
    stats_num_invariant_state_constructions++;
    for (auto const& eqclass : other.m_eqclasses) {
      m_eqclasses.push_back(eqclass->clone());
    }
  }

  ~invariant_state_t()
  {
    stats_num_invariant_state_destructions++;
    for (auto eqclass : m_eqclasses) {
      delete eqclass;
    }
  }

  static invariant_state_t
  pc_invariant_state(T_PC const &p, eqclasses_ref const &eqclasses, map<graph_loc_id_t, graph_cp_location> const& locs, context *ctx)
  {
    return invariant_state_t(p, eqclasses, locs, ctx);
  }

  static invariant_state_t
  empty_invariant_state(context *ctx)
  {
    return invariant_state_t(ctx/*g*/);
    //empty m_eqclasses
    //indicate TRUE
  }

  static invariant_state_t
  top()
  {
    invariant_state_t ret;
    ASSERT(ret.m_is_top);
    return ret;
  }

  eqclasses_ref const& get_expr_eqclasses() const { return m_expr_eqclasses; }

  void operator=(invariant_state_t const &other)
  {
    ASSERT(!other.m_is_top);
    m_eqclasses.clear();
    for (auto const& eqclass : other.m_eqclasses) {
      m_eqclasses.push_back(eqclass->clone());
    }
    m_is_top = other.m_is_top;
    m_is_stable = other.m_is_stable;
  }

  bool 
  xfer_on_incoming_edge(shared_ptr<T_E const> const &e, invariant_state_t const &from_pc_invariants, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g)
  {
    ASSERT(!this->m_is_top);
    //CPP_DBG_EXEC(INVARIANT_STATE, cout << __func__ << ':' << __LINE__ << ": " << timestamp() << ": in: " << &from_pc_invariants << '\n');
    // if predecessor is unstable then mark self as unstable
    if (!from_pc_invariants.m_is_stable) {
      bool already_unstable = !this->m_is_stable;
      this->m_is_stable = false;
      return !already_unstable;
    }

    // early exit if already bottom (or unstable)
    if (!this->m_is_stable) {
      return  false;
    }

    bool need_stability_at_this_pc = g.need_stability_at_pc(e->get_to_pc());
    bool changed = false;
    for (auto &eqclass : m_eqclasses) {
      changed = eqclass->eqclass_xfer_on_incoming_edge(e, from_pc_invariants, g) || changed;
      if (   need_stability_at_this_pc
          && !eqclass->check_stability()) {
        this->m_is_stable = false;
        DYN_DEBUG(eqcheck, cout << __func__ << ':' << __LINE__ << ": stability check failed for eqclass type " << expr_group_t::expr_group_type_to_string(eqclass->get_exprs_to_be_correlated()->get_type()) << " at " << e->get_to_pc().to_string() << endl);
        return true;
      }
    }
    return changed;
  }

  invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> const&
  get_eqclass_for_expr_group(expr_group_ref const &expr_group) const
  {
    for (auto const& eqclass : m_eqclasses) {
      if (eqclass->get_exprs_to_be_correlated() == expr_group) {
        return *eqclass;
      }
    }
    NOT_REACHED();
  }

  list<invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> const*>
  get_eqclasses() const
  {
    list<invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> const*> ret;
    for (auto const& eqclass : m_eqclasses) {
      ret.push_back(eqclass);
    }
    return ret;
  }

  unordered_set<shared_ptr<T_PRED const>>
  get_all_preds() const
  {
    ASSERT(!this->m_is_top);
    if (!this->m_is_stable) {
      return unordered_set<shared_ptr<T_PRED const>>();
    }
    unordered_set<shared_ptr<T_PRED const>> ret;
    for (auto const& eqclass : m_eqclasses) {
      unordered_set_union(ret, eqclass->get_preds());
    }
    return ret;
  }

  bool operator!=(invariant_state_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    ASSERT(!this->m_is_top);
    ASSERT(!other.m_is_top);
    ASSERT(m_eqclasses.size() == other.m_eqclasses.size());
    typename list<invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *>::const_iterator iter1, iter2;
    for (iter1 = m_eqclasses.cbegin(), iter2 = other.m_eqclasses.cbegin();
         iter1 != m_eqclasses.cend() && iter2 != other.m_eqclasses.cend();
         iter1++, iter2++) {
      if (*(*iter1) != *(*iter2)) {
        return true;
      }
    }
    return this->m_is_stable != other.m_is_stable;
  }

  string invariant_state_to_string(/*T_PC const &p, */string prefix, bool full = false) const;
  void invariant_state_get_asm_annotation(ostream& os) const;

  bool invariant_state_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g)
  {
    autostop_timer ft(__func__);
    ASSERT(!this->m_is_top);
    if (!this->m_is_stable) {
    	return false;
    }
    bool ret = false;
    for (auto &eqclass : m_eqclasses) {
      bool eret = eqclass->invariant_state_eqclass_add_point_using_CE(nce, g);
      if (!eqclass->check_stability()) {
        ASSERT(eret);
        this->m_is_stable = false;
        DYN_DEBUG(eqcheck, cout << __func__ << ':' << __LINE__ << ": stability check failed for eqclass type " << expr_group_t::expr_group_type_to_string(eqclass->get_exprs_to_be_correlated()->get_type()) << endl);
      }
      ret = ret || eret;
    }
    return ret;
  }

  bool invariant_state_is_well_formed() const
  {
    size_t eqclass_id = 0;
    for (auto const& eqclass : m_eqclasses) {
      if (!eqclass->invariant_state_eqclass_is_well_formed()) {
        cout << __func__ << " " << __LINE__ << ": returning false for eqclass id " << eqclass_id << endl;
        return false;
      }
      eqclass_id++;
    }
    return true;
  }

  eqclasses_ref invariant_state_get_expr_eqclasses() const
  {
    list<expr_group_ref> expr_groups;
    for (auto const& invstate_eqclass : m_eqclasses) {
      expr_groups.push_back(invstate_eqclass->get_exprs_to_be_correlated());
    }
    return mk_eqclasses(expr_groups);
  }

  bool is_stable() const { return m_is_stable; }

  void invariant_state_from_stream(istream& is/*, state const& start_state*/, context* ctx, string const& inprefix, map<graph_loc_id_t, graph_cp_location> const& locs)
  {
    string const prefix1 = "is_top ";
    string const prefix2 = "is_stable ";
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(line == "=" + inprefix + "invariant_state");
    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(string_has_prefix(line, prefix1));
    m_is_top = string_to_int(line.substr(prefix1.length()));
    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(string_has_prefix(line, prefix2));
    m_is_stable = string_to_int(line.substr(prefix2.length()));
    end = !getline(is, line);
    ASSERT(!end);
    size_t eqclass_num = 0;
    while (string_has_prefix(line, "=" + inprefix + "invariant_state_eqclass ")) {
      stringstream prefixss;
      prefixss << inprefix + "invariant_state_eqclass " << eqclass_num << " ";
      invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *eqclass;
      eqclass = invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_from_stream(is, prefixss.str()/*, start_state*/, ctx, locs);
      m_eqclasses.push_back(eqclass);
      end = !getline(is, line);
      ASSERT(!end);
      eqclass_num++;
    }
    if (line != "=" + inprefix + "invariant_state done") {
      cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    }
    ASSERT(line == "=" + inprefix + "invariant_state done");
  }

  void invariant_state_to_stream(ostream& os, string const& inprefix) const
  {
    os << "=" + inprefix + "invariant_state\nis_top " << m_is_top << "\nis_stable " << m_is_stable << "\n";
    size_t i = 0;
    for (auto const& eqclass : m_eqclasses) {
      stringstream prefixss;
      prefixss << inprefix + "invariant_state_eqclass " << i;
      os << "=" + prefixss.str() << "\n";
      eqclass->invariant_state_eqclass_to_stream(os, prefixss.str() + " ");
      i++;
    }
    os << "=" + inprefix + "invariant_state done\n";
  }
  
  void update_state_for_ranking() const
  {
    ASSERT(!this->m_is_top);
    for (auto &eqclass : m_eqclasses) {
      eqclass->update_state_for_ranking();
    }
  }

private:
  invariant_state_t(T_PC const &p, eqclasses_ref const &eqclasses, map<graph_loc_id_t, graph_cp_location> const& locs, context *ctx) : m_expr_eqclasses(eqclasses), m_is_top(false), m_is_stable(true)
  {
    stats_num_invariant_state_constructions++;
    for (auto iter_e = eqclasses->get_expr_group_ls().cbegin();
              iter_e != eqclasses->get_expr_group_ls().cend();
              iter_e++) {
      switch ((*iter_e)->get_type()) {
        case expr_group_t::EXPR_GROUP_TYPE_ARR_EQ:
          m_eqclasses.push_back(new invariant_state_eqclass_arr_t<T_PC, T_N, T_E, T_PRED>(*iter_e, locs, ctx));
          break;
        case expr_group_t::EXPR_GROUP_TYPE_BV_EQ:
          m_eqclasses.push_back(new invariant_state_eqclass_bv_t<T_PC, T_N, T_E, T_PRED>(*iter_e, ctx));
          break;
        case expr_group_t::EXPR_GROUP_TYPE_BV_INEQ:
          //m_eqclasses.push_back(new invariant_state_eqclass_ineq_t<T_PC, T_N, T_E, T_PRED>(*iter_e, ctx));
          m_eqclasses.push_back(new invariant_state_eqclass_ineq_loose_t<T_PC, T_N, T_E, T_PRED>(*iter_e, ctx));
          break;
        case expr_group_t::EXPR_GROUP_TYPE_HOUDINI:
          m_eqclasses.push_back(new invariant_state_eqclass_houdini_t<T_PC, T_N, T_E, T_PRED>(*iter_e, ctx));
          break;
        default: NOT_REACHED();
      }
    }
  }

  invariant_state_t(context *ctx) : m_expr_eqclasses(nullptr), m_is_top(false), m_is_stable(true)
  {
    stats_num_invariant_state_constructions++;
    //empty m_eqclasses
    //indicate TRUE
  }

  bool check_stability() const
  {
    for (auto const* eqclass : this->m_eqclasses) {
      if(!eqclass->check_stability()) 
        return false;
    }
    return true;
  }

};
}
