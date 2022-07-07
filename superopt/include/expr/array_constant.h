#pragma once

#include "support/debug.h"
#include "support/bv_const.h"

#include "expr/langtype.h"

namespace eqspace {
using namespace std;

class range_t;
class context;

class array_constant_string_t {
private:
  bool m_is_atomic;
  bool m_is_rac = false;
  string m_str;
  map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> m_array;
  map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> m_range_array;

public:
  array_constant_string_t(string const& str) : m_is_atomic(true), m_str(str)
  { }
  array_constant_string_t(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& m) : m_is_atomic(false), m_array(m)
  { }
  array_constant_string_t(vector<vector<shared_ptr<array_constant_string_t>>> const& ranges, shared_ptr<array_constant_string_t> const& rac_val) : m_is_atomic(false)
  {
    ASSERT(rac_val->is_atomic());
    for(auto const& range : ranges) {
      ASSERT(range.size() == 2);
      m_range_array.insert(make_pair(make_pair(range.front(),range.back()), rac_val));
    }
  }
  array_constant_string_t(vector<shared_ptr<array_constant_string_t>> const& range, shared_ptr<array_constant_string_t> const& val) : m_is_atomic(false)
  {
    ASSERT(range.size() == 2);
    ASSERT(val->is_atomic());
    m_range_array.insert(make_pair(make_pair(range.front(),range.back()), val));
  }
  array_constant_string_t(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& m, map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> range_array) : m_is_atomic(false), m_array(m), m_range_array(range_array)
  { ASSERT(!m.size() || !range_array.size());}
  array_constant_string_t(int m, int a) : m_is_atomic(true), m_is_rac(true)
  { m_str = string(ARRAY_CONSTANT_STRING_RAC_PREFIX ".m") + int_to_string(m) + ".a" + int_to_string(a); }

  bool is_atomic() const { return m_is_atomic; }
  bool is_rac() const { return m_is_rac; }
  string const& get_str() const { ASSERT(m_is_atomic); return m_str; }
  map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& get_array() const { ASSERT(!m_is_atomic); return m_array; }
  map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& get_range_array() const { ASSERT(!m_is_atomic); return m_range_array; }

  string to_string()
  {
    if (m_is_atomic)
      return m_str;
    ASSERT(!(m_array.size() && m_range_array.size()));
    stringstream ss;
    ss << "( ";
    for (auto const& mp : m_array) {
      ss << " ( ";
      for (auto const& vs : mp.first) {
        ss << vs->to_string() << ", ";
      }
      ss << " ) ";
      ss << "-> " << mp.second->to_string() << ", ";
    }
    for (auto const& mr : m_range_array) {
      ss << " [ " << mr.first.first->to_string() << " ; " << mr.first.second->to_string() << " ] ";
      ss << "-> " << mr.second->to_string() << ", ";
    }
    ss << " )";
    return ss.str();
  }

  // if comaparble domain array constant, it will have only range mappings; The atomic expression in the range mappings can be RAC
  // if non comparable domain ac, it will have only point mappings and a default value
  static array_constant_string_t union_array_constant_string(shared_ptr<array_constant_string_t> const& that, shared_ptr<array_constant_string_t> const& other)
  {
    ASSERT(!(that->is_atomic() && other->is_atomic()));

    map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> new_array;
    map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> new_range_array;
    if (that->is_atomic()) {
      ASSERT(!that->is_rac());
      new_array = other->get_array();
      ASSERT(!other->get_range_array().size());
      new_range_array = other->get_range_array();
      vector<shared_ptr<array_constant_string_t>> empty;
      shared_ptr<array_constant_string_t> atomic_item = that;
      new_array.insert(make_pair<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>>(move(empty), move(atomic_item)));
    } else if (other->is_atomic()) {
      ASSERT(!other->is_rac());
      new_array = that->get_array();
      ASSERT(!that->get_range_array().size());
      new_range_array = that->get_range_array();
      vector<shared_ptr<array_constant_string_t>> empty;
      shared_ptr<array_constant_string_t> atomic_item = other;
      new_array.insert(make_pair<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>>(move(empty), move(atomic_item)));
    } else {
      new_array = map_union(that->get_array(), other->get_array());
      new_range_array = map_union(that->get_range_array(), other->get_range_array());
    }
    ASSERT(!new_array.size() || !new_range_array.size());
    return array_constant_string_t(new_array, new_range_array);
  }
};

// Random array constnt class
// Takes a bv constant  multiplier and adder expr
// Domain of the AC should be of BV sort
class random_ac_default_value_t
{
  random_ac_default_value_t() { };
public:
  random_ac_default_value_t(expr_ref const& multiplier, expr_ref const& adder);
  expr_ref array_select_at_addr(expr_ref const& addr) const;
  mybitset mbs_array_select_at_addr(mybitset const& addr, unsigned addr_size) const;
  sort_ref get_sort() const;
  string to_string() const;
  bool operator==(random_ac_default_value_t const& other) const;
  bool operator!=(random_ac_default_value_t const& other) const
  {
    return !(*this == other);
  }
  context* get_context() const { return m_ctx; }
  expr_ref get_multiplier_value() const {return m_multiplier;}
  expr_ref get_adder_value() const {return m_adder;}
  bool is_empty() const { return (m_adder == nullptr); } 

