#include "db/db.h"
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <memory>
#include <functional>
#include <cassert>
#include "support/cl.h"
#include "support/debug.h"
#include "support/utils.h"
#include "superopt/itable.h"
#include "superopt/itable_position.h"

using namespace std;
static char as1[4096000];

int
main(int argc, char **argv)
{
  db_t db;
  cl::arg<string> output_filename(cl::implicit_prefix, "o", "out.peeptab", "output filename with peep entries (peeptab)");
  cl::cl cmd("db_download_peeptab");
  cmd.add_arg(&output_filename);
  cmd.parse(argc, argv);

  init_timers();

  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()
  //solver_init(); //this also involves a fork and should be as early as possible
  //g_query_dir_init();
  //src_init();
  dst_init();
  //g_se_init();
  //types_init();
  usedef_init();
  //types_init();

  db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
  ofstream ofs(output_filename.get_value());
  db.db_download_peep_entries(ofs);
  ofs.close();
  db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  return 0;
}
