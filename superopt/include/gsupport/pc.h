#pragma once

#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <list>
#include <vector>
#include "expr/comment.h"
#include "support/debug.h"
#include "support/types.h"
#include "support/utils.h"

namespace eqspace {

using namespace std;

class pc
{
public:
  enum pc_type
  {
    insn_label,
    exit,
  };
  static char const pc_index_sep = '%'; //do not use '#' because the solver renames identifiers containing the '#' character; do not use '@' because the solver expects function names after this separator;
  pc() : m_type(insn_label), m_index(NULL)/*, m_bblnum(0)*/, m_subindex(-1), m_subsubindex(-1) {}
  //pc(pc_type type, char const *index/*, int bblnum*/, int subindex, int subsubindex) : m_type(type)/*, m_bblnum(bblnum)*/, m_subindex(subindex), m_subsubindex(subsubindex) { m_index = pc_strbuf_search_or_add(index); }
  pc(pc_type type, char const *index, int subindex, int subsubindex) : m_type(type)/*, m_bblnum(0)*/, m_subindex(subindex), m_subsubindex(subsubindex) { m_index = pc_strbuf_search_or_add(index); }
  pc(pc_type type, string const &id) : m_type(type), m_index(NULL)/*, m_bblnum(0)*/, m_subindex(-1), m_subsubindex(-1) {
    //m_index = stoi(id.substr(1));
    m_index = pc_strbuf_search_or_add(id.c_str());
    m_subindex = 0;
    m_subsubindex = PC_SUBSUBINDEX_DEFAULT;
    //cout << __func__ << " " << __LINE__ << ": id = " << id << ": m_index = " << m_index << endl;
  }
  pc(pc_type type) : m_type(type), m_index(NULL)/*, m_bblnum(0)*/, m_subindex(-1), m_subsubindex(-1) {
    if (type == exit) {
      m_index = pc_strbuf_search_or_add("0");
      m_subindex = 0;
      m_subsubindex = PC_SUBSUBINDEX_DEFAULT;
      return;
    }
    cout << __func__ << " " << __LINE__ << ": " << this->to_string() << endl;
    NOT_IMPLEMENTED();
  }
  //string get_id() const {
  //  ASSERT(m_subindex == 0);
  //  ASSERT(m_subsubindex == 0);
  //  stringstream ss;
  //  ss << '%' << m_index;
  //  string ret = ss.str();
  //  //cout << __func__ << " " << __LINE__ << ": returning " << ret << endl;
  //  //NOT_IMPLEMENTED();
  //  return ret;
  //}

  pc_type get_type() const { return m_type; }
  void set_type(pc_type t) { m_type = t; }
  char const *get_index() const { return m_index; }
  //int get_bblnum() const { return m_bblnum; }
  int get_subindex() const { return m_subindex; }
  int get_subsubindex() const { return m_subsubindex; }
  void set_subindex(int i) { m_subindex = i; }
  void set_subsubindex(int i) { m_subsubindex = i; }
  pc increment_subindex() const;
  pc increment_subsubindex() const;
  void set_index(char const *i, int j, int k) { m_index = pc_strbuf_search_or_add(i); m_subindex = j; m_subsubindex = k; }

  string to_string() const;
  //string to_string_for_dot() const;
  //string to_string_for_eq() const;

  bool is_exit() const
  {
    return m_type == exit;
  }

  bool is_start() const
  {
    pc s = start();
    return (s.get_index() == get_index() && s.get_subindex() == get_subindex() && s.get_subsubindex() == get_subsubindex() && s.get_type() == get_type());
  }

  bool is_label() const
  {
    return m_type == insn_label;
  }

  bool is_fcall() const
  {
    return    this->get_subsubindex() == PC_SUBSUBINDEX_FCALL_START
           || this->get_subsubindex() == PC_SUBSUBINDEX_FCALL_END;
  }

