#pragma once

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/context.h"

#include "graph/point_exprs.h"
#include "graph/point_set_helper.h"

namespace eqspace {

using namespace std;

class expr_group_t;

using expr_group_ref = shared_ptr<expr_group_t const>;


class expr_group_t
{
public:
  using expr_idx_t = bv_solve_var_idx_t;
  static expr_idx_t EXPR_IDX_INVALID;
  using expr_group_type_t = enum { EXPR_GROUP_TYPE_ARR_EQ, EXPR_GROUP_TYPE_BV_EQ, EXPR_GROUP_TYPE_BV_INEQ, EXPR_GROUP_TYPE_MEMADDRS_DIFF, EXPR_GROUP_TYPE_HOUDINI, EXPR_GROUP_TYPE_HOUDINI_AXIOM_BASED, EXPR_GROUP_TYPE_HOUDINI_EXPECTS_STABILITY, EXPR_GROUP_TYPE_BV_CONST_INEQ };
private:
  string_ref m_name;
  expr_group_type_t m_type;
  map<expr_idx_t, point_exprs_t> m_exprs;
  //map<expr_ref, int> m_exprs_ranking;
  //list<expr_idx_t> m_bv_exprs;
  ssize_t m_max_bvlen;

  bool m_is_managed;
public:
  ~expr_group_t();
  //size_t size() const { /*cout << __func__ << " " << __LINE__ << ": this = " << this << endl; */return m_exprs.size(); }
  size_t expr_group_get_num_exprs() const { return m_exprs.size(); }
  bool is_array_group() const { ASSERT(m_exprs.size()); return m_exprs.begin()->second.get_expr()->is_array_sort(); }
  expr_idx_t expr_group_get_nth_index(size_t n) const
  {
    ASSERT(n < m_exprs.size());
    auto iter = m_exprs.begin();
    for (auto i = 0; i < n; i++, iter++);
    return iter->first;
  }
  bool operator==(expr_group_t const &other) const { return m_type == other.m_type && m_exprs == other.m_exprs; }
  bool expr_group_equals(expr_group_t const &other) const;
  string get_name() const { return m_name->get_str(); }
  string_ref get_name_ref() const { return m_name; }
  expr_group_type_t get_type() const { return m_type; }
  map<expr_idx_t, point_exprs_t> const &get_expr_vec() const { return m_exprs; }
  //void get_expr_vec(map<expr_idx_t, expr_ref> &ev) const { ev = m_exprs; }
  point_exprs_t const &expr_group_get_point_exprs_at_index(size_t idx) const { return m_exprs.at(idx); }
  bool count(size_t idx) const { return m_exprs.count(idx); }
  //list<expr_idx_t> const &get_arr_indices() const { return m_arr_exprs; }
  //list<expr_idx_t> const &get_bv_indices() const { return m_bv_exprs; }
  ssize_t get_max_bvlen() const { return m_max_bvlen; }
  //map<expr_ref, int> const& get_exprs_ranking_map() const { return m_exprs_ranking;}
  //int get_ranking_for_expr(expr_ref const& e) const {
  //  int ret = 0;
  //  if(m_exprs_ranking.count(e))
  //    ret = m_exprs_ranking.at(e);
  //  return ret;
  //}
  //int get_ranking_for_expr_at(expr_idx_t idx) const {
  //  expr_ref const& e = this->at(idx);
  //  return get_ranking_for_expr(e);
  //}
  expr_idx_t expr_group_get_index_for_expr(expr_ref const &e) const
  {
    for (auto const& [id,me] : m_exprs) {
      if (e == me.get_expr()) {
        return id;
      }
    }
    return EXPR_IDX_INVALID;
  }

  bool expr_group_has_ghost_exprs(context* ctx) const
  {
    for (auto const& [id,pe] : m_exprs) {
       expr_ref e = pe.get_expr();
       if (e->is_var() && ctx->key_represents_ghost_var(e->get_name()))
        return true;
    }
    return false;
  }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<expr_group_t> *get_expr_group_manager();

  friend shared_ptr<expr_group_t const> mk_expr_group(string const& name, expr_group_type_t t, map<expr_idx_t, point_exprs_t> const &exprs);
  friend shared_ptr<expr_group_t const> mk_expr_group(string const& name, expr_group_type_t t, map<expr_idx_t, point_exprs_t> const &exprs, map<expr_ref, int> const& exprs_ranking);
  friend shared_ptr<expr_group_t const> mk_expr_group(istream& is, context* ctx, string const& prefix);