  static random_ac_default_value_t empty_random_ac_default_value() { return random_ac_default_value_t(); }

private:
  expr_ref m_multiplier;
  expr_ref m_adder;
  context* m_ctx;
};

class array_constant;
using array_constant_ref = shared_ptr<array_constant const>;

using mbs_pair = pair<mybitset,mybitset>;

class array_constant
{
private:
  context *m_context;
  // Only for non-comparable domain sorts
  map<vector<expr_ref>, expr_ref> m_mapping; 
  expr_ref m_default;
  
  // Only for comparable domain sorts i.e. well-ordered domains
  map<pair<expr_ref,expr_ref>, expr_ref> m_range_mapping; 
  map<pair<expr_ref,expr_ref>, random_ac_default_value_t> m_rac_mapping;  // no singleton mappings here
  
  bool m_domain_is_comparable = true;
  vector<sort_ref> m_domain_sort;
  vector<pair<expr_ref, expr_ref>> m_hash_addr_data_mappings;
  array_constant_id_t m_id;

private:
  array_constant(context* ctx) : m_context(ctx), m_id(0)
  {
    stats_num_array_constant_constructions++;
  }
  array_constant(context* ctx, map<pair<expr_ref,expr_ref>, expr_ref> const& range_mapping, map<pair<expr_ref, expr_ref>, random_ac_default_value_t> const& rac_mappings);
  array_constant(context* ctx, sort_ref const& domain_sort, map<pair<expr_ref,expr_ref>, expr_ref> const& range_mapping, random_ac_default_value_t const& rac);
  array_constant(context* ctx, map<pair<expr_ref,expr_ref>, expr_ref> const& range_mapping);
  array_constant(context* ctx, sort_ref const& domain_sort, random_ac_default_value_t const& default_val);
  
  array_constant(context* ctx, vector<sort_ref> const& domain_sort, map<vector<expr_ref>, expr_ref> const &mapping, expr_ref const& default_val);
  array_constant(context* ctx, vector<sort_ref> const& domain_sort, expr_ref const& default_val);
  
  array_constant(context* ctx, array_constant const& other);


  bool is_similar_mem_AC(const array_constant& other) const;
  bool array_range_check_agreement_using_op(range_t const& target_range, function<bool(random_ac_default_value_t const&, range_t const&, expr_ref const& input_val)> agree) const;
  bool array_range_agree(range_t const& target_range, expr_ref const& target_val) const;
  bool array_range_disagree(range_t const& target_range, expr_ref const& target_val) const;

  expr_ref array_find_elem_in_range(vector<expr_ref> const& addr) const;

  unsigned get_bvsize() const;
  static bool expr_vector_sort_and_values_agree(vector<expr_ref> const &a, vector<expr_ref> const &b);

  expr_ref array_get_elem(vector<expr_ref> const &addr) const;
  expr_ref array_get_elem(expr_ref const& addr) const;
  pair<map<pair<expr_ref,expr_ref>, expr_ref>, map<pair<expr_ref,expr_ref>, random_ac_default_value_t>> get_masked_mappings(pair<expr_ref,expr_ref> const& range) const;
  bool has_compatible_addr_sort(array_constant const& other) const;
  bool has_compatible_sort(array_constant const& other) const;
  bool is_compose_compatible(array_constant_ref const& ac) const;
//  bool remove_addr_from_rac_mapping_if_val_equals(expr_ref const& addr, expr_ref const& val);
//  bool remove_addr_from_range_mappings_if_val_equals(expr_ref const& addr, expr_ref const& val);
//  void expand_the_RAC_range_from_constant_mappings();
  void simplify_mem_array_constant();
  void simplify_array_constant();
//  void convert_matching_range_mapping_boundary_elems_to_RAC(random_ac_default_value_t const& def_rac);
//  template<typename T_R> void merge_adjacent_mappings(map< pair<expr_ref, expr_ref>, T_R>& mappings);
//  void array_set_elem(vector<expr_ref> const &addr, expr_ref val);
  void populate_addr_data_mappings_for_hash();
  void mem_ac_get_mbs_mappings(map<mbs_pair, mybitset> &mbs_range_mapping, map<mbs_pair, random_ac_default_value_t> &mbs_rac_mapping) const;
  void mem_ac_set_using_mbs_mappings(map<mbs_pair, mybitset> const &mbs_range_mapping, map<mbs_pair, random_ac_default_value_t> const &mbs_rac_mapping);

public:

