#pragma once
#include <string>

class optflags_t
{
private:
  std::string m_cmdflags;
public:
  static optflags_t
  construct_optflags(bool vectorize_slp, bool omit_leaf_frame_pointer, bool O, bool O0, bool O1, bool O2, bool O3)
  {
    if (O0) {
      return optflags_t("-O0");
    } else if (O || O1) {
      return optflags_t("-O1");
    } else if (O2) {
      return optflags_t("-O2");
    } else if (O3) {
      return optflags_t("-O3");
    } else {
      return optflags_t("");
    }
  }
  std::string const& get_cmdflags() const { return m_cmdflags; }
private:
  optflags_t(std::string const& str) : m_cmdflags(str)
  { }
};
