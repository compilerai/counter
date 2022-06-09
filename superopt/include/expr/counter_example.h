#pragma once
#include <string>
#include <list>
#include <map>

#include "support/string_ref.h"
#include "support/dshared_ptr.h"

#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/alignment_map.h"
#include "expr/counterexample_translate_retval.h"

namespace eqspace {
using namespace std;

class range_t;
class aliasing_constraints_t;
class expr_to_z3_expr;

class relevant_memlabels_t;

class counter_example_t //: public memstats_object_t
{
private:
  context* m_ctx;
  string_ref m_ce_name;
  map<string_ref, expr_ref> m_map;
  //map<expr_id_t, pair<expr_ref,expr_ref>> m_rodata_submap;
  mutable unsigned int m_random_seed;

  //cache
  mutable map<unsigned, pair<expr_ref, expr_ref>> m_submap;
  mutable bool m_submap_needs_to_be_recomputed;

private:
  map<unsigned, pair<expr_ref, expr_ref>> compute_submap() const;
  expr_ref gen_random_map(context *ctx, sort_ref s) const;

  void add_missing_constants_using_var_list(list<expr_ref> const &ls, consts_struct_t const &cs);
  //bool counter_example_is_well_formed() const;
  //vector<pair<expr_ref,expr_ref>> get_all_lb_and_ub_const_pairs_except_nml() const;
  //range_t generate_lb_and_ub_range_for_memlabel(context* ctx, memlabel_t const& ml, list<range_t>& avail_range);
  //range_t generate_lb_and_ub_range_for_memlabel(context* ctx, memlabel_t const& ml, list<range_t>& avail_range, unsigned alignment_factor);
  map<string_ref, expr_ref> generate_RAC_mappings_for_mem_expr() const;
  void reconcile_memslots_using_concrete_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &orig_submap, relevant_memlabels_t const& relevant_memlabels);

  expr_ref reconcile_array_or_function_sort(expr_ref const& constant_expr, sort_ref const &ref_sort, dshared_ptr<expr_to_z3_expr> const& z3_converter) const;
  pair<bool,expr_ref> reconcile_sorts(expr_ref const &constant_expr, sort_ref const &ref_sort, dshared_ptr<expr_to_z3_expr> const& z3_converter) const;
  expr_ref reconcile_mem_alloc_value(expr_ref const& mem_alloc_constant_expr, relevant_memlabels_t const& relevant_memlabels) const;
  void ce_reconcile_alloca_ptr(dshared_ptr<expr_to_z3_expr> const& z3_converter);

public:
  counter_example_t() = delete;
  counter_example_t(context *ctx, string const& ce_name);
  counter_example_t(counter_example_t const& other) : m_ctx(other.m_ctx), m_ce_name(other.m_ce_name), m_map(other.m_map), m_random_seed(other.m_random_seed), m_submap(other.m_submap), m_submap_needs_to_be_recomputed(other.m_submap_needs_to_be_recomputed)
  {
    stats_num_counter_example_constructions++;
  }

  counter_example_t ce_clone_and_generate_new_name() const;

  ~counter_example_t()
  {
    stats_num_counter_example_destructions++;
  }
  //virtual string classname() const override { return "counter_example_t"; }
  counterexample_translate_retval_t counter_example_translate_using_to_state_mapping_assigning_random_consts_as_needed(map<string_ref, expr_ref> const& to_state_mapping, counter_example_t& rand_vals, relevant_memlabels_t const& relevant_memlabels, context* ctx);

