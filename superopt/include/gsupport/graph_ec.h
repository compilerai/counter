#pragma once

#include "support/manager.h"
#include "support/mytimer.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/types.h"
#include "support/serpar_composition.h"
#include "graph/te_comment.h"
#include "expr/defs.h"

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>
#include <numeric>

namespace eqspace {

class state;
class context;

template <typename T_PC, typename T_N, typename T_E> class graph;

template <typename T_PC, typename T_E> class graph_edge_composition_t;

template<typename T_PC, typename T_E>
using graph_edge_composition_ref = shared_ptr<graph_edge_composition_t<T_PC,T_E> const>;

template <typename T_PC, typename T_E>
class graph_edge_composition_t : public serpar_composition_node_t<T_E>
{
public:
  static map<set<graph_edge_composition_ref<T_PC,T_E>>, graph_edge_composition_ref<T_PC,T_E>> *get_graph_edge_composition_pterms_canonicalize_cache()
  {
    static map<set<graph_edge_composition_ref<T_PC,T_E>>, graph_edge_composition_ref<T_PC,T_E>> *ret = NULL;
    if (!ret) {
      ret = new map<set<graph_edge_composition_ref<T_PC,T_E>>, graph_edge_composition_ref<T_PC,T_E>>;
    }
    return ret;
  }
  static manager<graph_edge_composition_t<T_PC,T_E>> *get_ecomp_manager()
  {
    static manager<graph_edge_composition_t<T_PC,T_E>> *ret = NULL;
    if (!ret) {
      ret = new manager<graph_edge_composition_t<T_PC,T_E>>;
    }
    return ret;
  }
public:
  graph_edge_composition_t(enum serpar_composition_node_t<T_E>::type t, shared_ptr<graph_edge_composition_t const> const &a, shared_ptr<graph_edge_composition_t const> const &b) : serpar_composition_node_t<T_E>(t, a, b), m_is_managed(false)
  {
  }
  graph_edge_composition_t(T_E const &e) : serpar_composition_node_t<T_E>(e), m_is_managed(false)
  {
  }
  graph_edge_composition_t(shared_ptr<T_E const> const &e) : serpar_composition_node_t<T_E>(e), m_is_managed(false)
  {
  }
  bool operator==(graph_edge_composition_t const &other) const
  {
    return serpar_composition_node_t<T_E>::operator==(other);
  }
  string graph_edge_composition_to_string() const
  {
    return this->to_string_concise();
  }
  string graph_edge_composition_to_string_for_eq(string const& prefix) const
  {
    stringstream ret;
    ret << prefix << ".graph_edge_composition\n";
    ret << this->to_string_concise() << '\n';

    if (this->is_epsilon()) {
      return ret.str();
    }

    set<shared_ptr<T_E const>> visited;
    int i = 0;
    function<void (shared_ptr<T_E const> const&)> atom_f = [&ret,&prefix,&i,&visited](shared_ptr<T_E const> const& te) {
      if (set_belongs(visited, te)) {
        return;
      }
      visited.insert(te);
      ret << prefix << ".graph_edge_composition.Edge" << i++ << '\n';
      if (te->is_empty()) {
        ret << "(" << EPSILON << ")\n";
      }  else {
        ret << te->to_string_for_eq(prefix + ".Edge");
      }
    };
    this->visit_nodes_const(atom_f);

    return ret.str();
  }

  static string graph_edge_composition_from_stream(istream& in,  string const& line, string const& prefix/*, state const& start_state*/, context* ctx, graph_edge_composition_ref<T_PC,T_E>& ec);

