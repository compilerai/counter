#pragma once
#include "expr/memlabel.h"
#include "expr/expr_utils.h"
#include "support/string_ref.h"

namespace eqspace {

typedef enum graph_cp_location_type_t {
  GRAPH_CP_LOCATION_TYPE_MEMSLOT = 0, //the order of these values is important; they are in increasing order of location-size for the same reg_type, e.g., slot < masked < full_array
  GRAPH_CP_LOCATION_TYPE_MEMMASKED,
  GRAPH_CP_LOCATION_TYPE_REGMEM,
  //GRAPH_CP_LOCATION_TYPE_LLVMVAR,
  //GRAPH_CP_LOCATION_TYPE_IO,
} graph_cp_location_type_t;

typedef struct graph_cp_location
{
  reg_type m_reg_type;
  string_ref m_varname;
  expr_ref m_var;
  int m_reg_index_i, m_reg_index_j;
  graph_cp_location_type_t m_type;
  memlabel_ref m_memlabel;
  expr_ref m_addr;
  int m_nbytes;
  //string_ref m_memname;
  expr_ref m_memvar;
  expr_ref m_mem_allocvar;
  //bool m_is_bit_mode;
  //unsigned m_bit;
  bool m_bigendian;
  //unsigned m_local_nr;
  string to_string() const;
  string to_string_for_eq() const;
  bool equals(graph_cp_location const &other) const;
  bool operator==(graph_cp_location const& other) const { return this->equals(other); }
  bool is_memmask() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED; }
  //bool is_io() const { return m_type == GRAPH_CP_LOCATION_TYPE_IO; }
  bool is_memslot() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT; }
  //bool is_memslot_stack() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT && expr_contains_input_stack_pointer_const(m_addr); }
  bool is_memslot_symbol() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT && memlabel_t::memlabel_contains_symbol(&m_memlabel->get_ml()); }
  bool is_memslot_arg() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT && memlabel_t::memlabel_contains_arg(&m_memlabel->get_ml()); }
  bool is_memslot_pure_stack() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT && memlabel_t::memlabel_is_stack(&m_memlabel->get_ml()); }
  bool is_memslot_only_stack_locals() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT && memlabel_t::memlabel_contains_only_stack_or_local(&m_memlabel->get_ml()); }
  //bool is_reg() const { return m_type == GRAPH_CP_LOCATION_TYPE_REGMEM; }
  //bool is_var() const { return m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR; }
  bool is_memmask_local() const { return m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED && memlabel_t::memlabel_contains_local(&m_memlabel->get_ml()); }
  void graph_cp_location_convert_esp_to_esplocals(memlabel_t const &ml_esplocals)
  {
    //NOT_IMPLEMENTED();
    /*if (m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
      if (memlabel_is_stack(&m_memlabel)) {
        m_memlabel = ml_esplocals;
      }
    }*/
  }
  bool graph_cp_location_represents_hidden_var(context* ctx) const;
  bool graph_cp_location_represents_ghost_var(context* ctx) const;
  expr_ref graph_cp_location_get_value() const;
} graph_cp_location;
}
