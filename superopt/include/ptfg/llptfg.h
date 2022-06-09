#pragma once

#include <ostream>
#include <string>
#include <list>
#include <map>

#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"

#include "ptfg/ptfg_types.h"
#include "ptfg/function_signature.h"
#include "ptfg/ftmap.h"

namespace eqspace {

using namespace std;

class llptfg_t : public ftmap_t {
private:
  string m_llvm_header;
  list<string> m_type_decls;
  list<string> m_globals_with_initializers;
  list<string> m_function_declarations;
  map<string, function_signature_t> m_fname_signature_map;
  map<string, llvm_fn_attribute_id_t> m_fname_attributes_map;
  map<string, link_status_t> m_fname_linker_status_map;
  map<llvm_fn_attribute_id_t, string> m_attributes;
  list<string> m_llvm_metadata;
public:
  llptfg_t(string const& llvm_header,
           list<string> const& type_decls,
           list<string> const& globals_with_initializers,
           list<string> const& function_declarations,
           ftmap_t const& ftmap,
           map<string, function_signature_t> fname_signature_map,
           map<string, llvm_fn_attribute_id_t> const& fname_attributes_map,
           map<string, link_status_t> const& fname_linker_status_map,
           map<llvm_fn_attribute_id_t, string> const& attributes,
           list<string> const& llvm_metadata);

  llptfg_t(istream &is, context *ctx);

  dshared_ptr<tfg_llvm_t const> llptfg_get_tfg_llvm_for_function_name(string const& function_name) const;

  graph_symbol_map_t llptfg_get_symbol_map() const;

  relevant_memlabels_t llptfg_get_relevant_memlabels() const;

  void print(ostream &os) const;
  //void output_llvm_code(ostream &os) const;

private:
  //static void output_llvm_code_for_functions(ostream &os, map<string, unique_ptr<tfg_llvm_t>> const& function_tfg_map, map<string, function_signature_t> const& fname_signature_map, map<string, llvm_fn_attribute_id_t> const& fname_attributes_map, map<string, link_status_t> const& fname_linker_status_map);
  //static void output_llvm_code_for_tfg(ostream &os, unique_ptr<tfg_llvm_t> const& t);
};

}
