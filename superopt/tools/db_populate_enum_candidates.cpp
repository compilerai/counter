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

using namespace std;

int main(int argc, char **argv)
{
  db_t db;
  cl::arg<string> enum_candidates_filename(cl::implicit_prefix, "", "filename with enum candidates");
  cl::cl cmd("db_populate_enum_candidates");
  cmd.add_arg(&enum_candidates_filename);
  cmd.parse(argc, argv);

  ifstream infile;
  infile.open(enum_candidates_filename.get_value());

  while (1) {
    string src_iseq_str = read_section(infile, "--");
    if (src_iseq_str == "") {
      break;
    }
    string live_sets_str = read_section(infile, "--");
    //cout << "src_iseq_str = '" << src_iseq_str << "'" << endl;
    ASSERT(live_sets_str != "");
    string transmaps_str = read_section(infile, "==");
    ASSERT(transmaps_str != "");
    db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
    db.add_enum_candidate_if_does_not_exist(src_iseq_str, live_sets_str, transmaps_str);
    db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  }
  infile.close();
  return 0;
}
