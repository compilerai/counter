#pragma once

#include "expr/allocsite.h"

namespace eqspace {

class fallocsite_t {
private:
  string_ref m_function_name;
  allocsite_t m_allocsite;
public:
  fallocsite_t(string_ref const& function_name, allocsite_t const& allocsite)
    : m_function_name(function_name), m_allocsite(allocsite)
  { }

  string_ref const& get_function_name() const
  {
    return m_function_name;
  }

  allocsite_t const& get_allocsite() const
  {
    return m_allocsite;
  }

  bool operator<(fallocsite_t const& other) const
  {
    if (this->m_function_name != other.m_function_name) {
      return this->m_function_name->get_str() < other.m_function_name->get_str();
    }
    return this->m_allocsite < other.m_allocsite;
  }

  bool operator>(fallocsite_t const& other) const
  {
    if (this->m_function_name != other.m_function_name) {
      return this->m_function_name->get_str() > other.m_function_name->get_str();
    }
    return this->m_allocsite > other.m_allocsite;
  }

  bool operator==(fallocsite_t const& other) const
  {
    if (this->m_function_name != other.m_function_name) {
      return false;
    }
    return this->m_allocsite == other.m_allocsite;
  }
 
  bool operator!=(fallocsite_t const& other) const
  {
    return !(*this == other);
  }
};

}
