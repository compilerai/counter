#pragma once

#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <unordered_set>
#include <stdlib.h>

#include "expr/langtype.h"
#include "support/types.h"
#include "expr/expr.h"
#include "expr/solver_interface.h"
#include "expr/counter_example.h"
#include "expr/counter_example_set.h"
#include "expr/memlabel_map.h"
#include "support/manager.h"
#include "expr/fsignature.h"

struct input_exec_t;
struct symbol_map_t;

namespace eqspace {
using namespace std;

class symbol_or_section_id_t;
class context_cache_t;
struct consts_struct_t;
//class callee_summary_t;
class sprel_map_t;
class sprel_map_pair_t;
class counter_example_t;
class state;
class mybitset;
class query_comment;
class sage_interface;
class rodata_offset_t;
class graph_arg_regs_t;

typedef enum { bound_type_lower_bound, bound_type_upper_bound } bound_type_t;
typedef enum { proof_status_proven, proof_status_disproven, proof_status_timeout } proof_status_t;

class context
{
public:
  class config
  {
  public:
    config(unsigned smt_timeout, unsigned sage_timeout)
    {
      smt_timeout_secs = smt_timeout;
      sage_timeout_secs = sage_timeout;
      //USE_SMT_QUERY = use_smt_query;
      //USE_Z3 = use_z3;
      //USE_YICES = use_yices;
      DISABLE_CACHES = false;
      use_only_live_src_vars = false;
      use_invsketch = false;
      sage_dedup_threshold = 20;
      warm_start_param = 2;
      solver_config = 0;
      max_lookahead = 0;
      m_enable_query_caches = true;
      disable_pernode_CE_cache = false;
      add_mod_exprs = false;
      enable_local_sprel_guesses = false;
      unroll_factor = 1;
      dst_unroll_factor = 1;
      disable_points_propagation = false;
      disable_inequality_inference= false;
      anchor_loop_tail = false;
      disable_query_decomposition = false;
      disable_residual_loop_unroll = false;
      disable_loop_path_exprs = false;
      consider_non_vars_for_dst_ineq = false;
      disable_epsilon_src_paths = false;
      disable_dst_bv_rank= false;
      disable_src_bv_rank = false;
      disable_propagation_based_pruning = false;
      disable_all_static_heuristics = false;
      choose_longest_delta_first = false;
      choose_shortest_path_first = false;
    }

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

    //bool USE_SMT_QUERY;
    //bool USE_YICES;
    //bool USE_Z3;
    bool DISABLE_CACHES;
    bool use_only_live_src_vars;
    bool use_invsketch;
    unsigned sage_dedup_threshold;
    unsigned warm_start_param;
    unsigned smt_timeout_secs;
    unsigned sage_timeout_secs;
    unsigned solver_config;
    unsigned max_lookahead;
    bool m_enable_query_caches;
    bool disable_pernode_CE_cache;
    bool add_mod_exprs;
    bool enable_local_sprel_guesses;
    unsigned unroll_factor;
    unsigned dst_unroll_factor;
    bool disable_points_propagation;
    bool disable_inequality_inference;
    bool anchor_loop_tail;
    bool consider_non_vars_for_dst_ineq;
    bool use_sage = false;
    bool disable_query_decomposition = false;
    bool disable_residual_loop_unroll = false;
    bool disable_loop_path_exprs = false;
    bool disable_epsilon_src_paths = false;
    bool disable_dst_bv_rank= false;
    bool disable_src_bv_rank = false;
    bool disable_propagation_based_pruning = false;
    bool disable_all_static_heuristics = false;
    bool choose_longest_delta_first = false;
    bool choose_shortest_path_first = false;
  };

  context(config const &cfg, bool no_solver=false);
  ~context();
  bool disable_caches() const { return m_config.disable_caches(); }
  void disable_expr_query_caches() { m_config.disable_expr_query_caches(); }
  bool expr_query_caches_are_disabled() const { return m_config.expr_query_caches_are_disabled(); }
  void clear_counter_examples();

  const config& get_config() const;
  void set_config(const config& cfg);

