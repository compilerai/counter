#pragma once

#include "expr/context.h"

#include "gsupport/graph_full_pathset.h"

namespace eqspace {

class correl_hints_t {
private:
  string m_cg_name_to_reach;
  map<tfg_full_pathset_t, tfg_full_pathset_t> m_preferred_associations;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> m_preferred_local_sprels_alloca;
  pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> m_preferred_local_sprels_dealloc;
public:
  correl_hints_t()
  { }
  ssize_t correl_hints_score_for_srcdst_paths_and_lsprels(string const& correl_entry_name, shared_ptr<tfg_full_pathset_t const> const& src_path, shared_ptr<tfg_full_pathset_t const> const& dst_path, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::alloc> const& lsprels_alloca, pc_local_sprel_expr_guesses_t<pclsprel_kind_t::dealloc> const& lsprels_dealloc) const;
  static correl_hints_t read_correl_hints(istream& ifs, context* ctx);
  string correl_hints_to_string(string const& prefix = "") const;
  void set_cg_name_to_reach(string const& cg_name);
  string const& get_cg_name_to_reach() const { return m_cg_name_to_reach; }
};

}
