#pragma once

#include "i386/insn.h"
#include "x64/insntypes.h"

#define X64_MAX_NUM_OPERANDS I386_MAX_NUM_OPERANDS
#define x64_strict_canonicalization_state_t i386_strict_canonicalization_state_t

#define x64_iseq_compute_execution_cost i386_iseq_compute_execution_cost
#define x64_insn_to_int_array i386_insn_to_int_array
#define x64_insn_get_pcrel_values i386_insn_get_pcrel_values
#define x64_insn_set_pcrel_values i386_insn_set_pcrel_values
#define x64_insn_is_indirect_branch i386_insn_is_indirect_branch

#define x64_insn_put_nop i386_insn_put_nop
#define x64_insn_is_nop i386_insn_is_nop

#define x64_insn_copy i386_insn_copy
#define x64_iseq_copy i386_iseq_copy

#define x64_insn_put_jump_to_nextpc i386_insn_put_jump_to_nextpc

#define x64_insn_canonicalize i386_insn_canonicalize

#define x64_insn_get_max_num_imm_operands i386_insn_get_max_num_imm_operands

#define x64_operand_strict_canonicalize_regs i386_operand_strict_canonicalize_regs

#define x64_memtouch_strict_canonicalize i386_memtouch_strict_canonicalize
#define x64_operand_strict_canonicalize_imms i386_operand_strict_canonicalize_imms
#define x64_insn_inv_rename_registers i386_insn_inv_rename_registers
#define x64_insn_inv_rename_memory_operands i386_insn_inv_rename_memory_operands
#define x64_insn_inv_rename_constants i386_insn_inv_rename_constants
#define x64_insn_disassemble i386_insn_disassemble
#define x64_exregname_suffix i386_exregname_suffix
#define x64_infer_regcons_from_assembly i386_infer_regcons_from_assembly

#define x64_assemble x86_assemble

#define x64_insn_get_pcrel_operands i386_insn_get_pcrel_operands

#define x64_insn_add_to_pcrels i386_insn_add_to_pcrels

#define x64_insn_hash_canonicalize i386_insn_hash_canonicalize

#define x64_insn_canonicalize_locals_symbols i386_insn_canonicalize_locals_symbols
#define x64_insn_is_intdiv i386_insn_is_intdiv
#define x64_insn_is_unconditional_branch_not_fcall i386_insn_is_unconditional_indirect_branch_not_fcall
#define x64_insn_is_unconditional_branch i386_insn_is_unconditional_branch
#define x64_insn_is_unconditional_indirect_branch_not_fcall i386_insn_is_unconditional_indirect_branch_not_fcall
#define x64_insn_is_branch_not_fcall i386_insn_is_branch_not_fcall
#define x64_insn_is_indirect_function_call i386_insn_is_indirect_function_call
#define x64_insn_is_direct_function_call i386_insn_is_direct_function_call
#define x64_insn_fcall_target i386_insn_fcall_target
#define x64_insn_terminates_bbl i386_insn_terminates_bbl
#define x64_insn_is_unconditional_direct_branch_not_fcall i386_insn_is_unconditional_direct_branch_not_fcall
#define x64_insn_branch_targets i386_insn_branch_targets
#define x64_insn_is_indirect_branch_not_fcall i386_insn_is_indirect_branch_not_fcall

#define x64_insn_less i386_insn_less

#define x64_insn_get_constant_operand i386_insn_get_constant_operand
#define x64_insn_merits_optimization i386_insn_merits_optimization

#define x64_iseq_matches_template i386_iseq_matches_template

#define x64_insn_is_unconditional_direct_branch i386_insn_is_unconditional_direct_branch
#define x64_insn_is_function_call i386_insn_is_function_call

#define x64_insn_is_conditional_direct_branch i386_insn_is_conditional_direct_branch
#define x64_insn_invert_branch_condition i386_insn_invert_branch_condition
#define x64_insn_is_conditional_direct_branch i386_insn_is_conditional_direct_branch
#define x64_insn_is_conditional_indirect_branch i386_insn_is_conditional_indirect_branch
#define x64_insn_put_jump_to_pcrel i386_insn_put_jump_to_pcrel
#define x64_insn_put_jump_to_src_insn_pc i386_insn_put_jump_to_src_insn_pc

