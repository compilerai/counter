#include "support/debug.h"
#include "support/cmd_helper.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "rewrite/static_translate.h"

#include "support/timers.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "expr/z3_solver.h"

#include <fstream>

#include <iostream>
#include <string>

using namespace std;

void dst_free();

int
main(int argc, char **argv)
{
  // command line args processing
  cl::cl cmd("Generate the debug version of this program that can be used to debug the output produced by codegen");
  cl::arg<string> bcfile(cl::implicit_prefix, "", "path to .bc file");
  cmd.add_arg(&bcfile);
  cl::arg<string> output_file(cl::explicit_prefix, "o", "d.out", "path to output object file");
  cmd.add_arg(&output_file);
  cmd.parse(argc, argv);

  string bcfilename = bcfile.get_value();
  string output_filename = output_file.get_value();
  string instrumented_filename = bcfilename + ".instrumented";
  string object_filename = output_filename + ".o";

  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/opt -load " << SUPEROPT_INSTALL_DIR << "/lib/LLVMLockstep.so " << bcfilename << " -lockstep -o " << instrumented_filename;
  cout << "Executing command: '" << ss.str() << "'\n";
  cout.flush();
  xsystem(ss.str().c_str());

  ss.str("");
  ss << SUPEROPT_INSTALL_DIR << "/bin/llc -O3 " << instrumented_filename << " -filetype=obj -o " << object_filename;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());

  ss.str("");
  ss << "/usr/bin/c++ -m32 -O3 " << object_filename << " " << SUPEROPT_INSTALL_DIR << "/lib/libLockstepDbg.a " << SUPEROPT_INSTALL_DIR << "/lib/libmymalloc.a -o " << output_filename;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());

  return 0;
}