  consts_struct_t &get_consts_struct();

  bool timeout_occured();
  void clear_solver_cache();

  sort_ref mk_undefined_sort();
  sort_ref mk_bv_sort(unsigned size);
  sort_ref mk_bool_sort();
  sort_ref mk_array_sort(sort_ref const &domain, sort_ref const &range);
  sort_ref mk_array_sort(vector<sort_ref> const &domain, sort_ref const &range);
  //sort_ref mk_io_sort();
  sort_ref mk_int_sort();
  sort_ref mk_function_sort(vector<sort_ref> const &domain, sort_ref const &range);
  sort_ref mk_memlabel_sort();
  //sort_ref mk_comment_sort();
  sort_ref mk_langtype_sort();
  sort_ref mk_memaccess_size_sort();
  sort_ref mk_memaccess_type_sort();
  sort_ref mk_regid_sort();
  expr_ref mk_undefined(expr_id_t suggested_id = 0);
  expr_ref mk_var(string const &name, sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_var(string_ref const &name, sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_uninit_var(sort_ref const &, expr_id_t suggested_id = 0);
  expr_ref mk_function_decl(string const &name, const vector<sort_ref>& args, sort_ref const &ret, expr_id_t suggested_id = 0);

  sort_ref get_addr_sort();

  expr_ref mk_array_const_with_def(sort_ref const &, expr_ref const& def, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(array_constant_ref const &, sort_ref const &s, expr_id_t suggested_id = 0);
  //expr_ref mk_array_const(map<vector<string>, string> const &array_constant_map, sort_ref const &s, consts_struct_t const &cs, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const &array_constant_map, sort_ref const &s, expr_id_t suggested_id = 0);
  expr_ref mk_array_const(shared_ptr<array_constant_string_t> const &array_constant_str, sort_ref const &s, expr_id_t suggested_id = 0);


  //expr_ref mk_io_const(mybitset const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, int64_t value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, uint64_t value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, int value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, mybitset const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const(int size, mpz_class const &value, expr_id_t suggested_id = 0);
  expr_ref mk_bv_const_from_string(int size, string_ref const &value, expr_id_t suggested_id = 0);
  //expr_ref mk_io_const_from_string(string value, expr_id_t suggested_id = 0);
  expr_ref mk_int_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_regid_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_comment_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_langtype_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_int_const(int value, expr_id_t suggested_id = 0);
  expr_ref mk_memlabel_const(memlabel_t const &ml, expr_id_t suggested_id = 0);
  //expr_ref mk_memlabel_const_from_mybitset(mybitset const &mbs, expr_id_t suggested_id = 0);
  expr_ref mk_const_from_string(string_ref const &s, sort_ref const &sort, expr_id_t suggested_id = 0);
  //expr_ref mk_comment_const(comment_t c, expr_id_t suggested_id = 0);
  expr_ref mk_memaccess_size_const(memaccess_size_t memaccess_size, expr_id_t suggested_id = 0);
  expr_ref mk_memaccess_type_const(memaccess_type_t memaccess_type, expr_id_t suggested_id = 0);
  expr_ref mk_langtype_const(langtype_ref lt, expr_id_t suggested_id = 0);
  expr_ref mk_regid_const(int regid, expr_id_t suggested_id = 0);
  expr_ref mk_bool_true(expr_id_t suggested_id = 0);
  expr_ref mk_bool_false(expr_id_t suggested_id = 0);
  expr_ref mk_bool_const_from_string(string_ref const& value, expr_id_t suggested_id = 0);
  expr_ref mk_bool_const(bool value, expr_id_t suggested_id = 0);
  expr_ref mk_zerobv(int size, expr_id_t suggested_id = 0);
  expr_ref mk_onebv(int size, expr_id_t suggested_id = 0);
  expr_ref mk_minusonebv(int size, expr_id_t suggested_id = 0);

  expr_ref mk_app(expr::operation_kind kind, const expr_vector& args, expr_id_t suggested_id = 0);

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
  expr_ref mk_memmask(expr_ref a, memlabel_t const &ml/*, expr_ref addr, size_t memsize*/, expr_id_t suggested_id = 0);
  expr_ref mk_memmask_composite(expr_ref a, memlabel_t const &ml);
  expr_ref mk_memmasks_are_equal(expr_ref a, expr_ref b, memlabel_t const &ml, expr_id_t suggested_id = 0);
  //expr_ref mk_memsplice(expr_vector const &args, expr_id_t suggested_id = 0);
  //expr_ref mk_memjoin(expr_vector const &args);
  //expr_ref mk_memsplice(expr_ref a, expr_ref b);
  //expr_ref mk_memsplice_arg(expr_ref a, memlabel_t const &ml);
  expr_ref mk_alloca(expr_ref a, memlabel_t const &ml, expr_ref addr, expr_ref memsize, expr_id_t suggested_id = 0);

  expr_ref mk_bvmask(expr_ref a, unsigned masklen, expr_id_t suggested_id = 0);
  expr_ref mk_bvumask(expr_ref a, unsigned masklen, expr_id_t suggested_id = 0);

  //expr_ref mk_select(expr_ref arr, expr_ref addr, unsigned count);
  //expr_ref mk_store(expr_ref arr, expr_ref addr, expr_ref data, unsigned count);
  expr_ref mk_select(expr_ref const &arr, memlabel_t const &memlabel, expr_ref const &addr, unsigned count, bool bigendian,
      /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_select(expr_ref const &arr, mlvarname_t const &mlvarname, expr_ref const &addr, unsigned count, bool bigendian,
      /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_store(expr_ref const &arr, memlabel_t const &memlabel, expr_ref const &addr, expr_ref const &data, unsigned count,
      bool bigendian, /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_store(expr_ref const &arr, mlvarname_t const &mlvarname, expr_ref const &addr, expr_ref const &data, unsigned count,
      bool bigendian, /*comment_t comment, */expr_id_t suggested_id = 0);
  expr_ref mk_is_memaccess_langtype(expr_ref const& e, memaccess_size_t memaccess_size, langtype_ref const& lt, memaccess_type_t memaccess_type, expr_id_t suggested_id = 0);
  expr_ref mk_islangaligned(expr_ref e, size_t align, expr_id_t suggested_id = 0);
  expr_ref mk_isshiftcount(expr_ref e, size_t shifted_val_size, expr_id_t suggested_id = 0);
  //expr_ref mk_isgepoffset(expr_ref e, size_t gepoffset, expr_id_t suggested_id = 0);
  expr_ref mk_isgepoffset(expr_ref const &e, expr_ref const &gepoffset, expr_id_t suggested_id = 0);
  expr_ref mk_isindexforsize(expr_ref const &e, uint64_t size, expr_id_t suggested_id = 0);

  expr_ref mk_ismemlabel(expr_ref e, unsigned count, memlabel_t const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_ismemlabel(expr_ref e, unsigned count, memlabel_ref const &memlabel, expr_id_t suggested_id = 0);
  expr_ref mk_bool_to_bv(expr_ref const &e, expr_id_t suggested_id = 0);
  //expr_ref mk_switchmemlabel(expr_ref e, vector<memlabel_t> const &memlabels, vector<expr_ref> const &leaves);

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
  expr_ref expr_rename_memlabels(expr_ref const &in, alias_region_t::alias_type at, map<variable_id_t, variable_id_t> const &m);
  expr_ref expr_rename_memlabel_vars_prefix(expr_ref const &in, string const &from_prefix, string const &to_prefix);

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

  array_constant_ref mk_array_constant();
  array_constant_ref mk_array_constant(map<vector<expr_ref>, expr_ref> const &mapping, map<pair<expr_ref,expr_ref>,array_constant_default_value_t> const& range_mapping, array_constant_default_value_t const& default_val);
  array_constant_ref mk_array_constant(map<vector<expr_ref>, expr_ref> const &mapping, map<pair<expr_ref,expr_ref>,expr_ref> const& range_mapping, array_constant_default_value_t const& default_val);
  array_constant_ref mk_array_constant(map<vector<expr_ref>, expr_ref> const &mapping, expr_ref const& default_val);
  array_constant_ref mk_array_constant(map<vector<expr_ref>, expr_ref> const &mapping, array_constant_default_value_t const& default_val);
  array_constant_ref mk_array_constant(expr_ref const& default_val);
  array_constant_ref mk_array_constant(array_constant_default_value_t const& default_val);
  array_constant_ref mk_array_constant(array_constant const& ac);

  expr_ref get_input_expr_for_key(string_ref key, sort_ref const& s);
  bool expr_is_input_expr(expr_ref const& e) const;
  bool key_represents_hidden_var(string_ref const& s) const;
  bool key_represents_exreg(string_ref const& sr, exreg_group_id_t& exreg_group, exreg_id_t& exreg_id) const;

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
  string stat() const;
  //expr_map_cache& get_expr_simplify_cache() { return m_simplify_cache; }
  void clear_cache(void);
  void gen_sym_exec_reader_file_expr_table(FILE *ofp);
  //manager<expr> const *get_expr_manager_ptr() { return &m_expr_manager; }
  //manager<expr_sort> const *get_sort_manager_ptr() { return &m_sort_manager; }
  //bool expr_set_lhs_prove_rhs(set<pair<expr_ref, pair<expr_ref, expr_ref>>/*, expr_triple_compare*/> const &lhs, expr_ref rhs, query_comment const &comment, bool timeout_res, counter_example_t &counter_example);

  bool expr_is_less_than(expr_ref a, expr_ref b);

  //void add_counter_example(counter_example_t const &counter_example);
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

  bool context_counter_example_exists(expr_ref e, counter_example_t &counter_example, consts_struct_t const &cs);
  void context_add_counter_example(counter_example_t const &ce);
  size_t get_num_proof_queries() const;
  void increment_num_proof_queries();
  void increment_num_proof_queries_answered_by_syntactic_check();
  void increment_num_proof_queries_answered_by_counter_examples();

  //expr_ref expr_simplify_recursive(expr_ref const &e, long depth, consts_struct_t const &cs);
  expr_ref expr_do_simplify(expr_ref const &e);
  expr_ref expr_do_simplify_no_const_specific(expr_ref const &e);
  //expr_ref expr_replace_sp_memlabel_with_locals(expr_ref const &e, memlabel_t const &ml, consts_struct_t const &cs);

  expr_ref expr_substitute(expr_ref const &e, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap);
  expr_ref expr_substitute_using_var_to_expr_map(expr_ref const &e, map<string, expr_ref> const &var2expr);
  //expr_ref expr_substitute_using_counter_example(expr_ref const &e, counter_example_t const &counter_example);
  //expr_ref substitute_using_lhs_set(set<predicate> const &lhs_set, consts_struct_t const &cs, context *ctx);
  expr_ref expr_replace_memmask_and_memsplice_operators_using_counter_example(expr_ref const &e/*, counter_example_t &counter_example*/, consts_struct_t const &cs);
  expr_ref expr_eliminate_memlabel_esp(expr_ref const &e);
  //expr_ref substitute_till_fixpoint(map<unsigned, pair<expr_ref, expr_ref> > const &submap);
  //expr_ref specialize(expr_ref with, query_comment const &qc);
  expr_list expr_get_vars(expr_ref const &e);
  bool expr_contains_expr(expr_ref const &e, expr_ref const &var);
  bool expr_contains_memlabel_top(expr_ref const &e);
  //bool expr_contains_input_memory_constant_expr(expr_ref const &e, consts_struct_t const &cs);
  set<expr_ref, expr_compare> expr_get_varset(expr_ref const &e);
  expr_list expr_get_subexprs(expr_ref const &e, vector<memlabel_t> const &relevant_memlabels);
  expr_ref expr_model_stack_as_separate_mem(expr_ref const& e);
  expr_ref expr_try_converting_unaliased_memslots_to_fresh_vars(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>> &submap);
  expr_ref expr_try_breaking_bvextracts_to_fresh_vars(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>> &submap);
  solver::solver_res expr_is_satisfiable(expr_ref const &e, query_comment const &comment, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_examples);
  solver::solver_res expr_is_provable(expr_ref const &e, query_comment const &comment, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_examples);
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
  bool expr_contains_fcall_mem(expr_ref const &e);
  bool expr_contains_function_call_on_all_paths(expr_ref const &e);
  void expr_get_function_call_nextpc(expr_ref const &e, expr_ref &ret);
  void expr_get_function_call_memlabel_writeable(expr_ref const &e, memlabel_t &ret, graph_memlabel_map_t const &memlabel_map);
  bool expr_contains_only_consts_struct_constants_or_arguments_or_esp_versions(expr_ref const &e, graph_arg_regs_t const &argument_regs);
  set<mlvarname_t> expr_get_mlvarnames_for_op_and_argnum(expr_ref const &e, expr::operation_kind op, int argnum);
  set<mlvarname_t> expr_get_read_mlvars(expr_ref const &e);
  set<mlvarname_t> expr_get_write_mlvars(expr_ref const &e);
  //expr_ref expr_mask_fcall_mem_argument_with_callee_readable_memlabel(expr_ref const &src);

  vector<expr_ref> expr_get_ite_leaves(expr_ref const &e);
  expr_ref expr_convert_esp_to_esplocals(expr_ref const &e, memlabel_t const &ml_esplocals, graph_locals_map_t const &locals_map, consts_struct_t const &cs);

  string expr_to_string(expr_ref const &e) const;
  //string expr_to_string_dot() const;
  //void to_dot() const;
  string expr_to_string_table(expr_ref const &e, bool use_orig_ids = false) const;
  void expr_to_stream(ostream& os, expr_ref const &e, bool use_orig_ids = false) const;
  map<expr_id_t,expr_ref> expr_to_expr_map(expr_ref const &t) const;
  //string expr_to_string_node_dot() const;
  void relevant_memlabels_to_stream(ostream& os, vector<memlabel_ref> const& relevant_memlabels) const;
  vector<memlabel_ref> relevant_memlabels_from_stream(istream& is) const;

  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> expr_get_nextpc_args_memlabel_map(expr_ref const &e);
  expr_ref expr_add_nextpc_args(expr_ref const &e, vector<expr_ref> const& dst_fcall_args);
  //void expr_set_fcall_read_write_memlabels_to_bottom(expr_ref const &e, graph_memlabel_map_t &memlabel_map);

  void expr_get_callee_memlabels(expr_ref const &e, graph_memlabel_map_t const &memlabel_map, set<memlabel_t> &callee_readable_memlabels, set<memlabel_t> &callee_writeable_memlabels);
  void expr_update_callee_memlabels(expr_ref const &e, graph_memlabel_map_t &memlabel_map, memlabel_t const &callee_readable_memlabel, memlabel_t const &callee_writeable_memlabel);

  //expr_ref expr_replace_array_constants_with_memvars(expr_ref const &e);
  expr_ref expr_eliminate_constructs_that_the_solver_cannot_handle(expr_ref const &e);

  expr_ref get_memvar_for_array_const(expr_ref const &e);

  expr_ref expr_set_fcall_orig_memvar_to_array_constant_and_memlabels_to_heap(expr_ref const &e);
  bool expr_contains_only_sprel_compatible_ops(expr_ref const &e);
  //static string expr_var_is_tfg_argument(expr_ref const &e, map<string_ref, expr_ref> const &argument_regs);
  set<string> expr_contains_tfg_argument(expr_ref const &e, graph_arg_regs_t const &argument_regs);
  set<nextpc_id_t> expr_contains_nextpc_const(expr_ref const &e);
  expr_ref expr_replace_nextpc_consts_with_new_nextpc_const(expr_ref const &e, expr_ref const &new_nextpc_const, bool &found_nextpc_const);

  expr_ref expr_substitute_using_sprel_map_pair(expr_ref const &in, sprel_map_pair_t const &sprel_map_pair);
  expr_ref expr_substitute_using_sprel_map(expr_ref const &in, sprel_map_t const &sprel_map/*, map<graph_loc_id_t, expr_ref> const &locid2expr_map*/);

  expr_ref memlabel_var(string const &graph_name, int varnum);
  mlvarname_t memlabel_varname(string const &graph_name, int varnum);
  mlvarname_t memlabel_varname_for_fcall(string const &graph_name, int varnum);

  //expr_ref expr_resize_farg_expr_for_function_calling_conventions(expr_ref const &e);
  expr_ref get_fun_expr(vector<sort_ref> const &args_type, sort_ref ret_type);
  expr_ref get_local_size_expr_for_id(size_t id);
  //static bool expr_is_arg(map<string_ref, expr_ref> const &argument_regs, expr_ref const &e);
  static int argname_get_argnum(string const &argname);

  bool expr_assign_random_consts_to_vars(expr_ref &e, counter_example_t const &ce, counter_example_t &rand_vals);

  context_cache_t *m_cache;

  expr_ref create_disjunction_clause(vector<vector<expr_ref>> const& const_values, vector<expr_ref> const& expr_vector);

  expr_ref expr_add_unknown_arg_to_fcalls(expr_ref const &e);
  bool expr_has_stack_and_nonstack_memlabels_occuring_together(expr_ref const &e);
  expr_ref expr_replace_input_memvars_with_nonstack_memvar(expr_ref const &e, map<expr_id_t, expr_ref> const &input_memvars, expr_ref const &nonstack_memvar);

  //map<expr_id_t,pair<expr_ref,expr_ref>> const &get_memlabel_range_submap() const { return m_ml_range_submap; }
  //void set_memlabel_range_submap(map<expr_id_t,pair<expr_ref,expr_ref>> const& ml_submap) { m_ml_range_submap = ml_submap; }
  //string memlabel_range_submap_to_string() const;
  static memlabel_t get_memlabel_and_bound_type_from_bound_name(string_ref const &bound_name, bound_type_t &bound_type);

  expr_ref expr_get_base_mem(expr_ref const &e);

  string submap_from_stream(istream &in, map<expr_id_t, pair<expr_ref, expr_ref>>& submap);
  void submap_to_stream(ostream& os, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap) const;

  //string read_range_submap(istream &in);
  //expr_ref submap_generate_assertions(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap);
  static bool expr_id_compare(expr_ref const &a, expr_ref const &b) { return a->get_id() < b->get_id(); }

  static bool is_llvmvarname(string const &s) {
    string expected_prefix = G_LLVM_PREFIX "-";
    //return string_has_prefix(s, expected_prefix);
    return s.find(expected_prefix) != string::npos;
  }

  static bool is_llvmvar(string const &s) {
    string expected_prefix = G_INPUT_KEYWORD "." G_LLVM_PREFIX "-%";
    //return string_has_prefix(s, expected_prefix);
    return s.find(expected_prefix) != string::npos;
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
  expr_ref submap_get_expr(map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);
  expr_ref expr_convert_memlabels_to_bitvectors(expr_ref const& e, map<expr_id_t, pair<expr_ref, expr_ref>>& submap);
  set<memlabel_t> expr_identify_all_memlabels(expr_ref const& e);
  void expr_identify_rodata_accesses(expr_ref const& e, graph_symbol_map_t const& symbol_map, map<rodata_offset_t, size_t>& rodata_accesses);

private:
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
  counter_example_set_t m_counter_examples;
  size_t m_num_proof_queries;
  size_t m_num_proof_queries_answered_by_syntactic_check;
  size_t m_num_proof_queries_answered_by_counter_examples;

  //map<expr_id_t,pair<expr_ref,expr_ref>> m_ml_range_submap;
};

expr_ref assembly_fcall_arg_on_stack(expr_ref mem, expr_ref const &stackpointer, size_t argnum/*, graph_memlabel_map_t &memlabel_map*/, string const &graph_name, long &max_memlabel_varnum);
//void update_symbol_rename_submap(map<expr_id_t, pair<expr_ref, expr_ref>> &rename_submap, size_t from, size_t to, src_ulong to_offset, context *ctx);

}