  template<typename T_N>
  te_comment_t graph_edge_composition_get_comment(graph<T_PC,T_N,T_E> const &g) const
  {
    std::function<te_comment_t (shared_ptr<T_E const> const &)> f = [](shared_ptr<T_E const> const &e)
    {
      if (e->is_empty()) {
        return te_comment_t::te_comment_epsilon();
      }
      return e->get_te_comment();
    };

    std::function<te_comment_t (typename serpar_composition_node_t<T_E>::type, te_comment_t const &, te_comment_t const &)> f_fold =
      [/*&g*/](typename serpar_composition_node_t<T_E>::type t, te_comment_t const &l, te_comment_t const &r)
    {
      bbl_order_descriptor_t const& bbo = l.get_bbl_order_desc();
      bool is_phi = l.is_phi();
      int insn_idx = l.get_insn_idx();
      string const& lcode = l.get_code()->get_str();
      string const& rcode = r.get_code()->get_str();
      if (t == serpar_composition_node_t<T_E>::series) {
        return te_comment_t(bbo, is_phi, insn_idx, "(" + lcode + "*" + rcode + ")");
      } else if (t == serpar_composition_node_t<T_E>::parallel) {
        return te_comment_t(bbo, is_phi, insn_idx, "(" + lcode + "+" + rcode + ")");
      } else NOT_REACHED();
    };
    return this->visit_nodes(f, f_fold);
  }
  bool is_epsilon() const
  {
    return    this->get_type() == serpar_composition_node_t<T_E>::atom
           && this->get_atom()->is_empty();
  }
  graph_edge_composition_ref<T_PC,T_E> get_a_ec() const
  {
    shared_ptr<serpar_composition_node_t<T_E> const> a = this->get_a();
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a);
    ASSERT(ret);
    return ret;
  }
  graph_edge_composition_ref<T_PC,T_E> get_b_ec() const
  {
    shared_ptr<serpar_composition_node_t<T_E> const> b = this->get_b();
    graph_edge_composition_ref<T_PC,T_E> ret = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b);
    ASSERT(ret);
    return ret;
  }
  T_PC graph_edge_composition_get_from_pc() const
  {
    if (this->get_type() == serpar_composition_node_t<T_E>::atom) {
      return this->get_atom()->get_from_pc();
    } else if (this->get_type() == serpar_composition_node_t<T_E>::series) {
      return this->get_a_ec()->graph_edge_composition_get_from_pc();
    } else if (this->get_type() == serpar_composition_node_t<T_E>::parallel) {
      if (!this->get_a_ec()->is_epsilon()) {
        return this->get_a_ec()->graph_edge_composition_get_from_pc();
      }
      if (!this->get_b_ec()->is_epsilon()) {
        return this->get_b_ec()->graph_edge_composition_get_from_pc();
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }
  int find_max_node_occurence() const
  {
    if(this->is_epsilon())
      return 0;
    
    std::function<map<T_PC, int> (shared_ptr<T_E const> const &)> f = [](shared_ptr<T_E const> const &te)
    {
      map<T_PC, int> edge_occ;
      if (te->is_empty()) {
        return edge_occ;
      }
      auto p = te->get_to_pc();
      if(edge_occ.count(p) == 0)
        edge_occ[p] = 1;
      else
        edge_occ[p]++;
      return edge_occ;
    };
    
    std::function<map<T_PC, int> (typename serpar_composition_node_t<T_E>::type, map<T_PC, int> const &, map<T_PC, int> const &)> fold_f = [](typename serpar_composition_node_t<T_E>::type ctype, map<T_PC, int> const &a, map<T_PC, int> const &b)
    {
      map<T_PC, int> edge_occ;
      if (ctype == serpar_composition_node_t<T_E>::series) {
        edge_occ = a;
        for(auto const &e :b) {
          if(edge_occ.count(e.first) == 0)
            edge_occ[e.first] = e.second;
          else
            edge_occ[e.first] += e.second;
        }
        return edge_occ;
      } else if (ctype == serpar_composition_node_t<T_E>::parallel) {
        edge_occ = a;
        for(auto const &e :b) {
          if(edge_occ.count(e.first) == 0)
            edge_occ[e.first] = e.second;
          else
            edge_occ[e.first] = max(edge_occ[e.first], e.second);
        }
        return edge_occ;
      } else NOT_REACHED();
    };
    map<T_PC, int> freq_map =  this->visit_nodes(f, fold_f);
    int max_freq = 0;
    for(auto const& pc_freq : freq_map)
    {
      if (pc_freq.second > max_freq)
        max_freq = pc_freq.second;
    }
    ASSERT(max_freq > 0);
    return max_freq;
  }

  T_PC graph_edge_composition_get_to_pc() const
  {
    if (this->get_type() == serpar_composition_node_t<T_E>::atom) {
      return this->get_atom()->get_to_pc();
    } else if (this->get_type() == serpar_composition_node_t<T_E>::series) {
      return this->get_b_ec()->graph_edge_composition_get_to_pc();
    } else if (this->get_type() == serpar_composition_node_t<T_E>::parallel) {
      if (!this->get_a_ec()->is_epsilon()) {
        return this->get_a_ec()->graph_edge_composition_get_to_pc();
      }
      if (!this->get_b_ec()->is_epsilon()) {
        return this->get_b_ec()->graph_edge_composition_get_to_pc();
      }
      NOT_REACHED();
    }
    NOT_REACHED();
  }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  bool id_is_zero() const { return !m_is_managed ; }
  ~graph_edge_composition_t()
  {
    if (m_is_managed) {
      this->get_ecomp_manager()->deregister_object(*this);
    }
  }

  map<T_PC, size_t> graph_edge_composition_get_pc_seen_count() const
  {
    map<T_PC, size_t> ret;
    list<shared_ptr<T_E const>> edges = this->get_constituent_atoms();
    for (const auto &ed : edges) {
      ret[ed->get_from_pc()]++;
    }
    return ret;
  }

  template<typename T_VAL, xfer_dir_t T_DIR>
  T_VAL xfer_over_graph_edge_composition(T_VAL const& in, function<T_VAL (T_VAL const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f, function<T_VAL (T_VAL const&, T_VAL const&)> meet_f, bool simplified) const
  {
    if (this->is_epsilon())
      return in;

    std::function<T_VAL (shared_ptr<T_E const> const&, T_VAL const&)> fold_atom_f =
      [&atom_f, &meet_f, simplified](shared_ptr<T_E const> const &e, T_VAL const &postorder_val)
      {
        if (e->is_empty()) {
          return postorder_val;
        }
        T_VAL const& ret = e->template xfer<T_VAL,T_DIR>(postorder_val, atom_f, meet_f, simplified);
        return ret;
      };
    std::function<T_VAL (T_VAL const&, T_VAL const&)> fold_parallel_f =
      [&meet_f](T_VAL const& a, T_VAL const& b)
      {
        T_VAL const& ret = meet_f(a, b);
        return ret;
      };

    if (T_DIR == xfer_dir_t::forward) {
      return this->template visit_nodes_preorder_series<T_VAL>(fold_atom_f, fold_parallel_f, in);
    } else if (T_DIR == xfer_dir_t::backward) {
      return this->template visit_nodes_postorder_series<T_VAL>(fold_atom_f, fold_parallel_f, in);
    } else { NOT_REACHED(); }
  }

private:
  bool m_is_managed;
};

/*template <typename T_PC, typename T_E>
list<graph_edge_composition_ref<T_PC,T_E>>
graph_edge_composition_get_terms(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  list<graph_edge_composition_ref<T_PC,T_E>> ret;
  if (ec->get_type() == serpar_composition_node_t<shared_ptr<T_E>>::atom) {
    ret.push_back(ec);
    return ret;
  } else if (ec->get_type() == serpar_composition_node_t<shared_ptr<T_E>>::series) {
    ret.push_back(ec);
    return ret;
  } else if (ec->get_type() == serpar_composition_node_t<shared_ptr<T_E>>::parallel) {
    ret = graph_edge_composition_get_terms(dynamic_pointer_cast<graph_edge_composition_ref<T_PC,T_E>>(ec->get_a()));
    list<graph_edge_composition_ref<T_PC,T_E>> b = graph_edge_composition_get_terms(dynamic_pointer_cast<graph_edge_composition_ref<T_PC,T_E>>(ec->get_b()));
    list_append(ret, b);
    return ret;
  }
  NOT_IMPLEMENTED();
}*/

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_E>
struct hash<graph_edge_composition_t<T_PC,T_E>>
{
  std::size_t operator()(graph_edge_composition_t<T_PC,T_E> const &s) const
  {
    serpar_composition_node_t<T_E> const &sd = s;
    return hash<serpar_composition_node_t<T_E>>()(sd);
  }
};

}

namespace eqspace {
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_edge_composition(T_E const &e)
{
  graph_edge_composition_t<T_PC,T_E> ec(e);
  return graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_edge_composition(shared_ptr<T_E const> const &e)
{
  graph_edge_composition_t<T_PC,T_E> ec(e);
  return graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_serial_edgelist(list<graph_edge_composition_ref<T_PC,T_E>> const &edgelist)
{
  ASSERT(edgelist.size() > 0);
  typename list<graph_edge_composition_ref<T_PC,T_E>>::const_reverse_iterator iter = edgelist.rbegin();
  graph_edge_composition_ref<T_PC,T_E> ret = *iter;
  for (iter++; iter != edgelist.rend(); iter++) {
    ret = mk_series(*iter, ret);
  }
  return ret;
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_parallel_edgelist(list<graph_edge_composition_ref<T_PC,T_E>> const &edgelist)
{
  ASSERT(edgelist.size() > 0);
  typename list<graph_edge_composition_ref<T_PC,T_E>>::const_reverse_iterator iter = edgelist.rbegin();
  graph_edge_composition_ref<T_PC,T_E> ret = *iter;
  for (iter++; iter != edgelist.rend(); iter++) {
    ret = mk_parallel(*iter, ret);
  }
  return ret;
}

template<typename T_PC, typename T_E>
bool compare_based_on_first_nextpc(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  list<shared_ptr<serpar_composition_node_t<T_E> const>> aterms, bterms;
  aterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  ASSERT(aterms.size() > 0);
  bterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(b);
  ASSERT(bterms.size() > 0);
  shared_ptr<serpar_composition_node_t<T_E> const> afirst = *aterms.begin();
  ASSERT(afirst);
  shared_ptr<serpar_composition_node_t<T_E> const> bfirst = *bterms.begin();
  ASSERT(bfirst);
  graph_edge_composition_ref<T_PC,T_E> afirstg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(afirst);
  ASSERT(afirstg);
  graph_edge_composition_ref<T_PC,T_E> bfirstg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bfirst);
  ASSERT(bfirstg);
  if (afirstg->is_epsilon()) {
    return true;
  }
  if (bfirstg->is_epsilon()) {
    return false;
  }
  return afirstg->graph_edge_composition_get_to_pc() < bfirstg->graph_edge_composition_get_to_pc();
}

template<typename T_PC, typename T_E>
list<graph_edge_composition_ref<T_PC,T_E>> graph_edge_composition_get_canonical_list_for_parallel_edges(set<graph_edge_composition_ref<T_PC,T_E>> const &parallel_edges)
{
  list<graph_edge_composition_ref<T_PC,T_E>> ret;
  //cout << __func__ << " " << __LINE__ << ": parallel_edges.size() = " << parallel_edges.size() << endl;
  for (auto e : parallel_edges) {
    //cout << __func__ << " " << __LINE__ << ": e = " << e->graph_edge_composition_to_string() << endl;
    ret.push_back(e);
  }
  if (ret.size() > 1) {
    ret.sort(compare_based_on_first_nextpc<T_PC,T_E>);
  }
  return ret;
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> graph_edge_composition_construct_edge_from_parallel_edges(set<graph_edge_composition_ref<T_PC,T_E>> const &parallel_edges)
{
  list<graph_edge_composition_ref<T_PC,T_E>> p_edgelist = graph_edge_composition_get_canonical_list_for_parallel_edges(parallel_edges);

  ASSERT(p_edgelist.size() > 0);
  typename list<graph_edge_composition_ref<T_PC,T_E>>::const_reverse_iterator iter = p_edgelist.rbegin();
  graph_edge_composition_ref<T_PC,T_E> ret = *iter;
  for (iter++; iter != p_edgelist.rend(); iter++) {
    graph_edge_composition_t<T_PC,T_E> ec(serpar_composition_node_t<T_E>::parallel, *iter, ret);
    ret = graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
  }
  return ret;
}

/*void graph_edge_composition_split_on_pc(graph_edge_composition_ref<T_PC,T_E> const &a, T_PC const &p, graph_edge_composition_ref<T_PC,T_E> &a1, graph_edge_composition_ref<T_PC,T_E> &a2) const
{
  list<shared_ptr<serpar_composition_node_t<shared_ptr<T_E>> const>> terms;
  list<graph_edge_composition_ref<T_PC,T_E>> terms_left, terms_right;
  terms = serpar_composition_node_t<shared_ptr<T_E>>::serpar_composition_get_serial_terms(a);
  ASSERT(a->graph_edge_composition_get_from_pc() != p);
  ASSERT(a->graph_edge_composition_get_from_pc() != p);
  bool onleft = true;
  for (const auto &term : terms) {
    graph_edge_composition_ref<T_PC,T_E> termg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(term);
    if (onleft) {
      terms_left.push_back(termg);
    } else {
      terms_right.push_back(termg);
    }
    if (termg->graph_edge_composition_get_to_pc() == p) {
      if (!onleft) {
        cout << __func__ << " " << __LINE__ << ": a = " << a->graph_edge_composition_to_string() << ", p = " << p.to_string() << endl;
      }
      ASSERT(onleft);
      onleft = false;
    }
  }
  a1 = graph_edge_composition_construct_edge_from_serial_edgelist(terms_left);
  a2 = graph_edge_composition_construct_edge_from_serial_edgelist(terms_right);
}

static bool graph_edge_compositions_contain_common_serial_node(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b, T_PC &p)
{
  T_PC from_pc = a->graph_edge_composition_get_from_pc();
  ASSERT(from_pc == b->graph_edge_composition_get_from_pc());
  T_PC to_pc = a->graph_edge_composition_get_to_pc();
  ASSERT(to_pc == b->graph_edge_composition_get_to_pc());
  list<shared_ptr<serpar_composition_node_t<shared_ptr<T_E>> const>> aterms, bterms;
  aterms = serpar_composition_node_t<shared_ptr<T_E>>::serpar_composition_get_serial_terms(a);
  bterms = serpar_composition_node_t<shared_ptr<T_E>>::serpar_composition_get_serial_terms(b);
  set<T_PC> a_pcs, b_pcs, common_pcs;
  for (auto aterm : aterms) {
    graph_edge_composition_ref<T_PC,T_E> atermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterm);
    T_PC apc = atermg->graph_edge_composition_get_to_pc();
    a_pcs.insert(apc);
  }
  for (auto bterm : bterms) {
    graph_edge_composition_ref<T_PC,T_E> btermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bterm);
    T_PC bpc = btermg->graph_edge_composition_get_to_pc();
    b_pcs.insert(bpc);
  }
  set_intersect(a_pcs, b_pcs, common_pcs);
  common_pcs.erase(from_pc);
  common_pcs.erase(to_pc);
  if (common_pcs.size() == 0) {
    return false;
  } else {
    p = *common_pcs.begin();
    return true;
  }
}*/

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_epsilon_ec(T_PC const& p)
{
  graph_edge_composition_t<T_PC,T_E> ec(T_E::empty(p));
  graph_edge_composition_ref<T_PC,T_E> ret = graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
  return ret;
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_epsilon_ec()
{
  graph_edge_composition_t<T_PC,T_E> ec(T_E::empty());
  graph_edge_composition_ref<T_PC,T_E> ret = graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
  return ret;
}

template<typename T_PC, typename T_E>
bool graph_edge_composition_is_epsilon(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  return    ec->get_type() == serpar_composition_node_t<T_E>::atom
         && ec->get_atom()->is_empty();
}

// s_terms -- to-be-processed terms
// pc -- to-PC of added edge
// partial_suffixpath -- suffixpath computed so far (to-PC is `pc`)
//
//  [ ... s_terms (s_terms.back()) ] [ ... partial_suffixpath ... ]
//                              ^
//                       we are prcoessing this EC
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
mk_suffixpath_ending_at_pc(list<shared_ptr<serpar_composition_node_t<T_E> const>> s_terms, T_PC const &pc, graph_edge_composition_ref<T_PC,T_E> partial_suffixpath)
{
  // base case
  if (s_terms.size() == 0)
    return partial_suffixpath;

  // get the last element
  graph_edge_composition_ref<T_PC,T_E> a = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(s_terms.back());
  ASSERT(a != nullptr);
  s_terms.pop_back();

  list<shared_ptr<serpar_composition_node_t<T_E> const>> pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(a);

  if (pterms.size() == 1) {
    // epsilon edges don't have to/from PCs
    if (   !graph_edge_composition_is_epsilon(a)
        && a->get_atom()->get_to_pc() == pc)
      // we only check to-PC since every non-entry PC will always be to-PC of an edge before it is from-PC
      return mk_suffixpath_ending_at_pc(s_terms, pc, a); // drop the suffix
    else {
      return mk_suffixpath_ending_at_pc(s_terms, pc, mk_series(a, partial_suffixpath)); // keep the suffix and add the new edge
    }
  }
  else {
    list<graph_edge_composition_ref<T_PC,T_E>> p_edgelist;
    for (const auto &pterm: pterms) {
      graph_edge_composition_ref<T_PC,T_E> ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(pterm);
      list<shared_ptr<serpar_composition_node_t<T_E> const>> s_terms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(ptermg);
      // recurse
      auto p_sf = mk_suffixpath_ending_at_pc(s_terms, pc, partial_suffixpath);
      p_edgelist.push_back(p_sf);
    }

    // add in parallel; mk_parallel will stitch together the common ends (if there are any)
    // a commmon end may not be present if one (or more) of the inputs decides to drop its suffix
    auto new_partial_suffixpath = graph_edge_composition_construct_edge_from_parallel_edgelist(p_edgelist);
    return mk_suffixpath_ending_at_pc(s_terms, pc, new_partial_suffixpath);
  }
}

// assumes `a` is a suffixpath
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
mk_suffixpath_series_with_edge(graph_edge_composition_ref<T_PC,T_E> const &a, shared_ptr<T_E const> const &e)
{
  list<shared_ptr<serpar_composition_node_t<T_E> const>> tmp_terms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  return mk_suffixpath_ending_at_pc(tmp_terms, e->get_to_pc(), mk_edge_composition<T_PC,T_E>(e)); // suffixify it!
}

// assumes `a` is a suffixpath
// unfolds `ec` and uses `mk_suffixpath_series_with_edge` for building suffixpath
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
mk_suffixpath_series(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  list<shared_ptr<serpar_composition_node_t<T_E> const>> ec_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(ec);
  // base case
  if (ec_sterms.size() == 1) {
    list<shared_ptr<serpar_composition_node_t<T_E> const>> ec_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(ec);

    if (ec_pterms.size() == 1) {
      if (!graph_edge_composition_is_epsilon(ec))
        return mk_suffixpath_series_with_edge(a, ec->get_atom());
      else
        return a;
    }
    else {
      list<graph_edge_composition_ref<T_PC,T_E>> p_edgelist;
      for (const auto &pterm: ec_pterms) {
        graph_edge_composition_ref<T_PC,T_E> ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(pterm);
        // recurse
        auto p_sf = mk_suffixpath_series(a, ptermg);
        p_edgelist.push_back(p_sf);
      }
      return graph_edge_composition_construct_edge_from_parallel_edgelist(p_edgelist);
    }
  }

  graph_edge_composition_ref<T_PC,T_E> ret = a;
  for (const auto &sterm: ec_sterms) {
    graph_edge_composition_ref<T_PC,T_E> stermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(sterm);
    ret = mk_suffixpath_series(ret, stermg);
  }
  return ret;
}

// find a matching to-PC pair in the ranges [a_start, a_end) and [b_start, b_end)
template<typename T_PC, typename T_E>
pair<typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator, typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator>
find_next_matching_to_pc(typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &a_start,
                         typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &b_start,
                         typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &a_end,
                         typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &b_end)
{
  // early exit
  if (   a_start == a_end
      || b_start == b_end)
    return make_pair(a_end, b_end);

  // N^2 loop for finding matching to_pc
  for (auto first_iter = a_start; first_iter != a_end; ++first_iter)
    for (auto second_iter = b_start; second_iter != b_end; ++second_iter) {
      graph_edge_composition_ref<T_PC,T_E> ag = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(*first_iter);
      graph_edge_composition_ref<T_PC,T_E> bg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(*second_iter);
      if (ag->graph_edge_composition_get_to_pc() == bg->graph_edge_composition_get_to_pc())
        return make_pair(first_iter, second_iter);
    }

  // could not find matching pc
  return make_pair(a_end, b_end);
}

// get all element in the closed range [a_iter, b_iter]
// b_iter is assumed to be != .end()
template<typename T_PC, typename T_E>
list<graph_edge_composition_ref<T_PC,T_E>>
get_edgelist_from_to_iter_inclusive(typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator a_iter,
                                    typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &b_iter)
{
  list<graph_edge_composition_ref<T_PC,T_E>> ret;
  ret.push_back(dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(*a_iter));
  while (a_iter != b_iter) {
    ++a_iter;
    ret.push_back(dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(*a_iter));
  }

  return ret;
}

template<typename T_PC, typename T_E>
set<T_PC>
collect_intermediate_pcs(shared_ptr<serpar_composition_node_t<T_E> const> const& a)
{
  T_PC from_pc = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a)->graph_edge_composition_get_from_pc();
  list<shared_ptr<T_E const>> edgelist = a->get_constituent_atoms();
  set<T_PC> ret;
  for (auto const& e : edgelist) {
    if (!e->is_empty() && e->get_from_pc() != from_pc)
      ret.insert(e->get_from_pc());
  }
  return ret;
}
// invariant for this function: from and to PCs are equal
// we will ensure this at every recursive call
// this returns an over-approximate answer to the "are these edge compositions disjoint" query
// i.e. we may return FALSE even if the actual result is TRUE
// but we never return TRUE if actual result is FALSE
// IOW, if the edge compositions are non-disjoint then we will always return correct answer
template<typename T_PC, typename T_E>
bool
graph_edge_composition_are_disjoint_serial_terms(list<shared_ptr<serpar_composition_node_t<T_E> const>> aterms, list<shared_ptr<serpar_composition_node_t<T_E> const>> bterms)
{
  ASSERT(aterms.size() >= 1);
  ASSERT(bterms.size() >= 1);

  // assert from/to PCs
  graph_edge_composition_ref<T_PC,T_E> const &aterms_f = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterms.front());
  graph_edge_composition_ref<T_PC,T_E> const &bterms_f = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bterms.front());
  graph_edge_composition_ref<T_PC,T_E> const &aterms_b = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterms.back());
  graph_edge_composition_ref<T_PC,T_E> const &bterms_b = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bterms.back());
  ASSERT(aterms_f->graph_edge_composition_get_from_pc() == bterms_f->graph_edge_composition_get_from_pc());
  ASSERT(aterms_b->graph_edge_composition_get_to_pc() == bterms_b->graph_edge_composition_get_to_pc());

  // base case
  if (aterms.size() == 1 && bterms.size() == 1) {
    list<shared_ptr<serpar_composition_node_t<T_E> const>> a_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(aterms.front());
    list<shared_ptr<serpar_composition_node_t<T_E> const>> b_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(bterms.front());
    if (a_pterms.size() == 1 && b_pterms.size() == 1) {
      set<T_PC> aterms_pcs = collect_intermediate_pcs<T_PC,T_E>(a_pterms.front());
      set<T_PC> bterms_pcs = collect_intermediate_pcs<T_PC,T_E>(b_pterms.front());

      if (aterms_pcs.size() == 0 && bterms_pcs.size() == 0)
        return false;

      set_intersect(aterms_pcs, bterms_pcs);
      return aterms_pcs.size() == 0;
    } else {
      // return true if each parallel term of a is disjoint from every parallel term of b
      for (const auto &a_pterm : a_pterms) {
        graph_edge_composition_ref<T_PC,T_E> const &a_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a_pterm);
        for (const auto &b_pterm : b_pterms) {
          graph_edge_composition_ref<T_PC,T_E> const &b_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b_pterm);
          // have to pass serial terms
          list<shared_ptr<serpar_composition_node_t<T_E> const>> a_ptermg_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a_ptermg);
          list<shared_ptr<serpar_composition_node_t<T_E> const>> b_ptermg_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(b_ptermg);
          bool ret = graph_edge_composition_are_disjoint_serial_terms<T_PC,T_E>(a_ptermg_sterms, b_ptermg_sterms);
          if (!ret) {
            return false;
          }
        }
      }
      return true;
    }
  }

  pair<typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator, typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator> pp;
  auto aterm_iter = aterms.begin();
  auto bterm_iter = bterms.begin();
  // from PCs are equal by induction
  while ((pp = find_next_matching_to_pc<T_PC,T_E>(aterm_iter, bterm_iter, aterms.end(), bterms.end())).first != aterms.end()) {
    // get the correlated edgelist
    list<graph_edge_composition_ref<T_PC,T_E>> aterms_l = get_edgelist_from_to_iter_inclusive<T_PC,T_E>(aterm_iter, pp.first);
    list<graph_edge_composition_ref<T_PC,T_E>> bterms_l = get_edgelist_from_to_iter_inclusive<T_PC,T_E>(bterm_iter, pp.second);

    // compress it to an edge composition
    graph_edge_composition_ref<T_PC,T_E> const &atermg = graph_edge_composition_construct_edge_from_serial_edgelist<T_PC,T_E>(aterms_l);
    graph_edge_composition_ref<T_PC,T_E> const &btermg = graph_edge_composition_construct_edge_from_serial_edgelist<T_PC,T_E>(bterms_l);

    // make singleton list
    list<shared_ptr<serpar_composition_node_t<T_E> const>> atermg_slist;
    atermg_slist.push_back(atermg);
    list<shared_ptr<serpar_composition_node_t<T_E> const>> btermg_slist;
    btermg_slist.push_back(btermg);

    // recurse
    bool ret = graph_edge_composition_are_disjoint_serial_terms<T_PC,T_E>(atermg_slist, btermg_slist);
    if (ret)
      return true;

    // update the iterators
    aterm_iter = pp.first;
    bterm_iter = pp.second;

    aterm_iter++;
    bterm_iter++;
  }

  // the final to_pcs should match
  ASSERT(aterm_iter == aterms.end());
  ASSERT(bterm_iter == bterms.end());
  return false;
}

