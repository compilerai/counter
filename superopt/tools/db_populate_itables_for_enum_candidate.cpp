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
  cl::arg<string> enum_candidates_filename(cl::implicit_prefix, "", "filename with enum candidate");
  cl::arg<string> itables_filename(cl::implicit_prefix, "", "filename with itables");
  cl::cl cmd("db_populate_itables_for_enum_candidate");
  cmd.add_arg(&enum_candidates_filename);
  cmd.add_arg(&itables_filename);
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
  infile.open(enum_candidates_filename.get_value());
  string src_iseq_str = read_section(infile, "--");
  ASSERT(src_iseq_str != "");
  string live_sets_str = read_section(infile, "--");
  //cout << "src_iseq_str = '" << src_iseq_str << "'" << endl;
  ASSERT(live_sets_str != "");
  string transmaps_str = read_section(infile, "==");
  ASSERT(transmaps_str != "");
  infile.close();

  vector<itable_t> itables;
  vector<itable_position_t> ipositions;
  read_itables_from_file(itables_filename.get_value(), ipositions, itables);
  vector<pair<string, vector<string>>> itable_strings;
  for (const auto &itable : itables) {
    string itab_char_str = itable.get_itable_characteristics().itable_characteristics_to_string();
    vector<string> itab_dst_insns;
    for (const auto &entry : itable.get_entries()) {
      itab_dst_insns.push_back(entry.get_assembly());
    }
    itable_strings.push_back(make_pair(itab_char_str, itab_dst_insns));
  }
  db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
  db.populate_itables_for_enum_candidate(src_iseq_str, live_sets_str, transmaps_str, itable_strings, itable_position_to_string(itable_position_zero(), as1, sizeof as1));
  db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  return 0;
}
