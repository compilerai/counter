#pragma once
#include "eq/cg_with_asm_annotation.h"

namespace eqspace {

using namespace std;
class function_cg_map_t {
private:
  map<string, dshared_ptr<cg_with_asm_annotation const>> m_map;
public:
  function_cg_map_t(map<string, dshared_ptr<cg_with_asm_annotation const>> const& m) : m_map(m)
  { }
  function_cg_map_t(istream& is, context* ctx);
  void remove_marker_calls_and_gen_annotated_assembly(ostream& os, string const& asm_filename);
  void function_cg_map_xml_print(ostream& os/*, string const& asm_filename*/) const;
private:
};

}
