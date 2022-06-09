#include <iostream>
#include <fstream>

#include "support/cl.h"
#include "tfg/tfg_asm.h"
#include "tfg/tfg_llvm.h"
#include "eq/parse_input_eq_file.h"

using namespace std;

// "peek" next line
// get the next line without modifying current position
void peekline(istream& in, string& line)
{
  streampos oldpos = in.tellg(); // save current position
  getline(in, line);
  in.seekg(oldpos); // restore the position
}

dshared_ptr<tfg>
parse_tfg(istream& in, context* ctx, bool& is_dst)
{
  string line;
  peekline(in, line);
  string tfg_llvm_prefix = TFG_LLVM_IDENTIFIER " ";
  if (is_line(line, tfg_llvm_prefix)) {
    is_dst = false;
    getline(in, line);
    size_t colon = line.find(':');
    ASSERT(colon != string::npos);
    string tfg_name = line.substr(tfg_llvm_prefix.size(), colon - tfg_llvm_prefix.size());
    auto tfg_llvm = tfg_llvm_t::tfg_llvm_from_stream(in, tfg_name, ctx);
    ASSERT(tfg_llvm);
    return dynamic_pointer_cast<tfg>(tfg_llvm);
  }
  else if (is_line(line, "=Rodata-map")) {
    is_dst = true;
    dst_init();
    dst_parsed_tfg_t dst_parsed_tfg = dst_parsed_tfg_t::parse_dst_asm_tfg_for_eqcheck("dst", in, ctx);
    return dynamic_pointer_cast<tfg>(dst_parsed_tfg.get_tfg()->get_ssa_tfg());
  }
  else {
    cout << _FNLN_ << ": Unexpected line: " << line << endl;
    return dshared_ptr<tfg>::dshared_nullptr();
  }
}

int main(int argc, char **argv)
{
  cl::cl cmd("tfg2dot: .tfg file to .dot file converter");

  cl::arg<bool> collapse(cl::explicit_flag, "collapse", true, "collapse TFG");
  cl::arg<string> function_name(cl::implicit_prefix, "", "function name");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::arg<string> dot_path(cl::implicit_prefix, "", "", "output path of .dot file");

  cmd.add_arg(&collapse);
  cmd.add_arg(&function_name);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&dot_path);
  cmd.parse(argc, argv);

  context::config cfg(0, 0);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  string filename = tfg_file.get_value();
  ifstream in(filename);
  if (!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << filename << "'\n";
    return 1;
  }
  ASSERT(in.is_open());

  string line;
  string cur_function_name = function_name.get_value();
  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + cur_function_name);
  while (!is_line(line, match_line)) {
    bool end = !getline(in, line);
    if (end) {
      cout << _FNLN_ << ": function " << cur_function_name << " not found in llvm file " << filename << endl;
      return 2;
    }
  }

  bool is_dst;
  dshared_ptr<tfg> tfg_in = parse_tfg(in, &ctx, is_dst);
  if (!tfg_in) {
    cout << _FNLN_ << ": parsing failed!" << endl;
    return 3;
  }
  ASSERT(tfg_in);

  if (collapse.get_value()) {
    tfg_in->collapse_tfg();
  }

  string filename_dot = dot_path.get_value();
  if (filename_dot.empty()) {
    ostringstream ss;
    ss << function_name.get_value() << (!collapse.get_value() ? ".uncollapsed" : "") << (is_dst ? ".dst" : "") << ".dot";
    filename_dot = ss.str();
  }
  tfg_in->to_dot(filename_dot, false);

  return 0;
}
