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
  //stringstream ss;
  //db->execute_query("SELECT * from superoptdb.SRC_ISEQS", ss);
  //cout << ss.str() << endl;
  db->begin_transaction(SUFFIX_FILENAME32(argv[0]));
  db->execute_query_no_results(
      "CREATE TABLE src_iseqs\n"
      "(\n"
      "SRC_ISEQ_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "SRC_ISEQ_STR  NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE dst_iseqs\n"
      "(\n"
      "DST_ISEQ_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "DST_ISEQ_STR  NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE live_sets\n"
      "(\n"
      "LIVE_SET_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "LIVE_SET_STR  NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE transmaps\n"
      "(\n"
      "TRANSMAPS_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "TRANSMAPS_STR  NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE enum_candidates\n"
      "(\n"
      "ENUM_CANDIDATE_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "SRC_ISEQ_ID BIGINT REFERENCES src_iseqs (SRC_ISEQ_ID),\n"
      "LIVE_SET_ID BIGINT REFERENCES live_sets (LIVE_SET_ID),\n"
      "TRANSMAPS_ID BIGINT REFERENCES transmaps (TRANSMAPS_ID),\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE itable_characteristics\n"
      "(\n"
      "ITABLE_CHARACTERISTICS_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "ITABLE_CHARACTERISTICS_STR NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE itables\n"
      "(\n"
      "ITABLE_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "ITABLE_CHARACTERISTICS_ID BIGINT REFERENCES itable_characteristics (ITABLE_CHARACTERISTICS_ID),\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE itable_dst_iseq_ids\n"
      "(\n"
      "ITABLE_ID BIGINT REFERENCES itables (ITABLE_ID),\n"
      "DST_ISEQ_ID BIGINT REFERENCES dst_iseqs (DST_ISEQ_ID),\n"
      "PRIMARY KEY(ITABLE_ID, DST_ISEQ_ID),\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE enum_states\n"
      "(\n"
      "ENUM_CANDIDATE_ID BIGINT REFERENCES enum_candidates (ENUM_CANDIDATE_ID),\n"
      "ITABLE_ID BIGINT REFERENCES itables (ITABLE_ID),\n"
      "ENUM_STATE_STR NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      "PRIMARY KEY(ENUM_CANDIDATE_ID, ITABLE_ID),\n"
      ")\n"
  );
  db->execute_query_no_results(
      "CREATE TABLE peep_entries\n"
      "(\n"
      "PEEP_ENTRY_ID BIGINT IDENTITY PRIMARY KEY,\n"
      "ENUM_CANDIDATE_ID BIGINT REFERENCES enum_candidates (ENUM_CANDIDATE_ID),\n"
      "DST_ISEQ_ID BIGINT REFERENCES dst_iseqs (DST_ISEQ_ID),\n"
      "TIMESTAMP NVARCHAR(MAX) collate Latin1_General_CS_AS NOT NULL,\n"
      ")\n"
  );
  db->commit_transaction(SUFFIX_FILENAME32(argv[0]));
  delete db;
  return 0;
}
