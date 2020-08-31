#pragma once
#include "eq/corr_graph.h"

namespace eqspace {

using namespace std;
class function_cg_map_t {
private:
  map<string, shared_ptr<corr_graph const>> m_map;
public:
  function_cg_map_t(map<string, shared_ptr<corr_graph const>> const& m) : m_map(m)
  { }
  function_cg_map_t(istream& is, context* ctx);
  void remove_marker_calls_and_gen_annotated_assembly(ostream& os, string const& asm_filename);
private:
};

}