// collect all to-PCs in the range [a_iter, end)
template<typename T_PC, typename T_E>
set<T_PC>
collect_to_PCs_from_iter(typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator a_iter,
                         typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator const &end)
{
  set<T_PC> ret;
  while (a_iter != end) {
    graph_edge_composition_ref<T_PC,T_E> gec = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(*a_iter);
    list<shared_ptr<T_E const>> edgelist = gec->get_constituent_atoms();
    for (auto const& e : edgelist) {
      if (!e->is_empty())
        ret.insert(e->get_to_pc());
    }
    ++a_iter;
  }

  return ret;
}

/* returns over-approximate sound answer to the edge composition disjointness query
 * i.e. will return true iff the edge compositions are provable disjoint
 * otherwise return false
 */
template<typename T_PC, typename T_E>
bool
graph_edge_composition_are_disjoint(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  // trivial cases
  if (   !a || graph_edge_composition_is_epsilon(a)
      || !b || graph_edge_composition_is_epsilon(b))
    return true;

  if (a->graph_edge_composition_get_from_pc() != b->graph_edge_composition_get_from_pc()) {
    // conservatively return false since one may be embedded inside the other
    return false;
  }

  //CPP_DBG_EXEC(DEBUG_ONLY, cout << __func__ << ':' << __LINE__ << ": a = " << a->to_string() << endl);
  //CPP_DBG_EXEC(DEBUG_ONLY, cout << __func__ << ':' << __LINE__ << ": b = " << b->to_string() << endl);

  list<shared_ptr<serpar_composition_node_t<T_E> const>> aterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  list<shared_ptr<serpar_composition_node_t<T_E> const>> bterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(b);

  if (a->graph_edge_composition_get_to_pc() != b->graph_edge_composition_get_to_pc()) {
    // to-PCs are not equal; match the maximal common part and decide the rest using intersection of PCs
    pair<typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator, typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::iterator> pp;
    auto aterm_iter = aterms.begin();
    auto bterm_iter = bterms.begin();
    // from PCs are equal by induction
    while (   (pp = find_next_matching_to_pc<T_PC,T_E>(aterm_iter, bterm_iter, aterms.end(), bterms.end())).first != aterms.end()
           && pp.second != bterms.end()) {
      // get the correlated edgelist
      list<graph_edge_composition_ref<T_PC,T_E>> aterms_l = get_edgelist_from_to_iter_inclusive<T_PC,T_E>(aterm_iter, pp.first);
      list<graph_edge_composition_ref<T_PC,T_E>> bterms_l = get_edgelist_from_to_iter_inclusive<T_PC,T_E>(bterm_iter, pp.second);

      // compress it to an edge composition
      graph_edge_composition_ref<T_PC,T_E> const &atermg = graph_edge_composition_construct_edge_from_serial_edgelist<T_PC,T_E>(aterms_l);
      graph_edge_composition_ref<T_PC,T_E> const &btermg = graph_edge_composition_construct_edge_from_serial_edgelist<T_PC,T_E>(bterms_l);

      // make singleton list
      list<shared_ptr<serpar_composition_node_t<T_E> const>> atermg_slist;
      atermg_slist.push_back(atermg);
      list<shared_ptr<serpar_composition_node_t<T_E> const>> btermg_slist;
      btermg_slist.push_back(btermg);

      // from-to PCs are matching, send it down
      bool ret = graph_edge_composition_are_disjoint_serial_terms<T_PC,T_E>(atermg_slist, btermg_slist);
      if (ret)
        return true;

      // update the iterators
      aterm_iter = pp.first;
      bterm_iter = pp.second;

      aterm_iter++;
      bterm_iter++;
    }

    // one of the path exhausted, sine we couldn't find disjoint part return
    // conservative false
    if (   aterm_iter == aterms.end()
        || bterm_iter == bterms.end())
      return false;

    ASSERT(aterm_iter != aterms.end());
    ASSERT(bterm_iter != bterms.end());

    // if set of to-PCs of remaining part are disjoint then the paths are
    // surely disjoint
    set<T_PC> apcs = collect_to_PCs_from_iter<T_PC,T_E>(aterm_iter, aterms.end());
    set<T_PC> bpcs = collect_to_PCs_from_iter<T_PC,T_E>(bterm_iter, bterms.end());
    return set_intersection_is_empty(apcs, bpcs);
  }

  return graph_edge_composition_are_disjoint_serial_terms<T_PC,T_E>(aterms, bterms);
}


