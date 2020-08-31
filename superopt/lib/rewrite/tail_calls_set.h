#ifndef TAIL_CALLS_SET_H
#define TAIL_CALLS_SET_H

#include "support/types.h"

class tail_calls_set_t {
private:
  std::map<src_ulong, string> m_pcs; //map from pc to caller name
public:
  void tail_calls_set_add(src_ulong pc, string const &caller_name) { m_pcs.insert(make_pair(pc, caller_name)); } 
  bool tail_calls_set_belongs(src_ulong pc, string &caller_name) const
  {
    if (m_pcs.count(pc) != 0) {
      caller_name = m_pcs.at(pc);
      return true;
    } else {
      return false;
    }
  }
};

#endif
