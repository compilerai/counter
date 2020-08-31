#include <iostream>
#include <fstream>

#include "tfg/parse_input_eq_file.h"
#include "tfg/tfg.h"
#include "tfg/tfg_asm.h"
#include "graph/prove.h"
#include "gsupport/rodata_map.h"
#include "rewrite/dst-insn.h"
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
  cl::cl cmd("tfg2dot: .tfg file to .dot file converter");

  cl::arg<string> function_name(cl::implicit_prefix, "", "function name");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::arg<string> dot_path(cl::implicit_prefix, "", "output path of .dot file");

  cmd.add_arg(&function_name);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&dot_path);
  cmd.parse(argc, argv);

  context::config cfg(0, 0);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  string filename_dst = tfg_file.get_value();
  ifstream in_dst(filename_dst);
  if (!in_dst.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << filename_dst << "'\n";
  }
  ASSERT(in_dst.is_open());

  string line;
  string cur_function_name;
  bool found_function = false;
  while (getNextFunctionInTfgFile(in_dst, cur_function_name)) {
    if (function_name.get_value() != cur_function_name) {
      skip_till_next_function(in_dst);
    }
    else {
      found_function = true;
      break;
    }
  }
  if (!found_function)
    return 1;

  if (!is_next_line(in_dst, "=Rodata-map")) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    NOT_REACHED();
    return 2;
  }
  auto rodata_map = rodata_map_t(in_dst);

  bool end = !getline(in_dst, line);
  ASSERT(!end);
  ASSERT(line == "=dst_iseq");
  while (!!getline(in_dst, line) && line.at(0) != '=');
  ASSERT(line == "=dst_iseq done");

  end = !getline(in_dst, line);
  ASSERT(!end);
  ASSERT(line == "=dst_insn_pcs");
  while (!!getline(in_dst, line) && line.at(0) != '=');
  ASSERT(line == "=dst_insn_pcs done");

  end = !getline(in_dst, line);
  ASSERT(!end);
  shared_ptr<tfg> tfg_dst = make_shared<tfg_asm_t>(in_dst, "dst", &ctx);
  tfg_dst->collapse_tfg(true/*false*/);
  tfg_dst->to_dot(dot_path.get_value(), false);

  return 0;
}