  static string expr_group_type_to_string(expr_group_type_t t)
  {
    switch (t) {
      case EXPR_GROUP_TYPE_ARR_EQ : return "ARR_EQ";
      case EXPR_GROUP_TYPE_BV_EQ : return "BV_EQ";
      case EXPR_GROUP_TYPE_BV_INEQ : return "BV_INEQ";
      case EXPR_GROUP_TYPE_MEMADDRS_DIFF: return "MEMADDRS_DIFF";
      case EXPR_GROUP_TYPE_HOUDINI: return "HOUDINI";
      case EXPR_GROUP_TYPE_HOUDINI_AXIOM_BASED: return "HOUDINI_AXIOM_BASED";
      case EXPR_GROUP_TYPE_HOUDINI_EXPECTS_STABILITY: return "HOUDINI_EXPECTS_STABILITY";
      case EXPR_GROUP_TYPE_BV_CONST_INEQ: return "BV_CONST_INEQ";
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
    } else if (str == "HOUDINI_AXIOM_BASED") {
      return EXPR_GROUP_TYPE_HOUDINI_AXIOM_BASED;
    } else if (str == "HOUDINI_EXPECTS_STABILITY") {
      return EXPR_GROUP_TYPE_HOUDINI_EXPECTS_STABILITY;
    } else if (str == "BV_CONST_INEQ") {
      return EXPR_GROUP_TYPE_BV_CONST_INEQ;
    } else NOT_REACHED();
  }

  //static void expr_group_type_to_stream(ostream& os, string const& prefix, expr_group_type_t t)
  //{
  //  os << "=" + prefix + "expr_group_type " << expr_group_type_to_string(t) << endl;
  //}

  static expr_idx_t expr_idx_begin() { return 1; }

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

  static string_ref name_from_stream(istream& is, string const& inprefix)
  {
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    string const prefix = "=" + inprefix + "name ";
    if (!string_has_prefix(line, prefix)) {
      cout << __func__ << ':' << __LINE__ << ": expected `" << prefix << "' got `" << line << "'\n";
      cout.flush();
    }
    DYN_DEBUG(graph_parser, cout << _FNLN_ << ": line = '" << line << "'\n");
    ASSERT(string_has_prefix(line, prefix));
    return mk_string_ref(line.substr(prefix.length()));
  }

  string expr_group_to_string() const;
  void expr_group_to_stream(ostream& os, string const& inprefix) const
  {
    os << "=" << inprefix << "name " << m_name->get_str() << '\n';
    os << "=" + inprefix + "expr_group_type " << expr_group_type_to_string(m_type) << "\n";
    //context::expr_vector_to_stream(os, inprefix, m_exprs);
    point_exprs_map_to_stream(m_exprs, os, inprefix);
    os << "=" << inprefix << "done\n";
  }

  static void point_exprs_map_to_stream(map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs, ostream& os, string const& inprefix)
  {
    for (auto const& [id,pe] : exprs) {
      os << "=" << inprefix << "point_expr " << id << endl;
      pe.point_exprs_to_stream(os);
    }
  }
  
  static map<expr_idx_t, point_exprs_t> point_exprs_map_from_stream(istream& is, context* ctx, string const& prefix);

private:
  //expr_group_t(string const& name, expr_group_type_t t, map<expr_idx_t, expr_ref> const &exprs, map<expr_ref, int> const& exprs_ranking) : m_name(mk_string_ref(name)), m_type(t), m_exprs(exprs), m_exprs_ranking(exprs_ranking), m_is_managed(false)
  //{
  //  ASSERT(t == EXPR_GROUP_TYPE_BV_EQ);
  //  this->compute_max_bvlen();
  //}
  expr_group_t(string const& name, expr_group_type_t t, map<expr_idx_t, point_exprs_t> const &exprs) : m_name(mk_string_ref(name)), m_type(t), m_exprs(exprs), m_is_managed(false)
  {
    this->compute_max_bvlen();
  }

  expr_group_t(istream& is, context* ctx, string const& prefix) : m_name(name_from_stream(is, prefix)), m_type(expr_group_type_from_stream(is, prefix)), m_exprs(expr_group_t::point_exprs_map_from_stream(is, ctx, prefix)), m_is_managed(false)
  {
    this->compute_max_bvlen();
  }


  void compute_max_bvlen();
};

expr_group_ref mk_expr_group(string const& name, expr_group_t::expr_group_type_t t, map<expr_group_t::expr_idx_t, point_exprs_t> const &exprs);
expr_group_ref mk_expr_group(string const& name, expr_group_t::expr_group_type_t t, map<expr_group_t::expr_idx_t, point_exprs_t> const &exprs, map<expr_ref, int> const& exprs_ranking);
expr_group_ref mk_expr_group(istream& is, context* ctx, string const& prefix);
expr_group_ref expr_group_union(expr_group_ref const& a, expr_group_ref const& b);
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
    myhash_combine<size_t>(seed, hash<string_obj_t>()(*egroup.get_name_ref()));
    for (auto const& [id,pe] : egroup.get_expr_vec()) {
      myhash_combine<size_t>(seed, pe.get_expr()->get_id());
    }
    return seed;
  }
};

}
