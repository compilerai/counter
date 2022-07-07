#pragma once

#include "support/log.h"
#include "support/mymemory.h"
#include "support/bv_rank_val.h"

#include "expr/counter_example.h"

#include "rewrite/assembly_handling.h"

#include "gsupport/corr_graph_edge.h"
#include "gsupport/failcond.h"

#include "graph/eqclasses.h"
#include "graph/graph_with_guessing.h"

#include "tfg/tfg.h"

#include "eq/corr_graph.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

using namespace std;

class cg_with_rank : public corr_graph
{
private:
  mutable map<pcpair, bv_rank_val_t> m_bv_rank_map;
  mutable bv_rank_val_t m_bv_rank_total;
public:
  cg_with_rank(corr_graph const &cg, map<pcpair, bv_rank_val_t> const& bv_rank_map, bv_rank_val_t const& bv_rank_total) : corr_graph(cg), m_bv_rank_map(bv_rank_map), m_bv_rank_total(bv_rank_total)
  { }

  cg_with_rank(string_ref const &name, string_ref const& fname, dshared_ptr<eqcheck> const& eq) : corr_graph(name, fname, eq)
  { }

  cg_with_rank(cg_with_rank const& other) : corr_graph(other), m_bv_rank_map(other.m_bv_rank_map), m_bv_rank_total(other.m_bv_rank_total)
  { }

  bv_rank_val_t const& get_rank() const { return m_bv_rank_total; }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;
  static dshared_ptr<cg_with_rank> corr_graph_from_stream(istream& is, context* ctx);

  //add_CE_at_pc will now also update the rank
  virtual bool add_CE_at_pc(pcpair const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<pcpair, corr_graph_node, corr_graph_edge> const &nce) const override;
  void recompute_rank_at_pc(pcpair const& p) const;

protected:
private:
  bv_rank_val_t compute_bv_rank_at_pc(pcpair const& p) const;
};

}
