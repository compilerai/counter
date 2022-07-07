#pragma once

#include "support/types.h"
#include "support/manager.h"
#include "support/free_id_list.h"

#include "expr/langtype.h"
#include "expr/expr.h"
//#include "expr/solver_interface.h"
//#include "expr/memlabel_map.h"
#include "expr/mlvarname.h"
#include "expr/axpred_desc.h"
#include "expr/fsignature.h"
#include "expr/proof_result.h"
#include "expr/expr_eval_status.h"
#include "expr/axpred_context.h"
#include "expr/proof_status.h"

struct input_exec_t;
struct symbol_map_t;

namespace eqspace {
using namespace std;

class symbol_or_section_id_t;
class context_cache_t;
struct consts_struct_t;
//class callee_summary_t;
class sprel_map_t;
class graph_memlabel_map_t;
class sprel_map_pair_t;
class counter_example_t;
class state;
class mybitset;
class query_comment;
class sage_interface;
class rodata_offset_t;
class graph_arg_regs_t;
class solver;

typedef enum { bound_type_lower_bound, bound_type_upper_bound } bound_type_t;

string proof_status_to_string(proof_status_t);

class context
{
public:
  class config
  {
  public:
    config(unsigned smt_timeout, unsigned sage_timeout = 0)
    : smt_timeout_secs(smt_timeout),
      sage_timeout_secs(sage_timeout),
      prove_dump_threshold_secs(smt_timeout*2),
      dht_dump_threshold_secs(smt_timeout*2)
    { }

    bool disable_caches() const
    {
      return DISABLE_CACHES;
    }

    void disable_expr_query_caches(void)
    {
      m_enable_query_caches = false;
    }

    bool expr_query_caches_are_disabled(void) const
    {
      return !m_enable_query_caches;
    }

    unsigned max_stack_push_delta = (1<<31);
    unsigned smt_timeout_secs;
    unsigned sage_timeout_secs;
    unsigned prove_dump_threshold_secs;
    unsigned dht_dump_threshold_secs;
    bool dump_timed_out_decide_hoare_triple_queries = false;
    bool DISABLE_CACHES = false;
    //unsigned solver_config = 0;
    //unsigned max_lookahead = 0;
    bool m_enable_query_caches = true;
    bool add_mod_exprs = false;
    unsigned unroll_factor = 1;
    unsigned dst_unroll_factor = 1;
    unsigned path_exprs_lookahead = 1;
    bool disable_constant_inequality_inference = false;
    bool anchor_loop_tail = false;
    bool consider_non_vars_for_dst_ineq = false;
    bool use_sage = false;
    bool use_unique_smt_filenames = false;
    bool enable_query_decomposition = false;
    bool enable_residual_loop_unroll = false;
    bool enable_loop_path_exprs = false;
    bool disable_normalized_loop_iteration_variables= false;
    bool disable_epsilon_src_paths = false;
    bool disable_dst_bv_rank= false;
    bool disable_src_bv_rank = false;
    bool disable_propagation_based_pruning = false;
    bool disable_all_static_heuristics = false;
    bool choose_longest_delta_first = false;
    bool choose_shortest_path_first = false;
    bool disable_unaliased_memslot_conversion = false;
    bool enable_CE_fuzzing = false;
    std::string axpreds_file = "";
    std::string axpreds_debug_file = "";
    bool use_esp_versions_for_guessing_lsprels = true;
    bool do_not_use_debug_headers_for_lsprels = false;
    bool use_compile_log_for_guessing_lsprels = true;
    bool enable_semantic_simplification_in_prove_path = false;
    bool disable_SSA = false;
    bool disable_ssa_cloning = false;
    bool disable_solver_cache = false;
  };

  enum xml_output_format_t { XML_OUTPUT_FORMAT_HTML, XML_OUTPUT_FORMAT_TEXT_COLOR, XML_OUTPUT_FORMAT_TEXT_NOCOLOR };

  context(config const &cfg, bool no_solver=false);
  ~context();

  static xml_output_format_t xml_output_format_from_string(string const& xml_output_format_str);
  static string xml_output_format_to_string(xml_output_format_t fmt);


  bool disable_caches() const { return m_config.disable_caches(); }
  void disable_expr_query_caches() { m_config.disable_expr_query_caches(); }
  bool expr_query_caches_are_disabled() const { return m_config.expr_query_caches_are_disabled(); }
  void enable_mem_fuzzing() { m_config.enable_CE_fuzzing = true; }
  void disable_mem_fuzzing() { m_config.enable_CE_fuzzing = false; }

  const config& get_config() const;
  void set_config(const config& cfg);

  consts_struct_t &get_consts_struct();

  axpred::AxPredContext& get_axpred_context();

  bool timeout_occured();
  void clear_solver_cache();

