#include <iostream>
#include <fstream>

#include "tfg/parse_input_eq_file.h"
#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"
#include "expr/consts_struct.h"
#include "support/cl.h"

using namespace std;

static void
skip_till_next_function(ifstream &in)
{
  string line;
  do {
    if (!getline(in, line)) {
      cout << __func__ << " " << __LINE__ << ": reached end of file\n";
      return;
    }
  } while (!string_has_prefix(line, FUNCTION_FINISH_IDENTIFIER));
  return;
}

int main(int argc, char **argv)
{
  cl::cl cmd("etfg2dot: .etfg file to .dot file converter");

  cl::arg<string> function_name(cl::implicit_prefix, "", "function name");
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> dot_path(cl::implicit_prefix, "", "output path of .dot file");

  cmd.add_arg(&function_name);
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&dot_path);
  cmd.parse(argc, argv);

  string filename_llvm = etfg_file.get_value();

  context::config cfg(0, 0);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);

  ifstream in_llvm(filename_llvm);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    return 1;
  }

  string line;
  getline(in_llvm, line);
  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + function_name.get_value());
  while (!is_line(line, match_line)) {
    bool end = !getline(in_llvm, line);
    if (end) {
      cout << __func__ << " " << __LINE__ << ": function " << function_name.get_value() << " not found in llvm file " << filename_llvm << endl;
      return 2;
    }
  }
  if(!is_next_line(in_llvm, "=TFG")) {
    cout << __func__ << " " << __LINE__ << ": parsing failed\n";
    return 3;
  }
  auto tfg_llvm = make_shared<tfg_llvm_t>(in_llvm, function_name.get_value(), &ctx);
  ASSERT(tfg_llvm);
  tfg_llvm->rename_llvm_symbols_to_keywords(&ctx);
  tfg_llvm->replace_alloca_with_nop();
  tfg_llvm->collapse_tfg(true);
  tfg_llvm->to_dot(dot_path.get_value(), false);

  return 0;
}
