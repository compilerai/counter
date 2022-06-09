#pragma once

#include <memory>

#include "expr/memlabel.h"
#include "expr/memlabel_obj.h"
#include "expr/reachable_memlabels_map.h"
#include "expr/relevant_memlabels.h"

namespace eqspace {

using namespace std;

class callee_rw_memlabels_t {
private:
  memlabel_ref m_callee_readable, m_callee_writeable;
public:
  callee_rw_memlabels_t(memlabel_t const& callee_readable, memlabel_t const& callee_writeable);
  memlabel_t const& get_callee_readable() const { return m_callee_readable->get_ml(); }
  memlabel_t const& get_callee_writeable() const { return m_callee_writeable->get_ml(); }

  static callee_rw_memlabels_t get_readable_writeable_memlabels_for_function_name(string const& function_name, relevant_memlabels_t const& relevant_mls, vector<memlabel_t> const& farg_memlabels, reachable_memlabels_map_t const& reachable_mls);

  using get_callee_rw_memlabels_fn_t = std::function<callee_rw_memlabels_t (expr_ref const&, vector<memlabel_t> const&, reachable_memlabels_map_t const&)>;

  static get_callee_rw_memlabels_fn_t callee_rw_memlabels_nop();
  static bool function_name_represents_free(string const& fun_name);
  static bool function_name_represents_malloc(string const& fun_name);
  static bool function_name_represents_library_function_that_does_not_read_write_program_memory(string const& function_name);
};

}
