#include "ptfg/llptfg.h"
#include "support/debug.h"
#include "tfg/parse_input_eq_file.h"
#include "tfg/tfg_llvm.h"
#include <memory>
#include "support/mymemory.h"

namespace eqspace {

using namespace std;

void
llptfg_t::print(ostream &outputStream) const
{
  outputStream << "=llvm_header\n";
  outputStream << m_llvm_header << endl;
  outputStream << "=type_decls\n";
  for (auto const& type_decl : m_type_decls) {
    outputStream << type_decl << endl;
  }
  outputStream << "=globals_with_initializers\n";
  for (auto const& g : m_globals_with_initializers) {
    outputStream << g << endl;
  }
  outputStream << "=function_declarations\n";
  for (auto const& f : m_function_declarations) {
    outputStream << f << endl;
  }
  outputStream << "=FunctionTFGs\n";
  for (auto const& ft : m_fname_tfg_map) {
    outputStream << "=FunctionName: " << ft.first << "\n";
    outputStream << ft.second->graph_to_string() << "\n";
  }
  outputStream << "=FunctionSignatures\n";
  for (auto const& fs : m_fname_signature_map) {
    outputStream << "=FunctionSignature: " << fs.first << "\n";
    outputStream << fs.second.function_signature_to_string_for_eq() << "\n";
  }
  outputStream << "=FunctionCsums\n";
  for (auto const& fc : m_fname_csum_map) {
    outputStream << "=FunctionSummary: " << fc.first << "\n";
    outputStream << fc.second.callee_summary_to_string_for_eq() << "\n";
  }
  outputStream << "=FunctionAttributes\n";
  for (auto const& fa : m_fname_attributes_map) {
    outputStream << "=FunctionAttributesFor: " << fa.first << "\n";
    outputStream << fa.second << "\n";
  }
  outputStream << "=FunctionLinkage\n";
  for (auto const& fl : m_fname_linker_status_map) {
    outputStream << "=FunctionLinkageFor: " << fl.first << "\n";
    outputStream << fl.second << "\n";
  }
  outputStream << "=Attributes\n";
  for (auto const& a : m_attributes) {
    outputStream << a.first << " = " << a.second << "\n";
  }
  outputStream << "=Metadata\n";
  for (auto const& m : m_llvm_metadata) {
    outputStream << m << endl;
  }
}

void
llptfg_t::output_llvm_code(ostream &outputStream) const
{
  outputStream << m_llvm_header << endl;
  for (auto const& t : m_type_decls) {
    outputStream << t << endl;
  }
  outputStream << endl;
  for (auto const& g : m_globals_with_initializers) {
    outputStream << g << endl;
  }
  outputStream << endl;
  for (auto const& f : m_function_declarations) {
    outputStream << f << endl;
  }
  outputStream << endl;
  output_llvm_code_for_functions(outputStream, m_fname_tfg_map, m_fname_signature_map, m_fname_attributes_map, m_fname_linker_status_map);
  for (auto const& a : m_attributes) {
    outputStream << "attributes #" << a.first << " = { " << a.second << " }\n";
  }
  outputStream << endl;
  for (auto const& m : m_llvm_metadata) {
    outputStream << m << endl;
  }
}

void
llptfg_t::output_llvm_code_for_functions(ostream &os,
    map<string, unique_ptr<tfg_llvm_t>> const& function_tfg_map,
    map<string, function_signature_t> const& fname_signature_map,
    map<string, llvm_fn_attribute_id_t> const& fname_attributes_map,
    map<string, link_status_t> const& fname_linker_status_map)
{
  for (auto const& ft : function_tfg_map) {
    string const& return_type = fname_signature_map.at(ft.first).get_return_type();
    list<string> const& arg_types = fname_signature_map.at(ft.first).get_arg_types();
    list<string> const& arg_names = fname_signature_map.at(ft.first).get_arg_names();
    ASSERT(arg_types.size() == arg_names.size());
    string args_str;
    for (list<string>::const_iterator it = arg_types.begin(), in = arg_names.begin();
         it != arg_types.end() && in != arg_names.end();
         it++, in++) {
      if (it != arg_types.begin()) {
        args_str += ", ";
      }
      args_str += *it + " " + *in;
    }
    if (!fname_linker_status_map.count(ft.first)) {
      cout << __func__ << " " << __LINE__ << ": ft.first = " << ft.first << endl;
      cout << __func__ << " " << __LINE__ << ": no linker status available for " << ft.first << endl;
    }
    string attr;
    if (fname_attributes_map.count(ft.first)) {
      attr = fname_attributes_map.at(ft.first);
      //cout << __func__ << " " << __LINE__ << ": ft.first = " << ft.first << endl;
      //cout << __func__ << " " << __LINE__ << ": no attributes available for " << ft.first << endl;
    }
    //os << "define " << fname_linker_status_map.at(ft.first) << " " << return_type << " @" << ft.first << "(" << args_str << ") " << fname_attributes_map.at(ft.first) << " {\n";
    os << "define " << fname_linker_status_map.at(ft.first) << " " << return_type << " @" << ft.first << "(" << args_str << ") " << attr << " {\n";
    output_llvm_code_for_tfg(os, ft.second);
    os << "}\n\n";
  }
}

void
llptfg_t::output_llvm_code_for_tfg(ostream &os, unique_ptr<tfg_llvm_t> const& t)
{
  list<shared_ptr<tfg_edge const>> t_edges;
  set<te_comment_t> te_comments;
  t->get_edges(t_edges);
  for (auto const& te : t_edges) {
    te_comment_t const& te_comment = te->get_te_comment();
    te_comments.insert(te_comment);
    //pc const& from_pc = te->get_from_pc();
    //pc const& to_pc = te->get_to_pc();
    //char const* to_index = to_pc.get_index();
    //int to_subindex = to_pc.get_subindex();
    //int to_subsubindex = to_pc.get_subsubindex();
    //char const* from_index = from_pc.get_index();
    //int from_subindex = from_pc.get_subindex();
    //int from_subsubindex = from_pc.get_subsubindex();
    //if (te_comment.te_comment_is_special()) {
    //  continue;
    //}

    //if (   from_subindex == PC_SUBINDEX_FIRST_INSN_IN_BB
    //    && from_subsubindex == PC_SUBSUBINDEX_DEFAULT
    //    && !stringIsInteger(from_index)) {
    //  os << "\n" << from_index << ":\n";
    //}

    //if (from_subsubindex == PC_SUBSUBINDEX_DEFAULT) {
    //  string comment = te_comment.get_string();
    //  os << comment << endl;
    //}
  }
  bool found_ret = false;
  int last_bbl_num = -1;
  for (auto const& te_comment : te_comments) {
    if (te_comment.get_bbl_order_desc() == bbl_order_descriptor_t::invalid()) {
      continue;
    }
    int bbl_num = te_comment.get_bbl_order_desc().get_bbl_num();
    if (bbl_num != last_bbl_num) {
      string_ref const& name = te_comment.get_bbl_order_desc().get_bbl_name();
      os << name->get_str() << ":\n";
      last_bbl_num = bbl_num;
    }
    string const& code = te_comment.get_code()->get_str();
    if (string_has_prefix(code, "  ret")) {
      found_ret = true;
    }
    os << code << endl;
  }
  if (!found_ret) {
    os << "  ret void\n";
  }
}

llptfg_t
llptfg_t::read_from_stream(istream &in, context *ctx)
{
  string llvm_header;
  list<string> type_decls, globals, function_declarations;
  map<string, unique_ptr<tfg_llvm_t>> tfgs;
  map<string, function_signature_t> fsigs;
  map<string, callee_summary_t> fcsums;
  map<string, llvm_fn_attribute_id_t> fattrs;
  map<string, link_status_t> flinkage;
  map<llvm_fn_attribute_id_t, string> attr_descs;
  list<string> metadata;
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=llvm_header");
  end = !getline(in, line);
  ASSERT(!end);
  while (line.size() == 0 || line.at(0) != '=') {
    llvm_header += line + "\n";
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=type_decls");
  end = !getline(in, line);
  ASSERT(!end);
  while (line.size() == 0 || line.at(0) != '=') {
    type_decls.push_back(line);
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=globals_with_initializers");
  end = !getline(in, line);
  ASSERT(!end);
  while (line.size() == 0 || line.at(0) != '=') {
    globals.push_back(line);
    end = !getline(in, line);
    ASSERT(!end);
  }

  ASSERT(line == "=function_declarations");
  end = !getline(in, line);
  ASSERT(!end);
  while (line.size() == 0 || line.at(0) != '=') {
    function_declarations.push_back(line);
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=FunctionTFGs");
  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "=FunctionName:")) {
    string function_name = line.substr(strlen(FUNCTION_NAME_FIELD_IDENTIFIER));
    trim(function_name);
    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line == "=TFG:");
    //tfg *tfg_llvm = NULL;
    //auto pr = read_tfg(in, &tfg_llvm, "llvm", ctx, true);
    unique_ptr<tfg_llvm_t> tfg_llvm = make_unique<tfg_llvm_t>(in, "llvm", ctx);
    //line = tfg_llvm->graph_from_stream(in);
    //line = pr.second;
    //end = pr.first;
    //ASSERT(line == FUNCTION_FINISH_IDENTIFIER);
    end = false;
    //ASSERT(!end);
    do {
      end = !getline(in, line);
      ASSERT(!end);
    } while (line == "");
    tfgs[function_name] = std::move(tfg_llvm);
  }
  ASSERT(string_has_prefix(line, "=FunctionSignatures"));
  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "=FunctionSignature:")) {
    string function_name = line.substr(strlen("=FunctionSignature:"));
    trim(function_name);
    end = !getline(in, line);
    ASSERT(!end);
    function_signature_t fsig = function_signature_t::read_from_string(line);
    fsigs.insert(make_pair(function_name, fsig));
    end = !getline(in, line);
    ASSERT(!end);
  }
  if (line != "=FunctionCsums") {
    cout << __func__ << " " << __LINE__ << ": " << line << ".\n";
  }
  ASSERT(line == "=FunctionCsums");
  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "=FunctionSummary:")) {
    string function_name = line.substr(strlen("=FunctionSummary:"));
    trim(function_name);
    end = !getline(in, line);
    ASSERT(!end);
    callee_summary_t csum;
    csum.callee_summary_read_from_string(line);
    fcsums.insert(make_pair(function_name, csum));
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=FunctionAttributes");
  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "=FunctionAttributesFor:")) {
    string function_name = line.substr(strlen("=FunctionAttributesFor:"));
    trim(function_name);
    end = !getline(in, line);
    ASSERT(!end);
    fattrs.insert(make_pair(function_name, line));
    end = !getline(in, line);
    ASSERT(!end);
  }
  if (line != "=FunctionLinkage") {
    cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  }
  ASSERT(line == "=FunctionLinkage");
  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "=FunctionLinkageFor:")) {
    string function_name = line.substr(strlen("=FunctionLinkageFor:"));
    trim(function_name);
    end = !getline(in, line);
    ASSERT(!end);
    //cout << __func__ << " " << __LINE__ << ": function_name = " << function_name << ".\n";
    flinkage.insert(make_pair(function_name, line));
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=Attributes");
  end = !getline(in, line);
  ASSERT(!end);
  while (line.size() == 0 || line.at(0) != '=') {
    istringstream iss(line);
    string llvm_fn_attribute_id, attribute_desc;
    char ch;
    iss >> llvm_fn_attribute_id;
    iss >> ch;
    ASSERT(ch == '=');
    size_t eq = line.find('=');
    if (eq == string::npos) {
      cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    }
    ASSERT(eq != string::npos);
    attribute_desc = line.substr(eq + 1);
    trim(attribute_desc);
    attr_descs.insert(make_pair(llvm_fn_attribute_id, attribute_desc));
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=Metadata");
  end = !getline(in, line);
  while (!end) {
    metadata.push_back(line);
    end = !getline(in, line);
  }
  map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> function_tfg_map;
  for (auto & ft : tfgs) {
    callee_summary_t const& csum = fcsums.at(ft.first);
    function_tfg_map.insert(make_pair(ft.first, make_pair(csum, std::move(ft.second))));
  }
  llptfg_t ret(llvm_header, type_decls, globals, function_declarations, function_tfg_map, fsigs, fattrs, flinkage, attr_descs, metadata);
  return ret;
}

}
