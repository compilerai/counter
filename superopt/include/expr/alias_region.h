#pragma once
#include <set>
#include <vector>
#include <string>
#include <map>
#include <stdlib.h>

#include "support/types.h"
#include "support/src_tag.h"

#include "expr/defs.h"
#include "expr/allocstack.h"

namespace eqspace {

class alias_region_t {
public:
  enum alias_type {
    alias_type_symbol = 0,
    alias_type_local,
    alias_type_heap_alloc,
    alias_type_stack,
    alias_type_heap,
    alias_type_arg,
    alias_type_rodata,
    alias_type_dummy
  };
private:
  alias_type m_alias_type;
  variable_id_t m_var_id;
  allocstack_t m_allocstack;
  variable_constness_t m_is_const;
public:
  alias_region_t() {}
  alias_region_t(alias_type atype, variable_id_t var_id, allocstack_t const& allocstack, variable_constness_t is_const);
  alias_type alias_region_get_alias_type() const { return m_alias_type; }
  variable_id_t alias_region_get_var_id() const { return m_var_id; }
  allocstack_t const& alias_region_get_allocstack() const { return m_allocstack; }
  bool alias_region_is_const() const { return m_is_const; }
  bool operator<(alias_region_t const& other) const
  {
    if (m_alias_type != other.m_alias_type) {
      return m_alias_type < other.m_alias_type;
    }
    if (m_var_id != other.m_var_id) {
      return m_var_id < other.m_var_id;
    }
    if (m_allocstack != other.m_allocstack) {
      return m_allocstack < other.m_allocstack;
    }
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

}
