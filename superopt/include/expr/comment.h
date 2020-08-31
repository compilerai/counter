#pragma once
#include <string>
using std::string;
#include "support/consts.h"

namespace eqspace {

class comment_t {
private:
  int             m_val;
public:
  comment_t();
  //comment_t(string keyword);
  bool operator==(comment_t const &other) const;
  bool operator!=(comment_t const &other) const;
  bool operator<(comment_t const &other) const;
  bool operator>(comment_t const &other) const;
  string to_string() const;
  int to_int() const;
  void from_int(int i);
};

comment_t function_argument_comment(comment_t insn_id, int argnum);
comment_t comment_bottom(void);

//comment_t keyword_to_memlabel(string);
}