  static pc start()
  {
    return pc(insn_label, "0", PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
  }


  //static pc get_exit();
  //static pc get_pc(pc_type t, int index, int subindex, int subsubindex);

  char const *pc_type_to_string_full(pc_type t) const;
  string pc_type_to_string(pc_type t) const;

  static bool subsubindex_is_phi_node(int subsubindex)
  {
    return    subsubindex >= PC_SUBSUBINDEX_INTERMEDIATE_VAL(0)
           && subsubindex < PC_SUBSUBINDEX_INTERMEDIATE_VAL(PC_SUBSUBINDEX_MAX_INTERMEDIATE_VALS);
  }

  bool is_equal(const pc& p) const
  {
    //cout << __func__ << " " << __LINE__ << ": p.get_type = " << pc_type_to_string(p.get_type()) << ", m_type = " << pc_type_to_string(m_type) << endl;
    //cout << __func__ << " " << __LINE__ << ": p.get_index = " << p.get_index() << ", m_index = " << m_index << endl;
    //cout << __func__ << " " << __LINE__ << ": p.get_subindex = " << p.get_subindex() << ", m_subindex = " << m_subindex << endl;
    //cout << __func__ << " " << __LINE__ << ": p.get_subsubindex = " << p.get_subsubindex() << ", m_subsubindex = " << m_subsubindex << endl;
    bool ret = (p.get_type() == m_type && p.get_index() == m_index && p.get_subindex() == m_subindex && p.get_subsubindex() == m_subsubindex);
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    return ret;
  }

  bool is_less(const pc& that) const
  {
    if(get_type() == that.get_type()) {
      //if (m_bblnum == that.m_bblnum) {
        if (get_index() == that.get_index()) {
          if (get_subindex() == that.get_subindex()) {
            /*if (   subsubindex_is_phi_node(get_subsubindex())
                && !subsubindex_is_phi_node(that.get_subsubindex())) {
              return true;
            }
            if (   subsubindex_is_phi_node(that.get_subsubindex())
                && !subsubindex_is_phi_node(get_subsubindex())) {
              return false;
            }*/
            return get_subsubindex() < that.get_subsubindex();
          } else {
            return get_subindex() < that.get_subindex();
          }
        } else {
          return get_index() < that.get_index();
        }
      //} else {
      //  return m_bblnum < that.m_bblnum;
      //}
    } else {
      return get_type() < that.get_type();
    }
  }

  bool operator==(const pc& p) const { return is_equal(p); }
  bool operator!=(const pc& p) const { return !is_equal(p); }

  bool operator<(const pc& that) const
  {
    return is_less(that);
  }

  static pc create_from_string(const string& str);
  friend ostream& operator<<(ostream& os, const pc& p);

  const pc& get_first() const
  {
    return *this;
  }

private:
  char const *pc_strbuf_search_or_add(char const *i)
  {
    char *ptr = pc_strbuf;
    char *end = &pc_strbuf[pc_strbuflen];
    ASSERT(ptr <= end);
    while (ptr < end) {
      if (!strcmp(ptr, i)) {
        return ptr;
      }
      ptr += strlen(ptr) + 1;
      ASSERT(ptr <= end);
    }
    ASSERT(ptr == end);
    size_t remaining = &pc_strbuf[MAX_PC_STRBUF_SIZE - 1] - ptr;
    ASSERT(strlen(i) < remaining);
    strncpy(ptr, i, remaining);
    pc_strbuflen += strlen(i) + 1;
    return ptr;
  }
  pc_type m_type;
  //int m_index;
  char const *m_index;
  //int m_bblnum;
  int m_subindex;
  int m_subsubindex;
  static size_t const MAX_PC_STRBUF_SIZE = 40960;
  static char pc_strbuf[MAX_PC_STRBUF_SIZE];
  static size_t pc_strbuflen;
};

class pc_comp
{
public:
  bool operator() (const pc& left, const pc& right) const
  {
    return left.is_less(right);
  }
};

struct pcpair
{
  pcpair() : m_first(), m_second() {}
  pcpair(const pc& p1, const pc& p2) : m_first(p1), m_second(p2) {}

  static pcpair start()
  {
    pcpair ret(pc::start(), pc::start());
    return ret;
  }

  const pc& get_first() const
  {
    return m_first;
  }

  const pc& get_second() const
  {
    return m_second;
  }

  bool is_equal(const pcpair& pp) const
  {
    return get_first().is_equal(pp.get_first()) && get_second().is_equal(pp.get_second());
  }

  bool is_less(const pcpair& that) const
  {
    if(get_first() == that.get_first())
      return get_second().is_less(that.get_second());
    else
      return get_first().is_less(that.get_first());
  }

  bool is_exit() const
  {
    return (m_first.is_exit() && m_second.is_exit());
  }

  bool is_start() const
  {
    return (m_first.is_start() && m_second.is_start());
  }

  bool operator==(const pcpair& p) const { return is_equal(p); }
  bool operator!=(const pcpair& p) const { return !is_equal(p); }
  string to_string() const;

  bool operator<(const pcpair& that) const
  {
    return is_less(that);
  }
  static pcpair create_from_string(const string& str);

