#pragma once
#include "support/dshared_ptr.h"

#include "expr/expr.h"

namespace eqspace {

class cg_with_failcond;
class cg_with_asm_annotation;

class failed_cg_set_t {
private:
  list<dshared_ptr<cg_with_failcond>> m_ls;
public:
  void failed_cg_set_add(dshared_ptr<cg_with_failcond> const& cgf);
  list<dshared_ptr<cg_with_failcond>> const& failed_cg_set_get_ls() const;
  void failed_cg_set_compute_correctness_covers();
  void failed_cg_set_sort();
  void failed_cg_set_prune(int n);
  void failed_cg_set_to_stream(ostream& os) const;
  pair<dshared_ptr<cg_with_asm_annotation const>, expr_ref> failed_cg_set_proves_inequivalence() const;
  void clear() { m_ls.clear(); }
};

}