// invariant for this function: from and to PCs are equal
// we will ensure this at every recursive call
// minus is defined as:
// minus(a.b, x.y) =
//    minus(a, x) . minus(b, y)
// || intersect(a, x) . minus(b, y)
// || minus(a, x) . intersect(b, y)
//
// where, (a, x) and (b, y) have matching from/to PCs
//        . is serial composition
//        || is parallel composition
//THIS IS BUGGY; see superopt-docs! Don't use
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
graph_edge_composition_minus_serial_terms(list<shared_ptr<serpar_composition_node_t<T_E> const>> aterms, list<shared_ptr<serpar_composition_node_t<T_E> const>> bterms)
{
  ASSERT(aterms.size() >= 1);
  ASSERT(bterms.size() >= 1);

  // assert from/to PCs
  graph_edge_composition_ref<T_PC,T_E> const &aterms_f = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterms.front());
  graph_edge_composition_ref<T_PC,T_E> const &bterms_f = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bterms.front());
  graph_edge_composition_ref<T_PC,T_E> const &aterms_b = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterms.back());
  graph_edge_composition_ref<T_PC,T_E> const &bterms_b = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(bterms.back());
  ASSERT(aterms_f->graph_edge_composition_get_from_pc() == bterms_f->graph_edge_composition_get_from_pc());
  ASSERT(aterms_b->graph_edge_composition_get_to_pc() == bterms_b->graph_edge_composition_get_to_pc());

  // base case
  if (aterms.size() == 1 && bterms.size() == 1) {
    list<shared_ptr<serpar_composition_node_t<T_E> const>> a_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(aterms.front());
    list<shared_ptr<serpar_composition_node_t<T_E> const>> b_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(bterms.front());
    if (a_pterms.size() == 1 && b_pterms.size() == 1) {
      graph_edge_composition_ref<T_PC,T_E> const &a_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a_pterms.front());
      graph_edge_composition_ref<T_PC,T_E> const &b_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b_pterms.front());
      if (a_ptermg != b_ptermg) {
        return a_ptermg;
      } else {
        return nullptr;
      }
    } else {
      list<graph_edge_composition_ref<T_PC,T_E>> new_pterms;
      for (const auto &a_pterm : a_pterms) {
        graph_edge_composition_ref<T_PC,T_E> const &a_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a_pterm);
        bool found_pterm = false;
        for (const auto &b_pterm : b_pterms) {
          graph_edge_composition_ref<T_PC,T_E> const &b_ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b_pterm);
          // have to pass serial terms
          list<shared_ptr<serpar_composition_node_t<T_E> const>> a_ptermg_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a_ptermg);
          list<shared_ptr<serpar_composition_node_t<T_E> const>> b_ptermg_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(b_ptermg);
          graph_edge_composition_ref<T_PC,T_E> ret = graph_edge_composition_intersect_serial_terms(a_ptermg_sterms, b_ptermg_sterms);
          if (ret) {
            found_pterm = true;
            break;
          }
        }
        if (!found_pterm) {
          // this aterm is not present in b
          new_pterms.push_back(a_ptermg);
        }
      }
      if (new_pterms.size())
        return graph_edge_composition_construct_edge_from_parallel_edgelist(new_pterms);
      else
        return nullptr;
    }
  }

  auto aterm_iter = aterms.begin();
  auto bterm_iter = bterms.begin();
  auto pp = find_next_matching_to_pc<T_PC,T_E>(aterm_iter, bterm_iter, aterms.end(), bterms.end());

  if (pp.first == aterms.end()) {
    // couldn't find matching to_pc
    return nullptr;
  }

  // get the correlated edgelist
  list<graph_edge_composition_ref<T_PC,T_E>> aterms_l = get_edgelist_from_to_iter_inclusive(aterm_iter, pp.first);
  list<graph_edge_composition_ref<T_PC,T_E>> bterms_l = get_edgelist_from_to_iter_inclusive(bterm_iter, pp.second);

  // compress it to an edge composition
  graph_edge_composition_ref<T_PC,T_E> const &atermg = graph_edge_composition_construct_edge_from_serial_edgelist(aterms_l);
  graph_edge_composition_ref<T_PC,T_E> const &btermg = graph_edge_composition_construct_edge_from_serial_edgelist(bterms_l);

  // make singleton list
  list<shared_ptr<serpar_composition_node_t<T_E> const>> atermg_slist;
  atermg_slist.push_back(atermg);
  list<shared_ptr<serpar_composition_node_t<T_E> const>> btermg_slist;
  btermg_slist.push_back(btermg);

  graph_edge_composition_ref<T_PC,T_E> ret_ml = graph_edge_composition_minus_serial_terms(atermg_slist, btermg_slist);
  graph_edge_composition_ref<T_PC,T_E> ret_il = graph_edge_composition_intersect_serial_terms(atermg_slist, btermg_slist);

  // collect rest of the terms
  aterm_iter = pp.first;
  bterm_iter = pp.second;
  ++aterm_iter;
  ++bterm_iter;

  list<shared_ptr<serpar_composition_node_t<T_E> const>> atermg_rlist;
  list<shared_ptr<serpar_composition_node_t<T_E> const>> btermg_rlist;

  while (aterm_iter != aterms.end()) {
    atermg_rlist.push_back(*aterm_iter);
    ++aterm_iter;
  }
  while (bterm_iter != bterms.end()) {
    btermg_rlist.push_back(*bterm_iter);
    ++bterm_iter;
  }

  // assuming suffix paths
  // suffix paths do not contain loops; since we matched to-PCs earlier and
  // before this call the to-PC were equal then the following situation
  // will always be true
  ASSERT(atermg_rlist.empty() == btermg_rlist.empty());
  if (!atermg_rlist.empty()) {
    list<graph_edge_composition_ref<T_PC,T_E>> new_terms;
    graph_edge_composition_ref<T_PC,T_E> ret_mr = graph_edge_composition_minus_serial_terms(atermg_rlist, btermg_rlist);
    graph_edge_composition_ref<T_PC,T_E> ret_ir = graph_edge_composition_intersect_serial_terms(atermg_rlist, btermg_rlist);

    if (ret_ml && ret_mr)
      new_terms.push_back(mk_series(ret_ml, ret_mr));

    if (ret_ml && ret_ir)
      new_terms.push_back(mk_series(ret_ml, ret_ir));

    if (ret_il && ret_mr)
      new_terms.push_back(mk_series(ret_il, ret_mr));

    if (new_terms.size())
      return graph_edge_composition_construct_edge_from_parallel_edgelist(new_terms);
    else
      return nullptr;
  }
  else
    return ret_ml;
}

