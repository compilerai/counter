#pragma once

#include "stopwatch.h"

extern struct time total_timer;

/* rewrite timers */
extern struct time build_pred_row_timer;
extern struct time get_all_possible_transmaps_timer;
extern struct time peep_search_timer;
extern struct time peep_translate_timer;
extern struct time i386_iseq_fetch_timer;
extern struct time debug1_timer, debug2_timer, debug3_timer, debug4_timer;
extern struct time state_compute_fingerprint_timer;

extern struct time transmap_translation_code_timer;
extern struct time transmap_translation_insns_timer;

extern struct time chash_insert_timer;
extern struct time chash_find_timer;

/* peepgen timers */
extern struct time check_equivalence_timer;
extern struct time execution_check_equivalence_timer;
extern struct time boolean_check_equivalence_timer;

extern struct time itable_enumerate_timer;
extern struct time jtable_construct_executable_binary_timer;
extern struct time jtable_get_executable_binary_timer;
extern struct time jtable_get_save_restore_code_timer;
extern struct time jtable_get_header_and_footer_code_timer;
extern struct time fingerprintdb_query_using_bincode_timer;
extern struct time fingerprintdb_query_using_bincode_for_typestate_timer;
extern struct time dst_execute_code_on_state_and_typestate_timer;
extern struct time fingerprintdb_live_out_agrees_with_regset_timer;
extern struct time fingerprintdb_live_out_agrees_with_memslot_set_timer;

extern struct time eqcheck_check_equivalence_timer;
extern struct time eqcheck_finalization_step_timer;
extern struct time eqcheck_preprocessing_step_timer;
extern struct time eqcheck_addmission_control_timer;
extern struct time eqcheck_expr_simplify_timer;
extern struct time eqcheck_expr_simplify_miss_timer;
extern struct time eqcheck_z3_simplify_timer;
extern struct time eqcheck_expr_substitute_timer;
extern struct time eqcheck_z3_prove_timer;
extern struct time eqcheck_logging_timer;
extern struct time eqcheck_compute_dof_timer;
extern struct time eqcheck_compute_dof_create_bv_constant_timer;
extern struct time eqcheck_compute_dof_expr_substitute_timer;
extern struct time eqcheck_compute_dof_expr_simplify_timer;

extern struct time expr_simplify_solver_timer;
extern struct time expr_simplify_syntactic_timer;

extern struct time jtable_init_timer;
extern struct time jtable_entry_output_fixup_code_for_regs_timer;
extern struct time jtable_entry_output_fixup_code_for_vconsts_timer;
extern struct time jtable_entry_output_fixup_code_for_non_temp_regs_timer;
extern struct time jtable_entry_output_fixup_code_gen_binbufs_timer;
extern struct time i386_assemble_exec_iseq_using_regmap_timer;
extern struct time jtable_entry_output_regvar_code_timer;
extern struct time i386_iseq_assemble_timer;
extern struct time insn_md_assemble_timer;

extern struct time input_file_annotate_symbols_timer;
extern struct time dwarf_map_timer;
extern struct time src_iseq_annotate_symbols_timer;
extern struct time src_iseq_annotate_locals_timer;
extern struct time src_iseq_annotate_symbols_timer;
extern struct time is_local_addr_timer;

extern struct time get_addr_id_from_addr_timer;
extern struct time get_linearly_related_addr_ref_id_for_addr_timer;
extern struct time addr_is_independent_of_input_stack_pointer_timer;
//extern struct time addr_is_top_timer;
//extern struct time addr_is_bottom_timer;

//extern struct time loc_is_top_timer;
//extern struct time loc_is_bottom_timer;
extern struct time is_consistent_for_loc_timer;

extern struct time annotate_locals_expr_prove_timer;

extern struct time smt_query_timer;
extern struct time smt_query_false_timer;
extern struct time smt_query_true_timer;

extern struct time smt_helper_process_solve_timer;

extern struct time expr_to_loc_id_timer;
//extern struct time expr_represents_loc_in_state_timer;
extern struct time expr_linear_relation_holds_visit_timer;
extern struct time expr_unique_sp_relation_holds_visit_timer;

extern struct time expr_is_consts_struct_constant_timer;
extern struct time expr_is_consts_struct_addr_ref_timer;

extern struct time graph_cp_update_current_theorems_timer;
extern struct time expr_set_comments_to_zero_timer;
extern struct time graph_loc_get_value_timer;
extern struct time graph_add_addr_timer;

extern struct time manager_register_object_timer;
extern struct time manager_deregister_object_timer;

extern struct time expr_do_simplify_helper_timer;
extern struct time expr_do_simplify_helper_cache_timer;
extern struct time expr_eliminate_constructs_that_the_solver_cannot_handle_timer;

extern struct time expr_prune_obviously_false_branches_using_assume_clause_visitor_timer;
extern struct time expr_prune_obviously_false_branches_using_assume_clause_visitor_cache_timer;

extern struct time expr_evaluates_to_constant_timer;
extern struct time evaluate_expr_and_check_bounds_timer;
extern struct time array_constant_equals_timer;

extern struct time is_overlapping_syntactic_using_lhs_set_and_precond_timer;
extern struct time is_overlapping_atoms_pair_syntactic_timer;

extern struct time combo_dfa_xfer_and_meet_timer;
extern struct time alias_val_xfer_and_meet_timer;
extern struct time avail_exprs_xfer_and_meet_timer;
extern struct time avail_exprs_meet_timer;
extern struct time expr_substitute_using_available_exprs_timer;
extern struct time get_sprel_map_from_avail_exprs_timer;
extern struct time update_memlabels_for_memslot_locs_timer;
extern struct time populate_gen_and_kill_sets_for_edge_timer;
extern struct time compute_simplified_loc_exprs_for_edge_timer;
extern struct time compute_locs_definitely_written_on_edge_timer;
extern struct time get_locs_potentially_read_in_expr_using_locs_map_timer;
extern struct time edge_update_memlabel_map_for_mlvars_timer;
extern struct time add_new_locs_based_on_edge_timer;
extern struct time alias_val_meet_timer;
extern struct time expand_locset_to_include_slots_for_memmask_timer;

void init_timers(void);
void call_on_exit_function(void);
//void g_stats_print(void);
void print_all_timers();
extern unsigned global_timeout_secs;
