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
  cl::arg<string> peeptab_filename(cl::implicit_prefix, "", "filename with peep entries (peeptab filename)");
  cl::cl cmd("db_add_peep_entry");
  cmd.add_arg(&peeptab_filename);
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

  ifstream infile;
  infile.open(peeptab_filename.get_value());
  string line;

  while (getline(infile, line) && line.find("ENTRY") != string::npos) {
    string src_iseq_str = read_section(infile, "--");
    ASSERT(src_iseq_str != "");
    string live_sets_str = read_section(infile, "--");
    //cout << "src_iseq_str = '" << src_iseq_str << "'" << endl;
    ASSERT(live_sets_str != "");
    string transmaps_str = read_section(infile, "--");
    ASSERT(transmaps_str != "");
    string fixed_reg_mappings_str = read_section(infile, "--");
    ASSERT(fixed_reg_mappings_str != "");
    string dst_iseq_str = read_section(infile, "==");
    ASSERT(dst_iseq_str != "");
    infile.close();
    fixed_reg_mapping_t fixed_reg_mapping;
    fixed_reg_mapping.fixed_reg_mapping_from_string(fixed_reg_mappings_str);
    if (!fixed_reg_mapping.fixed_reg_mapping_agrees_with(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end())) {
      NOT_REACHED(); //we do not store fixed-reg-mappings in the DB; just assert that they are in canonical form
    }
    db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
    db.db_add_peep_entry(src_iseq_str, live_sets_str, transmaps_str, dst_iseq_str);
    db.commit_transaction(SUFFIX_FILENAME32(argv[0])) ;
  }
  return 0;
}
