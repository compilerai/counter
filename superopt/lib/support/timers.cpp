#include <stdio.h>
#include <signal.h>
#include <support/debug.h>
#include "timers.h"
#include "support/globals.h"
#include "expr/z3_solver.h"
#include "support/cmd_helper.h"

//int g_helper_pid = -1;

struct time total_timer;

/* rewrite timers */
struct time build_pred_row_timer;
struct time get_all_possible_transmaps_timer;
struct time peep_search_timer;
struct time peep_translate_timer;
struct time i386_iseq_fetch_timer;
struct time check_equivalence_timer;
struct time execution_check_equivalence_timer;
struct time boolean_check_equivalence_timer;

struct time itable_enumerate_timer;
struct time jtable_construct_executable_binary_timer;
struct time jtable_get_executable_binary_timer;
struct time jtable_get_save_restore_code_timer;
struct time jtable_get_header_and_footer_code_timer;
struct time fingerprintdb_query_using_bincode_timer;
struct time fingerprintdb_query_using_bincode_for_typestate_timer;
struct time dst_execute_code_on_state_and_typestate_timer;
struct time fingerprintdb_live_out_agrees_with_regset_timer;
struct time fingerprintdb_live_out_agrees_with_memslot_set_timer;


struct time jtable_init_timer;
struct time jtable_entry_output_fixup_code_for_regs_timer;
struct time jtable_entry_output_fixup_code_for_vconsts_timer;
struct time jtable_entry_output_fixup_code_for_non_temp_regs_timer;
struct time jtable_entry_output_fixup_code_gen_binbufs_timer;
struct time i386_assemble_exec_iseq_using_regmap_timer;
struct time jtable_entry_output_regvar_code_timer;
struct time i386_iseq_assemble_timer;
struct time insn_md_assemble_timer;
struct time eqcheck_check_equivalence_timer;
struct time eqcheck_finalization_step_timer;
struct time eqcheck_preprocessing_step_timer;
struct time eqcheck_expr_simplify_timer;
struct time eqcheck_expr_simplify_miss_timer;
struct time eqcheck_z3_simplify_timer;
struct time eqcheck_expr_substitute_timer;
struct time eqcheck_z3_prove_timer;
struct time eqcheck_logging_timer;
struct time eqcheck_compute_dof_timer;
struct time eqcheck_compute_dof_expr_substitute_timer;
struct time eqcheck_compute_dof_expr_simplify_timer;
struct time eqcheck_compute_dof_create_bv_constant_timer;
struct time eqcheck_addmission_control_timer;

struct time expr_simplify_solver_timer;
struct time expr_simplify_syntactic_timer;

struct time input_file_annotate_symbols_timer;
struct time dwarf_map_timer;
struct time src_iseq_annotate_locals_timer;
//struct time src_iseq_annotate_locals_timer;
struct time src_iseq_annotate_symbols_timer;
struct time is_local_addr_timer;
//struct time is_symbol_addr_timer;
struct time annotate_locals_expr_prove_timer;

struct time get_addr_id_from_addr_timer;
struct time get_linearly_related_addr_ref_id_for_addr_timer;
struct time addr_is_independent_of_input_stack_pointer_timer;
//struct time addr_is_top_timer;
//struct time addr_is_bottom_timer;

//struct time loc_is_top_timer;
//struct time loc_is_bottom_timer;
struct time is_consistent_for_loc_timer;

struct time debug1_timer, debug2_timer, debug3_timer, debug4_timer;
struct time state_compute_fingerprint_timer;

struct time transmap_translation_code_timer;
struct time transmap_translation_insns_timer;

struct time chash_insert_timer;
struct time chash_find_timer;

struct time smt_query_timer;
struct time smt_query_false_timer;
struct time smt_query_true_timer;

struct time smt_helper_process_solve_timer;

struct time expr_to_loc_id_timer;
//struct time expr_represents_loc_in_state_timer;
struct time expr_linear_relation_holds_visit_timer;
struct time expr_unique_sp_relation_holds_visit_timer;
struct time expr_is_consts_struct_constant_timer;
struct time expr_is_consts_struct_addr_ref_timer;
struct time graph_cp_update_current_theorems_timer;

struct time expr_set_comments_to_zero_timer;
struct time graph_loc_get_value_timer;
struct time graph_add_addr_timer;
struct time manager_register_object_timer;
struct time manager_deregister_object_timer;

