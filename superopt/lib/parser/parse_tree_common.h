#pragma once

#include <string>
#include <map>
#include <vector>
#include <stack>
#include <set>

#include "expr/defs.h"
#include "support/debug.h"
#include "support/bv_const.h"

using namespace std;

namespace eqspace {

struct Env {
  /* Why distinguish b/w global and local funcs?
   *  global funcs are passed down to user -- local funcs are not
   *
   * we maintain both the sort (i.e. domain and sort) and actual expression of the function
   * sort information is essential for type matching
   */
  set<string> m_global_funcs; /* these are returned to user */
  map<string, pair<expr_ref,sort_ref>> m_funcs;
  unsigned m_undefined_counter;    /* the counter of undefined functions, we use it for tracking completion
                                      0 => evaluation is complete */

  // env info specific to current function
  // TODO -- add sort?
  vector<pair<string, expr_ref>> m_letfuncs;   /* stack of funcs/exprs declared with "let"
                                                  we push/pop these as we eval */
  vector<expr_ref> m_curfuncparams;       /* the params of currently evaluated function;
                                             the exprs are just vars */
  stack<vector<expr_ref>> m_lambdaparams;    /* stack the params of currently evaluated lambda function */
  string m_curfuncname;
  sort_ref m_curfuncsort;

  void add_letfunc(string const& name, expr_ref const& e)
  {
    ASSERT(e != nullptr);
    m_letfuncs.push_back(make_pair(name, e));
  }

  void pop_letfunc()
  {
    ASSERT(m_letfuncs.size());
    m_letfuncs.pop_back();
  }

  expr_ref lookup_letfunc(string const& q)
  {
    /* search in stack order -- most recent to least */
    for (auto ritr = m_letfuncs.crbegin(); ritr != m_letfuncs.crend(); ++ritr) {
      if (ritr->first == q)
        return ritr->second;
    }
    return nullptr;
  }
};

pair<int,bv_const> str_to_bvlen_val_pair(string const& s);
expr_ref resolve_params(expr_ref const& e, vector<sort_ref> const& dom, sort_ref const& range, vector<expr_ref>const& params, string const& name = "");

}
