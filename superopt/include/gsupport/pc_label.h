#pragma once

#include "expr/allocsite.h"
#include "gsupport/alloc_dealloc.h"

namespace eqspace {

enum class pc_label_type_t { OTHER, ALLOC, DEALLOC };

class pc_label_t
{
public:
  static pc_label_t other()
  {
    pc_label_t ret;
    ret.m_label = pc_label_type_t::OTHER;
    return ret;
  }
  static pc_label_t alloc(allocsite_t const& allocsite)
  {
    return pc_label_t(pc_label_type_t::ALLOC, allocsite);
  }
  static pc_label_t dealloc(allocsite_t const& allocsite)
  {
    return pc_label_t(pc_label_type_t::DEALLOC, allocsite);
  }
  bool is_other() const { return m_label == pc_label_type_t::OTHER; }
  bool is_alloc() const { return m_label == pc_label_type_t::ALLOC; }
  bool is_dealloc() const { return m_label == pc_label_type_t::DEALLOC; }
  pc_label_type_t get_label_type() const { return m_label; }
  allocsite_t get_allocsite() const
  {
    ASSERT(this->is_alloc() || this->is_dealloc());
    return m_allocsite;
  }
  bool operator<(pc_label_t const& other) const
  {
    return make_pair(m_label,m_allocsite) < make_pair(other.m_label,other.m_allocsite);
  }
  bool operator==(pc_label_t const& other) const
  {
    return    m_label == other.m_label
           && m_allocsite == other.m_allocsite;
  }
  friend ostream& operator<<(ostream& os, pc_label_t const& label);

  static pair<bool,pc_label_t> pc2label(pc const& p, map<pc,alloc_dealloc_t> const& pc2ad, pc const& other_pc);

private:
  pc_label_t() = default;
  pc_label_t(pc_label_type_t const& lab, allocsite_t const& allocsite)
    : m_label(lab),
      m_allocsite(allocsite)
  { ASSERT(lab == pc_label_type_t::ALLOC || lab == pc_label_type_t::DEALLOC); }

  pc_label_type_t m_label;
  allocsite_t m_allocsite;
};

ostream& operator<<(ostream& os, pc_label_t const& label);

class label_set_t
{
public:
  label_set_t(set<pc_label_t> const& init) : m_labels(init) {}

  set<pc_label_t> get_set() const { return m_labels; }

private:
  set<pc_label_t> m_labels;
};

}
