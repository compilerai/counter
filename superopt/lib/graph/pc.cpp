#include "gsupport/pc.h"
#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"

#include <string>
#include <sstream>

namespace eqspace {

char pc::pc_strbuf[MAX_PC_STRBUF_SIZE];
size_t pc::pc_strbuflen = 0;

string pc::pc_type_to_string(pc_type t) const
{
  switch(t)
  {
  case insn_label: return string("L");
  case exit: return string("E");
  default: NOT_REACHED();//return string();
  }
}

char const *
pc::pc_type_to_string_full(pc_type t) const
{
  switch(t)
  {
  case insn_label: return "insn_label";
  case exit: return "exit";
  default: NOT_REACHED();
  }
}

string pc::to_string() const
{
  stringstream ss;
  //ss << pc_type_to_string(m_type) << m_index << pc::pc_index_sep << m_bblnum << pc::pc_index_sep << m_subindex << pc::pc_index_sep << m_subsubindex;
  //cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": m_index = " << m_index << ", m_subindex = " << m_subindex << ", m_subsubindex = " << m_subsubindex << endl;
  //fflush(stdout);
  ss << pc_type_to_string(m_type) << m_index << pc::pc_index_sep << m_subindex << pc::pc_index_sep << m_subsubindex;
  return ss.str();
}

/*string pc::to_string_for_dot() const
{
  stringstream ss;
  ss << pc_type_to_string(m_type) << m_index << "_" << m_subindex << "_" << m_subsubindex;
  return ss.str();
}*/

//string pc::to_string_for_eq() const
//{
//  stringstream ss;
//  ss << pc_type_to_string(m_type) << m_index << pc::pc_index_sep << m_subindex << pc::pc_index_sep << m_subsubindex;
//  return ss.str();
//}

pc pc::create_from_string(const string& s)
{
  pc p;
  string str = s;
  trim(str);
  pc::pc_type pc_t;
  switch(str.at(0))
  {
  case 'L':
    pc_t = pc::insn_label;
    break;
  case 'E':
    pc_t = pc::exit;
    break;
  default:
    cout << "str = " << str << endl;
    assert(false && "invalid pc");
  }

  size_t dot = str.find(pc_index_sep, 1);
  if (dot == string::npos) {
    cout << "str = " << str << endl;
  }
  ASSERT(dot != string::npos);
  string index_str = str.substr(1, dot - 1);
  size_t dot2 = str.find(pc_index_sep, dot + 1);
  if (dot2 == string::npos) {
    cout << "str = " << str << endl;
  }
  ASSERT(dot2 != string::npos);
  //size_t dot3 = str.find(pc_index_sep, dot2 + 1);
  //ASSERT(dot3 != string::npos);
  //string bblnum_str = str.substr(dot + 1, dot2 - (dot + 1));
  //string subindex_str = str.substr(dot2 + 1, dot3 - (dot2 + 1));
  //string subsubindex_str = str.substr(dot3 + 1);
  string subindex_str = str.substr(dot + 1, dot2 - (dot + 1));
  string subsubindex_str = str.substr(dot2 + 1);
  //int index = stoi(index_str);
  //int bblnum = stoi(bblnum_str);
  int subindex = stoi(subindex_str);
  int subsubindex = stoi(subsubindex_str);
  //p = pc(pc_t, index_str.c_str(), bblnum, subindex, subsubindex);
  p = pc(pc_t, index_str.c_str(), subindex, subsubindex);
  /*if(str.size() > 1)
    p = pc(pc_t, str.substr(1));
  else
    p = pc(pc_t);*/

  return p;
}

ostream& operator<<(ostream& os, const pc& p)
{
  os << p.to_string();
  return os;
}

/*bool
pc::subsubindex_is_phi_node(int subsubindex)
{
  return    subsubindex >= PC_SUBSUBINDEX_PHI_NODE(0)
         && subsubindex < PC_SUBSUBINDEX_PHI_NODE(PC_SUBSUBINDEX_MAX_PHI_NODES);
}*/

string pcpair::to_string() const
{
  return string(m_first.to_string() + "_" + m_second.to_string());
}

ostream& operator<<(ostream& os, const pcpair& p)
{
  os << p.to_string();
  return os;
}

/*pc pc::get_exit()
{
  return pc(exit, 0, 0, 0);
}*/

/*pc pc::get_pc(pc_type t, int index, int subindex, int subsubindex)
{
  return pc(t, index, subindex, subsubindex);
}*/

pcpair
pcpair::create_from_string(string const &s)
{
  size_t underscore = s.find("_");
  if (underscore == string::npos) {
    cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
  }
  ASSERT(underscore != string::npos);
  string src_pc_str = s.substr(0, underscore);
  string dst_pc_str = s.substr(underscore + 1);
  return pcpair(pc::create_from_string(src_pc_str), pc::create_from_string(dst_pc_str));
}

pc
pc::increment_subindex() const
{
  pc ret = *this;
  ret.m_subindex++;
  return ret;
}

pc
pc::increment_subsubindex() const
{
  pc ret = *this;
  ret.m_subsubindex++;
  return ret;
}

}
