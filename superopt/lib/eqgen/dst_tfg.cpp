#include "eqgen/dst_tfg.h"
#include "expr/context.h"
#include "expr/expr.h"
#include "tfg/tfg.h"
#include "sym_exec/sym_exec.h"
#include "rewrite/dst-insn.h"
#include "tfg/tfg_llvm.h"
#include "rewrite/translation_instance.h"

static char as1[409600];

static map<pc, transmap_t>
get_out_tmap_map_from_out_tmaps(transmap_t const *out_tmaps, size_t num_out_tmaps)
{
  map<pc, transmap_t> ret;
  for (size_t i = 0; i < num_out_tmaps; i++) {
    pc exit_pc(pc::exit, int_to_string(i).c_str(), 0, PC_SUBSUBINDEX_DEFAULT);
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", out_tmaps[i] =\n" << transmap_to_string(&out_tmaps[i], as1, sizeof as1);
    ret[exit_pc] = out_tmaps[i];
  }
  return ret;
}



shared_ptr<tfg>
dst_iseq_get_preprocessed_tfg(dst_insn_t const *dst_iseq, long dst_iseq_len, transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_live_out, graph_arg_regs_t const &argument_regs, map<pc, map<string_ref, expr_ref>> const &return_reg_map, fixed_reg_mapping_t const &fixed_reg_mapping, context *ctx, sym_exec &se)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  shared_ptr<tfg> dst_tfg = se.dst_get_tfg(dst_iseq, dst_iseq_len, ctx);
  pc pc_cur_start(pc::insn_label, pc::start().get_index(), pc::start().get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, pc::start().get_subsubindex());
  dst_tfg->add_extra_node_at_start_pc(pc_cur_start);
  //cout << __func__ << " " << __LINE__ << ": before add transmap_translations_for_input_arguments: tfg =\n" << dst_tfg->graph_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  int stack_gpr = fixed_reg_mapping.get_mapping(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM);
  dst_tfg->populate_symbol_map_using_memlocs_in_tmaps(in_tmap, out_tmaps, num_live_out);
  dst_tfg->tfg_add_transmap_translations_for_input_arguments(argument_regs, *in_tmap, -1/*stack_gpr*/);
  if (stack_gpr != -1) {
    dst_tfg->dst_tfg_add_stack_pointer_translation_at_function_entry(stack_gpr);
  }
  /*if (stack_gpr != -1) {
    ASSERT(stack_gpr >= 0 && stack_gpr < DST_NUM_GPRS);
    dst_tfg->dst_tfg_add_stack_pointer_translation_at_function_entry(stack_gpr);
  }*/
  //cout << __func__ << " " << __LINE__ << ": after add transmap_translations_for_input_arguments: tfg =\n" << dst_tfg->graph_to_string() << endl;
  auto out_tmap_map = get_out_tmap_map_from_out_tmaps(out_tmaps, num_live_out);
  //int stack_gpr = fixed_reg_mapping.get_mapping(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM);
  dst_tfg->populate_exit_return_values_using_out_tmaps(return_reg_map, out_tmap_map, -1/*stack_gpr*/);


  dst_tfg->collapse_tfg(true);
  dst_tfg->tfg_preprocess(true);
  CPP_DBG_EXEC(ISEQ_GET_TFG, cout << __func__ << " " << __LINE__ << ": returning\n" << dst_tfg->graph_to_string() << endl);
  return dst_tfg;
}