struct time expr_do_simplify_helper_timer;
struct time expr_do_simplify_helper_cache_timer;

struct time expr_prune_obviously_false_branches_using_assume_clause_visitor_timer;
struct time expr_prune_obviously_false_branches_using_assume_clause_visitor_cache_timer;

struct time expr_evaluates_to_constant_timer;
struct time evaluate_expr_and_check_bounds_timer;
struct time array_constant_equals_timer;

struct time is_overlapping_syntactic_using_lhs_set_and_precond_timer;
struct time is_overlapping_atoms_pair_syntactic_timer;

/* peepgen timers */
//struct time fingerprintdb_check_timer;
//struct time fingerprintdb_check_equivtest_timer;

//unsigned global_timeout_secs = 2*60*60;
unsigned global_timeout_secs = 2*60*60*60*60;

static void
sigint_handler(int signum)
{
  call_on_exit_function();
  printf("\nERROR_KILLED_INT\n");
  fflush(stdout);
  signal(signum, SIG_DFL);
  raise(signum);
}

static void
sigquit_handler(int signum)
{
  call_on_exit_function();
  printf("\nERROR_KILLED_QUIT\n");
  fflush(stdout);
  //signal(signum, SIG_DFL);
  //raise(signum);
}

static void
sigalrm_handler(int signum)
{
  call_on_exit_function();
  printf("\nERROR_GLOBAL_TIME_OUT %d secs\n", global_timeout_secs);
  fflush(stdout);
  exit(0);
}

static void
sigterm_handler(int signum)
{
  call_on_exit_function();
  printf("\nERROR_KILLED_TERM\n");
  fflush(stdout);
  exit(0);
}

static void
init_timer(struct time *timer)
{
  //timer->flags = SW_WALLCLOCK;
  stopwatch_reset(timer);
}

