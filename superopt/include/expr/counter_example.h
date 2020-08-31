#pragma once
#include "expr/expr.h"
//#include "expr/rodata_map.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/alignment_map.h"
#include <string>
#include <list>
#include <map>
#include "support/string_ref.h"
//#include "support/memstats_object.h"

namespace eqspace {
using namespace std;

class range_t;
class aliasing_constraints_t;

class counter_example_t //: public memstats_object_t
{
private:
  context* m_ctx;
  map<string_ref, expr_ref> m_map;
  map<expr_id_t, pair<expr_ref,expr_ref>> m_rodata_submap;
  local_sprel_expr_guesses_t m_local_sprel_expr_assumes;

  //cache
  mutable map<unsigned, pair<expr_ref, expr_ref>> m_submap;
  mutable bool m_submap_needs_to_be_recomputed;

private:
  map<unsigned, pair<expr_ref, expr_ref>> compute_submap() const;
  static expr_ref gen_random_map(context *ctx, sort_ref s);

  void add_missing_constants_using_var_list(list<expr_ref> const &ls, consts_struct_t const &cs);
  static pair<bool,expr_ref> reconcile_sorts(expr_ref const &constant_expr, expr_ref const &ref_expr, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap, consts_struct_t const &cs);
  //bool counter_example_is_well_formed() const;
  vector<pair<expr_ref,expr_ref>> get_all_lb_and_ub_const_pairs_except_nml() const;
  //range_t generate_lb_and_ub_range_for_memlabel(context* ctx, memlabel_t const& ml, list<range_t>& avail_range);
  //range_t generate_lb_and_ub_range_for_memlabel(context* ctx, memlabel_t const& ml, list<range_t>& avail_range, unsigned alignment_factor);

public:
  counter_example_t() = delete;
  counter_example_t(context *ctx);
  counter_example_t(map<string_ref, expr_ref> const& ce_map, map<expr_id_t, pair<expr_ref,expr_ref>> const &rodata_submap, context* ctx);
  counter_example_t(counter_example_t const& other) : m_ctx(other.m_ctx), m_map(other.m_map), m_rodata_submap(other.m_rodata_submap), m_local_sprel_expr_assumes(other.m_local_sprel_expr_assumes), m_submap(other.m_submap), m_submap_needs_to_be_recomputed(other.m_submap_needs_to_be_recomputed)
  {
    stats_num_counter_example_constructions++;
  }
  ~counter_example_t()
  {
    stats_num_counter_example_destructions++;
  }
  //virtual string classname() const override { return "counter_example_t"; }

  context* get_context() const { return m_ctx; }
  bool counter_example_equals(counter_example_t const &other) const;
  bool is_empty() const;
  size_t size() const { return m_map.size(); }
  void ce_reconcile_sorts(expr_ref query_expr, map<expr_id_t, pair<expr_ref, expr_ref>> const& submap);
  map<string_ref,expr_ref> const& get_mapping() const { return m_map; }
  void counter_example_union(counter_example_t const &other);
  string counter_example_to_string() const;
  void counter_example_from_string(string const &s, context *ctx);
  map<unsigned, pair<expr_ref, expr_ref>> const &get_submap() const;

  void counter_example_to_stream(ostream& os) const;
  void counter_example_from_stream(istream& is);

  bool get_lb_and_ub_consts_for_memlabel(memlabel_t const& ml, expr_ref& lb_expr, expr_ref& ub_expr) const;

  void set_rodata_submap(map<expr_id_t, pair<expr_ref,expr_ref>> const& rodata_submap);
  map<expr_id_t, pair<expr_ref,expr_ref>> const &get_rodata_submap() const;

  void counter_example_set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &local_sprels);
  local_sprel_expr_guesses_t const &counter_example_get_local_sprel_expr_assumes() const;

  bool evaluate_expr_and_check_bounds(expr_ref const& input_expr/*, counter_example_t &rand_vals*/, expr_ref &ret_expr, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels, bool check_bounds) const;
  bool evaluate_expr_assigning_random_consts_as_needed(expr_ref const& input_expr, expr_ref &ret_expr, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels, bool check_bounds = true) const;
  bool evaluate_expr_assigning_random_consts_and_check_bounds(expr_ref const& input_expr/*, counter_example_t &rand_vals*/, expr_ref &ret_expr, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels, bool &is_outof_bounds) const;

  bool value_exists_for_varname(string_ref const &varname) const;
  expr_ref get_value_for_varname(string_ref const &varname) const;
  void add_varname_value_pair(string_ref const &varname, expr_ref const &value);
  void set_varname_value(string_ref const& varname, expr_ref const& value);

  void reconciliate_stack_mem_in_counter_example();
  void reconcile_unaliased_memslots_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, set<memlabel_ref> const& relevant_memlabels);
  void reconcile_unaliased_bvextracts_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &orig_submap, set<memlabel_ref> const& relevant_memlabels);
  //void add_missing_memlabel_lb_and_ub_consts(context* ctx, /*aliasing_constraints_t const &alias_cons,*/vector<memlabel_ref> const& relevant_memlabels, alignment_map_t const& align_map);
  //void add_memlabel_lb_and_ub_consts_using_submap(context* ctx/*, vector<memlabel_ref> const& relevant_memlabels*/, map<expr_id_t,pair<expr_ref,expr_ref>> const& ml_range_submap);
  void add_input_memvars_using_nonstack_memvar(map<expr_id_t, expr_ref> const &input_memvars, expr_ref const &nonstack_memvar);

  static void read_counter_examples_from_channel(int recv_channel, list<counter_example_t>& counter_examples, context* ctx);
  static expr_ref gen_random_value_for_counter_example(context *ctx, sort_ref s);
};
}
