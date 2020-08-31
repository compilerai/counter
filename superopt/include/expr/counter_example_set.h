#pragma once
#include "expr/expr.h"
//#include "expr/counter_example.h"
#include <string>
#include <list>

namespace eqspace {
using namespace std;

class counter_example_t;
class counter_example_set_t
{
private:
  list<counter_example_t> m_ls;
  static int const max_elems = 1;

public:
  void push_front(counter_example_t const &ce);
  bool counter_example_exists(context *ctx, expr_ref e, counter_example_t &counter_example, consts_struct_t const &cs);
  void clear() { m_ls.clear(); }
  //string to_string() const;
};

}
