#ifndef EXPR_STACK_H
#define EXPR_STACK_H

#include "support/types.h"

class expr_stack {
public:
  expr_stack(void) {}
  void push(expr_ref e) { lstack.push(e); }
  expr_ref pop(void) { expr_ref ret = lstack.top(); lstack.pop(); return ret; }
  expr_ref top(void) const { return lstack.top(); }
  size_t size(void) const { return lstack.size(); }
private:
  stack<expr_ref> lstack;
};

#endif