namespace eqspace {

void
tfg::gen_tfg_for_dst_iseq(char const *outfile,
                     char const *execfile,
                     dst_insn_t const *dst_iseq_orig,
                     size_t dst_iseq_len,
                     regmap_t const& regmap,
                     char const *filename_llvm,
                     char const *function_name,
                     struct symbol_map_t const *dst_symbol_map,
                     nextpc_function_name_map_t& new_nextpc_function_name_map,
                     //char const *peep_comment,
                     size_t num_live_out,
                     vector<dst_ulong> const& insn_pcs_vec,
                     struct context *ctx)
{
  regmap_t regmap_inv;
  regmap_invert(&regmap_inv, &regmap);
  dst_insn_t *dst_iseq = new dst_insn_t[dst_iseq_len];

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": dst_iseq_orig:\n" << dst_iseq_to_string(dst_iseq_orig, dst_iseq_len, as1, sizeof as1) << endl);
  dst_iseq_copy(dst_iseq, dst_iseq_orig, dst_iseq_len);
  dst_iseq_rename(dst_iseq, dst_iseq_len, &regmap_inv, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
  vector<dst_insn_t> dst_iseq_vec;
  for (size_t i = 0; i < dst_iseq_len; i++) {
    dst_insn_inv_rename_registers(&dst_iseq[i], dst_identity_regmap());
    dst_insn_inv_rename_memory_operands(&dst_iseq[i], dst_identity_regmap(), nullptr);
    dst_iseq_vec.push_back(dst_iseq[i]);
  }
  ASSERT(insn_pcs_vec.size() == dst_iseq_len);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": dst_iseq:\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);

  //int const r_esp = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  //int const r_ebp = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_EBP).reg_identifier_get_id();
  //int const r_esi = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_ESI).reg_identifier_get_id();
  //int const r_ebx = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_EBX).reg_identifier_get_id();
  //int const r_edi = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_EDI).reg_identifier_get_id();
  //int const r_eax = regmap_get_elem(&regmap, I386_EXREG_GROUP_GPRS, R_EAX).reg_identifier_get_id();

  int const r_esp = R_ESP;
  int const r_ebp = R_EBP;
  int const r_esi = R_ESI;
  int const r_ebx = R_EBX;
  int const r_edi = R_EDI;
  int const r_eax = R_EAX;



  consts_struct_t &cs = ctx->get_consts_struct();

  ifstream in_llvm(filename_llvm);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open llvm eqfile " << filename_llvm << "." << endl;
    NOT_REACHED();
  }
  string line;
  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER " ") + function_name);
  while (!is_line(line, match_line)) {
    if (!getline(in_llvm, line)) {
      cout << __func__ << " " << __LINE__ << ": could not find function " << function_name << " in " << filename_llvm << endl;
      NOT_REACHED();
    }
  }
  getline(in_llvm, line);
  if(!is_line(line, "=TFG")) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    NOT_REACHED();
  }

  //tfg tfg_llvm(g_ctx, &cs, llvm_base_state);
  //tfg *tfg_llvm = NULL;
  //read_tfg(in_llvm, &tfg_llvm, "llvm", ctx, false);
  tfg *tfg_llvm = new tfg_llvm_t(in_llvm, "llvm", ctx);
  //tfg_llvm->graph_from_stream(in_llvm);
  in_llvm.close();

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": tfg_llvm:\n" << tfg_llvm->graph_to_string() << endl);

  map<nextpc_id_t, callee_summary_t> const &llvm_callee_summaries = tfg_llvm->get_callee_summaries();
  const auto &llvm_symbol_map = tfg_llvm->get_symbol_map();
  const auto &llvm_locals_map = tfg_llvm->get_locals_map();
  const auto &llvm_string_contents = tfg_llvm->get_string_contents();
  const auto &llvm_nextpc_map = tfg_llvm->get_nextpc_map();
  const auto &argument_regs = tfg_llvm->get_argument_regs();
  const auto &llvm_locs = tfg_llvm->get_locs();

  map<pc, map<string_ref, expr_ref>> const &llvm_return_reg_map = tfg_llvm->get_return_regs();
  pair<transmap_t, map<pc, transmap_t>> io_tmaps_pair = construct_transmaps_using_function_calling_conventions(argument_regs, llvm_return_reg_map, r_eax);
  transmap_t const &in_tmap = io_tmaps_pair.first;
  map<pc, transmap_t> const &out_tmaps = io_tmaps_pair.second;

  //INFO("eqcheck2_start\n");
  stopwatch_run(&eqcheck_logging_timer);

  shared_ptr<tfg> dst_tfg = g_se->dst_get_tfg(dst_iseq, dst_iseq_len, ctx);
  dst_tfg->set_callee_summaries(llvm_callee_summaries);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after dst_get_tfg, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //nextpc_function_name_map_t new_nextpc_function_name_map;
  //nextpc_function_name_map_from_peep_comment(&new_nextpc_function_name_map, peep_comment);
  //printf("%s() %d: peep_comment= %s. new_nextpc_function_name_map.num_nextpcs = %d.\n", __func__, __LINE__, peep_comment, new_nextpc_function_name_map.num_nextpcs);
  //nextpc_function_name_map_copy(&new_nextpc_function_name_map, nextpc_function_name_map);
  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": new_nextpc_function_name_map =\n" << nextpc_function_name_map_to_string(&new_nextpc_function_name_map, as1, sizeof as1) << endl);
  dst_tfg->rename_recursive_calls_to_nextpcs(&new_nextpc_function_name_map);
  dst_tfg->expand_calls_to_gcc_standard_functions(&new_nextpc_function_name_map, r_esp);
  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": new_nextpc_function_name_map =\n" << nextpc_function_name_map_to_string(&new_nextpc_function_name_map, as1, sizeof as1) << endl);

  input_exec_t in;
  read_input_file(&in, execfile);
  rodata_map_t rodata_map = dst_tfg->populate_symbol_map_using_llvm_symbols_and_exec(dst_symbol_map, llvm_symbol_map, llvm_string_contents, &in);
  rodata_map.prune_non_const_symbols(llvm_symbol_map);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after populate_symbol_map_using_llvm_symbols_and_exec, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  dst_tfg->populate_nextpc_map(&new_nextpc_function_name_map);

  //dst_tfg->collapse_rodata_symbols_into_one();

  //cout << __func__ << " " << __LINE__ << ": calling rename_nextpcs" << endl;
  dst_tfg->rename_nextpcs(ctx, llvm_nextpc_map);
  //cout << __func__ << " " << __LINE__ << ": calling rename_symbols" << endl;
  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": before renaming symbols, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);
  //dst_tfg->rename_symbols(ctx, *tfg_llvm);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": before adding transmap_translations, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  pc pc_cur_start(pc::insn_label, pc::start().get_index(), pc::start().get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, pc::start().get_subsubindex());
  dst_tfg->add_extra_node_at_start_pc(pc_cur_start);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after adding transmap_translations, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //string peeptab_filename_str = string(peeptab_filename);

  dst_tfg->tfg_add_transmap_translations_for_input_arguments(argument_regs, in_tmap, r_esp);
  dst_tfg->dst_tfg_add_stack_pointer_translation_at_function_entry(r_esp);
  dst_tfg->dst_tfg_add_retaddr_translation_at_function_entry(r_esp);
  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

