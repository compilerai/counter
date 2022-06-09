#pragma once

namespace eqspace {

class commonMEM_opt_res_t
{
private:
  bool m_nonstack_modeled_as_common_mem;
  map<expr_ref, set<expr_ref>> m_nonstck_memvar_to_orig_mem_map;
  bool m_stack_modeled_as_separate_mem;
  map<expr_ref, expr_ref> m_stck_memvar_to_orig_dst_memvar_map;
public:
  commonMEM_opt_res_t(bool nonstack_modeled_as_common_mem, map<expr_ref, set<expr_ref>> const& nonstck_memvar_to_orig_mem_map, bool stack_modeled_as_separate_mem, map<expr_ref, expr_ref> const& stck_memvar_to_orig_dst_memvar_map)
    : m_nonstack_modeled_as_common_mem(nonstack_modeled_as_common_mem), m_nonstck_memvar_to_orig_mem_map(nonstck_memvar_to_orig_mem_map), m_stack_modeled_as_separate_mem(stack_modeled_as_separate_mem), m_stck_memvar_to_orig_dst_memvar_map(stck_memvar_to_orig_dst_memvar_map)
  { }

  bool nonstack_modeled_as_common_mem() const { return m_nonstack_modeled_as_common_mem; }
  map<expr_ref, set<expr_ref>> const& get_nonstck_memvar_to_orig_mem_map() const { return m_nonstck_memvar_to_orig_mem_map; }
  bool stack_modeled_as_separate_mem() const { return m_stack_modeled_as_separate_mem; }
  map<expr_ref, expr_ref> const& get_stck_memvar_to_orig_dst_memvar_map() const { return m_stck_memvar_to_orig_dst_memvar_map; }
};

}