template<typename T_PC, typename T_E>
bool
graph_edge_composition_has_non_empty_intersection(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  NOT_IMPLEMENTED();
}

//THIS IS BUGGY; see superopt-docs! Don't use
template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
graph_edge_composition_minus(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  if (   !a.get()
      || !b.get())
    return nullptr;
  // epsilon ec do not have to/from PC which can complicate matters in next
  // step
  if (graph_edge_composition_is_epsilon(a))
    return nullptr;
  if (graph_edge_composition_is_epsilon(b))
    return a;

  if (   a->graph_edge_composition_get_from_pc() != b->graph_edge_composition_get_from_pc()
      || a->graph_edge_composition_get_to_pc() != b->graph_edge_composition_get_to_pc())
    return a;

  list<shared_ptr<serpar_composition_node_t<T_E> const>> aterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  list<shared_ptr<serpar_composition_node_t<T_E> const>> bterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(b);

  return graph_edge_composition_minus_serial_terms<T_PC,T_E>(aterms, bterms);
}


// a POS string is (supposed to be) easier to understand for humans than normal graph_edge_composition_to_string
// by removing extra parentheses wherever applicable (without introducing ambiguity)
// e.g. POS string of
//          ((a=>b).((b=>c).((c=>d)+(c=>e))))
//  is
//          (a=>b).(b=>c).((c=>d)+(c=>e))
template<typename T_PC, typename T_E>
string graph_edge_composition_to_pos_string(graph_edge_composition_ref<T_PC,T_E> const &a)
{
  auto sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  if (sterms.size() == 1) {
    auto pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(sterms.front());
    if (pterms.size() == 1) {
      if (a->is_epsilon())
        return EPSILON;
      else return a->graph_edge_composition_to_string();
    }
    else {
      stringstream ss;
      ss << "(";
      int i = pterms.size();
      for (auto const &pt: pterms) {
        graph_edge_composition_ref<T_PC,T_E> ptg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(pt);
        ss << graph_edge_composition_to_pos_string(ptg);
        if (--i > 0)
          ss << '+';
      }
      ss << ")";
      return ss.str();
    }
  }
  else {
    stringstream ss;
    ss << "(";
    int i = sterms.size();
    for (auto const &st: sterms) {
      graph_edge_composition_ref<T_PC,T_E> stg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(st);
      ss << graph_edge_composition_to_pos_string(stg);
      if (--i > 0)
        ss << ".";
    }
    ss << ")";
    return ss.str();
  }
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_series(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  if (graph_edge_composition_is_epsilon(a)) {
    return b;
  }
  if (graph_edge_composition_is_epsilon(b)) {
    return a;
  }
  ASSERT(a->graph_edge_composition_get_to_pc() == b->graph_edge_composition_get_from_pc());
  list<shared_ptr<serpar_composition_node_t<T_E> const>> a_terms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(a);
  graph_edge_composition_ref<T_PC,T_E> ret = b;
  //construct in the canonical: cons (hd, tl) form
  for (typename list<shared_ptr<serpar_composition_node_t<T_E> const>>::const_reverse_iterator iter = a_terms.rbegin();
       iter != a_terms.rend();
       iter++) {
    shared_ptr<serpar_composition_node_t<T_E> const> aterm = *iter;
    graph_edge_composition_ref<T_PC,T_E> atermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(aterm);
    ASSERT(atermg);
    graph_edge_composition_t<T_PC,T_E> ec(serpar_composition_node_t<T_E>::series, atermg, ret);
    ret = graph_edge_composition_t<T_PC,T_E>::get_ecomp_manager()->register_object(ec);
  }
  //cout << __func__ << " " << __LINE__ << ": returning " << ret->graph_edge_composition_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": m_ecomp_manager.size() = " << m_ecomp_manager.get_size() << ": adding " << ec.graph_edge_composition_to_string() << endl;
  return ret;
}

//template<typename T_PC, typename T_E>
//int
//graph_edge_composition_get_max_edge_freq(graph_edge_composition_ref<T_PC,T_E> const &ec) {
//  map<T_E, int> ret;
//	list<T_E> edgelist = ec->get_constituent_atoms();
//	for(auto const &e: edgelist) {
//		if(ret.count(e))
//			ret[e]++;
//		else
//			ret[e] = 1;
//	}
//	int max_freq = 0;
//	for(auto const &e_freq : ret) {
//		if(e_freq.second > max_freq)
//			max_freq = e_freq.second;
//	}
//  return max_freq;
//}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
graph_edge_composition_minus_first_series_terms(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  graph_edge_composition_ref<T_PC,T_E> ret;
	if(ec->get_type() == serpar_composition_node_t<T_E>::parallel) {
    ret = ec;
	}
	else if(ec->get_type() == serpar_composition_node_t<T_E>::atom) {
		ret = nullptr;
	}
	else if(ec->get_type() == serpar_composition_node_t<T_E>::series) {
    shared_ptr<serpar_composition_node_t<T_E> const> a = ec->get_a();
    shared_ptr<serpar_composition_node_t<T_E> const> b = ec->get_b();
    ASSERT(a->get_type() != serpar_composition_node_t<T_E>::series);
    if(a->get_type() == serpar_composition_node_t<T_E>::atom) {
      graph_edge_composition_ref<T_PC,T_E> bg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b);
			ret = graph_edge_composition_minus_first_series_terms(bg);
    }
    else if(a->get_type() == serpar_composition_node_t<T_E>::parallel) {
    	ret = ec;
    }
  }
  return ret;
}

