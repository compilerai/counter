#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"

#include "cutils/chash.h"

#include "expr/eval.h"

#include "graph/point_set_helper.h"
#include "graph/expr_group.h"
#include "graph/point_val.h"

namespace eqspace {

using namespace std;

class point_t
{
private:
  string_ref m_point_name;
  map<expr_group_t::expr_idx_t, point_val_t> m_vals;

  bool m_is_managed;
public:
  ~point_t();
  /*void push_back(point_val_t const &pval)
  {
    m_vals.push_back(pval);
  }*/
  //size_t size() const
  //{
  //  return m_vals.size();
  //}
  map<expr_group_t::expr_idx_t, point_val_t> const& get_vals() const { return m_vals; }

  bool count(size_t idx) const
  {
    return m_vals.count(idx);
  }

  point_val_t const &at(size_t idx) const
  {
    return m_vals.at(idx);
  }
  bool operator==(point_t const &other) const
  {
    return (m_vals == other.m_vals && m_point_name == other.m_point_name);
  }

  void point_to_stream(ostream& os, map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs, string const& prefix) const;

  string point_to_string(string const& prefix, map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs) const
  {
    //ASSERT(this->size() == exprs->size());
    //ASSERT(map_get_keys(exprs->get_expr_vec()) == map_get_keys(m_vals));
    ostringstream ss;
    string new_prefix = prefix + "  ";
    ss << new_prefix << m_point_name->get_str() << endl;
    for (auto const& [i, pval] : m_vals) {
      if(exprs.count(i)) {
        ss << new_prefix << expr_string(exprs.at(i).get_expr()) << " -> " << pval.point_val_to_string() << endl;
      }
    }
    return ss.str();
  }

  string_ref get_point_name() const { return m_point_name; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<point_t> *get_point_manager();

  friend shared_ptr<point_t const> mk_point(string const& point_name, map<expr_group_t::expr_idx_t, point_val_t> const &vals);
  friend shared_ptr<point_t const> mk_point(istream& is, map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs, string const& prefix);
private:
  point_t(string const& point_name, map<expr_group_t::expr_idx_t, point_val_t> const &vals) : m_point_name(mk_string_ref(point_name)), m_vals(vals), m_is_managed(false)
  { }
  point_t(istream& is, map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs, string const& prefix);
};

using point_ref = shared_ptr<point_t const>;
point_ref mk_point(string const& point_name, map<expr_group_t::expr_idx_t, point_val_t> const &vals);
point_ref mk_point(istream& is, map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs, string const& prefix);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<point_t>
{
  std::size_t operator()(point_t const &p) const
  {
    std::size_t seed = 0;
    myhash_combine(seed, p.get_point_name()->get_str());
    for (auto const& [i, pval] : p.get_vals()) {
      myhash_combine<size_t>(seed, p.at(i).get_hash_value());
    }
    return seed;
  }
};

}
