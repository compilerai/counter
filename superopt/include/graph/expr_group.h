#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"
#include "graph/point_set_helper.h"
#include "expr/expr.h"
#include "expr/context.h"

namespace eqspace {

using namespace std;

class expr_group_t;

using expr_group_ref = shared_ptr<expr_group_t const>;


class expr_group_t
{
public:
  using expr_idx_t = size_t;
  using expr_group_type_t = enum { EXPR_GROUP_TYPE_ARR_EQ, EXPR_GROUP_TYPE_BV_EQ, EXPR_GROUP_TYPE_BV_INEQ, EXPR_GROUP_TYPE_MEMADDRS_DIFF, EXPR_GROUP_TYPE_HOUDINI};
private:
  expr_group_type_t m_type;
  vector<expr_ref> m_exprs;
  //list<expr_idx_t> m_bv_exprs;
  ssize_t m_max_bvlen;

  bool m_is_managed;
public:
  ~expr_group_t();
  size_t size() const { /*cout << __func__ << " " << __LINE__ << ": this = " << this << endl; */return m_exprs.size(); }
  bool is_array_group() const { return m_exprs.at(0)->is_array_sort(); }
  bool operator==(expr_group_t const &other) const { return m_type == other.m_type && m_exprs == other.m_exprs; }
  expr_group_type_t get_type() const { return m_type; }
  vector<expr_ref> const &get_expr_vec() const { return m_exprs; }
  void get_expr_vec(vector<expr_ref> &ev) const { ev = m_exprs; }
  expr_ref const &at(size_t idx) const { return m_exprs.at(idx); }
  //list<expr_idx_t> const &get_arr_indices() const { return m_arr_exprs; }
  //list<expr_idx_t> const &get_bv_indices() const { return m_bv_exprs; }
  ssize_t get_max_bvlen() const { return m_max_bvlen; }
  expr_idx_t expr_group_get_index_for_expr(expr_ref const &e) const
  {
    for (size_t i = 0; i < m_exprs.size(); i++) {
      if (e == m_exprs.at(i)) {
        return i;
      }
    }
    return (expr_idx_t)-1;
  }


  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<expr_group_t> *get_expr_group_manager();

  friend shared_ptr<expr_group_t const> mk_expr_group(expr_group_type_t t, vector<expr_ref> const &exprs);
  friend shared_ptr<expr_group_t const> mk_expr_group(istream& is, context* ctx, string const& prefix);

  static string expr_group_type_to_string(expr_group_type_t t)
  {
    switch (t) {
      case EXPR_GROUP_TYPE_ARR_EQ : return "ARR_EQ";
      case EXPR_GROUP_TYPE_BV_EQ : return "BV_EQ";
      case EXPR_GROUP_TYPE_BV_INEQ : return "BV_INEQ";
      case EXPR_GROUP_TYPE_MEMADDRS_DIFF: return "MEMADDRS_DIFF";
      case EXPR_GROUP_TYPE_HOUDINI: return "HOUDINI";
      default: NOT_REACHED();
    }
  }

  static expr_group_type_t expr_group_type_from_string(string const& str)
  {
    if (str == "ARR_EQ") {
      return EXPR_GROUP_TYPE_ARR_EQ;
    } else if (str == "BV_EQ") {
      return EXPR_GROUP_TYPE_BV_EQ;
    } else if (str == "BV_INEQ") {
      return EXPR_GROUP_TYPE_BV_INEQ;
    } else if (str == "MEMADDRS_DIFF") {
      return EXPR_GROUP_TYPE_MEMADDRS_DIFF;
    } else if (str == "HOUDINI") {
      return EXPR_GROUP_TYPE_HOUDINI;
    } else NOT_REACHED();
  }

  static void expr_group_type_to_stream(ostream& os, string const& prefix, expr_group_type_t t)
  {
    os << "=" + prefix + "expr_group_type " << expr_group_type_to_string(t) << endl;
  }

  static expr_group_type_t expr_group_type_from_stream(istream& is, string const& inprefix)
  {
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    string const prefix = "=" + inprefix + "expr_group_type ";
    if (!string_has_prefix(line, prefix)) {
      cout << __func__ << ':' << __LINE__ << ": expected `" << prefix << "' got `" << line << "'\n";
      cout.flush();
    }
    ASSERT(string_has_prefix(line, prefix));
    return expr_group_type_from_string(line.substr(prefix.length()));
  }

  void expr_group_to_stream(ostream& os, string const& inprefix) const
  {
    os << "=" + inprefix + "expr_group_type " << expr_group_type_to_string(m_type) << "\n";
    context::expr_vector_to_stream(os, inprefix, m_exprs);
  }

private:
  expr_group_t(expr_group_type_t t, vector<expr_ref> const &exprs) : m_type(t), m_exprs(exprs), m_is_managed(false)
  {
    this->compute_max_bvlen();
  }

  expr_group_t(istream& is, context* ctx, string const& prefix) : m_type(expr_group_type_from_stream(is, prefix)), m_exprs(ctx->expr_vector_from_stream(is, prefix)), m_is_managed(false)
  {
    this->compute_max_bvlen();
  }

  void compute_max_bvlen()
  {
    size_t idx = 0;
    m_max_bvlen = -1;
    for (auto const& e : m_exprs) {
      if(m_type == EXPR_GROUP_TYPE_HOUDINI) {
        ASSERT(e->is_bool_sort());
      } else if (e->is_array_sort()) {
        ASSERT(e->get_operation_kind() == expr::OP_MEMMASK);
      } else if (e->is_bv_sort()) {
        m_max_bvlen = max(m_max_bvlen, (ssize_t)e->get_sort()->get_size());
      } else if (e->is_bool_sort()) {
        m_max_bvlen = max(m_max_bvlen, (ssize_t)1);
      } else NOT_REACHED();
      idx++;
    }
  }
};

expr_group_ref mk_expr_group(expr_group_t::expr_group_type_t t, vector<expr_ref> const &exprs);
expr_group_ref mk_expr_group(istream& is, context* ctx, string const& prefix);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<expr_group_t>
{
  std::size_t operator()(expr_group_t const &egroup) const
  {
    std::size_t seed = 0;
    for (size_t i = 0; i < egroup.size(); ++i) {
      myhash_combine<size_t>(seed, egroup.at(i)->get_id());
    }
    return seed;
  }
};

}
