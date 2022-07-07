#pragma once

#include "graph/expr_group.h"

namespace eqspace {

using namespace std;

class eqclasses_t
{
public:
  using expr_group_id_t = int;
private:
  map<expr_group_id_t, expr_group_ref> m_classes;

  bool m_is_managed;
public:
  ~eqclasses_t();
  map<expr_group_id_t, expr_group_ref> const &get_expr_group_ls() const
  { return m_classes; }
  bool operator==(eqclasses_t const &eqc) const { return m_classes == eqc.m_classes; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  void eqclasses_to_stream(ostream& os, string const& prefix) const;
  static manager<eqclasses_t> *get_eqclasses_manager();
  friend shared_ptr<eqclasses_t const> mk_eqclasses(map<expr_group_id_t, expr_group_ref> const &expr_group_ls);
  friend shared_ptr<eqclasses_t const> mk_eqclasses(istream& is, context* ctx, string const& prefix);
private:
  eqclasses_t() : m_is_managed(false)
  { }
  eqclasses_t(map<expr_group_id_t, expr_group_ref> const &classes) : m_classes(classes), m_is_managed(false)
  { }
  eqclasses_t(istream& is, context* ctx, string const& prefix);
};

using eqclasses_ref = shared_ptr<eqclasses_t const>;
eqclasses_ref mk_eqclasses(map<eqclasses_t::expr_group_id_t, expr_group_ref> const &expr_group_ls);
eqclasses_ref mk_eqclasses(istream& is, context* ctx, string const& prefix);

bool expr_pred_def_is_likely_nearer(expr_ref const &a, expr_ref const &b);
expr_group_ref sort_exprs_and_compute_bv_eqclass(set<expr_ref> const& input_exprs/*, map<expr_ref, int> const& exprs_ranking = map<expr_ref, int>()*/);
}

namespace std
{
using namespace eqspace;
template<>
struct hash<eqclasses_t>
{
  std::size_t operator()(eqclasses_t const &eqc) const
  {
    std::size_t seed = 0;
    std::hash<expr_group_t> h;
    for (auto const& eg : eqc.get_expr_group_ls()) {
      myhash_combine<size_t>(seed, h(*eg.second));
    }
    return seed;
  }
};
}