template<typename T_PC, typename T_E>
pair<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>>
separate_first_series_term(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  ASSERT(ec->get_type() == serpar_composition_node_t<T_E>::series);
  shared_ptr<serpar_composition_node_t<T_E> const> a = ec->get_a();
  shared_ptr<serpar_composition_node_t<T_E> const> b = ec->get_b();
  ASSERT(a->get_type() != serpar_composition_node_t<T_E>::series);
  graph_edge_composition_ref<T_PC,T_E> ag = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a);
  graph_edge_composition_ref<T_PC,T_E> bg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b);
  return make_pair(ag, bg);
}

template<typename T_PC, typename T_E>
pair<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>>
separate_last_series_term(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  //cout << __func__ << " " << __LINE__ << ": ec = " << ec->graph_edge_composition_to_string() << endl;
  ASSERT(ec->get_type() == serpar_composition_node_t<T_E>::series);
  shared_ptr<serpar_composition_node_t<T_E> const> a = ec->get_a();
  shared_ptr<serpar_composition_node_t<T_E> const> b = ec->get_b();
  ASSERT(a->get_type() != serpar_composition_node_t<T_E>::series);
  graph_edge_composition_ref<T_PC,T_E> ag = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a);
  graph_edge_composition_ref<T_PC,T_E> bg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b);

  if (b->get_type() != serpar_composition_node_t<T_E>::series) {
    return make_pair(bg, ag);
  } else {
    pair<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>> ret;
    ret = separate_last_series_term(bg);
    graph_edge_composition_ref<T_PC,T_E> remaining = mk_series(ag, ret.second);
    return make_pair(ret.first, remaining);
  }
}

