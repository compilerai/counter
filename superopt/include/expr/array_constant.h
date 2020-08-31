#pragma once
#include <memory>
#include <map>
#include "support/debug.h"
#include "expr/langtype.h"
#include "support/bv_const.h"

namespace eqspace {
using namespace std;

class range_t;
class context;

class array_constant_string_t {
private:
  bool m_is_atomic;
  string m_str;
  map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> m_array;
  map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> m_range_array;

public:
  array_constant_string_t(string const& str) : m_is_atomic(true), m_str(str)
  { }
  array_constant_string_t(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& m) : m_is_atomic(false), m_array(m)
  { }
  array_constant_string_t(vector<shared_ptr<array_constant_string_t>> const& range, shared_ptr<array_constant_string_t> const& val) : m_is_atomic(false)
  {
    ASSERT(range.size() == 2);
    m_range_array.insert(make_pair(make_pair(range.front(),range.back()), val));
  }
  array_constant_string_t(map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& m, map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> range_array) : m_is_atomic(false), m_array(m), m_range_array(range_array)
  { }
  array_constant_string_t(int m, int a) : m_is_atomic(true)
  { m_str = string(ARRAY_CONSTANT_STRING_RAC_PREFIX ".m") + int_to_string(m) + ".a" + int_to_string(a); }

  bool is_atomic() const { return m_is_atomic; }
  string const& get_str() const { ASSERT(m_is_atomic); return m_str; }
  map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& get_array() const { ASSERT(!m_is_atomic); return m_array; }
  map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> const& get_range_array() const { ASSERT(!m_is_atomic); return m_range_array; }

  string to_string()
  {
    if (m_is_atomic)
      return m_str;
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

  static array_constant_string_t union_array_constant_string(shared_ptr<array_constant_string_t> const& that, shared_ptr<array_constant_string_t> const& other)
  {
    ASSERT(!(that->is_atomic() && other->is_atomic()));

    map<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> new_array;
    map<pair<shared_ptr<array_constant_string_t>,shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>> new_range_array;
    if (that->is_atomic()) {
      new_array = other->get_array();
      new_range_array = other->get_range_array();
      vector<shared_ptr<array_constant_string_t>> empty;
      shared_ptr<array_constant_string_t> atomic_item = that;
      new_array.insert(make_pair<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>>(move(empty), move(atomic_item)));
    } else if (other->is_atomic()) {
      new_array = that->get_array();
      new_range_array = that->get_range_array();
      vector<shared_ptr<array_constant_string_t>> empty;
      shared_ptr<array_constant_string_t> atomic_item = other;
      new_array.insert(make_pair<vector<shared_ptr<array_constant_string_t>>, shared_ptr<array_constant_string_t>>(move(empty), move(atomic_item)));
    } else {
      new_array = map_union(that->get_array(), other->get_array());
      new_range_array = map_union(that->get_range_array(), other->get_range_array());
    }
    return array_constant_string_t(new_array, new_range_array);
  }
};

class array_constant_default_value_t
{

public:

  array_constant_default_value_t(expr_ref const& default_val);
  array_constant_default_value_t(expr_ref const& multiplier, expr_ref const& adder);
  array_constant_default_value_t() {}
  expr_ref array_select_at_addr(vector<expr_ref> const& addr) const;
  expr_ref array_select_at_addr(expr_ref const& addr) const;
  sort_ref get_sort() const;
  string to_string() const;
  bool operator==(array_constant_default_value_t const& other) const;
  bool operator!=(array_constant_default_value_t const& other) const
  {
    return !(*this == other);
  }
  //bool array_constant_default_values_agree(array_constant_default_value_t const& other) const;
  array_constant_default_value_t dup_array_constant_default_value(context* nctx) const;
  context* get_context() const { return m_ctx; }
  bool is_RAC() const { return m_is_RAC;}
  expr_ref get_multiplier_value() const {return m_multiplier;}
  expr_ref get_adder_value() const {return m_adder;}
  expr_ref get_default_value() const {return m_default;}

private:
  expr_ref m_default;
  expr_ref m_multiplier;
  expr_ref m_adder;
  bool m_is_RAC = false;
  context* m_ctx;
};

class array_constant;
using array_constant_ref = shared_ptr<array_constant const>;

class array_constant
{
private:
  context *m_context;
  map<vector<expr_ref>, expr_ref> m_mapping;
  array_constant_default_value_t m_default;
  map<pair<expr_ref,expr_ref>, array_constant_default_value_t> m_range_mapping; // optional: for well-ordered domain