  friend class context;
  array_constant(array_constant const& other); //this needs to be public because manager uses the copy constructor inside `make_shared`
  ~array_constant();
  static random_ac_default_value_t array_constant_get_atomic_rac(string_ref const& str, sort_ref const& srt);
  array_constant_ref apply_mask_and_overlay_array_constant(pair<expr_ref,expr_ref> const& mask_range, array_constant_ref const& overlay) const;
  array_constant_ref get_masked_array_constant(vector<pair<expr_ref,expr_ref>> const& mask_ranges) const;
  void set_id_to_free_id(array_constant_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }
  string to_string() const;
  bool equals(const array_constant &other) const;
  bool operator==(array_constant const& other) const;
  bool operator!=(array_constant const& other) const;
  vector<pair<expr_ref, expr_ref>> const& get_addr_data_mappings_for_hash() const;
  expr_ref array_select(vector<expr_ref> const &addr, unsigned nbytes, bool bigendian) const;
  array_constant_ref array_store(vector<expr_ref> const &addr, expr_ref const& data, unsigned nbytes, bool bigendian) const;

  array_constant_ref reconcile_sorts_using_func(sort_ref const& ref_sort, function<expr_ref(expr_ref const&, sort_ref const& ref)> const& func) const;
  array_constant_ref convert_function_to_apply_on_memlabel_sort() const;
  array_constant_ref convert_alloca_ptr_function_to_apply_on_count_and_memlabel_sorts() const;

  static bool sorts_agree(sort_ref constant_sort, sort_ref ref_sort);

  bool agrees_with_sort(sort_ref s) const;
  array_constant_ref update_value_at_address(expr_ref const &start_addr, expr_ref const &data, int nbytes, bool bigendian) const;
  vector<pair<expr_ref,expr_ref>> addrs_where_mapped_value_equals(expr_ref const& val) const;
  array_constant_ref compose(array_constant_ref const& ac) const;
  vector<expr_ref> array_constant_get_struct_domain_vector() const;
  bool is_default_array_constant() const;
  bool is_non_rac_default_array_constant() const;
  expr_ref get_default_value() const;
  map<vector<expr_ref>,expr_ref> const& get_mapping() const { ASSERT(!m_domain_is_comparable); return m_mapping; }
  map<pair<expr_ref,expr_ref>, expr_ref> const& get_range_mapping() const { ASSERT(m_domain_is_comparable); return m_range_mapping; }
  map<pair<expr_ref,expr_ref>, random_ac_default_value_t> const& get_rac_mapping() const { ASSERT(m_domain_is_comparable); return m_rac_mapping; }
  bool const& is_domain_comparable() const { return m_domain_is_comparable; }
  random_ac_default_value_t get_rac_default_value() const;
  sort_ref get_result_sort() const;
  bool is_memory_array_constant() const;
  vector<sort_ref> get_domain_sort() const { return m_domain_sort;}

  bool array_constant_all_addrs_in_range_agree_with_value(expr_ref const& lb, expr_ref const& ub, expr_ref const& value) const;
  bool array_constant_all_addrs_in_range_agree_with_one_of_values(expr_ref const& lb, expr_ref const& ub, vector<expr_ref> const& values) const;
  bool array_constant_all_addrs_in_range_disagree_with_value(expr_ref const& lb, expr_ref const& ub, expr_ref const& value) const;
  bool array_constant_all_addrs_outside_range_disagree_with_value(expr_ref const& lb, expr_ref const& ub, expr_ref const& value) const;
  bool array_constant_all_addrs_in_range_agree_on_some_value(expr_ref const& lb_in, expr_ref const& ub_in) const;
  expr_ref array_constant_to_ite(expr_vector const& domvars) const;

  array_constant_ref array_constant_map_value(function<expr_ref(expr_ref const&)> f) const;
};


}
