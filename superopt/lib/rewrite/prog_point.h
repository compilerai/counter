#pragma once

class prog_point_t
{
public:
  typedef enum { IN, OUT } inout_t;
  prog_point_t() : m_pc(PC_UNDEFINED), m_inout(IN), m_outnum(0) {} //needed for alloc_elem_t()
  prog_point_t(src_ulong pc, inout_t inout, int outnum) : m_pc(pc), m_inout(inout), m_outnum(outnum)
  {
  }
  src_ulong get_pc() const { return m_pc; }
  inout_t get_inout() const { return m_inout; }
  int get_outnum() const { return m_outnum; }
  string to_string() const
  {
    std::stringstream ss;
    ss << "(" << std::hex << m_pc << std::dec << ", " << ((m_inout == IN) ? "IN" : "OUT") << ", " << m_outnum << ")";
    return ss.str();
  }
  static inout_t inout_from_string(string const &s)
  {
    if (s == "IN") {
      return IN;
    } else if (s == "OUT") {
      return OUT;
    } else {
      cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
      NOT_REACHED();
    }
  }
  static prog_point_t prog_point_from_string(string const &s)
  {
    size_t comma = s.find(',');
    ASSERT(comma != string::npos);
    ASSERT(s.at(0) == '(');
    string pc_str = s.substr(1, comma);
    //std::cout << __func__ << " " << __LINE__ << ": pc_str = " << pc_str << endl;
    //fflush(stdout);
    src_ulong pc = stol(pc_str, NULL, 16);
    size_t comma2 = s.find(',', comma + 1);
    ASSERT(comma2 != string::npos);
    string inout_str = s.substr(comma + 1, comma2 - comma - 1);
    inout_t inout = inout_from_string(inout_str);
    size_t rparen = s.find(')');
    ASSERT(rparen != string::npos);
    string outnum_str = s.substr(comma2 + 1, rparen - comma2 - 1);
    int outnum = stol(outnum_str);
    return prog_point_t(pc, inout, outnum);
  }
  bool operator<(prog_point_t const &other) const
  {
    if (this->m_pc < other.m_pc) {
      return true;
    } else if (other.m_pc < this->m_pc) {
      return false;
    }
    if (this->m_inout != other.m_inout) {
      return this->m_inout < other.m_inout;
    }
    return this->m_outnum < other.m_outnum;
  }
  bool operator==(prog_point_t const &other) const
  {
    return this->m_pc == other.m_pc && this->m_inout == other.m_inout && this->m_outnum == other.m_outnum;
  }
  bool prog_point_has_same_pc_with(prog_point_t const &other) const
  {
    return this->m_pc == other.m_pc;
  }
private:
  src_ulong m_pc;
  inout_t m_inout;
  int m_outnum;
};
