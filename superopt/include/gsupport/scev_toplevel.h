#pragma once

#include "support/mybitset.h"
#include "support/debug.h"

#include "gsupport/scev.h"

namespace eqspace {

class scev_with_bounds_t {
private:
  scev_ref m_scev;
  mybitset m_unsigned_lb, m_unsigned_ub;
  mybitset m_signed_lb, m_signed_ub;
public:
  scev_with_bounds_t() { }
  scev_with_bounds_t(scev_ref const& scev, mybitset const& unsigned_lb, mybitset const& unsigned_ub, mybitset const& signed_lb, mybitset const& signed_ub)
    : m_scev(scev), m_unsigned_lb(unsigned_lb), m_unsigned_ub(unsigned_ub), m_signed_lb(signed_lb), m_signed_ub(signed_ub)
  { }
  scev_with_bounds_t(istream& is, string const& prefix);
  scev_ref const& get_scev() const { return m_scev; }
  mybitset const& get_unsigned_lb() const { return m_unsigned_lb; };
  mybitset const& get_unsigned_ub() const { return m_unsigned_ub; };
  mybitset const& get_signed_lb() const { return m_signed_lb; };
  mybitset const& get_signed_ub() const { return m_signed_ub; };

  void scev_with_bounds_to_stream(ostream& os, string const& prefix) const;
};

template<typename T_PC>
class scev_toplevel_t {
private:
  scev_with_bounds_t m_val_scevb;
  scev_with_bounds_t m_atuse_scevb;
  scev_ref m_atexit_scev;
  T_PC m_loop;
public:
  scev_toplevel_t(scev_with_bounds_t const& val_scevb, scev_with_bounds_t const& atuse_scevb, scev_ref const& atexit_scev, T_PC const& loop) : m_val_scevb(val_scevb), m_atuse_scevb(atuse_scevb), m_atexit_scev(atexit_scev), m_loop(loop)
  { }
  scev_toplevel_t(istream& is, string const& vname);
  scev_with_bounds_t const& get_val_scevb() const { return m_val_scevb; }
  scev_with_bounds_t const& get_atuse_scevb() const { return m_atuse_scevb; }
  scev_ref const& get_atexit_scev() const { return m_atexit_scev; }
  T_PC const& get_loop() const { return m_loop; }
  void scev_toplevel_to_stream(ostream& os, string const& valname) const;
};

}