  array_constant_id_t m_id;

private:
  array_constant(context* ctx) : m_context(ctx), m_id(0)
  {
    stats_num_array_constant_constructions++;
  }
  array_constant(context* ctx, array_constant const& other) : m_context(ctx), m_mapping(other.m_mapping), m_default(other.m_default), m_range_mapping(other.m_range_mapping), m_id(0)
  {
    stats_num_array_constant_constructions++;
  }
  array_constant(context* ctx, map<vector<expr_ref>, expr_ref> const &mapping, map<pair<expr_ref,expr_ref>,array_constant_default_value_t> const& range_mapping, array_constant_default_value_t const& default_val);
  //array_constant(context* ctx, map<vector<expr_ref>, expr_ref> const &mapping, map<pair<expr_ref,expr_ref>,expr_ref> const& range_mapping, array_constant_default_value_t const& default_val);
  array_constant(context* ctx, map<vector<expr_ref>, expr_ref> const &mapping, expr_ref const& default_val);
  array_constant(context* ctx, map<vector<expr_ref>, expr_ref> const &mapping, array_constant_default_value_t const& default_val);
  array_constant(context* ctx, expr_ref const& default_val);
  array_constant(context* ctx, array_constant_default_value_t const& default_val);

private:
  static bool expr_vector_is_comparable(expr_vector const& evec);
  bool array_range_agree(pair<expr_ref,expr_ref> const& range, array_constant_default_value_t const& val) const;
  bool array_range_agree(range_t const& range, array_constant_default_value_t const& val) const;
  expr_ref array_find_elem_in_range(vector<expr_ref> const& addr) const;
  expr_ref array_get_elem(vector<expr_ref> const &addr) const;
  expr_ref mem_ac_get_elem(vector<expr_ref> const& addr) const;

  sort_ref get_result_sort() const;
  bool has_compatible_addr_sort(array_constant const& other) const;
  void simplify_array_constant();


public:

  array_constant(array_constant const& other); //this needs to be public because manager uses the copy constructor inside `make_shared`
  ~array_constant();
  static bool sorts_agree(sort_ref a, sort_ref b);
  static bool expr_constant_values_agree(vector<expr_ref> const &a, vector<expr_ref> const &b);

  static array_constant_default_value_t array_constant_get_atomic_expr_ref(string_ref const& str, sort_ref const& s);

  friend class context;

  void set_id_to_free_id(array_constant_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }

  bool operator==(array_constant const& other) const;

  bool range_mappings_differ(array_constant const& other) const;
  list<expr_ref> addr_mappings_differ(array_constant const& other) const;
  bool agrees_with_sort(sort_ref s) const;
  array_constant_ref convert_function_to_apply_on_memlabel_sort() const;
  array_constant_ref set_sort(sort_ref s) const;
  size_t num_unique_range_values() const;

  // for copy only -- used by parser
  array_constant_ref dup_array_constant(context* nctx) const;

  bool is_masked_array_constant_for_range(pair<expr_ref,expr_ref> const& range) const;
  array_constant_ref create_masked_array_constant(pair<expr_ref,expr_ref> const& range, bool default_range_required) const;
  array_constant_ref overlay_masked_array_constant(pair<expr_ref,expr_ref> const& range, array_constant const& overlay) const;
  //void overlay_heap(array_constant const& other, set<memlabel_ref> const& relevant_memlabels);
  array_constant_ref update_value_at_address(expr_ref const &addr, expr_ref const &data, int nbytes, bool bigendian) const;

  //bool operator==(const array_constant&) const;
  bool has_compatible_sort(array_constant const& other) const;
  //bool equals(const array_constant &other) const;

  array_constant_default_value_t const& get_default_value() const { return m_default; }
  map<vector<expr_ref>,expr_ref> const& get_mapping() const { return m_mapping; }
  map<pair<expr_ref,expr_ref>, array_constant_default_value_t> const& get_range_mapping() const { return m_range_mapping; }

  expr_ref array_select(vector<expr_ref> const &addr, unsigned nbytes, bool bigendian) const;
  array_constant_ref array_store(vector<expr_ref> const &addr, expr_ref data, unsigned nbytes, bool bigendian) const;
 // in-place store
  array_constant_ref array_set_elem(vector<expr_ref> const &addr, expr_ref data) const;

  string to_string() const;
  //void from_string() const;

  // memory array constant fast path
  bool is_memory_array_constant() const;
  bool memory_array_constant_equals(array_constant const& other) const;
};


}
