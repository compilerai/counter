#pragma once

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"

#include "cutils/chash.h"

#include "expr/eval.h"

#include "graph/point_set_helper.h"
#include "graph/expr_group.h"

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
    } else if (e->is_float_sort()) {
      context* ctx = e->get_context();
      expr_ref b = expr_evaluates_to_constant(ctx->mk_float_to_ieee_bv(e));
      ASSERT(b->is_const());
      m_bv_val = expr2bv_const(b);
      m_is_arr = false;
    } else NOT_REACHED();
  }
  point_val_t(point_val_t const& other) : m_bv_val(other.m_bv_val), m_arr_val(other.m_arr_val), m_is_arr(other.m_is_arr)
  {
    stats_num_point_val_constructions++;
  }
  point_val_t(istream& is, expr_ref const& e);
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
    stringstream ss;
    point_val_to_stream(ss);
    return ss.str();
  }

  void point_val_to_stream(ostream& os) const
  {
    if (is_arr()) {
      os << m_arr_val->to_string();
    } else {
      os << m_bv_val;
    }
  }
};

}
