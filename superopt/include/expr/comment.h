#pragma once

using std::string;

#include "support/consts.h"
#include "support/string_ref.h"

namespace eqspace {

class comment_t {
private:
  //we expect a comment to be of the form AB1234... (two letters followed by a number less than 10000)
  string_ref   m_val;
public:
  comment_t() = delete;
  comment_t(string const& keyword);
  string_ref get_string_ref() const { return m_val; }
  bool operator==(comment_t const &other) const;
  bool operator!=(comment_t const &other) const;
  bool operator<(comment_t const &other) const;
  bool operator>(comment_t const &other) const;
  string to_string() const;
  unsigned comment_to_int() const;
  static comment_t comment_from_int(unsigned i);
};

//comment_t function_argument_comment(comment_t insn_id, int argnum);
//comment_t comment_bottom(void);

//comment_t keyword_to_memlabel(string);
}