template<typename T_PC, typename T_E>
map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>>
group_pterms_by_common_edge(set<graph_edge_composition_ref<T_PC,T_E>> const &pterms, bool group_by_front)
{
  map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>> ret;
  for (const auto &pterm : pterms) {
    ASSERT(pterm->get_type() != serpar_composition_node_t<T_E>::parallel);
    if (pterm->get_type() == serpar_composition_node_t<T_E>::atom) {
      /*set<graph_edge_composition_ref<T_PC,T_E>> bg_set;
      if (ret.count(pterm) == 0) {
        ret[pterm] = bg_set;
      }*/
      graph_edge_composition_ref<T_PC,T_E> epsilon_ec = mk_epsilon_ec<T_PC,T_E>();
      ret[pterm].insert(epsilon_ec);
    } else if (pterm->get_type() == serpar_composition_node_t<T_E>::series) {
      pair<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>> ab;
      if (group_by_front) {
        ab = separate_first_series_term(pterm);
      } else {
        ab = separate_last_series_term(pterm);
      }
      ret[ab.first].insert(ab.second);
    } else NOT_REACHED();
  }
  return ret;
}

template<typename T_PC, typename T_E>
map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>>
group_pterms_by_common_front_edge(set<graph_edge_composition_ref<T_PC,T_E>> const &pterms)
{
  return group_pterms_by_common_edge(pterms, true);
}

