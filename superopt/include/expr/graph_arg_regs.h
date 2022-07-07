#pragma once

#include "support/types.h"

#include "expr/context.h"
#include "expr/expr.h"
#include "expr/graph_arg.h"

namespace eqspace {

using namespace std;

class graph_arg_regs_t {
private:
  map<string_ref, graph_arg_t> m_regs;
public:
  graph_arg_regs_t()
  { }
  graph_arg_regs_t(map<string_ref, graph_arg_t> const& regs) : m_regs(regs)
  { }
  graph_arg_regs_t(istream& is, context* ctx)
  {
    string line;
    bool end;

    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(line == "=graph_arg_regs");

    end = !getline(is, line);
    ASSERT(!end);

    while (line != "=graph_arg_regs done") {
      string prefix = "=graph_arg_reg ";
      ASSERT(string_has_prefix(line, prefix));
      string regname = line.substr(prefix.size());

      graph_arg_t arg(is, ctx, "");

      end = !getline(is, line);
      ASSERT(!end);

      m_regs.insert(make_pair(mk_string_ref(regname), arg));
    }
    ASSERT(line == "=graph_arg_regs done");
  }
  map<string_ref, graph_arg_t> const& get_map() const { return m_regs; }
  size_t size() const { return m_regs.size(); }
  size_t get_num_bytes_on_stack_including_retaddr() const
  {
    size_t ret = SRC_LONG_BITS/BYTE_LEN; //for return address
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    for (const auto &a : m_regs) {
      ASSERT(a.second.get_val()->is_bool_sort() || a.second.get_val()->is_bv_sort() || a.second.get_val()->is_float_sort() || a.second.get_val()->is_floatx_sort());
      if (a.second.get_val()->is_bool_sort()) {
        ret += SRC_LONG_BITS/BYTE_LEN;
        //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      } else {
        ret += DIV_ROUND_UP(a.second.get_val()->get_sort()->get_size(), SRC_LONG_BITS) * (SRC_LONG_BITS/BYTE_LEN);
        //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      }
    }
    return ret;
  }
  string expr_get_argname(expr_ref const &e) const;
  string expr_get_arg_address(expr_ref const &e) const;
  bool argname_represents_vararg(string const &argname) const;
  bool expr_is_arg(expr_ref const& e) const
  {
    return expr_get_argname(e) != "";
  }
  static graph_arg_id_t get_argnum_from_argname(string const& argname)
  {
    ASSERT(string_has_prefix(argname, LLVM_METHOD_ARG_PREFIX));
    return string_to_int(argname.substr(strlen(LLVM_METHOD_ARG_PREFIX)));
  }
  static string get_argname_from_argnum(graph_arg_id_t arg_id)
  {
    stringstream ss;
    ss << LLVM_METHOD_ARG_PREFIX << arg_id;
    return ss.str();
  }



  bool count(string_ref const& s) const { return m_regs.count(s) != 0; }
  expr_ref const& at(string_ref const& s) const { return m_regs.at(s).get_val(); }
  expr_ref const& addr_at(string_ref const& s) const { return m_regs.at(s).get_addr(); }

  void graph_arg_regs_substitute_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);
  void graph_arg_regs_update_addr_for_argname(string_ref const& argname, expr_ref const& addr);
  bool graph_arg_regs_are_equal(graph_arg_regs_t const& other) const;

  string_ref get_argument_with_name(string const& s) const;

  void graph_arg_regs_to_stream(ostream& os) const
  {
    os << "=graph_arg_regs\n";
    for (auto const& r : m_regs) {
      os << "=graph_arg_reg " << r.first->get_str() << endl;

      r.second.graph_arg_to_stream(os, "");
      //context* ctx = r.second->get_context();
      //os << ctx->expr_to_string_table(r.second) << endl;
    }
    os << "=graph_arg_regs done\n";
  }
};

}
