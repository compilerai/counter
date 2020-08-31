#ifndef FARG_TYPE_H
#define FARG_TYPE_H

#include <vector>
#include <string>
#include "expr/expr.h"

class farg_t {
private:
  int m_size;
public:
  farg_t(int sz) : m_size(sz)
  { }
  int get_size() const { return m_size; }
  int get_alignment() const { return DIV_ROUND_UP(m_size, BYTE_LEN); }
};

#endif
