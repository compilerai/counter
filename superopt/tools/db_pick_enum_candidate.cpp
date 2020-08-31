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
#include "rewrite/transmap.h"

using namespace std;
static char as1[4096000];

int main(int argc, char **argv)
{
  db_t db;
  cl::cl cmd("db_pick_enum_candidates");
  cmd.parse(argc, argv);

  size_t max_tries = 2000;
  string src_iseq_str, live_sets_str, transmaps_str;
  transmap_t in_tmap;
  long max_out_tmaps = 512;
  transmap_t *out_tmaps = new transmap_t[max_out_tmaps];
  long num_out_tmaps = 0;
  size_t i;
  db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
  for (i = 0; i < max_tries; i++) {
    db.pick_random_enum_candidate(src_iseq_str, live_sets_str, transmaps_str);
    transmaps_from_string(&in_tmap, out_tmaps, max_out_tmaps, transmaps_str.c_str(), &num_out_tmaps);
    transmaps_canonicalize(in_tmap, out_tmaps, num_out_tmaps);
    transmaps_to_string(&in_tmap, out_tmaps, num_out_tmaps, as1, sizeof as1);
    if (!db.peep_entry_exists_for_enum_candidate(src_iseq_str, live_sets_str, transmaps_str)) {
      break;
    }
  }
  db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  ASSERT(i < max_tries);
  cout << src_iseq_str << "--\n" << live_sets_str << "--\n" << transmaps_str << "==\n";
  delete [] out_tmaps;
  return 0;
}
