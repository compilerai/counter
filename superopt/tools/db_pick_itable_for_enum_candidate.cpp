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
#include "rewrite/transmap.h"
#include "superopt/itable.h"
#include "superopt/itable_position.h"

using namespace std;
static char as1[4096000];

static bool
itable_is_more_promising(itable_t const &itable1, itable_position_t const &ipos1, itable_t const &itable2, itable_position_t const &ipos2)
{
  if (itable1.get_num_entries() < itable2.get_num_entries()) {
    return true;
  }
  typesystem_t const &typesystem1 = itable1.get_typesystem();
  typesystem_t const &typesystem2 = itable2.get_typesystem();
  return typesystem1.get_use_types() < typesystem2.get_use_types() || typesystem2.typesystem_dominates(typesystem1);
}

static pair<itable_t, itable_position_t>
pick_most_promising_itable(vector<itable_t> const &itables, vector<itable_position_t> const &itable_positions)
{
  size_t winner = -1;
  for (size_t i = 0; i < itables.size(); i++) {
    if (   winner == -1
        || itable_is_more_promising(itables.at(i), itable_positions.at(i), itables.at(winner), itable_positions.at(winner))) {
      winner = i;
    }
  }
  ASSERT(winner != -1);
  return make_pair(itables.at(winner), itable_positions.at(winner));
}

int main(int argc, char **argv)
{
  db_t db;
  cl::arg<string> enum_candidate_filename(cl::implicit_prefix, "", "filename with src iseq, live out, and tmaps");
  cl::cl cmd("db_pick_itable_for_enum_candidate");
  cmd.add_arg(&enum_candidate_filename);
  cmd.parse(argc, argv);
  init_timers();
  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()
  dst_init();
  usedef_init();

  ifstream infile;
  infile.open(enum_candidate_filename.get_value());
  string src_iseq_str = read_section(infile, "--");
  ASSERT(src_iseq_str != "");
  string live_sets_str = read_section(infile, "--");
  //cout << "src_iseq_str = '" << src_iseq_str << "'" << endl;
  ASSERT(live_sets_str != "");
  string transmaps_str = read_section(infile, "==");
  ASSERT(transmaps_str != "");
  infile.close();


  db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
  vector<string> itable_strings = db.get_itables_for_enum_candidate(src_iseq_str, live_sets_str, transmaps_str);
  db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  vector<itable_t> itables;
  vector<itable_position_t> itable_positions;
  for (auto itable_string : itable_strings) {
    //cout << __func__ << " " << __LINE__ << ": itable_string =\n" << itable_string << endl;
    FILE *ifp = fmemopen((void *)itable_string.c_str(), itable_string.size(), "r");
    ASSERT(ifp);
    itable_position_t ipos = itable_position_from_stream(ifp);
    //cout << __func__ << " " << __LINE__ << ": ipos = " << itable_position_to_string(&ipos, as1, sizeof as1) << endl;
    itable_t itable = itable_from_stream(ifp);
    fclose(ifp);
    itables.push_back(itable);
    itable_positions.push_back(ipos);
  }
  pair<itable_t, itable_position_t> pick = pick_most_promising_itable(itables, itable_positions);
  cout << "itable 0:\noptimal_found : false\n";
  cout << "max_vector: " << itable_position_to_string(&pick.second, as1, sizeof as1) << endl << "==\n";
  cout << itable_to_string(&pick.first, as1, sizeof as1);
  return 0;
}
