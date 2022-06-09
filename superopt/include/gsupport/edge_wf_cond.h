#pragma once
#include <map>
#include <sstream>

#include "gsupport/graph_ec.h"
#include "gsupport/predicate.h"
#include "gsupport/predicate_set.h"
#include "gsupport/src_dst_cg_path_tuple.h"

namespace eqspace {

using namespace std;

class edge_wf_cond_t {
private:
  map<src_dst_cg_path_tuple_t, predicate_set_t> m_map;
public:
  edge_wf_cond_t() {}
  //edge_wf_cond_t(istream& is, string const& prefix = "");
  void edge_wf_cond_to_stream(ostream& os, string const& prefix = "") const;
  map<src_dst_cg_path_tuple_t, predicate_set_t> const& get_map() const { return m_map; }
  void add_wf_cond(src_dst_cg_path_tuple_t const& src_dst_cg_path_tuple, predicate_ref const& pred)
  {
    m_map[src_dst_cg_path_tuple].insert(pred);
  }
  void edge_wf_cond_union(edge_wf_cond_t const& other)
  {
    for (auto const& p : other.get_map()) {
      predicate_set_union(m_map[p.first], p.second);
    }
  }
};

template<typename T_E>
string
wfcond_comment(T_E const& e, string const& comment_suffix)
{
  ostringstream ss;
  ss << "wfcond.from_pc" << e->get_from_pc().to_string() << ".to_pc" << e->get_to_pc().to_string() << comment_suffix;
  return ss.str();
}

}
