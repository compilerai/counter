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
  cl::arg<string> itable_filename(cl::implicit_prefix, "", "filename with itables");
  cl::arg<string> ipos_filename(cl::implicit_prefix, "", "filename with ipos (enum_state)");
  cl::cl cmd("db_update_itable_enum_state_for_enum_candidate");
  cmd.add_arg(&enum_candidates_filename);
  cmd.add_arg(&itable_filename);
  cmd.add_arg(&ipos_filename);
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
  read_itables_from_file(itable_filename.get_value(), ipositions, itables);
  ASSERT(itables.size() == 1);
  itable_t const &itable = itables.at(0);
  string itable_string = itable_to_string(&itable, as1, sizeof as1);

  ifstream ipos_fp(ipos_filename.get_value());
  std::string ipos_string((std::istreambuf_iterator<char>(ipos_fp)),
                        std::istreambuf_iterator<char>());

  string itab_char_str = itable.get_itable_characteristics().itable_characteristics_to_string();
  vector<string> itab_dst_insns;
  for (const auto &entry : itable.get_entries()) {
    itab_dst_insns.push_back(entry.get_assembly());
  }
  //cout << __func__ << " " << __LINE__ << ": itable.get_entries() = " << itable.get_num_entries() << endl;
  //cout << __func__ << " " << __LINE__ << ": itab_dst_insns.size() = " << itab_dst_insns.size() << endl;
  auto itable_string_pair = make_pair(itab_char_str, itab_dst_insns);
  db.begin_transaction(SUFFIX_FILENAME32(argv[0]));
  db.db_update_itable_ipos_for_enum_candidate(src_iseq_str, live_sets_str, transmaps_str, itable_string_pair, ipos_string);
  db.commit_transaction(SUFFIX_FILENAME32(argv[0]));
  return 0;
}