#define x64_insn_rename_constants i386_insn_rename_constants
#define x64_insn_put_movl_nextpc_constant_to_mem i386_insn_put_movl_nextpc_constant_to_mem
#define x64_insn_put_jump_to_indir_mem i386_insn_put_jump_to_indir_mem
#define x64_insn_put_movl_reg_var_to_mem i386_insn_put_movl_reg_var_to_mem
#define x64_insn_put_movl_mem_to_reg_var i386_insn_put_movl_mem_to_reg_var
#define x64_insn_put_move_mem_to_mem i386_insn_put_move_mem_to_mem
#define x64_insn_put_xchg_exreg_with_exreg i386_insn_put_xchg_exreg_with_exreg
#define x64_insn_put_xchg_reg_with_mem i386_insn_put_xchg_reg_with_mem
#define x64_insn_put_move_exreg_to_mem i386_insn_put_move_exreg_to_mem
#define x64_insn_put_zextend_exreg i386_insn_put_zextend_exreg
#define x64_iseq_inv_rename_imms_using_imm_vt_map i386_iseq_inv_rename_imms_using_imm_vt_map
#define x64_insn_put_move_mem_to_exreg i386_insn_put_move_mem_to_exreg
#define x64_insn_put_move_exreg_to_exreg i386_insn_put_move_exreg_to_exreg
#define x64_insn_inv_rename_registers i386_insn_inv_rename_registers

#define x64_iseq_rename i386_iseq_rename

#define x64_insn_inv_rename_locals_symbols i386_insn_inv_rename_locals_symbols
#define x64_insn_rename_symbols_to_addresses i386_insn_rename_symbols_to_addresses
#define x64_insns_equal i386_insns_equal
#define x64_pc_get_function_name_and_insn_num i386_pc_get_function_name_and_insn_num
#define x64_enumerate_in_out_transmaps_for_usedef i386_enumerate_in_out_transmaps_for_usedef
#define x64_insn_hash_func i386_insn_hash_func
#define x64_iseq_has_marker_prefix i386_iseq_has_marker_prefix
#define x64_insn_rename_cregs_to_vregs i386_insn_rename_cregs_to_vregs
#define x64_insn_rename_vregs_to_cregs i386_insn_rename_vregs_to_cregs
#define x64_opcodes_related i386_opcodes_related
#define x64_insn_rename_memory_operand_to_symbol i386_insn_rename_memory_operand_to_symbol
#define x64_insn_supports_boolean_test i386_insn_supports_boolean_test

#define MAX_X64_COSTFNS MAX_I386_COSTFNS
#define x64_costfn_t i386_costfn_t
#define x64_costfns i386_costfns
#define x64_num_costfns i386_num_costfns

#define x64_reserved_regs i386_reserved_regs
extern regset_t x64_callee_save_regs;

long x64_insn_sandbox(struct x64_insn_t *dst, size_t dst_size,
    struct x64_insn_t const *src,
    struct regmap_t const *c2v, int tmpreg, long insn_index);

#define x64_strict_canonicalization_state_init i386_strict_canonicalization_state_init
#define x64_strict_canonicalization_state_free i386_strict_canonicalization_state_free
#define x64_insn_rename_vregs_using_regmap i386_insn_rename_vregs_using_regmap
#define x64_iseq_count_touched_regs_as_cost i386_iseq_count_touched_regs_as_cost
#define x64_insn_rename_tag_src_insn_pcs_to_tag_vars i386_insn_rename_tag_src_insn_pcs_to_tag_vars

#define X64_MEM_TO_EXREG_COST I386_MEM_TO_EXREG_COST
#define X64_EXREG_TO_MEM_COST I386_EXREG_TO_MEM_COST
#define X64_EXREG_MODIFIER_TRANSLATION_COST I386_EXREG_MODIFIER_TRANSLATION_COST
#define X64_EXREG_TO_EXREG_COST I386_EXREG_TO_EXREG_COST
#define X64_AVG_REG_MOVE_COST_FOR_LOOP_EDGE_AND_ZERO_DEAD_REGS I386_AVG_REG_MOVE_COST_FOR_LOOP_EDGE_AND_ZERO_DEAD_REGS

#define x64_insn_supports_execution_test i386_insn_supports_execution_test
#define x64_iseq_supports_execution_test i386_iseq_supports_execution_test
#define x64_insn_to_string i386_insn_to_string
#define x64_iseq_types_equal i386_iseq_types_equal
#define x64_insn_is_function_return i386_insn_is_function_return
#define x64_iseq_mark_used_vconstants i386_iseq_mark_used_vconstants
#define x64_insn_is_lea i386_insn_is_lea
#define x64_iseq_with_relocs_to_string i386_iseq_with_relocs_to_string

#define x64_iseq_assemble i386_iseq_assemble
#define x64_iseq_contains_reloc_reference i386_iseq_contains_reloc_reference

