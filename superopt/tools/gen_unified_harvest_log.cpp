#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "expr/z3_solver.h"
#include "support/timers.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/optflags.h"
#include "support/c_utils.h"
#include "valtag/elf/elf.h"
#include "valtag/symbol.h"
#include "superopt/harvest.h"

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#define DEBUG_TYPE "main"

using namespace std;
namespace fs = boost::filesystem;

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);

  cl::cl cmd("gen_unified_harvest_log : Generate a unified harvest log from individual harvest logs (for each split module)");
  // command line args processing
  cl::arg<string> harvest_logs_prefix(cl::implicit_prefix, "", "path prefix to individual harvest logs (e.g., /tmp/qcc-files/a-820dd4.bc.split. for /tmp/qcc-files/a-820dd4.bc.split.[012...]");
  cmd.add_arg(&harvest_logs_prefix);

  for (int i = 0; i < argc; i++) {
    cout << " " << argv[i];
  }
  cout << endl;
  cmd.parse(argc, argv);

  vector<harvest_log_t> harvest_logs;
  
  boost::filesystem::path prefixpath(harvest_logs_prefix.get_value());
  boost::filesystem::path dir = prefixpath.parent_path();
  string prefixname = prefixpath.filename().native();
  for (const auto & entry : fs::directory_iterator(dir)) {
    //std::cout << entry.path() << std::endl;
    auto pth = entry.path();
    string fname = pth.filename().native();
    if (!string_has_prefix(fname, prefixname)) {
      continue;
    }
    if (!string_has_suffix(fname, ".harvest.log")) {
      continue;
    }
    string pthstr = pth.native();
    cout << "found: " << pthstr << endl;
    harvest_logs.push_back(pthstr);
  }

  string harvest_log = "unified.harvest.log";
  gen_unified_harvest_log(harvest_logs, harvest_log);

  return 0;
}
