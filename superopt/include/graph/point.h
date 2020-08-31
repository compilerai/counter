#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"
#include "graph/point_set_helper.h"
#include "cutils/chash.h"

namespace eqspace {

using namespace std;

class point_val_t
{
private:
  bv_const m_bv_val;
  array_constant_ref m_arr_val;
  bool m_is_arr;
public:
  //point_val_t(bv_const const &val) : m_bv_val(val), is_arr(false)
  //{ }
  //point_val_t(array_constant const &val) : m_arr_val(val), is_arr(true)
  //{ }
  point_val_t(expr_ref const &e)
  {
    stats_num_point_val_constructions++;
    ASSERT(e->is_const());
    if (e->is_array_sort()) {
      m_arr_val = e->get_array_constant();
      m_is_arr = true;
    } else if (e->is_bv_sort() || e->is_bool_sort()) {
      m_bv_val = expr2bv_const(e);
      m_is_arr = false;
    } else NOT_REACHED();
  }
  point_val_t(point_val_t const& other) : m_bv_val(other.m_bv_val), m_arr_val(other.m_arr_val), m_is_arr(other.m_is_arr)
  {
    stats_num_point_val_constructions++;
  }
  ~point_val_t()
  {
    stats_num_point_val_destructions++;
  }

  bool operator==(point_val_t const &other) const
  {
    if (this->m_is_arr != other.m_is_arr) {
      return false;
    }
    if (this->m_is_arr) {
      //return m_arr_val->equals(*other.m_arr_val);
      return m_arr_val == other.m_arr_val;
    } else {
      return m_bv_val == other.m_bv_val;
    }
  }

  bool is_arr() const { return m_is_arr; }
  array_constant_ref const &get_arr_const() const { ASSERT(m_is_arr); return m_arr_val; }
  bv_const const &get_bv_const() const { ASSERT(!m_is_arr); return m_bv_val; }

  size_t get_hash_value() const
  {
    return myhash_string(this->point_val_to_string().c_str());
  }

  string point_val_to_string() const
  {
    if (is_arr()) {
      return m_arr_val->to_string();
    } else {
      stringstream ss;
      ss << m_bv_val;
      return ss.str();
    }
  }
};

class point_t
{
private:
  vector<point_val_t> m_vals;

  bool m_is_managed;
public:
  ~point_t();
  /*void push_back(point_val_t const &pval)
  {
    m_vals.push_back(pval);
  }*/
  size_t size() const
  {
    return m_vals.size();
  }
  vector<point_val_t> const& get_vals() const { return m_vals; }
  point_val_t const &at(size_t idx) const
  {
    return m_vals.at(idx);
  }
  bool operator==(point_t const &other) const
  {
    return m_vals == other.m_vals;
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<point_t> *get_point_manager();

  friend shared_ptr<point_t const> mk_point(vector<point_val_t> const &vals);
private:
  point_t(vector<point_val_t> const &vals) : m_vals(vals), m_is_managed(false)
  { }
};

using point_ref = shared_ptr<point_t const>;
point_ref mk_point(vector<point_val_t> const &vals);

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
    for (size_t i = 0; i < p.size(); i++) {
      myhash_combine<size_t>(seed, p.at(i).get_hash_value());
    }
    return seed;
  }
};

}