#define x64_iseq_rename_var i386_iseq_rename_var
#define x64_iseq_get_touch_not_live_in_regs i386_iseq_get_touch_not_live_in_regs
#define x64_iseq_rename_randomly_and_assemble i386_iseq_rename_randomly_and_assemble
#define x64_iseq_replace_tmap_loc_with_const i386_iseq_replace_tmap_loc_with_const
#define x64_insn_set_nextpc i386_insn_set_nextpc

void x64_init(void);
void x64_free(void);
long x64_insn_put_function_return(fixed_reg_mapping_t const &fixed_reg_mapping, struct x64_insn_t *insns, size_t insns_size);
#define x64_iseq_compute_num_nextpcs i386_iseq_compute_num_nextpcs
#define x64_iseq_convert_peeptab_mem_constants_to_symbols i386_iseq_convert_peeptab_mem_constants_to_symbols
#define x64_iseq_rename_nextpc_consts i386_iseq_rename_nextpc_consts
#define x64_insn_is_move_insn i386_insn_is_move_insn
#define x64_assemble_exec_iseq_using_regmap i386_assemble_exec_iseq_using_regmap
#define x64_iseq_get_used_vconst_regs i386_iseq_get_used_vconst_regs

#define x64_insn_reg_is_replaceable i386_insn_reg_is_replaceable
#define x64_insn_reg_def_is_replaceable i386_insn_reg_def_is_replaceable
#define x64_insn_rename_dst_reg i386_insn_rename_dst_reg
#define x64_insn_rename_regs i386_insn_rename_regs
#define x64_iseq_rename_nextpc_vars_to_pcrel_relocs i386_iseq_rename_nextpc_vars_to_pcrel_relocs
#define x64_insn_put_add_imm_with_reg i386_insn_put_add_imm_with_reg
#define x64_insn_branch_target_src_insn_pc i386_insn_branch_target_src_insn_pc
#define x64_insn_rename_immediate_operands_containing_code_addresses i386_insn_rename_immediate_operands_containing_code_addresses
#define x64_insn_put_jump_to_abs_inum i386_insn_put_jump_to_abs_inum
#define x64_emit_translation_marker_insns i386_emit_translation_marker_insns
#define x64_iseq_rename_locals_using_locals_map i386_iseq_rename_locals_using_locals_map
#define x64_insn_convert_function_call_to_branch i386_insn_convert_function_call_to_branch
#define x64_insn_rename_tag_src_insn_pcs_to_tag_var_using_tcodes i386_insn_rename_tag_src_insn_pcs_to_tag_var_using_tcodes
#define x64_insn_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map i386_insn_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map
#define x64_insn_convert_malloc_to_mymalloc i386_insn_convert_malloc_to_mymalloc

#define x64_insn_fetch_preprocess i386_insn_fetch_preprocess

#define x64_insn_symbols_are_contained_in_map i386_insn_symbols_are_contained_in_map
#define symbol_is_contained_in_x64_insn symbol_is_contained_in_i386_insn

#define x64_iseq_is_function_tailcall_opt_candidate i386_iseq_is_function_tailcall_opt_candidate

long x64_insn_put_move_imm_to_mem(dst_ulong imm, uint64_t addr, int membase, x64_insn_t *insns,
    size_t insns_size);

size_t x64_insn_put_ret_insn(x64_insn_t *insns, size_t insns_size, int retval);
#define x64_iseq_rename_regs_and_imms i386_iseq_rename_regs_and_imms
#define x64_insn_put_nop_as i386_insn_put_nop_as
size_t x64_insn_put_linux_exit_syscall_as(char *buf, size_t size);
#define x64_insn_copy_symbols i386_insn_copy_symbols

#define x64_insn_get_min_max_vconst_masks i386_insn_get_min_max_vconst_masks
#define x64_insn_get_min_max_mem_vconst_masks i386_insn_get_min_max_mem_vconst_masks
#define x64_insn_type_signature_supported i386_insn_type_signature_supported
#define x64_insn_convert_pcrels_and_symbols_to_reloc_strings i386_insn_convert_pcrels_and_symbols_to_reloc_strings
#define x64_insn_rename_addresses_to_symbols i386_insn_rename_addresses_to_symbols

bool x64_insn_fetch_raw(i386_insn_t *insn, input_exec_t const *in, src_ulong eip,
    unsigned long *size);
char *x64_iseq_to_string(x64_insn_t const *iseq, long iseq_len, char *buf, size_t buf_size);
