#include <glob.h>

#include "parser/parse_tree_z3.h"
#include "support/utils.h"
#include "support/dyn_debug.h"
#include "support/cl.h"

vector<string>
glob(string const& pattern)
{
  glob_t glob_result;
  memset(&glob_result, 0, sizeof(glob_result));

  int ret = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
  if (ret != 0) {
    globfree(&glob_result);
    cerr << "glob() failed with error code " << ret << endl;
    NOT_IMPLEMENTED();
  }
  vector<string> fnames;
  fnames.reserve(glob_result.gl_pathc);
  for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
    fnames.push_back(string(glob_result.gl_pathv[i]));
  }
  globfree(&glob_result);
  return fnames;
}

int main(int argc,char** argv)
{
  cl::arg<string> input_glob(cl::implicit_prefix, "", "glob to input files");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<bool> debug_output(cl::explicit_flag, "debug-output", false, "Generate extra debug output.");
  cl::arg<bool> progress(cl::explicit_flag, "progress", false, "Print progress");

  cl::cl cmd("Z3 CE Parser");
  cmd.add_arg(&input_glob);
  cmd.add_arg(&debug);
  cmd.add_arg(&debug_output);
  cmd.add_arg(&progress);
  cmd.parse(argc, argv);

  g_ctx_init();

  assert(g_ctx);
  init_dyn_debug_from_string(debug.get_value());

  stats::get().clear();
  progress_flag = progress.get_value();

  for (auto const& fname : glob(input_glob.get_value())) {
    cout << "== Filename: " << fname << endl << endl;
    map<string_ref,expr_ref> r = parse_z3_model_neo(g_ctx, fname, /*debug*/debug_output.get_value());
    for (auto const& [name,e] : r) {
      cout << name->get_str() << "[" << e->get_sort()->to_string() << "] = " << expr_string(e) << endl;
    }
  }

  print_all_timers();
  cout << "stats:\n" << stats::get();
  return 0;
}
