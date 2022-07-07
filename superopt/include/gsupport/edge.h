#pragma once

#include "gsupport/edge_id.h"

namespace eqspace {

template <typename T_PC>
class edge : public enable_shared_from_this<edge<T_PC>>
{
public:
  edge(const T_PC& pc_from, const T_PC& pc_to) : m_from_pc(pc_from), m_to_pc(pc_to), m_is_back_edge(false)
  {
/*#ifndef NDEBUG
    m_num_allocated_edge++;
#endif*/
  }

  edge(edge<T_PC> const &other) : m_from_pc(other.m_from_pc), m_to_pc(other.m_to_pc), m_is_back_edge(other.m_is_back_edge)
  {
/*#ifndef NDEBUG
    m_num_allocated_edge++;
#endif*/
  }

  virtual ~edge()
  {
/*#ifndef NDEBUG
    m_num_allocated_edge--;
#endif*/
  }

  const T_PC& get_from_pc() const
  {
    //ASSERT(!this->is_empty());
    return m_from_pc;
  }

  const T_PC& get_to_pc() const
  {
    //ASSERT(!this->is_empty());
    return m_to_pc;
  }

  edge_id_t<T_PC> get_edge_id() const { return edge_id_t<T_PC>(m_from_pc, m_to_pc); }

  string to_string_for_eq(string const& prefix) const
  {
    stringstream ss;
    ss << prefix << ": " << this->get_from_pc().to_string() << " => " << this->get_to_pc().to_string() << endl;
    return ss.str();
  }

  bool is_back_edge() const
  {
    return m_is_back_edge;
  }

  void set_backedge(bool be) const
  {
    m_is_back_edge = be;
  }

  //void set_from_pc(const T_PC& p)
  //{
  //  m_from_pc = p;
  //}

  //void set_to_pc(const T_PC& p)
  //{
  //  m_to_pc = p;
  //}

  virtual bool is_empty() const { return false; }

  virtual string to_string() const
  {
    stringstream ss;
    ss << m_from_pc.to_string() << "=>" << m_to_pc.to_string();
    return ss.str();
  }

  friend ostream& operator<<(ostream& os, const edge& e)
  {
    os << e.to_string();
    return os;
  }
  bool operator==(edge const& other) const
  {
    return    this->m_from_pc == other.m_from_pc
           && this->m_to_pc == other.m_to_pc
           //&& this->m_is_back_edge == other.m_is_back_edge // mutables should not be used in equality check
    ;
  }

  static void edge_read_pcs_and_comment(const string& pcs, T_PC& p1, T_PC& p2, string &comment)
  {
    //cout << __func__ << " " << __LINE__ << ": pcs = " << pcs << ".\n";
    size_t pos = pcs.find("=>");
    if (pos == string::npos) {
      cout << __func__ << " " << __LINE__ << ": pcs = " << pcs << endl;
      NOT_REACHED();
    }
    string pc1 = pcs.substr(0, pos);
    size_t lbrack = pcs.find("[", pos + 2);
    string pc2;
    if (lbrack == string::npos) {
      pc2 = pcs.substr(pos + 2);
      comment = "";
    } else {
      pc2 = pcs.substr(pos + 2, pos + 2 + lbrack - 1);
      comment = pcs.substr(lbrack);
    }
    p1 = T_PC::create_from_string(pc1);
    p2 = T_PC::create_from_string(pc2);
  }

private:
  T_PC const m_from_pc;
  T_PC const m_to_pc;
  mutable bool m_is_back_edge;

/*#ifndef NDEBUG
  static long long m_num_allocated_edge;
#endif*/
};

}

namespace std
{
template<typename T_PC>
struct hash<edge<T_PC>>
{
  std::size_t operator()(edge<T_PC> const &e) const
  {
    hash<T_PC> pc_hasher;
    size_t seed = 0;
    if (!e.is_empty()) {
      myhash_combine<size_t>(seed, pc_hasher(e.get_from_pc()));
      myhash_combine<size_t>(seed, pc_hasher(e.get_to_pc()));
    }
    return seed;
  }
};
}