  context* get_context() const { return m_ctx; }
  string_ref get_ce_name() const { return m_ce_name; }
  bool counter_example_equals(counter_example_t const &other) const;
  bool is_empty() const;
  size_t size() const { return m_map.size(); }
  void ce_reconcile_sorts(expr_ref const& query_expr, dshared_ptr<expr_to_z3_expr> const& z3_converter);
  map<string_ref,expr_ref> const& get_mapping() const { return m_map; }
  void counter_example_union(counter_example_t const &other);
  void counter_example_map_difference(counter_example_t const& other);
  string counter_example_to_string() const;
  static counter_example_t counter_example_from_string(string const &s, context *ctx);
  map<unsigned, pair<expr_ref, expr_ref>> const &get_submap() const;
  bool counter_example_has_axiom_based_uifs() const;
  void ce_perform_memory_fuzzing(expr_ref unsat_expr, context* ctx, relevant_memlabels_t const& relevant_memlabels);

  void counter_example_to_stream(ostream& os, string const& comment = "") const;
  static counter_example_t counter_example_from_stream(istream& is, context* ctx);

  void counter_example_erase_value_for_vars(set<string_ref> const &vars)
  {
    for(auto const& var : vars) {
      if (m_map.count(var)) {
        m_map.erase(var);
        m_submap_needs_to_be_recomputed = true;
      }
    }
  }

  bool get_lb_and_ub_consts_for_memlabel(memlabel_t const& ml, expr_ref& lb_expr, expr_ref& ub_expr) const;

  //void set_rodata_submap(map<expr_id_t, pair<expr_ref,expr_ref>> const& rodata_submap);
  //map<expr_id_t, pair<expr_ref,expr_ref>> const &get_rodata_submap() const;

  unsigned long counter_example_get_hash_code() const;

  bool evaluate_expr_and_check_bounds(expr_ref const& input_expr, expr_ref &ret_expr, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels);
  bool evaluate_expr_assigning_random_consts_as_needed(expr_ref const& input_expr, expr_ref &ret_expr, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels);
  bool evaluate_expr_assigning_random_consts_and_check_bounds(expr_ref const& input_expr/*, counter_example_t &rand_vals*/, expr_ref &ret_expr, counter_example_t &rand_vals, relevant_memlabels_t const& relevant_memlabels, bool &is_outof_bounds);

  bool value_exists_for_varname(string_ref const &varname) const;
  expr_ref get_value_for_varname(string_ref const &varname) const;
  void add_varname_value_pair(string_ref const &varname, expr_ref const &value);
  void set_varname_value(string_ref const& varname, expr_ref const& value);
  void counter_example_rename_vars_using_string_submap(map<string_ref, string_ref> const& m);

  static unsigned int gen_fresh_random_seed();

  void reconciliate_stack_mem_in_counter_example(expr_ref const& stk_mem, expr_ref const& orig_dst_memvar);
  void reconcile_unaliased_memslots_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, relevant_memlabels_t const& relevant_memlabels);
  void reconcile_unaliased_bvextracts_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &orig_submap, relevant_memlabels_t const& relevant_memlabels);
  //void add_missing_memlabel_lb_and_ub_consts(context* ctx, /*aliasing_constraints_t const &alias_cons,*/vector<memlabel_ref> const& relevant_memlabels, alignment_map_t const& align_map);
  //void add_memlabel_lb_and_ub_consts_using_submap(context* ctx/*, vector<memlabel_ref> const& relevant_memlabels*/, map<expr_id_t,pair<expr_ref,expr_ref>> const& ml_range_submap);
  void add_input_memvars_using_nonstack_memvar(set<expr_ref> const &input_memvars, expr_ref const &nonstack_memvar);

  static void read_counter_examples_from_channel(int recv_channel, list<counter_example_t>& counter_examples, context* ctx);
  expr_ref gen_random_value_for_counter_example(context *ctx, sort_ref s) const;
  //bool add_mapping_for_memlabel(memlabel_t const& ml, expr_ref const& addr_begin, expr_ref const& addr_end, relevant_memlabels_t const& relevant_memlabels);

  static void counter_examples_to_stream(ostream& os, list<counter_example_t> const& ls);

private:
  counter_example_t(string const& ce_name, map<string_ref, expr_ref> const& ce_map, /*map<expr_id_t, pair<expr_ref,expr_ref>> const &rodata_submap, */unsigned int random_seed, context* ctx);
};
}
