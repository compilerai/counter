#pragma once
#include "support/types.h"
#include "stdlib.h"
#include <set>
#include <vector>
#include <string>
#include <map>
#include "support/src_tag.h"
#include "expr/defs.h"

using namespace std;

//namespace eqspace {
//class memlabel_assertions_t;
//}

struct imm_vt_map_t;

namespace eqspace {
class memlabel_obj_t;
class graph_symbol_map_t;
class graph_locals_map_t;
class consts_struct_t;

using memlabel_ref = shared_ptr<memlabel_obj_t const>;
//using std::string;

#define MAX_ALIAS_SET_SIZE 256

class alias_region_t {
public:
  enum alias_type {
    alias_type_symbol = 0,
    alias_type_local,
    alias_type_stack,
    alias_type_heap,
    alias_type_arg,
    alias_type_dummy
  };
private:
  alias_type m_alias_type;
  variable_id_t m_var_id;
  //variable_size_t m_size;
  variable_constness_t m_is_const;
public:
  alias_region_t() {}
  alias_region_t(alias_type atype, variable_id_t var_id/*, variable_size_t sz*/, variable_constness_t is_const) : m_alias_type(atype), m_var_id(var_id)/*, m_size(sz)*/, m_is_const(is_const)
  { }
  alias_type alias_region_get_alias_type() const { return m_alias_type; }
  variable_id_t alias_region_get_var_id() const { return m_var_id; }
  //variable_size_t alias_region_get_size() const { return m_size; }
  bool alias_region_is_const() const { return m_is_const; }
  bool operator<(alias_region_t const& other) const
  {
    if (m_alias_type != other.m_alias_type) {
      return m_alias_type < other.m_alias_type;
    }
    if (m_var_id != other.m_var_id) {
      return m_var_id < other.m_var_id;
    }
    //if (m_size != other.m_size) {
    //  return m_size < other.m_size;
    //}
    if (m_is_const != other.m_is_const) {
      return !m_is_const && other.m_is_const;
    }
    return false;
  }
  bool operator==(alias_region_t const& other) const
  {
    return !(*this < other || other < *this);
  }
};

class memlabel_t {
public:
  enum memlabel_type_t {
    //MEMLABEL_INVALID = 0,
    MEMLABEL_TOP = 0,
    MEMLABEL_MEM,
  };

private:
  memlabel_type_t m_type;
  set<alias_region_t> m_alias_set;
public:

  static void memlabel_init_using_keyword(memlabel_t *ml, char const *keyword, variable_size_t memsize);
  static void keyword_to_memlabel(memlabel_t *ml, char const *keyword, size_t memsize);
  static void memlabel_init(memlabel_t *ml);
  static void memlabel_init_to_top(memlabel_t *ml);
  static bool memlabels_equal(memlabel_t const *a, memlabel_t const *b);
  static bool memlabel_contains(memlabel_t const *haystack, memlabel_t const *needle);
  static bool memlabel_less(memlabel_t const *a, memlabel_t const *b);
  //static char *memlabel_to_string_c(memlabel_t const *a, char *buf, size_t size);
  static bool memlabel_is_top(memlabel_t const *ml);
  static bool memlabels_are_independent(memlabel_t const *a, memlabel_t const *b);
  static memlabel_t const memlabel_function_callee_accessible_const();
  static bool memlabel_is_atomic(memlabel_t const *ml);
  static bool memlabel_intersection_is_empty(memlabel_t const &ml1, memlabel_t const &ml2);
  static void memlabels_intersect(memlabel_t *dst, memlabel_t const *src);
  static memlabel_t memlabels_intersect(set<memlabel_t> const &mls);
  static void memlabels_union(memlabel_t *dst, memlabel_t const *src);
  static memlabel_t memlabel_union(vector<memlabel_ref> const &mls);
  static memlabel_t memlabel_union(set<memlabel_ref> const &mls);
  static void memlabel_from_int(memlabel_t *ml, int i);
  static void memlabel_copy(memlabel_t *dst, memlabel_t const *src);
  static memlabel_t memlabel_local(local_id_t local_id/*, variable_size_t size*/);
  static memlabel_t memlabel_symbol(symbol_id_t symbol_id/*, variable_size_t size*/, bool is_const);
  static memlabel_t memlabel_stack();
  //static memlabel_t memlabel_dummy();
  static memlabel_t memlabel_heap();
  static memlabel_t memlabel_top();
  static memlabel_t memlabel_arg(int argnum);
  static memlabel_t memlabel_dummy();
  static bool memlabel_contains_stack_or_local(memlabel_t const *ml);
  static bool memlabel_contains_stack(memlabel_t const *ml);
  static bool memlabel_is_stack(memlabel_t const *ml);
  static bool memlabel_contains_local(memlabel_t const *ml);
  //static bool memlabel_contains_local_id(memlabel_t const *ml, local_id_t local_id, variable_size_t *local_size);
  static bool memlabel_contains_heap(memlabel_t const *ml);
  static bool memlabel_contains_symbol(memlabel_t const *ml);
  static bool memlabel_represents_symbol(memlabel_t const *ml);
  static size_t memlabel_to_int_array(memlabel_t const *ml, int64_t arr[], size_t len);
  static bool memlabel_is_empty(memlabel_t const *ml);
  static char *memlabel_to_string(memlabel_t const *ml, char *buf, size_t size);
  static memlabel_t get_memlabel_for_addr_ref_ids(set<cs_addr_ref_id_t> const &addr_ref_ids, eqspace::consts_struct_t const &cs, eqspace::graph_symbol_map_t const &symbol_map, eqspace::graph_locals_map_t const &locals_map);
  static size_t memlabel_get_rodata_symbols(memlabel_t const &ml, symbol_id_t *rodata_symbol_ids, eqspace::graph_symbol_map_t const& symbol_map);
  //static void memlabel_remove_symbols(memlabel_t *ml, symbol_id_t const *rodata_symbol_ids, size_t num_rodata_symbol_ids);
  static memlabel_t memlabel_replace_sp_memlabel_with_locals(memlabel_t const *ml, memlabel_t const *ml_locals);
  static bool is_rodata_symbol_string(string const& s);
  static void get_atomic_memlabels(memlabel_t const &in, vector<memlabel_t> &out);
  //static bool memlabel_is_local_or_symbol(memlabel_t const &ml, tuple<eqspace::expr_ref, size_t, memlabel_t> &t, eqspace::consts_struct_t const &cs);
  //static bool memlabel_contains_only_symbols_or_locals(memlabel_t const *ml);
  static symbol_id_t memlabel_get_symbol_id(memlabel_t const *ml);
  static void memlabel_remove_local_id(memlabel_t *ml, local_id_t local_id);
  static bool memlabel_contains_arg(memlabel_t const *ml);
  //static memlabel_t memlabel_retain_only_locals(memlabel_t const *ml);
  static void memlabel_from_string(memlabel_t *ml, char const *buf);
  static void memlabel_init_to_memset(vector<memlabel_t> const &ml_mem_set, memlabel_t *ml);
  static memlabel_t memlabel_esplocals(eqspace::graph_locals_map_t const &locals_map);
  static set<memlabel_ref> eliminate_locals_and_stack(set<memlabel_ref> const &mls);
  static set<memlabel_ref> memlabel_set_eliminate_ro_symbols(set<memlabel_ref> const &mls);
  static alias_region_t::alias_type get_extra_type_using_regtype(reg_type rt);



