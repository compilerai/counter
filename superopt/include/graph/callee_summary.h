#ifndef CALLEE_SUMMARY_H
#define CALLEE_SUMMARY_H
#include "expr/memlabel.h"
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include "support/debug.h"
#include "expr/consts_struct.h"

namespace eqspace {

class callee_summary_t
{
private:
  //bool m_is_nop;
  set<memlabel_t> m_reads;
  set<memlabel_t> m_writes;
public:
  callee_summary_t() {}
  //void set_is_nop(bool b) { m_is_nop = b; }
  //bool is_nop() const { return m_is_nop; }
  void set_reads(set<memlabel_t> const &s) { m_reads = s; }
  //bool reads_heap_or_symbols() const { return m_reads_heap_or_symbols; }
  void set_writes(set<memlabel_t> const &s) { m_writes = s; }
  //bool writes_heap_or_symbols() const { return m_writes_heap_or_symbols; }
  //memlabel_t get_read_memlabel() const;
  //memlabel_t get_write_memlabel() const;
  pair<memlabel_t, memlabel_t> get_readable_writeable_memlabels(vector<memlabel_t> const &farg_memlabels, memlabel_t const &ml_fcall_bottom) const;
  //void rename_symbol_ids(map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &from, map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &to);

  static memlabel_t memlabel_fcall_bottom(consts_struct_t const &cs, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map);
  static callee_summary_t callee_summary_bottom(consts_struct_t const &cs, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, size_t num_fargs);

  string callee_summary_to_string_for_eq() const
  {
    stringstream ss;
    /*if (m_is_nop) {
      ss << " is_nop";
    }*/
    ss << " reads:( ";
    for (const auto &ml : m_reads) {
      ss << ml.to_string() << ", ";
    }
    ss << " )";
    ss << " writes:( ";
    for (const auto &ml : m_writes) {
      ss << ml.to_string() << ", ";
    }
    ss << " )";
    return ss.str();
  }

  void callee_summary_read_from_string(string const &s)
  {
    istringstream iss(s);
    m_reads.clear();
    m_writes.clear();
    //cout << "s: " << s << endl;
    while (iss) {
      string token;
      iss >> token;
      //cout << "token = " << token << endl;
      if (token == "reads:(" || token == "writes:(") {
        bool is_read = false;
        if (token == "reads:(") {
          is_read = true;
        }
        while (1) {
          iss >> token;
          if (token == ")") {
            break;
          }
          //cout << __func__ << " " << __LINE__ << ": token = " << token << endl;
          ASSERT(token.back() == ',');
          token.pop_back();
          //cout << __func__ << " " << __LINE__ << ": token = " << token << endl;
          memlabel_t ml;
          memlabel_t::memlabel_from_string(&ml, token.c_str());
          if (is_read) {
            m_reads.insert(ml);
          } else {
            m_writes.insert(ml);
          }
        }
      }
    }
    //cout << "this = " << this->callee_summary_to_string_for_eq() << endl;
  }
};

}

#endif
