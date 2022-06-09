#pragma once

#include "expr/memlabel.h"

namespace eqspace {

class relevant_memlabels_t {
private:
  set<memlabel_ref> m_relevant_memlabels;
public:
  relevant_memlabels_t(set<memlabel_ref> const& mls);

  set<memlabel_ref> const& relevant_memlabels_get_ml_set() const { return m_relevant_memlabels; }
  bool relevant_memlabels_includes_stack() const;
  static bool relevant_memlabels_memlabel_is_preallocated(memlabel_t const& ml);
  bool empty() const;

  set<memlabel_ref> relevant_memlabels_get_ml_set_with_unique_allocsites() const;

  memlabel_t get_memlabel_all() const;
  //memlabel_t get_memlabel_all_except_stack() const;
  memlabel_t get_memlabel_all_except_args() const;

  memlabel_t get_memlabel_all_except_locals_and_stack_and_args() const;
  //memlabel_t relevant_memlabels_get_preallocated_locals() const;
  //memlabel_t get_memlabel_all_except_locals_and_stack() const;
  memlabel_t get_memlabel_locals_and_stack() const;
  set<memlabel_ref> get_preallocated_memlabels() const;
  memlabel_t memlabel_local_for_allocsite(string_ref const& fname, allocsite_t const& allocsite) const;

  set<memlabel_ref> get_memlabel_set_all_except_heap() const;
  set<memlabel_ref> get_memlabel_set_all_except_args() const;
  set<memlabel_ref> get_memlabel_set_all_except_heap_and_args() const;

  relevant_memlabels_t relevant_memlabels_eliminate_args() const;

  void relevant_memlabels_to_stream(ostream& os, string const& prefix) const;
  static relevant_memlabels_t relevant_memlabels_from_stream(istream& in, string const& prefix);
};

}