  memlabel_type_t get_type() const { return m_type; }
  set<alias_region_t> const& get_alias_set() const { return m_alias_set; }
  size_t get_alias_set_size() const { return m_alias_set.size(); }
  string to_string() const;
  void memlabel_to_stream(ostream& os) const;
  void memlabel_from_stream(istream& is);
  static bool alias_set_contains_only_readonly_symbols(set<alias_region_t> const &alias_set);
  bool memlabel_contains_only_readonly_symbols() const;
  bool operator<(memlabel_t const &other) const;
  bool operator==(memlabel_t const &other) const;
  set<memlabel_ref> get_atomic_memlabels() const;
  void set_to_empty_mem() { m_type = MEMLABEL_MEM; m_alias_set.clear(); }
  int memlabel_is_arg() const;
  void memlabel_eliminate_args();
  void memlabel_eliminate_stack();
  //void memlabel_eliminate_readonly();
  bool memlabel_is_top() const;
  bool memlabel_is_stack() const;
  bool memlabel_is_heap() const;
  bool memlabel_is_local() const;
  bool memlabel_is_dummy() const;
  bool memlabel_is_empty() const;
  bool memlabel_is_atomic() const;
  bool memlabel_contains_stack() const;
  bool memlabel_contains_only_stack_or_local() const;
  bool memlabel_represents_symbol() const;
  //bool memlabel_get_size(variable_size_t& sz) const;
  bool memlabel_has_symbol(symbol_id_t const& sid) const;
  symbol_id_t memlabel_get_symbol_id() const;
  local_id_t memlabel_get_local_id() const;
  void memlabel_rename_variable_ids(alias_region_t::alias_type at, map<variable_id_t, variable_id_t> const &id_map);
  //bool memlabel_captures_all_nonstack_mls(eqspace::memlabel_assertions_t const& mlasserts/*vector<memlabel_ref> const &relevant_memlabels*/) const;
private:
  static int get_extra_id_using_regtype_and_index( reg_type rt, int rt_index);
  static bool memlabel_is_subset_of_memlabel(memlabel_t const *haystack, memlabel_t const *needle);
};

//void memlabel_set_to_bottom(memlabel_t *ml);
//bool memlabel_is_subset_of_memlabel(memlabel_t const *a, memlabel_t const *b);
//bool memlabel_is_independent_of_input_stack_pointer(memlabel_t const *ml);
//void memlabel_canonicalize_locals_symbols(memlabel_t *ml, struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
//void memlabels_minus(memlabel_t *dst, memlabel_t const *src);
//int memlabel_get_symbol_id(memlabel_t const *ml);
//bool memlabel_is_mem_set(memlabel_t const *ml);
//size_t memlabel_get_num_alias(memlabel_t const *ml);
//memlabel_t memlabel_dst_rodata();

memlabel_t get_memlabel_for_addr_containing_only_consts_struct_constants(expr_ref addr, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, set<cs_addr_ref_id_t> const &relevant_addr_refs, map<string, expr_ref> const &argument_regs, consts_struct_t const &cs/*, map<graph_loc_id_t, expr_ref> const &locid2expr_map, sprel_map_t const *sprel_map*/);

}
