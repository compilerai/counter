#pragma once
#include <ostream>
#include <string>
#include <list>
#include <map>
#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/ptfg_types.h"
#include "ptfg/function_signature.h"

namespace eqspace {

using namespace std;

class llptfg_t {
private:
  string m_llvm_header;
  list<string> m_type_decls;
  list<string> m_globals_with_initializers;
  list<string> m_function_declarations;
  map<string, unique_ptr<tfg_llvm_t>> m_fname_tfg_map;
  map<string, function_signature_t> m_fname_signature_map;
  map<string, callee_summary_t> m_fname_csum_map;
  map<string, llvm_fn_attribute_id_t> m_fname_attributes_map;
  map<string, link_status_t> m_fname_linker_status_map;
  map<llvm_fn_attribute_id_t, string> m_attributes;
  list<string> m_llvm_metadata;
public:
  llptfg_t(string const& llvm_header,
           list<string> const& type_decls,
           list<string> const& globals_with_initializers,
           list<string> const& function_declarations,
           map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>>& function_tfg_map,
           map<string, function_signature_t> fname_signature_map,
           map<string, llvm_fn_attribute_id_t> const& fname_attributes_map,
           map<string, link_status_t> const& fname_linker_status_map,
           map<llvm_fn_attribute_id_t, string> const& attributes,
           list<string> const& llvm_metadata) : m_llvm_header(llvm_header),
                                              m_type_decls(type_decls),
                                              m_globals_with_initializers(globals_with_initializers),
                                              m_function_declarations(function_declarations),
                                              m_fname_signature_map(fname_signature_map),
                                              m_fname_attributes_map(fname_attributes_map),
                                              m_fname_linker_status_map(fname_linker_status_map),
                                              m_attributes(attributes),
                                              m_llvm_metadata(llvm_metadata)
  {
    for (auto& ft : function_tfg_map) {
      m_fname_tfg_map.insert(make_pair(ft.first, std::move(ft.second.second)));
      m_fname_csum_map.insert(make_pair(ft.first, ft.second.first));
    }
  }

  void print(ostream &os) const;
  void output_llvm_code(ostream &os) const;
  static llptfg_t read_from_stream(istream &is, context *ctx);
private:
  static void output_llvm_code_for_functions(ostream &os, map<string, unique_ptr<tfg_llvm_t>> const& function_tfg_map, map<string, function_signature_t> const& fname_signature_map, map<string, llvm_fn_attribute_id_t> const& fname_attributes_map, map<string, link_status_t> const& fname_linker_status_map);
  static void output_llvm_code_for_tfg(ostream &os, unique_ptr<tfg_llvm_t> const& t);
};

}
