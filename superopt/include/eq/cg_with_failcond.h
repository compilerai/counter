#pragma once


#include "support/log.h"
#include "support/mymemory.h"

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include "tfg/tfg.h"
//#include "gsupport/edge_guard.h"
#include "expr/counter_example.h"
#include "graph/eqclasses.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"

#include "graph/graph_with_guessing.h"
//#include "graph/predicate_acceptance.h"
#include "gsupport/corr_graph_edge.h"
#include "gsupport/failcond.h"

namespace eqspace {

class cg_with_failcond : public cg_with_asm_annotation
{
private:
  pcpair m_pp;
  failcond_t m_failcond;
public:
  cg_with_failcond(cg_with_asm_annotation const &cg, pcpair const& pp, failcond_t const& failcond) : cg_with_asm_annotation(cg), m_pp(pp), m_failcond(failcond)
  { }
  pcpair const& get_pp() const { return m_pp; };
  failcond_t const& cg_get_failcond() const { return m_failcond; }
  unsigned long long cg_failcond_get_promise_metric() const;
  expr_ref cg_with_failcond_has_non_empty_incorrectness_cond_at_pc(pcpair const& pp) const;

  static dshared_ptr<cg_with_failcond> corr_graph_from_stream(istream& is, context* ctx);

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;
  virtual void cg_xml_print(ostream& os, string const& prefix) const override;
private:
  size_t count_num_unique_dst_nodes() const;
  size_t count_num_unique_src_nodes() const;
  size_t count_unique_nodes_using_select_function(std::function<pc (pcpair const&)> select_fn) const;
};

}
