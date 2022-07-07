#pragma once

#include "support/types.h"
#include "support/utils.h"
#include "support/mybitset.h"
#include "support/manager.h"
#include "support/mytimer.h"

#include "expr/langtype.h"
#include "expr/memaccess_size.h"
#include "expr/memlabel.h"
#include "support/mybitset.h"
#include "expr/constant_value.h"
//#include "expr/container_type.h"

namespace eqspace {

class expr_sort// : public ref_object
{
public:
  enum kind
  {
    BV_SORT,
    FLOAT_SORT,
    FLOATX_SORT,
    BOOL_SORT,
    ARRAY_SORT,
    STRUCT_SORT,
    FUNCTION_SORT,
    INT_SORT,
    MEMLABEL_SORT,
    AXPRED_DESC_SORT,
    COMMENT_SORT,
    MEMACCESS_LANGTYPE_SORT,
    MEMACCESS_SIZE_SORT,
    MEMACCESS_TYPE_SORT,
    REGID_SORT,
    ROUNDING_MODE_SORT,
    COUNT_SORT,
    UNDEFINED_SORT,
    UNRESOLVED_SORT
  };
  expr_sort(kind, context*);
  expr_sort(kind, unsigned, context*);
  expr_sort(kind, vector<sort_ref> domain, sort_ref range, context*);
  expr_sort(kind, vector<sort_ref> domain, context*);
  ~expr_sort();

  bool operator==(expr_sort const &) const;
  string to_string() const;
  unsigned get_size() const;
  bool is_undefined_kind() const;
  bool is_unresolved_kind() const;
  bool is_bool_kind() const;
  bool is_bv_kind() const;
  bool is_float_kind() const;
  bool is_floatx_kind() const;
  bool sort_involves_float_kind() const;
  bool is_int_kind() const;
  bool is_array_kind() const;
  bool is_struct_kind() const;
  bool is_function_kind() const;
  bool is_language_level_function_kind() const;
  bool is_memlabel_kind() const;
  bool is_rounding_mode_kind() const;
  bool is_axpred_desc_kind() const;
  bool is_count_kind() const;
  //bool is_container_type_kind() const;
  bool is_comment_kind() const;
  bool is_langtype_kind() const;
  bool is_memaccess_size_kind() const;
  bool is_memaccess_type_kind() const;
  bool is_regid_kind() const;
  vector<sort_ref> const& get_domain_sort() const;
  sort_ref get_range_sort() const;
  void dec_ref_count();
  unsigned get_id() const;
  kind get_kind() const;
  size_t get_mybitset_size() const;
  //void clear_id_to_zero() { m_id = 0; }
  void set_id_to_free_id(expr_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }
  context *get_context() const { return m_context; }
  context* get_context() { return m_context; }
  bool sort_represents_alloca_ptr_fn_sort() const;
  bool sort_represents_alloca_size_fn_sort() const;
  bool sort_represents_mem_data_sort() const;
  bool sort_represents_mem_alloc_sort() const;

private:
  kind m_kind;
  unsigned m_size;
  vector<sort_ref> m_domain;
  sort_ref m_range;
  unsigned m_id;
  context* m_context;  // reference to parent
};

}

namespace std
{
using namespace eqspace;
template<>
struct hash<expr_sort>
{
  std::size_t operator()(expr_sort const &s) const
  {
    std::size_t seed = 0;
    myhash_combine<size_t>(seed, (size_t)s.get_kind());
    if (s.is_bv_kind()) {
      myhash_combine(seed, s.get_size());
    }
    if (s.is_array_kind() || s.is_function_kind()) {
      vector<sort_ref> const& dom = s.get_domain_sort();
      for (unsigned i = 0; i < dom.size(); ++i) {
        myhash_combine(seed, dom[i]->get_id());
      }
      myhash_combine(seed, s.get_range_sort()->get_id());
    }
    //cout << "hash<expr_sort>::" << __func__ << " " << __LINE__ << ": returning " << seed << " for " << s.to_string() << endl;
    return seed;
  }
};
}
