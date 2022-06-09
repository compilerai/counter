#pragma once

#include "support/mybitset.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/free_id_list.h"
#include "support/utils.h"

#include "expr/expr.h"
#include "expr/pc.h"

namespace eqspace {

class scev_t;

using scev_ref = shared_ptr<scev_t const>;

enum scev_op_t { scev_op_constant, scev_op_truncate, scev_op_zeroext, scev_op_signext, scev_op_anyext, scev_op_addrec, scev_op_add, scev_op_mul, scev_op_umax, scev_op_smax, scev_op_umin, scev_op_smin, scev_op_udiv, scev_op_urem, scev_op_unknown, scev_op_couldnotcompute };

class scev_overflow_flag_t {
public:
  using enum_flags_t = enum { scev_overflow_flag_nuw, scev_overflow_flag_nsw, scev_overflow_flag_nw };
private:
  set<enum_flags_t> m_enum_flags;
public:
  scev_overflow_flag_t() {}

  bool operator==(scev_overflow_flag_t const& other) const;
  void add(enum_flags_t flag)
  {
    m_enum_flags.insert(flag);
  }
  bool is_empty() const { return m_enum_flags.empty(); }

  static string enum_flag_to_string(enum_flags_t enum_flag)
  {
    switch (enum_flag) {
      case scev_overflow_flag_nuw: return "nuw";
      case scev_overflow_flag_nsw: return "nsw";
      case scev_overflow_flag_nw: return "nw";
      default: NOT_REACHED();
    }
  }

  static enum_flags_t enum_flag_from_string(string const& s)
  {
    if (s == "nuw") return scev_overflow_flag_nuw;
    if (s == "nsw") return scev_overflow_flag_nsw;
    if (s == "nw")  return scev_overflow_flag_nw;
    cout << _FNLN_ << ": s = '" << s << "'\n";
    NOT_REACHED();
  }

  string scev_overflow_flag_to_string() const
  {
    stringstream ss;
    bool first = true;

    for (auto const& ef : m_enum_flags) {
      if (!first) {
        ss << " ";
      }
      ss << enum_flag_to_string(ef);
      first = false;
    }
    return ss.str();
  }

  static scev_overflow_flag_t scev_overflow_flag_from_string(string s);
};

class scev_t {
private:
  scev_op_t m_scev_op;
  mybitset m_init;
  vector<scev_ref> m_args;
  pc m_loop;
  scev_overflow_flag_t m_scev_overflow_flag;
  string m_varname;
  size_t m_varname_bvsize = 0;

  scev_id_t m_id;
public:
  scev_t() = delete;
  ~scev_t();
  friend scev_ref mk_scev(scev_op_t op, mybitset const& init_val, vector<scev_ref> const& args, pc const& loop_pc, scev_overflow_flag_t scev_overflow_flag);
  friend scev_ref mk_scev(istream& is, string const& scev_name);
  friend scev_ref mk_scev(scev_op_t op, string const& varname, size_t varname_bvsize);
  bool operator==(scev_t const &) const;

  bool id_is_zero() const { return m_id == 0; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(m_id == 0);
    m_id = get_free_scev_id_list().get_free_id(suggested_id);
  }
  static manager<scev_t>& get_scev_manager();
  unsigned get_id() const { return m_id; }
  static free_id_list& get_free_scev_id_list();

  scev_op_t get_scev_op() const { return m_scev_op; }
  mybitset const& get_init_val() const { return m_init; }
  vector<scev_ref> const& get_args() const { return m_args; }
  pc const& get_loop_pc() const { return m_loop; }
  scev_overflow_flag_t const& get_scev_overflow_flag() const { return m_scev_overflow_flag; }
  string const& get_varname() const { return m_varname; }
  size_t get_varname_bvsize() const { return m_varname_bvsize; }
  void scev_to_stream(ostream& os, string const& prefix) const;
  string scev_to_string() const;

  static bool scev_is_loop_invariant(scev_ref const& scev, pc const& loop_head, set<expr_ref> const& loop_invariant_exprs, context* ctx);

  static string scev_op_to_string(scev_op_t op);
  static scev_op_t read_scev_op_string(string const& s);

private:
  scev_t(scev_op_t scev_op, mybitset const& init_val, vector<scev_ref> const& args, pc const& loop_pc, scev_overflow_flag_t scev_overflow_flag);
  scev_t(istream& is, string const& scev_name);
  scev_t(scev_op_t op, string const& varname, size_t varname_bvsize);
};

scev_ref mk_scev(scev_op_t op, mybitset const& init_val, vector<scev_ref> const& args, pc const& loop_pc = pc::start(), scev_overflow_flag_t scev_overflow_flag = scev_overflow_flag_t());
scev_ref mk_scev(istream& is, string const& scev_name);
scev_ref mk_scev(scev_op_t op, string const& varname, size_t varname_bvsize);

class scev_expr_visitor
{
public:
  scev_expr_visitor()
  { }
  virtual ~scev_expr_visitor() = default;

protected:
  void visit_recursive(scev_t const &s)
  {
    if (m_visited.count(s.get_id()) == 0) {
      m_visited.insert(s.get_id());
      for (auto const& arg : s.get_args()) {
        visit_recursive(*arg);
      }
      visit(s);
    }
  }

  virtual void visit(scev_t const &e) = 0;

private:
  set<scev_id_t> m_visited;
};

}

pair<scev_ref, map<scev_id_t, scev_ref>> parse_scev(const char* str);

namespace std
{
template<>
struct hash<eqspace::scev_t>
{
  std::size_t operator()(eqspace::scev_t const &s) const
  {
    return 0;
  }
};
}