template<typename T_PC, typename T_E>
map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>>
group_pterms_by_common_back_edge(set<graph_edge_composition_ref<T_PC,T_E>> const &pterms)
{
  return group_pterms_by_common_edge(pterms, false);
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
graph_edge_composition_pterms_canonicalize(set<graph_edge_composition_ref<T_PC,T_E>> const &pterms)
{
  //autostop_timer func_timer(__func__); // function called too frequently, use low overhead timers
  if (pterms.size() == 0) {
    return nullptr;
  }
  if (   pterms.size() == 1
      && graph_edge_composition_is_epsilon(*pterms.begin())) {
    return *pterms.begin();
  }
  if (graph_edge_composition_t<T_PC,T_E>::get_graph_edge_composition_pterms_canonicalize_cache()->count(pterms)) {
    return graph_edge_composition_t<T_PC,T_E>::get_graph_edge_composition_pterms_canonicalize_cache()->at(pterms);
  }
  graph_edge_composition_ref<T_PC,T_E> ret = graph_edge_composition_construct_edge_from_parallel_edges(pterms);
  graph_edge_composition_ref<T_PC,T_E> old_ret;

  while (ret != old_ret) {
    set<graph_edge_composition_ref<T_PC,T_E>> all_pterms, new_pterms, new_pterms2;
    list<shared_ptr<serpar_composition_node_t<T_E> const>> pterms2 = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(ret);
    // collect all parallel path( composition)s in all_pterms; individual paths in all_pterms will either be series composition or atom (but not parallel)
    for (const auto &pterm2 : pterms2) {
      graph_edge_composition_ref<T_PC,T_E> pterm2_g = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(pterm2);
      ASSERT(pterm2_g->get_type() != serpar_composition_node_t<T_E>::parallel); // this is ensured by serpar_composition_get_parallel_terms
      all_pterms.insert(pterm2_g);
    }
    old_ret = ret;
    map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>> grouped_pterms = group_pterms_by_common_front_edge(all_pterms);
    for (const auto &grouped_pterm : grouped_pterms) {
      graph_edge_composition_ref<T_PC,T_E> const &head = grouped_pterm.first;
      graph_edge_composition_ref<T_PC,T_E> tail = graph_edge_composition_pterms_canonicalize(grouped_pterm.second);
      if (!tail) {
        new_pterms.insert(head);
      } else {
        new_pterms.insert(/*this->*/mk_series(head, tail));
      }
    }
    //cout << __func__ << " " << __LINE__ << ": old_ret = " << old_ret->graph_edge_composition_to_string() << ": after group by common front edge, new_pterms.size() = " << new_pterms.size() << endl;
    map<graph_edge_composition_ref<T_PC,T_E>, set<graph_edge_composition_ref<T_PC,T_E>>> grouped_pterms2 = group_pterms_by_common_back_edge(new_pterms);
    for (const auto &grouped_pterm : grouped_pterms2) {
      //cout << __func__ << " " << __LINE__ << endl;
      graph_edge_composition_ref<T_PC,T_E> head = graph_edge_composition_pterms_canonicalize(grouped_pterm.second);
      //cout << __func__ << " " << __LINE__ << endl;
      graph_edge_composition_ref<T_PC,T_E> const &tail = grouped_pterm.first;
      if (!head) {
        new_pterms2.insert(tail);
      } else {
        new_pterms2.insert(/*this->*/mk_series(head, tail));
      }
    }
    //cout << __func__ << " " << __LINE__ << ": old_ret = " << old_ret->graph_edge_composition_to_string() << ": after group by common back edge, new_pterms.size() = " << new_pterms2.size() << endl;
    ret = graph_edge_composition_construct_edge_from_parallel_edges(new_pterms2);
    //cout << __func__ << " " << __LINE__ << ": old_ret = " << old_ret->graph_edge_composition_to_string() << ": ret = " << ret->graph_edge_composition_to_string() << endl;
  }
  //m_graph_edge_composition_pterms_canonicalize_cache.insert(make_pair(pterms, ret));
  graph_edge_composition_t<T_PC,T_E>::get_graph_edge_composition_pterms_canonicalize_cache()->insert(make_pair(pterms, ret));
  return ret;
}

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E> mk_parallel(graph_edge_composition_ref<T_PC,T_E> const &a, graph_edge_composition_ref<T_PC,T_E> const &b)
{
  //cout << __func__ << " " << __LINE__ << ": a = " << a->graph_edge_composition_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": b = " << b->graph_edge_composition_to_string() << endl;
  //ASSERT(a->graph_edge_composition_get_from_pc() == b->graph_edge_composition_get_from_pc());
  //ASSERT(a->graph_edge_composition_get_to_pc() == b->graph_edge_composition_get_to_pc());

  /*list<shared_ptr<serpar_composition_node_t<shared_ptr<T_E>> const>> a_pterms = serpar_composition_node_t<shared_ptr<T_E>>::serpar_composition_get_parallel_terms(a);
  list<shared_ptr<serpar_composition_node_t<shared_ptr<T_E>> const>> b_pterms = serpar_composition_node_t<shared_ptr<T_E>>::serpar_composition_get_parallel_terms(b);
  set<graph_edge_composition_ref<T_PC,T_E>> all_pterms;
  for (const auto &a_pterm : a_pterms) {
    graph_edge_composition_ref<T_PC,T_E> a_pterm_g = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(a_pterm);
    all_pterms.insert(a_pterm_g);
  }
  for (const auto &b_pterm : b_pterms) {
    graph_edge_composition_ref<T_PC,T_E> b_pterm_g = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(b_pterm);
    all_pterms.insert(b_pterm_g);
  }*/
  set<graph_edge_composition_ref<T_PC,T_E>> parallel_edges;
  if (!graph_edge_composition_is_epsilon(a))
    parallel_edges.insert(a);
  if (!graph_edge_composition_is_epsilon(b))
    parallel_edges.insert(b);

  if (parallel_edges.size())
    return graph_edge_composition_pterms_canonicalize(parallel_edges);
  else
    return mk_epsilon_ec<T_PC,T_E>();
  /*bool done = false;
  while (!done) {
    set<tuple<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>, T_PC>> need_merging;
    for (auto iter1 = all_pterms.begin();
         iter1 != all_pterms.end();
         iter1++) {
      auto iter2 = iter1;
      for (iter2++; iter2 != all_pterms.end(); iter2++) {
        T_PC p;
        if (graph_edge_compositions_contain_common_serial_node(*iter1, *iter2, p)) {
          need_merging.insert(make_tuple(*iter1, *iter2, p));
          break;
        }
      }
      if (need_merging.size() > 0) {
        ASSERT(need_merging.size() == 1);
        break;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": need_merging.size() = " << need_merging.size() << endl;
    ASSERT(need_merging.size() <= 1);
    if (need_merging.size() == 0) {
      done = true;
    } else {
      tuple<graph_edge_composition_ref<T_PC,T_E>, graph_edge_composition_ref<T_PC,T_E>, T_PC> nm = *need_merging.begin();
      graph_edge_composition_ref<T_PC,T_E> x_pterm = get<0>(nm);
      graph_edge_composition_ref<T_PC,T_E> y_pterm = get<1>(nm);
      T_PC p = get<2>(nm);
      all_pterms.erase(x_pterm);
      all_pterms.erase(y_pterm);
      graph_edge_composition_ref<T_PC,T_E> x_pterm1, x_pterm2, y_pterm1, y_pterm2;
      graph_edge_composition_split_on_pc(x_pterm, p, x_pterm1, x_pterm2);
      graph_edge_composition_split_on_pc(y_pterm, p, y_pterm1, y_pterm2);
      graph_edge_composition_ref<T_PC,T_E> merged_pterm1 = this->mk_parallel(x_pterm1, y_pterm1);
      graph_edge_composition_ref<T_PC,T_E> merged_pterm2 = this->mk_parallel(x_pterm2, y_pterm2);
      graph_edge_composition_ref<T_PC,T_E> merged_pterm = this->mk_series(merged_pterm1, merged_pterm2);
      all_pterms.insert(merged_pterm);
    }
  }

  //construct a parallel ec using a,b,c terms
  graph_edge_composition_ref<T_PC,T_E> ret = graph_edge_composition_construct_edge_from_parallel_edges(all_pterms);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret->graph_edge_composition_to_string() << endl;
  return ret;*/
}

/*graph_edge_composition_ref<T_PC,T_E>
graph_edge_composition_add_path_in_parallel(graph_edge_composition_ref<T_PC,T_E> const &ec, list<shared_ptr<T_E>> const &path) const
{
  for (auto &pterm : pterms) {
    if (graph_edge_composition_add_path_in_parallel_if_common_node_exists(pterm, path)) {
      added = true;
    }
  }
  if (!added) {
    pterms.push_back(
  }
  graph_edge_composition_ref<T_PC,T_E> new_ec;
  NOT_IMPLEMENTED();
  return new_ec;
}*/

template<typename T_PC, typename T_E>
list<graph_edge_composition_ref<T_PC,T_E>>
graph_edge_composition_get_parallel_paths(graph_edge_composition_ref<T_PC,T_E> const &ec)
{
  list<shared_ptr<serpar_composition_node_t<T_E> const>> ec_pterms = serpar_composition_node_t<T_E>::serpar_composition_get_parallel_terms(ec);
  // base case
  if (ec_pterms.size() == 1) {
    list<shared_ptr<serpar_composition_node_t<T_E> const>> ec_sterms = serpar_composition_node_t<T_E>::serpar_composition_get_serial_terms(ec);

    if (ec_sterms.size() == 1) {
      return { ec };
    } else {
      // serial composition
      list<graph_edge_composition_ref<T_PC,T_E>> ret_list = { mk_epsilon_ec<T_PC,T_E>() };
      for (const auto &sterm: ec_sterms) {
        graph_edge_composition_ref<T_PC,T_E> stermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(sterm);
        list<graph_edge_composition_ref<T_PC,T_E>> p_list = graph_edge_composition_get_parallel_paths(stermg);
        ASSERT(p_list.size());
        list<graph_edge_composition_ref<T_PC,T_E>> new_list;
        for (auto const& ret : ret_list) {
          for (auto const& pt_ec : p_list) {
            graph_edge_composition_ref<T_PC,T_E> se = mk_series(ret, pt_ec);
            new_list.push_back(se);
          }
        }
        ret_list = new_list;
      }
      return ret_list;
    }
  }

  list<graph_edge_composition_ref<T_PC,T_E>> ret_list;
  for (const auto &pterm: ec_pterms) {
    graph_edge_composition_ref<T_PC,T_E> ptermg = dynamic_pointer_cast<graph_edge_composition_t<T_PC,T_E> const>(pterm);
    list_append(ret_list, graph_edge_composition_get_parallel_paths(ptermg));
  }
  return ret_list;
}

// returns (path_count, edge count (incl. epsilon), sum-of-product form edge_count)
// sum-of-product form is just parallel combination of all paths
template<typename T_PC, typename T_E>
tuple<int,int,int,int,int>
get_path_and_edge_counts_for_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& ec)
{
  if (!ec) {
    return make_tuple(0, 0, 0, 0, 0);
  }

  typedef int path_count_t;
  typedef int edge_count_t;
  typedef list<edge_count_t> sop_edge_count_t;
  function<sop_edge_count_t (sop_edge_count_t const& a, sop_edge_count_t const& b)> sop_cartesian_product = [](sop_edge_count_t const& a, sop_edge_count_t const& b) -> sop_edge_count_t
  {
    sop_edge_count_t ret;
    for (auto const& ea : a) {
      for (auto const& eb : b) {
        ret.push_back(ea+eb);
      }
    }
    return ret;
  };
  function<sop_edge_count_t (sop_edge_count_t const& a, sop_edge_count_t const& b)> sop_union = [](sop_edge_count_t const& a, sop_edge_count_t const& b) -> sop_edge_count_t
  {
    sop_edge_count_t ret = a;
    list_append(ret, b);
    return ret;
  };
  typedef tuple<path_count_t,edge_count_t,sop_edge_count_t> retval_t;
  function<retval_t (shared_ptr<T_E const> const&)> f = [](shared_ptr<T_E const> const &te) -> retval_t
  {
    if (te->is_empty()) {
      return retval_t(1, 1, {0}); // no epsilon edges in SOP form
    } else {
      return retval_t(1, 1, {1});
    }
  };
  function<retval_t (typename graph_edge_composition_t<T_PC,T_E>::type, retval_t const&, retval_t const&)> fold_f = [&sop_cartesian_product,&sop_union](typename graph_edge_composition_t<T_PC,T_E>::type ctype, retval_t const &a, retval_t const &b) -> retval_t
  {
    path_count_t pa = get<0>(a);             path_count_t pb = get<0>(b);
    edge_count_t ea = get<1>(a);             edge_count_t eb = get<1>(b);
    sop_edge_count_t const& sea = get<2>(a); sop_edge_count_t const& seb = get<2>(b);
    if (ctype == graph_edge_composition_t<T_PC,T_E>::series) {
      return retval_t(pa*pb, ea+eb, sop_cartesian_product(sea,seb));
    } else if (ctype == graph_edge_composition_t<T_PC,T_E>::parallel) {
      return retval_t(pa+pb, ea+eb, sop_union(sea,seb));
    } else NOT_REACHED();
  };
  retval_t ret = ec->visit_nodes(f, fold_f);
  int path_count = get<0>(ret);
  int edge_count = get<1>(ret);
  int sop_edge_count = accumulate(get<2>(ret).begin(), get<2>(ret).end(), 0);
  int max_edges_in_path = *max_element(get<2>(ret).begin(), get<2>(ret).end());
  int min_edges_in_path = *min_element(get<2>(ret).begin(), get<2>(ret).end());
  return make_tuple(path_count, edge_count, sop_edge_count, max_edges_in_path, min_edges_in_path);
}

template<typename T_PC, typename T_E>
string
edge_and_path_stats_string_for_edge_composition(graph_edge_composition_ref<T_PC,T_E> const& ec)
{
  auto path_edge_count = get_path_and_edge_counts_for_edge_composition(ec);
  int path_count = get<0>(path_edge_count);
  int edge_count = get<1>(path_edge_count);
  int sop_edge_count = get<2>(path_edge_count);
  int max_edges_in_path_count = get<3>(path_edge_count);
  int min_edges_in_path_count = get<4>(path_edge_count);

  stringstream ss;
  ss << "stats; path_count = " << path_count << "; edge_count = " << edge_count << "; sop_edge_count = " << sop_edge_count << "; max_path_length = " << max_edges_in_path_count << "; min_path_length = " << min_edges_in_path_count;
  return ss.str();
}

}