  bool is_fcall_pc() const
  {
    bool ret = (m_first.get_subsubindex() == PC_SUBSUBINDEX_FCALL_END);
    if(ret)
      ASSERT(m_second.get_subsubindex() == PC_SUBSUBINDEX_FCALL_END);
    return ret;
  }

  friend ostream& operator<<(ostream& os, const pcpair& p);

private:
  pc m_first;
  pc m_second;
};

struct pcpair_comp
{
  bool operator() (const pcpair& left, const pcpair& right) const
  {
    return left.is_less(right);
  }
};

class pc_locid_pair {
public:
  pc const m_pc;
  graph_loc_id_t m_loc_id;

  pc_locid_pair(pc const &pc, int loc_id) : m_pc(pc), m_loc_id(loc_id) {}
  string to_string() const { 
    stringstream ss;
    ss << "(" << m_pc.to_string() << ", " << m_loc_id << ")";
    return ss.str();
  }
};

struct pc_locid_pair_comp {
  bool operator()(pc_locid_pair const &left, pc_locid_pair const &right) const
  {
    pc_comp pcc;
    if (left.m_loc_id < right.m_loc_id) {
      return true;
    }
    if (left.m_loc_id > right.m_loc_id) {
      return false;
    }
    assert(left.m_loc_id == right.m_loc_id);
    if (pcc(left.m_pc, right.m_pc)) {
      return true;
    }
    if (pcc(right.m_pc, left.m_pc)) {
      return false;
    }
    return false;
  }
};

class pc_exprid_pair {
public:
  pc const m_pc;
  unsigned m_expr_id;

  pc_exprid_pair(pc const &pc, unsigned expr_id) : m_pc(pc), m_expr_id(expr_id) {}
};

struct pc_exprid_pair_comp {
  bool operator()(pc_exprid_pair const &left, pc_exprid_pair const &right) const
  {
    pc_comp pcc;
    if (left.m_expr_id < right.m_expr_id) {
      return true;
    }
    if (left.m_expr_id > right.m_expr_id) {
      return false;
    }
    assert(left.m_expr_id == right.m_expr_id);
    if (pcc(left.m_pc, right.m_pc)) {
      return true;
    }
    if (pcc(right.m_pc, left.m_pc)) {
      return false;
    }
    return false;
  }
};

class pc_addr_id_pair {
public:
  pc const m_pc;
  addr_id_t m_addr_id;

  pc_addr_id_pair(pc const &pc, addr_id_t addr_id) : m_pc(pc), m_addr_id(addr_id) {}
  string to_string() const {
    stringstream ss;
    ss << "(" << m_pc.to_string() << ", " << m_addr_id << ")";
    return ss.str();
  }
};

struct pc_addr_id_pair_comp {
  bool operator()(pc_addr_id_pair const &left, pc_addr_id_pair const &right) const
  {
    pc_comp pcc;
    if (left.m_addr_id < right.m_addr_id) {
      return true;
    }
    if (left.m_addr_id > right.m_addr_id) {
      return false;
    }
    assert(left.m_addr_id == right.m_addr_id);
    if (pcc(left.m_pc, right.m_pc)) {
      return true;
    }
    if (pcc(right.m_pc, left.m_pc)) {
      return false;
    }
    return false;
  }
};

void get_insn_and_op_index_from_comment(eqspace::comment_t const &comment, int *insn_index, int *op_index);


}
//
//namespace std
//{
//using namespace eqspace;
//template<>
//struct hash<eqspace::pc>
//{
//  std::size_t operator()(eqspace::pc const &p) const
//  {
//    hash<string> string_hasher;
//    return string_hasher(p.get_index()) * 17 + p.get_subindex()*7 + p.get_subsubindex();
//  }
//};
//}


namespace std{
using namespace eqspace;
template<>
struct hash<pc>
{
  std::size_t operator()(pc const &p) const
  {
    size_t seed = 0;
    myhash_combine<std::size_t>(seed, std::hash<unsigned long>()(p.get_type()));
    myhash_combine<std::size_t>(seed, std::hash<unsigned long>()((unsigned long)p.get_index()));
    myhash_combine<std::size_t>(seed, std::hash<unsigned long>()(p.get_subindex()));
    myhash_combine<std::size_t>(seed, std::hash<unsigned long>()(p.get_subsubindex()));
    return seed;
  }
};

template<>
struct hash<pcpair>
{
  std::size_t operator()(pcpair const &p) const
  {
    size_t seed = 0;
    myhash_combine<std::size_t>(seed, std::hash<pc>()(p.get_first()));
    myhash_combine<std::size_t>(seed, std::hash<pc>()(p.get_second()));
    return seed;
  }
};

}