  sort_ref mk_unresolved_sort();
  sort_ref mk_undefined_sort();
  sort_ref mk_bv_sort(unsigned size);
  sort_ref mk_float_sort(unsigned size);
  sort_ref mk_floatx_sort(unsigned size);
  sort_ref mk_bool_sort();
  sort_ref mk_array_sort(sort_ref const &domain, sort_ref const &range);
  sort_ref mk_array_sort(vector<sort_ref> const &domain, sort_ref const &range);
  sort_ref mk_struct_sort(vector<sort_ref> const &domain);
  sort_ref mk_int_sort();
  sort_ref mk_function_sort(vector<sort_ref> const &domain, sort_ref const &range);
  sort_ref mk_memlabel_sort();
  sort_ref mk_comment_sort();
  sort_ref mk_axpred_desc_sort();
  sort_ref mk_langtype_sort();
  sort_ref mk_memaccess_size_sort();
  sort_ref mk_memaccess_type_sort();
  sort_ref mk_regid_sort();
  sort_ref mk_rounding_mode_sort();
  sort_ref mk_count_sort();
  expr_ref mk_undefined(expr_id_t suggested_id = 0);
  expr_ref mk_var(string const &name, sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_var(string_ref const &name, sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_uninit_var(sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_function_decl(string const &name, const vector<sort_ref>& args, sort_ref const &ret, expr_id_t suggested_id = 0);
  expr_ref mk_expr_ref(istream& is, string const &name);

  sort_ref get_addr_sort()       { return this->mk_bv_sort(this->get_addr_size()); }
  unsigned get_addr_size() const;
  sort_ref get_i386_fpstack_sort();

  expr_ref get_dummy_arg_addr();

  expr_ref mk_array_const_with_def(sort_ref const &, expr_ref const& def, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(array_constant_ref const &, sort_ref const &s, expr_id_t suggested_id = 0);
  //expr_ref mk_array_const(map<vector<string>, string> const &array_constant_map, sort_ref const &s, consts_struct_t const &cs, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const &array_constant_map, sort_ref const &s, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(shared_ptr<array_constant_string_t> const &array_constant_str, sort_ref const &s, expr_id_t suggested_id = 0);


  expr_ref mk_float_const_using_rounding_mode(rounding_mode_t rm, int size, float_max_t value, expr_id_t suggested_id = 0);
  expr_ref mk_float_const(int size, float_max_t value, expr_id_t suggested_id = 0);
  expr_ref mk_float_nan(int size, expr_id_t suggested_id = 0);
  expr_ref mk_float_positive_infinity(int size, expr_id_t suggested_id = 0);
  expr_ref mk_float_negative_infinity(int size, expr_id_t suggested_id = 0);
  expr_ref mk_float_const(bv_const const& sign, size_t exponent_sz, bv_const const& exponent, size_t significand_sz, bv_const const& significand, expr_id_t suggested_id = 0);
  expr_ref mk_float_const(size_t numbits, mybitset const& mbs, expr_id_t suggested_id = 0);

  expr_ref mk_floatx_const(size_t numbits, mybitset const& mbs, expr_id_t suggested_id = 0);

  //expr_ref mk_io_const(mybitset const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, int64_t value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, uint64_t value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, int value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, mybitset const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, bv_const const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const_from_string(int size, string_ref const &value, expr_id_t suggested_id = 0);
  expr_ref mk_float_const_from_string(int size, string_ref const &value, expr_id_t suggested_id = 0);
  expr_ref mk_floatx_const_from_string(int size, string_ref const &value, expr_id_t suggested_id = 0);
  //expr_ref mk_io_const_from_string(string value, expr_id_t suggested_id = 0);
  expr_ref mk_int_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_regid_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_rounding_mode_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_count_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_comment_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  //expr_ref mk_container_type_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_axpred_desc_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_langtype_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_int_const(int value, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_const(memlabel_t const &ml, expr_id_t suggested_id = 0);
  expr_ref mk_axpred_desc_const(axpred_desc_ref const &axpred_desc, expr_id_t suggested_id = 0);
  //expr_ref mk_memlabel_const_from_mybitset(mybitset const &mbs, expr_id_t suggested_id = 0);
  expr_ref mk_const_from_string(string_ref const &s, sort_ref const &sort, expr_id_t suggested_id = 0);
  expr_ref mk_comment_const(comment_t const& c, expr_id_t suggested_id = 0);
  expr_ref mk_memaccess_size_const(memaccess_size_t memaccess_size, expr_id_t suggested_id = 0);
  expr_ref mk_memaccess_type_const(memaccess_type_t memaccess_type, expr_id_t suggested_id = 0);
  expr_ref mk_langtype_const(langtype_ref lt, expr_id_t suggested_id = 0);
  expr_ref mk_regid_const(int regid, expr_id_t suggested_id = 0);
  expr_ref mk_rounding_mode_const(rounding_mode_t rounding_mode, expr_id_t suggested_id = 0);
  expr_ref mk_count_const(int count, expr_id_t suggested_id = 0);
  expr_ref mk_bool_true(expr_id_t suggested_id = 0);
  expr_ref mk_bool_false(expr_id_t suggested_id = 0);
  expr_ref mk_bool_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_bool_const(bool value, expr_id_t suggested_id = 0);
  expr_ref mk_zerobv(int size, expr_id_t suggested_id = 0);
  expr_ref mk_onebv(int size, expr_id_t suggested_id = 0);
  expr_ref mk_minusonebv(int size, expr_id_t suggested_id = 0);

  expr_ref array_constant_struct_range_true_value(expr_id_t suggested_id = 0);
  expr_ref array_constant_struct_range_false_value(expr_id_t suggested_id = 0);

  expr_ref mk_app(expr::operation_kind kind, const expr_vector& args, expr_id_t suggested_id = 0);
  expr_ref mk_axpred(const axpred_desc_ref& axpred_desc, const expr_vector& args, expr_id_t suggested_id = 0);
  expr_ref mk_app_unresolved(expr::operation_kind kind, const expr_vector& args, expr_id_t suggested_id = 0);

  expr_ref mk_iff(expr_ref const &a, expr_ref const &b, expr_id_t suggested_id = 0);
  expr_ref mk_eq(expr_ref const &a, expr_ref const &b, expr_id_t suggested_id = 0);

  expr_ref mk_or(vector<expr_ref> const &args, expr_id_t suggested_id = 0);
  expr_ref mk_or(expr_ref const &a, expr_ref const &b, expr_id_t suggested_id = 0);
  expr_ref mk_and(vector<expr_ref> const &args, expr_id_t suggested_id = 0);
  expr_ref mk_and(expr_ref const &a, expr_ref const &b, expr_id_t suggested_id = 0);
  expr_ref mk_andnot1(expr_ref const &a, expr_ref const &b, expr_id_t suggested_id = 0);
  expr_ref mk_andnot2(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_not(expr_ref a, expr_id_t suggested_id = 0);
  expr_ref mk_xor(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_implies(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);

  expr_ref mk_ite(expr_ref cond, expr_ref a, expr_ref b, expr_id_t suggested_id = 0);

  expr_ref mk_bvnot(expr_ref a, expr_id_t suggested_id = 0);
  expr_ref mk_bvor(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvand(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvnor(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvnand(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvxor(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvxnor(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvneg(expr_ref a, expr_id_t suggested_id = 0);
  expr_ref mk_bvsub(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvadd(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvmul(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvudiv(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvsdiv(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvurem(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvsrem(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvconcat(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvconcat(list<expr_ref> const &a, expr_id_t suggested_id = 0);

  // l : high bit (left bit)
  // r : low bit (right bit)
  // l is more significant than r
  // final size would be l - r + 1
  expr_ref mk_bvextract(expr_ref a, unsigned l, unsigned r, expr_id_t suggested_id = 0);
  expr_ref mk_bvset(expr_ref a, int stop, int start, expr_ref data, expr_id_t suggested_id = 0);

  expr_ref mk_bvabs(expr_ref a, expr_id_t suggested_id = 0);
  expr_ref mk_bvsign(expr_ref a, expr_id_t suggested_id = 0);
  expr_ref mk_donotsimplify_using_solver_subsign(expr_ref orig, expr_ref cmp_arg1, expr_ref cmp_arg2, expr_id_t suggested_id = 0);
  expr_ref mk_donotsimplify_using_solver_suboverflow(expr_ref orig, expr_ref cmp_arg1, expr_ref cmp_arg2, expr_id_t suggested_id = 0);
  expr_ref mk_donotsimplify_using_solver_subzero(expr_ref orig, expr_ref cmp_arg1, expr_ref cmp_arg2, expr_id_t suggested_id = 0);
  //expr_ref mk_function_argument_ptr(expr_ref a, memlabel_t const &ml, expr_ref mem, expr_id_t suggested_id = 0);
  expr_ref mk_return_value_ptr(expr_ref a, memlabel_t const &ml, expr_ref mem, expr_id_t suggested_id = 0);
  expr_ref mk_memmask(expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const &ml/*, expr_ref addr, size_t memsize*/, expr_id_t suggested_id = 0);
  expr_ref mk_memmask_composite(expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const &ml);
  expr_ref mk_memmasks_are_equal(expr_ref const& mem1, expr_ref const& mem_alloc1, expr_ref const& mem2, expr_ref const& mem_alloc2, memlabel_t const &ml, expr_id_t suggested_id = 0);
  expr_ref mk_memmasks_are_equal_else(expr_ref const& mem1, expr_ref const& mem_alloc1, expr_ref const& mem2, expr_ref const& mem_alloc2, memlabel_t const &ml, expr_ref const& def_val, expr_id_t suggested_id = 0);
  //expr_ref mk_memsplice(expr_vector const &args, expr_id_t suggested_id = 0);
  //expr_ref mk_memjoin(expr_vector const &args);
  //expr_ref mk_memsplice(expr_ref a, expr_ref b);
  //expr_ref mk_memsplice_arg(expr_ref a, memlabel_t const &ml);
  expr_ref mk_alloca(expr_ref const& mem_alloc, memlabel_t const &ml, expr_ref const& addr, expr_ref const& size, expr_id_t suggested_id = 0);
  //expr_ref mk_alloca_ptr(expr_ref const& local_alloc_count, expr_ref const& mem_alloc, memlabel_t const& ml, expr_ref const& local_size, expr_id_t suggested_id = 0);
  //expr_ref mk_alloca_size(expr_ref const& local_alloc_count, expr_ref const& mem_alloc, memlabel_t const& ml, expr_id_t suggested_id = 0);

  expr_ref mk_dealloc(expr_ref const& mem_alloc, memlabel_t const &ml, expr_ref const& addr, expr_ref const& size, expr_id_t suggested_id = 0);

  expr_ref mk_select_shadow_bool(expr_ref const& mem, expr_ref const& addr, size_t count, expr_id_t suggested_id = 0);
  expr_ref mk_store_shadow_bool(expr_ref const& mem, expr_ref const& addr, expr_ref const& data, size_t count, expr_id_t suggested_id = 0);

  expr_ref mk_heap_alloc(expr_ref const& mem_alloc, memlabel_t const &ml, expr_ref const& addr, expr_ref const& size, expr_id_t suggested_id = 0);
  expr_ref mk_heap_dealloc(expr_ref const& mem_alloc, memlabel_t const &ml, expr_ref const& addr, expr_ref const& size, expr_id_t suggested_id = 0);
  expr_ref mk_heap_alloc_ptr(expr_ref const& ptr, memlabel_t const& ml, expr_id_t suggested_id = 0);

  expr_ref mk_bvmask(expr_ref a, unsigned masklen, expr_id_t suggested_id = 0);
  expr_ref mk_bvumask(expr_ref a, unsigned masklen, expr_id_t suggested_id = 0);

  //expr_ref mk_select(expr_ref arr, expr_ref addr, unsigned count);
  //expr_ref mk_store(expr_ref arr, expr_ref addr, expr_ref data, unsigned count);
  expr_ref mk_select(expr_ref const &mem, expr_ref const& mem_alloc, memlabel_t const &memlabel, expr_ref const &addr, unsigned count, bool bigendian,
      /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_select(expr_ref const &mem, expr_ref const& mem_alloc, mlvarname_t const &mlvarname, expr_ref const &addr, unsigned count, bool bigendian,
      /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_store(expr_ref const &mem, expr_ref const& mem_alloc, memlabel_t const &memlabel, expr_ref const &addr, expr_ref const &data, unsigned count,
      bool bigendian, /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_store(expr_ref const &mem, expr_ref const& mem_alloc, mlvarname_t const &mlvarname, expr_ref const &addr, expr_ref const &data, unsigned count,
      bool bigendian, /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_store_uninit(expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const& memlabel, expr_ref const& addr, expr_ref const& count, expr_ref const& nonce, expr_id_t suggested_id = 0);
  expr_ref mk_is_memaccess_langtype(expr_ref const& e, memaccess_size_t memaccess_size, langtype_ref const& lt, memaccess_type_t memaccess_type, expr_id_t suggested_id = 0);
  expr_ref mk_islangaligned(expr_ref e, size_t align, expr_id_t suggested_id = 0);
  expr_ref mk_isshiftcount(expr_ref e, size_t shifted_val_size, expr_id_t suggested_id = 0);
  //expr_ref mk_isgepoffset(expr_ref e, size_t gepoffset, expr_id_t suggested_id = 0);
  expr_ref mk_isgepoffset(expr_ref const &e, expr_ref const &gepoffset, expr_id_t suggested_id = 0);
  expr_ref mk_isindexforsize(expr_ref const &e, uint64_t size, expr_id_t suggested_id = 0);

  //expr_ref mk_ismemlabel(expr_ref const& mem_alloc, expr_ref const& addr, unsigned count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  //expr_ref mk_ismemlabel(expr_ref const& mem_alloc, expr_ref const& addr, expr_ref const& count, memlabel_ref const &memlabel, expr_id_t suggested_id = 0);
  //expr_ref mk_ismemlabel(expr_ref const& mem_alloc, expr_ref const& addr, expr_ref const& count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_belongs_to_same_region(expr_ref const& mem_alloc, expr_ref const& addr, unsigned count, expr_id_t suggested_id = 0);
  expr_ref mk_belongs_to_same_region(expr_ref const& mem_alloc, expr_ref const& addr, expr_ref const& count, expr_id_t suggested_id = 0);
  expr_ref mk_region_agrees_with_memlabel(expr_ref const& mem_alloc, expr_ref const& addr, unsigned count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_region_agrees_with_memlabel(expr_ref const& mem_alloc, expr_ref const& addr, expr_ref const& count, memlabel_ref const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_region_agrees_with_memlabel(expr_ref const& mem_alloc, expr_ref const& addr, expr_ref const& count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_iscontiguous_memlabel(expr_ref const& mem_alloc, expr_ref const& lb, expr_ref const& ub, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_isprobably_contiguous_memlabel(expr_ref const& mem_alloc, expr_ref const& lb, expr_ref const& ub, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_issubsuming_memlabel_for(expr_ref const& mem_alloc, expr_ref const& subsuming_lb, expr_ref const& subsuming_ub, memlabel_t const &subsuming_ml, memlabel_t const& subsumed_mls, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_is_absent(expr_ref const& mem_alloc, memlabel_t const& ml, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_is_absent(expr_ref const& mem_alloc, expr_ref const& ml, expr_id_t suggested_id = 0);
  //expr_ref mk_region_agrees_with_memlabel(expr_ref const& mem_alloc, expr_ref const& start, expr_ref const& count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);

  expr_ref mk_bool_to_bv(expr_ref const &e, expr_id_t suggested_id = 0);
  //expr_ref mk_switchmemlabel(expr_ref e, vector<memlabel_t> const &memlabels, vector<expr_ref> const &leaves);

  expr_ref mk_fpext(expr_ref const &e, int ebits, int sbits, expr_id_t suggested_id = 0);
  expr_ref mk_fptrunc(expr_ref const& rm, expr_ref const &e, int ebits, int sbits, expr_id_t suggested_id = 0);

  expr_ref mk_fcmp_false(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_oeq(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ogt(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_oge(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_olt(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ole(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_one(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ord(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_uno(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ueq(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ugt(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_uge(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ult(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_ule(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fcmp_une(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
	expr_ref mk_fcmp_true(expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);

	expr_ref mk_float_is_nan(expr_ref const& a, expr_id_t suggested_id = 0);

  expr_ref mk_fdiv(expr_ref const& rm, expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fmul(expr_ref const& rm, expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fadd(expr_ref const& rm, expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fsub(expr_ref const& rm, expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_frem(expr_ref const& rm, expr_ref const& a, expr_ref const& b, expr_id_t suggested_id = 0);
  expr_ref mk_fneg(expr_ref const& a, expr_id_t suggested_id = 0);

  expr_ref mk_fp_to_ubv(expr_ref const& rm, expr_ref const& a, int ubvsize, expr_id_t suggested_id = 0);
  expr_ref mk_fp_to_sbv(expr_ref const& rm, expr_ref const& a, int sbvsize, expr_id_t suggested_id = 0);
  expr_ref mk_ubv_to_fp(expr_ref const& rm, expr_ref const& a, int fpsize, expr_id_t suggested_id = 0);
  expr_ref mk_sbv_to_fp(expr_ref const& rm, expr_ref const& a, int fpsize, expr_id_t suggested_id = 0);

  expr_ref mk_float_to_ieee_bv(expr_ref const& a, expr_id_t suggested_id = 0);
  expr_ref mk_ieee_bv_to_float(expr_ref const& a, expr_id_t suggested_id = 0);
  expr_ref mk_floatx_to_ieee_bv(expr_ref const& a, expr_id_t suggested_id = 0);
  expr_ref mk_ieee_bv_to_floatx(expr_ref const& a, expr_id_t suggested_id = 0);

  expr_ref mk_float_to_floatx(expr_ref const& a, expr_id_t suggested_id = 0);
  expr_ref mk_floatx_to_float(expr_ref const& a, expr_id_t suggested_id = 0);

  expr_ref mk_bvsign_ext(expr_ref a, unsigned l, expr_id_t suggested_id = 0);
  expr_ref mk_bvzero_ext(expr_ref a, unsigned l, expr_id_t suggested_id = 0);
  expr_ref mk_bvzero_ext1(expr_ref a, expr_id_t suggested_id = 0);

  expr_ref mk_bvshl(expr_ref a, unsigned shlen, expr_id_t suggested_id = 0);
  expr_ref mk_bvlshr(expr_ref a, unsigned shlen, expr_id_t suggested_id = 0);
  expr_ref mk_bvashr(expr_ref a, unsigned shlen, expr_id_t suggested_id = 0);
  expr_ref mk_bvexshl(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvexlshr(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvexashr(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);

  expr_ref mk_bvrotate_left(expr_ref a, unsigned l, expr_id_t suggested_id = 0);
  expr_ref mk_bvrotate_right(expr_ref a, unsigned l, expr_id_t suggested_id = 0);
  expr_ref mk_bvextrotate_left(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvextrotate_right(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);

  expr_ref mk_bvult(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvule(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvsle(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvslt(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvugt(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvuge(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvsge(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);
  expr_ref mk_bvsgt(expr_ref a, expr_ref b, expr_id_t suggested_id = 0);

  expr_ref mk_function_call(expr_ref func, const vector<expr_ref>& args, expr_id_t suggested_id = 0);
  // used in counter example parsing
  expr_ref mk_apply(expr_ref func, vector<expr_ref> const& args, expr_id_t suggested_id = 0);
  expr_ref mk_lambda(expr_ref const& formal_arg, expr_ref const& body, expr_id_t suggested_id = 0);
  expr_ref mk_lambda(vector<expr_ref> const& formal_args, expr_ref const& body, expr_id_t suggested_id = 0);

  expr_ref mk_mkstruct(vector<expr_ref> const& domain, expr_id_t suggested_id = 0);
  expr_ref mk_getfield(expr_ref const& structv, int fieldnum, expr_id_t suggested_id = 0);

  expr_ref mk_increment_count(expr_ref const& incount, expr_id_t suggested_id = 0);

  //expr_ref expr_rename_memlabels(expr_ref const &in, alias_region_t::alias_type at, map<variable_id_t, variable_id_t> const &m);
  //expr_ref expr_rename_memlabel_vars_prefix(expr_ref const &in, string const &from_prefix, string const &to_prefix);

  shared_ptr<constant_value> mk_constant_value();
  shared_ptr<constant_value> mk_constant_value_from_string(string_ref const &data, sort_ref const &s);
  shared_ptr<constant_value> mk_constant_value_bitset_from_string(string_ref const &data, sort_ref const &s);
  shared_ptr<constant_value> mk_constant_value(int data, sort_ref s);
  shared_ptr<constant_value> mk_constant_value(uint64_t data, sort_ref s);
  shared_ptr<constant_value> mk_constant_value(int64_t data, sort_ref s);
  shared_ptr<constant_value> mk_constant_value(sort_ref s, context* ctx);
  shared_ptr<constant_value> mk_constant_value(array_constant_ref a, context* ctx);
  shared_ptr<constant_value> mk_constant_value(mybitset const &mbs);
  shared_ptr<constant_value> mk_constant_value_from_memlabel(memlabel_t const &ml);
  shared_ptr<constant_value> mk_constant_value_from_rounding_mode(rounding_mode_t rounding_mode);
  shared_ptr<constant_value> mk_constant_value(float_max_t f);

  array_constant_ref mk_array_constant();
  array_constant_ref mk_array_constant(map<pair<expr_ref, expr_ref>, expr_ref> const&  range_mapping, map<pair<expr_ref, expr_ref>, random_ac_default_value_t> const& rac_mappings);
  array_constant_ref mk_array_constant(sort_ref const& domain_sort, map<pair<expr_ref, expr_ref>, expr_ref> const&  range_mapping, random_ac_default_value_t const& default_val);
  array_constant_ref mk_array_constant(map<pair<expr_ref, expr_ref>, expr_ref> const&  range_mapping);
  array_constant_ref mk_array_constant(sort_ref const& domain_sort, random_ac_default_value_t const& default_val);
  array_constant_ref mk_array_constant(vector<sort_ref> const& domain_sort, map<vector<expr_ref>, expr_ref> const &mapping, expr_ref const& default_val);
  array_constant_ref mk_array_constant(vector<sort_ref> const& domain_sort, expr_ref const& default_val);
  array_constant_ref mk_array_constant(array_constant const& ac);

  expr_ref get_input_expr_for_key(string_ref key, sort_ref const& s);
  string_ref get_key_from_input_expr(expr_ref const& e) const;
  bool expr_is_input_expr(expr_ref const& e) const;
  bool key_represents_hidden_var(string_ref const& s) const;
  static bool varname_represents_lambda(string_ref const& s);
  bool key_represents_ghost_var(string_ref const& s) const;
  bool key_represents_phi_tmpvar(string_ref const& s) const;
  bool key_represents_exreg(string_ref const& sr, exreg_group_id_t& exreg_group, exreg_id_t& exreg_id) const;

  bool key_represents_memlabel_bound(string_ref const& sr) const;
  bool expr_is_symbol_memlabel_bound(expr_ref const& e) const;
  bool expr_is_rodata_memlabel_bound(expr_ref const& e) const;
  string_ref get_key_for_memlabel_lb(memlabel_t const& ml) const;
  string_ref get_key_for_memlabel_ub(memlabel_t const& ml) const;
  expr_ref get_lb_expr_for_memlabel(memlabel_t const& ml);
  expr_ref get_ub_expr_for_memlabel(memlabel_t const& ml);

  string_ref get_key_for_local_memlabel_lb(memlabel_t const& ml, string const& srcdst_keyword) const;
  string_ref get_key_for_local_memlabel_ub(memlabel_t const& ml, string const& srcdst_keyword) const;
  string_ref get_key_for_local_memlabel_is_absent(memlabel_t const& ml, string const& srcdst_keyword) const;
  expr_ref get_lb_expr_for_local_memlabel(memlabel_t const& ml, string const& srcdst_keyword);
  expr_ref get_ub_expr_for_local_memlabel(memlabel_t const& ml, string const& srcdst_keyword);
  expr_ref get_is_absent_expr_for_local_memlabel(memlabel_t const& ml, string const& srcdst_keyword);
  //expr_ref get_alloca_seen_expr_for_local(local_id_t lid, sort_ref const& s);

  //pair<vector<sort_ref>, sort_ref> get_argument_retval_sort_for_op(expr::operation_kind op, container_type_ref const& container_type);

  expr_ref get_vararg_local_expr();
  bool is_vararg_local_expr(expr_ref const& e) const;

  expr_ref get_alloca_size_fn(sort_ref const& s);

  bool src_dst_represent_axpred_query(expr_ref& src, expr_ref& dst);

  static string get_sort_name_for_struct_naming(sort_ref const& s);
  static string get_aggregate_struct_name_from_domain_names(vector<string> const& domain_names);

  char* write_and_get_buffer(const string& s);
  manager<expr_sort>& get_sort_manager();
  manager<expr_sort> const & get_sort_manager() const;
  manager<expr>& get_expr_manager();
  manager<expr> const & get_expr_manager() const;
  manager<constant_value>& get_constant_value_manager();
  manager<constant_value> const & get_constant_value_manager() const;
  manager<array_constant>& get_array_constant_manager();
  manager<array_constant> const& get_array_constant_manager() const;
  solver* get_solver();
  sage_interface* get_sage_interface();
  free_id_list& get_free_sort_ids();
  free_id_list& get_free_constant_value_ids() { return m_free_constant_value_ids; }
  free_id_list& get_free_array_constant_ids() { return m_free_array_constant_ids; }
  free_id_list& get_free_expr_ids();
  string stats() const;
  //expr_map_cache& get_expr_simplify_cache() { return m_simplify_cache; }
  void clear_cache(void);
  void gen_sym_exec_reader_file_expr_table(FILE *ofp);
  //manager<expr> const *get_expr_manager_ptr() { return &m_expr_manager; }
  //manager<expr_sort> const *get_sort_manager_ptr() { return &m_sort_manager; }
  //bool expr_set_lhs_prove_rhs(set<pair<expr_ref, pair<expr_ref, expr_ref>>/*, expr_triple_compare*/> const &lhs, expr_ref rhs, query_comment const &comment, bool timeout_res, counter_example_t &counter_example);

  bool expr_is_less_than(expr_ref a, expr_ref b);

  expr_ref get_lambda_var(int lambda_num, sort_ref const& s);
  expr_ref expr_replace_with_lambda(expr_ref const& haystack, expr_ref const& needle, expr_ref const& lambda_var);

  static string get_cur_rounding_mode_varname_for_prefix(string const& prefix);

  /*bool sat_check(expr_ref rhs, query_comment const &comment, bool timeout_res)
  {
    //expr_ref rhs_sub = expr_substitute_global_theorems(rhs);
    return rhs->is_satisfiable(comment, timeout_res);
  }*/
  expr_ref create_new_expr(expr::operation_kind k, const vector<expr_ref>&, expr_id_t suggested_id);
  bool is_memlabel_sort_in_solver(sort_ref const& ref_sort, consts_struct_t const &cs);
  sort_ref memlabel_sort_in_solver(consts_struct_t const &cs);
  bool is_regid_sort_in_solver(sort_ref const& ref_sort);
  //sort_ref regid_sort_in_solver();
  //sort_ref io_sort_in_solver();
  void parse_consts_db(char const *filename);

  size_t get_num_proof_queries() const;
  void increment_num_proof_queries();
  void increment_num_proof_queries_answered_by_syntactic_check();
  //void increment_num_proof_queries_answered_by_counter_examples();

  //expr_ref expr_simplify_recursive(expr_ref const &e, long depth, consts_struct_t const &cs);
  expr_ref expr_do_simplify(expr_ref const &e);
  expr_ref expr_do_simplify_no_const_specific(expr_ref const &e);
  //expr_ref expr_replace_sp_memlabel_with_locals(expr_ref const &e, memlabel_t const &ml, consts_struct_t const &cs);

  string_ref get_local_alloc_count_varname(string const& srcdst_keyword) const;
  string_ref get_local_alloc_count_ssa_varname(string const& srcdst_keyword, allocsite_t const& allocsite) const;

  expr_ref get_local_alloc_count_var(string const& srcdst_keyword);
  expr_ref get_local_alloc_count_ssa_var(string const& srcdst_keyword, allocsite_t const& allocsite);

  expr_ref expr_substitute(expr_ref const &e, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, set<pair<expr::operation_kind, int>> const& ops_to_avoid_descending = {});
  expr_ref expr_substitute_using_var_to_expr_map(expr_ref const &e, map<string, expr_ref> const &var2expr);
  //expr_ref expr_substitute_using_counter_example(expr_ref const &e, counter_example_t const &counter_example);
  //expr_ref substitute_using_lhs_set(set<predicate> const &lhs_set, consts_struct_t const &cs, context *ctx);
  expr_ref expr_replace_memmask_and_memsplice_operators_using_counter_example(expr_ref const &e/*, counter_example_t &counter_example*/, consts_struct_t const &cs);
  expr_ref expr_eliminate_memlabel_esp(expr_ref const &e);
  //expr_ref substitute_till_fixpoint(map<unsigned, pair<expr_ref, expr_ref> > const &submap);
  //expr_ref specialize(expr_ref with, query_comment const &qc);
  expr_list expr_get_vars(expr_ref const &e);
  bool expr_contains_expr(expr_ref const &e, expr_ref const &var);
  bool expr_contains_memlabel_address_in_arith_relation(expr_ref const& e);
  bool expr_contains_memlabel_top(expr_ref const &e);
  //bool expr_contains_input_memory_constant_expr(expr_ref const &e, consts_struct_t const &cs);
  set<expr_ref, expr_compare> expr_get_varset(expr_ref const &e);
  expr_list expr_get_subexprs(expr_ref const &e, vector<memlabel_t> const &relevant_memlabels);
  bool expr_represents_memory_access(expr_ref const& e);
  set<expr_ref> expr_get_stack_mem_exprs(expr_ref const& e);
  expr_ref expr_model_stack_as_separate_mem(expr_ref const& e, expr_ref const& mem_version, expr_ref &stk_mem);
  expr_ref expr_try_converting_unaliased_memslots_to_fresh_vars(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>> &submap);
  expr_ref expr_try_breaking_bvextracts_to_fresh_vars(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>> &submap);
  //expr_ref expr_zero_out_comments(expr_ref const &e);

  expr_ref expr_remove_donotsimplify_using_solver_operators(expr_ref const &e);
  expr_ref expr_replace_donotsimplify_using_solver_expressions_by_free_vars(expr_ref const &e, map<unsigned, pair<expr_ref, expr_ref> > &submap);
  //expr_ref expr_rename_locals_to_stack_pointer(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref expr_rename_farg_ptr_memlabel_top_to_notstack(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref expr_substitute_rodata_accesses(expr_ref const &e, tfg const &t, consts_struct_t const &cs);

  expr_ref expr_model_callee_readable_writeable_regions_in_fcalls(expr_ref const &e);

  expr_ref expr_label_memlabels_using_alias_and_memlabel_maps(expr_ref const &e, map<expr_ref, memlabel_t> const &alias_map, graph_memlabel_map_t const &memlabel_map);
  expr_ref expr_simplify_using_sprel_and_memlabel_maps(expr_ref const &e, sprel_map_t const &sprel_map, graph_memlabel_map_t const &memlabel_map);
  expr_ref expr_simplify_using_sprel_pair_and_memlabel_maps(expr_ref const &e, sprel_map_pair_t const &sprel_map_pair, graph_memlabel_map_t const &memlabel_map/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, map<string, expr_ref> const &argument_regs*/);

  bool msbits_are_zero(expr_ref const &e, size_t start_bitindex);
  //expr_ref expr_canonicalize_llvm_symbols(expr_ref const &e, graph_symbol_map_t const &symbol_map/*, map<string, pair<size_t, bool>> const &symbol_size_map*/);
  expr_ref expr_canonicalize_llvm_nextpcs(expr_ref const &e, map<int, string> &nextpc_map);
  expr_ref expr_replace_alloca_with_store_zero(expr_ref const &e);
  expr_ref expr_replace_alloca_with_nop(expr_ref const &e);
  //expr_ref expr_set_caller_save_registers_to_zero(expr_ref const &e, state const &start_state);
  set<expr_ref> expr_substitute_function_arguments_with_debug_probes(expr_ref const &e, string keyword);
  //expr_ref expr_add_fcall_arguments_using_nextpc_args_map(expr_ref const &e, map<nextpc_id_t, pair<size_t, callee_summary_t>> const &nextpc_args_map, expr_ref esp_expr, consts_struct_t const &cs);
  //expr_ref tighten_memlabels_using_switchmemlabel(void);
  //expr_ref expr_remove_fcall_side_effects(expr_ref const &e);
  //expr_ref expr_substitute_fcalls_with_free_vars(expr_ref const &e, list<pair<set<expr_ref, expr_compare>, pair<expr_ref, expr_ref>>> &fcall_map);
  //expr_ref expr_rename_stack_and_local_arguments(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref expr_mask_fcalls_memory_argument(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref expr_zero_out_function_argument_ptr_mem(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref expr_make_fcalls_semantic(expr_ref const &e, consts_struct_t const &cs);
  //expr_ref mask_input_stack_to_zero(vector<memlabel_t> const &relevant_memlabels, consts_struct_t const &cs);
  bool expr_contains_function_call(expr_ref const &e);
  bool expr_contains_unknown_function_call(expr_ref const &e);
  bool expr_contains_fcall_mem(expr_ref const &e);
  bool expr_contains_function_call_on_all_paths(expr_ref const &e);
  bool expr_get_function_call_nextpc(expr_ref const &e, expr_ref &ret);
  void expr_get_function_call_memlabel_writeable(expr_ref const &e, memlabel_t &ret, graph_memlabel_map_t const &memlabel_map);
  set<mlvarname_t> expr_get_mlvarnames_for_op_and_argnum(expr_ref const &e, expr::operation_kind op, int argnum);
  set<mlvarname_t> expr_get_read_mlvars(expr_ref const &e);
  set<mlvarname_t> expr_get_write_mlvars(expr_ref const &e);
  //expr_ref expr_mask_fcall_mem_argument_with_callee_readable_memlabel(expr_ref const &src);

  vector<expr_ref> expr_get_ite_leaves(expr_ref const &e);
  //expr_ref expr_convert_esp_to_esplocals(expr_ref const &e, memlabel_t const &ml_esplocals, graph_locals_map_t const &locals_map, consts_struct_t const &cs);

  string expr_to_string(expr_ref const &e) const;
  //string expr_to_string_dot() const;
  //void to_dot() const;
  string expr_to_string_table(expr_ref const &e, bool use_orig_ids = false) const;
  void expr_to_stream(ostream& os, expr_ref const &e, bool use_orig_ids = false) const;
  map<expr_id_t,expr_ref> expr_to_expr_map(expr_ref const &t) const;
  //string expr_to_string_node_dot() const;
  //void relevant_memlabels_to_stream(ostream& os, vector<memlabel_ref> const& relevant_memlabels) const;
  vector<memlabel_ref> relevant_memlabels_from_stream(istream& is) const;

  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> expr_get_nextpc_args_memlabel_map(expr_ref const &e);
  expr_ref expr_add_nextpc_args(expr_ref const &e, vector<expr_ref> const& dst_fcall_args);
  //void expr_set_fcall_read_write_memlabels_to_bottom(expr_ref const &e, graph_memlabel_map_t &memlabel_map);

  void expr_get_callee_memlabels(expr_ref const &e, graph_memlabel_map_t const &memlabel_map, set<memlabel_t> &callee_readable_memlabels, set<memlabel_t> &callee_writeable_memlabels);
  void expr_update_callee_memlabels(expr_ref const &e, graph_memlabel_map_t &memlabel_map, memlabel_t const &callee_readable_memlabel, memlabel_t const &callee_writeable_memlabel);

  //expr_ref expr_replace_array_constants_with_memvars(expr_ref const &e);
  expr_ref expr_eliminate_constructs_that_the_solver_cannot_handle(expr_ref const &e);

  bool expr_is_provable_using_lhs_expr_with_solver(expr_ref const& lhs_expr, expr_ref const& e, string const& comment, relevant_memlabels_t const& relevant_memlabels);
  bool check_is_overlapping_using_lhs_expr_with_solver(expr_ref const& lhs_expr, expr_ref const& a_begin, expr_ref const& a_size, expr_ref const& b_begin, expr_ref const& b_size, relevant_memlabels_t const& relevant_memlabels);

  sort_ref mk_mem_data_sort();
  sort_ref mk_mem_alloc_sort();

  expr_ref get_memvar_for_array_const(expr_ref const &e);
  expr_ref get_memzero_memalloc_var(sort_ref const& s);

  expr_ref expr_set_fcall_orig_memvar_to_array_constant_and_memlabels_to_heap(expr_ref const &e);
  bool expr_contains_only_sprel_compatible_ops(expr_ref const &e);
  //static string expr_var_is_tfg_argument(expr_ref const &e, map<string_ref, expr_ref> const &argument_regs);
  set<string> expr_contains_tfg_argument(expr_ref const &e, graph_arg_regs_t const &argument_regs);
  set<string> expr_contains_tfg_argument_address(expr_ref const &e, graph_arg_regs_t const &argument_regs);
  set<nextpc_id_t> expr_contains_nextpc_const(expr_ref const &e);
  expr_ref expr_replace_nextpc_consts_with_new_nextpc_const(expr_ref const &e, expr_ref const &new_nextpc_const, bool &found_nextpc_const);

  expr_ref expr_substitute_using_sprel_map_pair(expr_ref const &in, sprel_map_pair_t const &sprel_map_pair);
  expr_ref expr_substitute_using_sprel_map(expr_ref const &in, sprel_map_t const &sprel_map/*, map<graph_loc_id_t, expr_ref> const &locid2expr_map*/);

  expr_ref memlabel_var(string const &graph_name, int varnum);

  //expr_ref expr_resize_farg_expr_for_function_calling_conventions(expr_ref const &e);
  expr_ref get_fun_expr(vector<sort_ref> const &args_type, sort_ref ret_type);
  expr_ref get_local_ptr_expr_for_id(allocsite_t const& id, expr_ref const& loop_alloc_count_var, expr_ref const& mem_alloc, memlabel_t const& ml_local, expr_ref const& local_size);
  string_ref get_local_size_key_for_id(allocsite_t const& id, /*expr_ref const& loop_alloc_count_var,*/ string const& srcdst_keyword);
  expr_ref get_local_size_expr_for_id(allocsite_t const& id, /*expr_ref const& loop_alloc_count_var,*/ sort_ref const& local_size_sort, string const& srcdst_keyword);
  //expr_ref get_local_alloc_count_var(string const& srcdst_keyword);
  int get_nonce_byte_for_local_alloc_count(int local_alloc_count);
  //expr_ref get_uninit_nonce_expr_for_local_id(local_id_t id, string const& srcdst_keyword);
  //static bool expr_is_arg(map<string_ref, expr_ref> const &argument_regs, expr_ref const &e);
  static int argname_get_argnum(string const &argname);

  bool expr_assign_random_consts_to_vars(expr_ref &e, counter_example_t const &ce, counter_example_t &rand_vals);

  context_cache_t *m_cache;

  expr_ref create_disjunction_clause(vector<vector<expr_ref>> const& const_values, vector<expr_ref> const& expr_vector);

  expr_ref expr_add_unknown_arg_to_fcalls(expr_ref const &e);
  bool expr_has_stack_and_nonstack_memlabels_occuring_together(expr_ref const &e);
  expr_ref expr_replace_input_memvars_with_new_memvar(expr_ref const &e, set<expr_ref> const &input_memvars, expr_ref const &new_memvar);

  //map<expr_id_t,pair<expr_ref,expr_ref>> const &get_memlabel_range_submap() const { return m_ml_range_submap; }
  //void set_memlabel_range_submap(map<expr_id_t,pair<expr_ref,expr_ref>> const& ml_submap) { m_ml_range_submap = ml_submap; }
  //string memlabel_range_submap_to_string() const;
  static memlabel_t get_memlabel_and_bound_type_from_bound_name(string_ref const &bound_name, bound_type_t &bound_type);

  set<expr_ref> expr_get_base_mem_vars(expr_ref const &e);

  void submap_from_stream(istream &in, map<expr_id_t, pair<expr_ref, expr_ref>>& submap);
  void submap_to_stream(ostream& os, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap) const;

  //string read_range_submap(istream &in);
  //expr_ref submap_generate_assertions(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap);
  static bool expr_id_compare(expr_ref const &a, expr_ref const &b) { return a->get_id() < b->get_id(); }

  static bool is_llvmvarname(string const &s) {
    string expected_prefix = G_LLVM_PREFIX "-";
    //return string_has_prefix(s, expected_prefix);
    return s.find(expected_prefix) != string::npos;
  }

  static bool is_fparith_op(expr::operation_kind op);
  static bool is_local_var(string const &s);
  //static bool is_llvmvar(string const &s) {
  //  string expected_prefix = G_INPUT_KEYWORD "." G_LLVM_PREFIX "-%";
  //  //return string_has_prefix(s, expected_prefix);
  //  return s.find(expected_prefix) != string::npos;
  //}

  static bool is_dst_llvmvar(string const &s) {
    string expected_prefix = G_INPUT_KEYWORD "." G_DST_KEYWORD "." G_LLVM_PREFIX "-%";
    //return string_has_prefix(s, expected_prefix);
    return s.find(expected_prefix) != string::npos;
  }

  //static bool varname_has_clonedpc_suffix(string const &s) {
  //  //return string_has_prefix(s, expected_prefix);
  //  size_t cloned_pc_idx = s.find(CLONED_PC_PREFIX);
  //  if (cloned_pc_idx == string::npos)
  //    return false;
  //  string s1 = s.substr(cloned_pc_idx);
  //  // if a variable name has word cloned, check for '.' also
  //  if (s1.find(".") == string::npos)
  //    return false;
  //  return true;
  //}

  static bool is_src_var(string const &s) {
    string expected_prefix = G_INPUT_KEYWORD "." G_SRC_KEYWORD ".";
    //return string_has_prefix(s, expected_prefix);
    return (s.find(expected_prefix) != string::npos);
  }

  static bool is_dst_var(string const &s) {
    string expected_prefix = G_INPUT_KEYWORD "." G_DST_KEYWORD ".";
    //return string_has_prefix(s, expected_prefix);
    return (s.find(expected_prefix) != string::npos);
  }

  static bool is_src_dst_var(string const &s) {
    return (is_src_var(s) || is_dst_var(s));
  }

  static bool is_src_llvmvar(string const &s) {
    string expected_prefix = G_INPUT_KEYWORD "." G_SRC_KEYWORD "." G_LLVM_PREFIX "-%";
    //return string_has_prefix(s, expected_prefix);
    return s.find(expected_prefix) != string::npos;
  }

  static void expr_vector_to_stream(ostream& os, string const& prefix, expr_vector const& ev);
  vector<expr_ref> expr_vector_from_stream(istream& is, string const& prefix);

  static string proof_status_to_string(proof_status_t ps);
  expr_ref expr_rename_rodata_symbols_to_their_section_names_and_offsets(expr_ref const& e, struct input_exec_t const* in, symbol_map_t const& symbol_map);
  set<symbol_or_section_id_t> expr_get_used_symbols_and_sections(expr_ref const& e);
  expr_ref rename_vars_using_string_submap(expr_ref const& e, map<string_ref, string_ref> const& string_submap);

  expr_ref expr_sink_memmasks_to_bottom(expr_ref const& e);
  expr_ref expr_eliminate_allocas(expr_ref const& e);

  expr_ref submap_get_expr(map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);
  expr_ref expr_convert_memlabels_to_bitvectors(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>>& submap);
  set<memlabel_t> expr_identify_all_memlabels(expr_ref const& e);
  void expr_identify_rodata_accesses(expr_ref const& e, graph_symbol_map_t const& symbol_map, map<rodata_offset_t, size_t>& rodata_accesses);
  string expr_xml_print(expr_ref const& e, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, context::xml_output_format_t xml_output_format);
  string counter_example_xml_print(counter_example_t const& ce, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map, context::xml_output_format_t xml_output_format, string const& prefix = "");
  expr_ref get_riprel();
  bool expr_represents_alloca_ptr_fn(expr_ref const& e) const;
  bool expr_represents_alloca_size_fn(expr_ref const& e) const;

  string get_next_ce_name(string const& prefix);
  long get_next_memslot_varnum();

  static void expr_visitor_previsit_mem_only(expr_ref const &e, int interesting_args[], int &num_interesting_args);
  static void expr_visitor_previsit_mem_alloc_only(expr_ref const &e, int interesting_args[], int &num_interesting_args);

  void save_context_state_to_stream(ostream& out);
  void restore_context_state_from_stream(istream& in);

private:
  expr_ref mk_float_infinity_helper(int size, bool negative, expr_id_t suggested_id);
  expr_ref __expr_do_simplify_helper(expr_ref const &e, bool allow_const_specific_simplifications);
  sort_ref get_new_sort(expr::operation_kind kind, const vector<expr_ref>& args);

private:
  manager<expr> m_expr_manager;
  manager<constant_value> m_constant_value_manager;
  manager<array_constant> m_array_constant_manager;
  manager<expr_sort> m_sort_manager;
  free_id_list m_free_sort_ids;
  free_id_list m_free_expr_ids;
  free_id_list m_free_constant_value_ids;
  free_id_list m_free_array_constant_ids;
  char m_string_buffer[4096000];
  config m_config;
  consts_struct_t *m_cs;
  solver* m_solver;
  sage_interface* m_sage_iface;
  size_t m_num_proof_queries;
  size_t m_num_proof_queries_answered_by_syntactic_check;
  //size_t m_num_proof_queries_answered_by_counter_examples;
  axpred::AxPredContext* mAxPredContext;

  // saved/restored state
  long m_global_ce_count = 0;
  long m_global_memslot_varnum = 0;
};

expr_ref assembly_fcall_arg_on_stack(expr_ref const& mem, expr_ref const& mem_alloc, expr_ref const &stackpointer, size_t argnum/*, graph_memlabel_map_t &memlabel_map*/, string const &graph_name, long &max_memlabel_varnum);
//void update_symbol_rename_submap(map<expr_id_t, pair<expr_ref, expr_ref>> &rename_submap, size_t from, size_t to, src_ulong to_offset, context *ctx);

}