void
init_timers(void)
{
  /*build_pred_row_timer.flags = SW_WALLCLOCK;
  get_all_possible_transmaps_timer.flags = SW_WALLCLOCK;
  peep_translate_timer.flags = SW_WALLCLOCK;
  peep_search_timer.flags = SW_WALLCLOCK;
  total_timer.flags = SW_WALLCLOCK;
  i386_iseq_fetch_timer.flags = SW_WALLCLOCK;
  debug1_timer.flags = SW_WALLCLOCK;
  debug2_timer.flags = SW_WALLCLOCK;
  debug3_timer.flags = SW_WALLCLOCK;
  debug4_timer.flags = SW_WALLCLOCK;*/

  init_timer(&total_timer);
  init_timer(&build_pred_row_timer);
  init_timer(&get_all_possible_transmaps_timer);
  init_timer(&peep_translate_timer);
  init_timer(&peep_search_timer);
  init_timer(&i386_iseq_fetch_timer);
  //init_timer(&fingerprintdb_check_timer);
  //init_timer(&fingerprintdb_check_equivtest_timer);
  init_timer(&check_equivalence_timer);
  init_timer(&execution_check_equivalence_timer);
  init_timer(&boolean_check_equivalence_timer);

  init_timer(&itable_enumerate_timer);
  init_timer(&jtable_construct_executable_binary_timer);
  init_timer(&jtable_get_executable_binary_timer);
  init_timer(&jtable_get_save_restore_code_timer);
  init_timer(&jtable_get_header_and_footer_code_timer);
  init_timer(&fingerprintdb_query_using_bincode_timer);
  init_timer(&fingerprintdb_query_using_bincode_for_typestate_timer);
  init_timer(&dst_execute_code_on_state_and_typestate_timer);
  init_timer(&fingerprintdb_live_out_agrees_with_regset_timer);
  init_timer(&fingerprintdb_live_out_agrees_with_memslot_set_timer);

  init_timer(&jtable_init_timer);
  init_timer(&jtable_entry_output_fixup_code_for_regs_timer);
  init_timer(&jtable_entry_output_fixup_code_for_vconsts_timer);
  init_timer(&jtable_entry_output_fixup_code_for_non_temp_regs_timer);
  init_timer(&jtable_entry_output_fixup_code_gen_binbufs_timer);
  init_timer(&i386_assemble_exec_iseq_using_regmap_timer);
  init_timer(&jtable_entry_output_regvar_code_timer);
  init_timer(&i386_iseq_assemble_timer);
  init_timer(&insn_md_assemble_timer);
  init_timer(&eqcheck_check_equivalence_timer);
  init_timer(&eqcheck_finalization_step_timer);
  init_timer(&eqcheck_preprocessing_step_timer);
  init_timer(&eqcheck_expr_simplify_timer);
  init_timer(&eqcheck_expr_simplify_miss_timer);
  init_timer(&eqcheck_z3_simplify_timer);
  init_timer(&eqcheck_expr_substitute_timer);
  init_timer(&eqcheck_z3_prove_timer);
  init_timer(&eqcheck_logging_timer);
  init_timer(&eqcheck_compute_dof_timer);
  init_timer(&eqcheck_compute_dof_expr_substitute_timer);
  init_timer(&eqcheck_compute_dof_expr_simplify_timer);
  init_timer(&eqcheck_compute_dof_create_bv_constant_timer);
  init_timer(&eqcheck_addmission_control_timer);
  init_timer(&input_file_annotate_symbols_timer);
  init_timer(&dwarf_map_timer);
  init_timer(&src_iseq_annotate_locals_timer);
  //init_timer(&src_iseq_annotate_locals_timer);
  init_timer(&src_iseq_annotate_symbols_timer);
  init_timer(&is_local_addr_timer);
  //init_timer(&is_symbol_addr_timer);
  init_timer(&annotate_locals_expr_prove_timer);

  init_timer(&expr_simplify_solver_timer);
  init_timer(&expr_simplify_syntactic_timer);

  init_timer(&get_addr_id_from_addr_timer);
  init_timer(&get_linearly_related_addr_ref_id_for_addr_timer);
  init_timer(&addr_is_independent_of_input_stack_pointer_timer);
  //init_timer(&addr_is_top_timer);
  //init_timer(&addr_is_bottom_timer);

  //init_timer(&loc_is_top_timer);
  //init_timer(&loc_is_bottom_timer);
  init_timer(&is_consistent_for_loc_timer);


  init_timer(&transmap_translation_code_timer);
  init_timer(&transmap_translation_insns_timer);

  init_timer(&chash_insert_timer);
  init_timer(&chash_find_timer);

  init_timer(&smt_query_timer);
  init_timer(&smt_query_false_timer);
  init_timer(&smt_query_true_timer);
  init_timer(&smt_helper_process_solve_timer);

  init_timer(&expr_to_loc_id_timer);
  //init_timer(&expr_represents_loc_in_state_timer);
  init_timer(&expr_linear_relation_holds_visit_timer);
  init_timer(&expr_unique_sp_relation_holds_visit_timer);
  init_timer(&expr_is_consts_struct_constant_timer);
  init_timer(&expr_is_consts_struct_addr_ref_timer);
  init_timer(&graph_cp_update_current_theorems_timer);
  init_timer(&expr_set_comments_to_zero_timer);
  init_timer(&graph_loc_get_value_timer);
  init_timer(&graph_add_addr_timer);
  init_timer(&manager_register_object_timer);
  init_timer(&manager_deregister_object_timer);

  init_timer(&expr_do_simplify_helper_timer);
  init_timer(&expr_do_simplify_helper_cache_timer);

  init_timer(&expr_prune_obviously_false_branches_using_assume_clause_visitor_timer);
  init_timer(&expr_prune_obviously_false_branches_using_assume_clause_visitor_cache_timer);

  init_timer(&expr_evaluates_to_constant_timer);
  init_timer(&evaluate_expr_and_check_bounds_timer);
  init_timer(&array_constant_equals_timer);

  init_timer(&is_overlapping_syntactic_using_lhs_set_and_precond_timer);
  init_timer(&is_overlapping_atoms_pair_syntactic_timer);

  init_timer(&state_compute_fingerprint_timer);

  init_timer(&debug1_timer);
  init_timer(&debug2_timer);
  init_timer(&debug3_timer);
  init_timer(&debug4_timer);
  stopwatch_run(&total_timer);
  signal(SIGINT, &sigint_handler);
  signal(SIGQUIT, &sigquit_handler);
  signal(SIGTERM, &sigterm_handler);

  signal(SIGALRM, &sigalrm_handler);
  struct itimerval timer;
  timer.it_value.tv_sec = global_timeout_secs;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  int flag = setitimer(ITIMER_REAL, &timer, NULL);
  ASSERT(!flag);

  int i, j;
  for (i = 0; i < ENUM_MAX_LEN; i++) {
    stats_num_fingerprint_tests[i] = 0;
    for (j = 0; j < i; j++) {
      stats_num_fingerprint_type_failures[i][j] = 0;
    }
    stats_num_fingerprint_typecheck_success[i] = 0;
  }
  stats_num_syntactic_match_tests = 0;
  stats_num_execution_tests = 0;
  stats_num_boolean_tests = 0;
}