#if DST == ARCH_I386
  // ebp, esi, ebx, edi
  vector<exreg_id_t> callee_save_gpr = { r_ebp, r_esi, r_ebx, r_edi };
  dst_tfg->dst_tfg_add_callee_save_translation_at_function_entry(callee_save_gpr);
#endif

  dst_tfg->allocate_stack_memory_at_start_pc();

  //map<nextpc_id_t, pair<size_t, callee_summary_t>> nextpc_args_map;
  //tfg_llvm->get_nextpc_args_map(nextpc_args_map);
  //dst_tfg->add_fcall_arguments_using_nextpc_args_map(nextpc_args_map, r_esp);

  //dst_tfg->convert_jmp_nextpcs_to_callret(ctx, se, nextpc_args_map, r_esp, r_eax, false);
  dst_tfg->convert_jmp_nextpcs_to_callret(r_esp, r_eax);
  dst_tfg->dst_tfg_add_single_assignments_for_register(r_esp);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //vector<pair<pc, list<expr_ref>>> exit_conds;
  //dst_tfg->get_exit_constraints(ctx, live_out, nextpc_indir, num_live_out, tfg_llvm, exit_conds, se.get_consts_struct());

  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": before add_exit_return_value_ptr_if_needed, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);
  //dst_tfg->add_exit_return_value_ptr_if_needed(*tfg_llvm);
  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": after add_exit_return_value_ptr_if_needed, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  dst_tfg->populate_exit_return_values_using_out_tmaps(llvm_return_reg_map, out_tmaps, r_esp);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after populate exit return values, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  CPP_DBG(GENERAL, "%s: Calling dst tfg_preprocess\n", get_timestamp(as1, sizeof as1));

  /*list<pair<string, size_t>> locals_map;
  for (auto l : tfg_llvm->get_locals_map()) {
    locals_map.push_back(l);
  }*/
  dst_tfg->set_locals_map(llvm_locals_map);

  //vector<memlabel_ref> relevant_memlabels;
  //dst_tfg->graph_get_relevant_memlabels(relevant_memlabels);
  //vector<memlabel_t> relevant_memlabels;
  //for (auto const& mlr : relevant_memlabels_ref) {
  //  relevant_memlabels.push_back(mlr->get_ml());
  //}
  //cs.solver_set_relevant_memlabels(relevant_memlabels);
  //ctx->set_memlabel_assertions(memlabel_assertions_t(dst_tfg->get_symbol_map(), dst_tfg->get_locals_map(), ctx, true));

  dst_tfg->tfg_add_unknown_arg_to_fcalls();

  //CPP_DBG(GENERAL, "%s: Calling transform_function_arguments_to_avoid_stack_pointers, tfg->max_expr_size() = %zd.\n", get_timestamp(as1, sizeof as1), dst_tfg->max_expr_size());
  //dst_tfg->transform_function_arguments_to_avoid_stack_pointers();
  DYN_DEBUG2(eqgen, cout << __func__ << " " << __LINE__ << ": before preprocess, dst_tfg:\n" << dst_tfg->graph_to_string(/*true*/) << endl);

  dst_tfg->tfg_preprocess(true, list<string>(), llvm_locs);
  CPP_DBG(GENERAL, "%s: done dst tfg_preprocess\n", get_timestamp(as1, sizeof as1));

  dst_tfg->tfg_identify_rodata_accesses_and_populate_string_contents(in);
  input_exec_free(&in);

  dst_tfg->tfg_add_global_assumes_for_string_contents(true);

  //dst_tfg->mask_input_stack_and_locals_to_zero();
  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": after mask_input_stack_and_locals_to_zero, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //dst_tfg->substitute_rodata_accesses(*dst_tfg);

  //CPP_DBG_EXEC(EQGEN, cout << __func__ << " " << __LINE__ << ": after substitute rodata reads, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //dst_tfg->tighten_memlabels_using_switchmemlabel_and_simplify_until_fixpoint(cs);

  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": after preprocess, dst_tfg:\n" << dst_tfg->graph_to_string() << endl);

  //assert(peeptab_filename_str.substr(peeptab_filename_str.length() - 5) == ".ptab");
  //string eqfile = outfile; //peeptab_filename_str.substr(0, peeptab_filename_str.length() - 5) + ".tfg";
  /*string peeptabs_str = "peeptabs";
  size_t found = eqfile.find(peeptabs_str);
  if (found != string::npos) {
    eqfile.replace(found, peeptabs_str.length(), "eqfiles");
  }

  string::size_type filePos = eqfile.rfind('/');
  if (filePos != string::npos) {
    string dir = eqfile.substr(0, filePos);
    if (!file_exists(dir)) {
      cout << "calling mkdir on " << dir;
      int ret = system(("mkdir -p " + dir).data());
      //ASSERT(ret != -1);
    }
  }*/

  CPP_DBG_EXEC(SYM_EXEC, cout << __func__ << " " << __LINE__ << ": dst_tfg:\n" << dst_tfg->graph_to_string(/*true*/) << endl);

  tfg::generate_dst_tfg_file(outfile, function_name, rodata_map, *dst_tfg, dst_iseq_vec, insn_pcs_vec);

  //delete dst_tfg;
  delete[] dst_iseq;
  stopwatch_stop(&eqcheck_logging_timer);
  CPP_DBG_EXEC2(STATS, g_stats_print());
}

}
