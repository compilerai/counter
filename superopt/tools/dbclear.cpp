
#include "db/db.h"
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <memory>
#include <functional>
#include <cassert>
#include "support/c_utils.h"

/*
  tables:
    src_iseqs :: src_iseq_id -> src_iseq_str
    dst_iseqs :: dst_iseq_id -> dst_iseq_str
    live_sets :: live_set_id -> live_set_str
    transmaps :: transmap_id -> transmap_str
    enum_candidates :: enum_candidate_id -> src_iseq_id |X| live_set_id |X| transmap_id
    itable_characteristics :: itable_characteristics_id |X| itable_characteristics_str
    itables :: itable_id -> itable_characteristics_id |X| (array dst_iseq_id) # use an m:n junction table to represent the relationship between itable_id and dst_iseq_ids
    enum_states :: enum_candidate_id x itable_id -> enum_state_str
    peep_entries :: peep_entry_id -> enum_candidate_id |X| dst_iseq_id
*/

using namespace std;

int main(int argc, char **argv)
{
  //unique_ptr<db_t> db = make_unique<db_t>();
  db_t *db = new db_t();
  db->begin_transaction(SUFFIX_FILENAME32(argv[0]));
  db->execute_query_no_results("drop table peep_entries\n");
  db->execute_query_no_results("drop table enum_states\n");
  db->execute_query_no_results("drop table enum_candidates\n");
  db->execute_query_no_results("drop table src_iseqs\n");
  db->execute_query_no_results("drop table live_sets\n");
  db->execute_query_no_results("drop table transmaps\n");
  db->execute_query_no_results("drop table itable_dst_iseq_ids\n");
  db->execute_query_no_results("drop table itables\n");
  db->execute_query_no_results("drop table itable_characteristics\n");
  db->execute_query_no_results("drop table dst_iseqs\n");
  db->commit_transaction(SUFFIX_FILENAME32(argv[0]));

  delete db;
  return 0;
}