void
print_all_timers()
{
#define PRINT_TIMER(name) do { \
  long long num_starts = name##_timer.num_starts; \
  if (num_starts) { \
    double secs; \
    stopwatch_tell(&name##_timer); \
    secs = (double)name##_timer.wallClock_elapsed/1e6 ; \
    printf (#name ": num_starts %lld time spent: %d:%d:%d [%f]\n", \
        num_starts, \
        (int)(secs/3.6e3), \
        ((int)((secs/60))) % 60, \
        ((int)secs)%60, secs); \
  } \
} while(0)

  putchar('\n');
  PRINT_TIMER(build_pred_row);
  PRINT_TIMER(get_all_possible_transmaps);
  PRINT_TIMER(peep_translate);
  PRINT_TIMER(peep_search);
  PRINT_TIMER(i386_iseq_fetch);
  PRINT_TIMER(check_equivalence);
  PRINT_TIMER(execution_check_equivalence);
  PRINT_TIMER(boolean_check_equivalence);

  PRINT_TIMER(itable_enumerate);
  PRINT_TIMER(jtable_construct_executable_binary);
  PRINT_TIMER(jtable_get_executable_binary);
  PRINT_TIMER(jtable_get_save_restore_code);
  PRINT_TIMER(jtable_get_header_and_footer_code);
  PRINT_TIMER(fingerprintdb_query_using_bincode);
  PRINT_TIMER(fingerprintdb_query_using_bincode_for_typestate);
  PRINT_TIMER(dst_execute_code_on_state_and_typestate);
  PRINT_TIMER(fingerprintdb_live_out_agrees_with_regset);
  PRINT_TIMER(fingerprintdb_live_out_agrees_with_memslot_set);

  PRINT_TIMER(eqcheck_preprocessing_step);
  PRINT_TIMER(eqcheck_check_equivalence);
  PRINT_TIMER(eqcheck_finalization_step);
  PRINT_TIMER(eqcheck_z3_simplify);
  PRINT_TIMER(eqcheck_expr_simplify);
  PRINT_TIMER(eqcheck_expr_simplify_miss);
  PRINT_TIMER(eqcheck_expr_substitute);
  PRINT_TIMER(eqcheck_z3_prove);
  PRINT_TIMER(eqcheck_addmission_control);
  PRINT_TIMER(input_file_annotate_symbols);
  PRINT_TIMER(dwarf_map);
  PRINT_TIMER(src_iseq_annotate_locals);
  //PRINT_TIMER(src_iseq_annotate_locals);
  PRINT_TIMER(src_iseq_annotate_symbols);
  PRINT_TIMER(is_local_addr);
  //PRINT_TIMER(is_symbol_addr);
  PRINT_TIMER(annotate_locals_expr_prove);

  PRINT_TIMER(expr_simplify_solver);
  PRINT_TIMER(expr_simplify_syntactic);

  PRINT_TIMER(get_addr_id_from_addr);
  PRINT_TIMER(get_linearly_related_addr_ref_id_for_addr);
  PRINT_TIMER(addr_is_independent_of_input_stack_pointer);
  //PRINT_TIMER(addr_is_top);
  //PRINT_TIMER(addr_is_bottom);

  //PRINT_TIMER(loc_is_top);
  //PRINT_TIMER(loc_is_bottom);
  PRINT_TIMER(is_consistent_for_loc);

  PRINT_TIMER(jtable_init);
  PRINT_TIMER(jtable_entry_output_fixup_code_for_regs);
  PRINT_TIMER(jtable_entry_output_fixup_code_for_vconsts);
  PRINT_TIMER(jtable_entry_output_fixup_code_for_non_temp_regs);
  PRINT_TIMER(jtable_entry_output_fixup_code_gen_binbufs);
  PRINT_TIMER(i386_assemble_exec_iseq_using_regmap);
  PRINT_TIMER(jtable_entry_output_regvar_code);
  PRINT_TIMER(i386_iseq_assemble);
  PRINT_TIMER(insn_md_assemble);
  PRINT_TIMER(transmap_translation_code);
  PRINT_TIMER(transmap_translation_insns);

  PRINT_TIMER(chash_insert);
  PRINT_TIMER(chash_find);

  PRINT_TIMER(smt_query);
  PRINT_TIMER(smt_query_false);
  PRINT_TIMER(smt_query_true);

  PRINT_TIMER(smt_helper_process_solve);

  PRINT_TIMER(expr_to_loc_id);
  //PRINT_TIMER(expr_represents_loc_in_state);
  PRINT_TIMER(expr_linear_relation_holds_visit);
  PRINT_TIMER(expr_unique_sp_relation_holds_visit);
  PRINT_TIMER(expr_is_consts_struct_constant);
  PRINT_TIMER(expr_is_consts_struct_addr_ref);
  PRINT_TIMER(graph_cp_update_current_theorems);
  PRINT_TIMER(expr_set_comments_to_zero);
  PRINT_TIMER(graph_loc_get_value);
  PRINT_TIMER(graph_add_addr);
  PRINT_TIMER(manager_register_object);
  PRINT_TIMER(manager_deregister_object);
  PRINT_TIMER(state_compute_fingerprint);
  PRINT_TIMER(expr_do_simplify_helper);
  PRINT_TIMER(expr_do_simplify_helper_cache);
  PRINT_TIMER(expr_prune_obviously_false_branches_using_assume_clause_visitor_cache);
  PRINT_TIMER(expr_prune_obviously_false_branches_using_assume_clause_visitor);
  PRINT_TIMER(expr_evaluates_to_constant);
  PRINT_TIMER(evaluate_expr_and_check_bounds);
  PRINT_TIMER(array_constant_equals);
  PRINT_TIMER(is_overlapping_syntactic_using_lhs_set_and_precond);
  PRINT_TIMER(is_overlapping_atoms_pair_syntactic);

  PRINT_TIMER(debug1);
  PRINT_TIMER(debug2);
  PRINT_TIMER(debug3);
  PRINT_TIMER(debug4);
  PRINT_TIMER(total);

  STATS(
      int i;
      int j;
      for (i = 0; i < ENUM_MAX_LEN && stats_num_fingerprint_tests[i]; i++) {
        printf("num_fingerprint_tests[%d] = %lld\n", i, stats_num_fingerprint_tests[i]);
      }
      for (i = 0; i < ENUM_MAX_LEN && stats_num_fingerprint_tests[i]; i++) {
        printf("num_fingerprint_type_failures[%d] =", i);
        for (j = 0; j < i; j++) {
          printf(" %lld", stats_num_fingerprint_type_failures[i][j]);
        }
        printf("\n");
      }
      for (i = 0; i < ENUM_MAX_LEN && stats_num_fingerprint_tests[i]; i++) {
        printf("num_fingerprint_typecheck_success[%d] = %lld\n", i,
            stats_num_fingerprint_typecheck_success[i]);
      }
      printf("num_syntactic_match_tests = %lld.\n", stats_num_syntactic_match_tests);
      printf("num_execution_tests = %lld.\n", stats_num_execution_tests);
      printf("num_boolean_tests = %lld.\n", stats_num_boolean_tests);
  );
  long long total_num_fingerprint_tests = 0;
  int i;
  for (i = 0; i < ENUM_MAX_LEN; i++) {
    total_num_fingerprint_tests += stats_num_fingerprint_tests[i];
  }
  long long cpu_MHz = 2600;
  printf("fingerprinting rate: %.2f per second (total), %.2f per second (itable_enumerate_timer), %.2f per second (excluding execution/boolean tests).\n",
      (float)total_num_fingerprint_tests*1e6 * cpu_MHz/total_timer.wallClock_elapsed,
      (float)total_num_fingerprint_tests*1e6 * cpu_MHz/itable_enumerate_timer.wallClock_elapsed,
      (float)total_num_fingerprint_tests*1e6 * cpu_MHz/(itable_enumerate_timer.wallClock_elapsed -
          (execution_check_equivalence_timer.wallClock_elapsed + boolean_check_equivalence_timer.wallClock_elapsed)));
  putchar('\n');
  fflush(stdout);
}

void
call_on_exit_function()
{
  //printf("%s() %d: g_helper_pid = %d\n", __func__, __LINE__, g_helper_pid);
  cmd_helper_kill();
  fflush(stdout);
}
