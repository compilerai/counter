#pragma once

#include "support/dyn_debug.h"

#include "expr/local_sprel_expr_guesses.h"
#include "expr/pc.h"

namespace eqspace {

enum class pclsprel_kind_t { alloc, dealloc };
// Well-formed lsprel mappings with corresponding PC:
// * Each (pc,local-ID) can have atmost one mapping
// * No conflicts between addresses at any PC.
template<pclsprel_kind_t KIND>
class pc_local_sprel_expr_guesses_t
{
  //map<allocsite_t,pair<pc,expr_ref>> m_allocsite_to_pc_expr;
  map<pc,local_sprel_expr_guesses_t> m_pc_lsprels;

  map<variable_id_t, offset_t> m_local_offsets_from_compile_log; //offsets are w.r.t. input stack pointer (before pushing retaddr)

  // assumes well-formedness of pc_lsprels
  pc_local_sprel_expr_guesses_t(map<pc,local_sprel_expr_guesses_t> const& pc_lsprels) : m_pc_lsprels(pc_lsprels) {}

public:
  pc_local_sprel_expr_guesses_t() {}
  pc_local_sprel_expr_guesses_t(pc const& p, allocsite_t const& local, expr_ref const& addr)
  {
    bool ret = add_pc_lsprel_guess(p, local_sprel_expr_guesses_t({ local, addr }));
    ASSERT(ret);
  }
  pc_local_sprel_expr_guesses_t(pc const& p, local_sprel_expr_guesses_t const& lsprels)
  {
    bool ret = add_pc_lsprel_guess(p, lsprels);
    ASSERT(ret);
  }

  bool empty() const { return m_pc_lsprels.empty(); }
  size_t size() const
  {
    size_t ret = 0;
    for (auto const& [p,lsprels] : m_pc_lsprels) {
      ret += lsprels.size();
    }
    return ret;
  }
  map<pc,local_sprel_expr_guesses_t> const& get_map() const { return m_pc_lsprels; }
  map<variable_id_t, offset_t> const& get_local_offsets_from_compile_log() const { return m_local_offsets_from_compile_log; }

  bool have_mapping_for_local(allocsite_t const& lid) const;
  vector<pair<pc,expr_ref>> get_mappings_for_local(allocsite_t const& lid) const;
  pc_local_sprel_expr_guesses_t get_pc_lsprels_for_locals(set<allocsite_t> const& lids) const;

  local_sprel_expr_guesses_t get_all_lsprels() const;

  void read_guesses_from_compile_log(istream& is, context* ctx);

  bool add_pc_lsprel_guess(pc const& p, local_sprel_expr_guesses_t const& lsprels);
  bool pc_local_sprel_expr_guesses_union(pc_local_sprel_expr_guesses_t const& other);

  pc_local_sprel_expr_guesses_t pc_local_sprel_expr_guesses_intersection(pc_local_sprel_expr_guesses_t const& other) const;

  bool pc_local_sprel_expr_guesses_contains(pc_local_sprel_expr_guesses_t const& other) const;

  string to_string() const;
  void pc_local_sprel_expr_guesses_to_stream(ostream& os) const;
  string pc_local_sprel_expr_guesses_from_stream(istream& in, context* ctx);

  bool operator<(pc_local_sprel_expr_guesses_t const& other) const
  {
    return m_pc_lsprels < other.m_pc_lsprels;
  }
};

}
