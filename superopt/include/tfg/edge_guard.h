#pragma once
#include "expr/expr.h"
#include <vector>
#include <list>
#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"
#include "support/utils.h"
#include "expr/consts_struct.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {
class tfg;

class edge_guard_t {
private:
  shared_ptr<tfg_edge_composition_t> m_guard_node;

  //bool edge_guard_evaluates_to_true(tfg const &t) const;
public:
  edge_guard_t() {}
  const void *edge_guard_get_ptr() const { return m_guard_node.get(); }
  shared_ptr<tfg_edge_composition_t> get_edge_composition() const { return m_guard_node; }
  //edge_guard_t(shared_ptr<tfg_edge_composition_t> g) : m_guard_node(g) {}
  //edge_guard_t(edge_guard_t const &other);
  edge_guard_t(shared_ptr<tfg_edge const> const& e, tfg const &t);
  edge_guard_t(list<shared_ptr<tfg_edge const>> const &path, tfg const &t);
  edge_guard_t(shared_ptr<tfg_edge_composition_t> const &ec)
  {
    m_guard_node = ec;
  }
  //bool operator<(edge_guard_t const &other) const;
  //edge_guard_t &operator=(edge_guard_t const &other);
  bool operator==(edge_guard_t const &other) const;
  bool operator<(edge_guard_t const &other) const;
  //bool equals(edge_guard_t const &other) const;
  string edge_guard_to_string() const;
  string edge_guard_to_string_for_eq() const;
  void conjunct_guard(edge_guard_t const &other, tfg const &t);
  void add_guard_in_series(edge_guard_t const &other/*, tfg const &g*/);
  void add_guard_in_parallel(edge_guard_t const &other);
  expr_ref edge_guard_get_expr(tfg const &t, bool simplified) const;
  void edge_guard_to_stream(ostream& os) const;
  //pc edge_guard_get_to_pc() const;
  bool is_true() const;
  static string read_edge_guard(istream &in, string const& first_line/*, state const& start_state*/, context* ctx, edge_guard_t &guard);
  bool edge_guard_implies(edge_guard_t const &other) const;
  bool edge_guard_represents_all_possible_paths(tfg const &t) const;
  void edge_guard_trim_true_prefix_and_true_suffix(tfg const &t);
  //tfg const &get_tfg() const { return m_tfg; }
  void edge_guard_clear();
  bool edge_guard_is_trivial() const;
  bool edge_guard_may_imply(edge_guard_t const& other) const;
  static bool edge_guard_compare_ids(edge_guard_t const &a, edge_guard_t const &b);
};
}

namespace std{
using namespace eqspace;

template<>
struct hash<edge_guard_t>
{
  std::size_t operator()(edge_guard_t const &e) const
  {
    size_t seed = 0;
    if (e.get_edge_composition() == nullptr) {
      myhash_combine<size_t>(seed, std::hash<unsigned long>()(0ul));
    } else {
      myhash_combine<size_t>(seed, std::hash<graph_edge_composition_t<pc,tfg_edge>>()(*e.get_edge_composition().get()));
    }
    return seed;
  }
};


}
