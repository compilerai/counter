#include "db/db.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <ctpublic.h>
#include <list>
//#include "tds++/tdspp.hh"
#include "support/debug.h"
#include "support/utils.h"

#define SERVERNAME "superoptdb.database.windows.net"
#define SERVERPORT "1433"
#define USERNAME "sbansal"
#define PASSWORD "super0ptDB"
#define DBNAME "superoptdb"

#define db_error(str) do { \
  cout << __func__ << " " << __LINE__ << ": " << str << endl; \
  NOT_REACHED(); \
} while(0)

using namespace std;

int db_t::m_num_connections = 0;
char as1[4096];

db_t::db_t()
{
  ASSERT(m_num_connections == 0);
  LOGINREC *login;
  RETCODE erc;
  erc = dbinit();
  if (erc == FAIL) {
    db_error("dbinit() failed");
  }
  login = dblogin();
  DBSETLAPP(login, "superoptdb-client");
  DBSETLUSER(login, USERNAME);
  DBSETLHOST(login, "worker1.centralindia.cloudapp.azure.com");
  DBSETLPWD(login, PASSWORD);
  DBSETLDBNAME(login, DBNAME);
  if ((dbproc = dbopen(login, SERVERNAME)) == NULL) {
    db_error("dbopen() failed");
  }
  m_num_connections++;
}

void
db_t::submit_query(string const &query_str)
{
  RETCODE erc;
  dbfreebuf(dbproc);
  erc = dbcmd(dbproc, query_str.c_str());
  if (erc == FAIL) {
    cout << query_str << endl;
    db_error("query failed\n");
  }
  erc = dbsqlsend(dbproc);
  if (erc == FAIL) {
    cout << query_str << endl;
    db_error("query send failed\n");
  }
  erc = dbsqlok(dbproc);
  if (erc == FAIL) {
    cout << query_str << endl;
    db_error("dbsqlok() failed\n");
  }
}

void
db_t::execute_query(string const &query_str, stringstream &ss)
{
  RETCODE erc;
  this->submit_query(query_str);
  int row_code;
  int iresultset;
  int id;
  char str[512];

  /*
   * Set up each result set with dbresults()
   * This is more commonly implemented as a while() loop, but we're counting the result sets.
   */
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&id);
    dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *)str);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ss << id << ": " << str << endl;
    }
  }
  //ss << iresultset << " results\n";
}

void
db_t::execute_query_no_results(string const &query_str)
{
  this->submit_query(query_str);
}

db_t::db_id_t
db_t::entry_exists(char const *table, char const *field, string const &value)
{
  stringstream query;
  RETCODE erc;
  query << "SELECT * from " << table << " WHERE " << field << " = '" << value << "'\n";
  this->submit_query(query.str());
  int row_code;
  int iresultset;
  int num_results = 0;
  db_id_t id;

  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, BIGINTBIND, 0, (BYTE *)&id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only on result
      num_results++;
    }
  }
  if (num_results == 0) {
    return DB_ID_INVALID;
  }
  ASSERT(num_results == 1);
  return id;
}

db_t::db_id_t
db_t::insert_if_does_not_exist(char const *table, char const *field, string const &value)
{
  stringstream query;
  db_id_t id;
  id = this->entry_exists(table, field, value);
  if (id != DB_ID_INVALID) {
    return id;
  }
  query << "INSERT INTO " << table << "(" << field << ")\n"
      "VALUES ('" << value << "')\n";
  this->execute_query_no_results(query.str());
  id = this->entry_exists(table, field, value);
  ASSERT(id != DB_ID_INVALID);
  return id;
}

string
db_t::get_str_value_for_id(char const *table, char const *fieldname, db_id_t id)
{
  ASSERT(id != DB_ID_INVALID);
  stringstream query, ss;
  query << "SELECT * FROM " << table << " WHERE " << fieldname << " = " << id << "\n";
  RETCODE erc;
  this->submit_query(query.str());
  int row_code;
  int iresultset;
  size_t max_alloc = 409600;
  char *str = new char[max_alloc];

  /*
   * Set up each result set with dbresults()
   * This is more commonly implemented as a while() loop, but we're counting the result sets.
   */
  int num_results = 0;
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&id);
    dbbind(dbproc, 2, STRINGBIND, 0, (BYTE *)str);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      //ss << id << ": " << str << endl;
      num_results++;
    }
  }
  if (num_results != 1) {
    cout << __func__ << " " << __LINE__ << ": num_results = " << num_results << " for " << query.str() << endl;
  }
  ASSERT(num_results == 1);
  string ret = str;
  delete[] str;
  return ret;
}

bool
db_t::enum_candidate_exists(db_id_t src_iseq_id, db_id_t live_set_id, db_id_t transmaps_id, db_id_t &enum_candidate_id)
{
  stringstream query;
  RETCODE erc;
  query << "SELECT * from enum_candidates WHERE SRC_ISEQ_ID = " << src_iseq_id << " AND LIVE_SET_ID = " << live_set_id << " AND TRANSMAPS_ID = " << transmaps_id << "\n";
  this->submit_query(query.str());
  int row_code;
  int iresultset;

  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&enum_candidate_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      return true;
    }
  }
  return false;
}

db_t::db_id_t
db_t::add_enum_candidate_if_does_not_exist(string const &src_iseq_str, string const &live_sets_str, string const &transmaps_str)
{
  db_id_t src_iseq_id = insert_if_does_not_exist("src_iseqs", "SRC_ISEQ_STR", src_iseq_str);
  db_id_t live_sets_id = insert_if_does_not_exist("live_sets", "LIVE_SET_STR", live_sets_str);
  db_id_t transmaps_id = insert_if_does_not_exist("transmaps", "TRANSMAPS_STR", transmaps_str);
  stringstream query;

  db_id_t enum_candidate_id = DB_ID_INVALID;
  if (enum_candidate_exists(src_iseq_id, live_sets_id, transmaps_id, enum_candidate_id)) {
    return enum_candidate_id;
  }

  query << "INSERT INTO enum_candidates (SRC_ISEQ_ID, LIVE_SET_ID, TRANSMAPS_ID)\n"
     "VALUES (" << src_iseq_id << ", " << live_sets_id << ", " << transmaps_id << ")\n";
  this->execute_query_no_results(query.str());

  enum_candidate_id = DB_ID_INVALID;
  bool exists = enum_candidate_exists(src_iseq_id, live_sets_id, transmaps_id, enum_candidate_id);
  ASSERT(exists);
  return enum_candidate_id;
}

void
db_t::output_id_list(stringstream &ss, list<db_id_t> const &ids)
{
  ASSERT(ids.size() > 0);
  size_t id_num = 0;
  ss << "(";
  for (auto id : ids) {
    ss << (id_num == 0 ? ' ' : ',') << id;
    id_num++;
  }
  ss << " )\n";
}

bool
db_t::itable_exists(db_id_t characteristics_id, list<db_id_t> const &dst_iseq_ids, db_id_t &itable_id)
{
  stringstream query;
  if (dst_iseq_ids.size() > 0) {
    query << "SELECT itables.ITABLE_ID FROM (itables JOIN itable_dst_iseq_ids ON itables.ITABLE_ID = itable_dst_iseq_ids.ITABLE_ID) WHERE (itable_characteristics_id = " << characteristics_id;
    query << " AND DST_ISEQ_ID IN ";
    output_id_list(query, dst_iseq_ids);
    query << ") EXCEPT (SELECT ITABLE_ID FROM itable_dst_iseq_ids WHERE NOT (DST_ISEQ_ID IN ";
    output_id_list(query, dst_iseq_ids);
    query << "))\n";
  } else {
    query << "SELECT ITABLE_ID FROM itables WHERE (itable_characteristics_id = " << characteristics_id << ") EXCEPT (SELECT ITABLE_ID FROM itable_dst_iseq_ids)\n";
  }
  //cout << __func__ << " " << __LINE__ << ": query = " << query.str() << endl;
  RETCODE erc;
  this->submit_query(query.str());
  int row_code;
  int iresultset;
  db_id_t id = DB_ID_INVALID;

  list<db_id_t> itable_ids;
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(itable_ids.size() == 0); //we expect only one result
      itable_ids.push_back(id);
    }
  }
  ASSERT(itable_ids.size() <= 1);
  if (itable_ids.size() == 0) {
    return false;
  }
  itable_id = *itable_ids.begin();
  return true;
}

db_t::db_id_t
db_t::add_itable(db_id_t characteristics_id, list<db_id_t> const &dst_iseq_ids)
{
  stringstream query;
  RETCODE erc;
  int row_code;
  int iresultset, num_results = 0;
  db_id_t itable_id = DB_ID_INVALID;

  query << "INSERT INTO itables (ITABLE_CHARACTERISTICS_ID) OUTPUT INSERTED.ITABLE_ID VALUES (" << characteristics_id << ")\n";
  this->submit_query(query.str());
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&itable_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      num_results++;
    }
  }
  ASSERT(num_results == 1);
  ASSERT(itable_id != DB_ID_INVALID);
  for (auto dst_iseq_id : dst_iseq_ids) {
    query.str(std::string()); //clear the stringstream
    query << "SELECT * FROM itable_dst_iseq_ids WHERE ITABLE_ID = " << itable_id << " AND DST_ISEQ_ID = " <<  dst_iseq_id << "\n";
    if (this->count_results(query.str())) {
      continue;
    }

    query.str(std::string()); //clear the stringstream
    query << "INSERT INTO itable_dst_iseq_ids (ITABLE_ID, DST_ISEQ_ID) VALUES (" << itable_id << ", " << dst_iseq_id << ")\n";
    this->execute_query_no_results(query.str());
  }
  return itable_id;
}

void
db_t::add_enum_state_mapping_if_does_not_exist(db_id_t enum_candidate_id, db_id_t itable_id, string const &enum_state_str)
{
  stringstream query;
  RETCODE erc;
  query << "SELECT ENUM_STATE_STR FROM enum_states WHERE (ENUM_CANDIDATE_ID=" << enum_candidate_id << " AND ITABLE_ID=" << itable_id << ")\n";
  if (count_results(query.str()) > 0) {
    return;
  }
  query.str(string());//clear query
  query << "INSERT INTO enum_states (ENUM_CANDIDATE_ID, ITABLE_ID, ENUM_STATE_STR) VALUES (" << enum_candidate_id << "," << itable_id << ",'" << enum_state_str << "')\n";
  this->execute_query_no_results(query.str());
}

void
db_t::populate_itables_for_enum_candidate(string const &src_iseq_str, string const &live_sets_str, string const &transmaps_str, vector<pair<std::string, vector<std::string>>> const &itable_strings, string const &enum_state_str)
{
  db_id_t enum_candidate_id = this->add_enum_candidate_if_does_not_exist(src_iseq_str, live_sets_str, transmaps_str);
  for (const auto &itable_string : itable_strings) {
    string const &itable_characteristics = itable_string.first;
    vector<string> const &entries = itable_string.second;
    db_id_t itable_characteristics_id = insert_if_does_not_exist("itable_characteristics", "ITABLE_CHARACTERISTICS_STR", itable_characteristics);
    list<db_id_t> dst_iseq_ids;
    for (const auto &entry : entries) {
      db_id_t dst_iseq_id = insert_if_does_not_exist("dst_iseqs", "DST_ISEQ_STR", entry);
      dst_iseq_ids.push_back(dst_iseq_id);
    }
    db_id_t itable_id = DB_ID_INVALID;
    if (!this->itable_exists(itable_characteristics_id, dst_iseq_ids, itable_id)) {
      itable_id = this->add_itable(itable_characteristics_id, dst_iseq_ids);
    }
    this->add_enum_state_mapping_if_does_not_exist(enum_candidate_id, itable_id, enum_state_str);
  }
}

void
db_t::pick_random_enum_candidate(/*db_id_t &enum_candidate_id_found, */string &src_iseq_str, string &live_sets_str, string &transmaps_str)
{
  stringstream query;
  int iresultset, enum_candidate_id, src_iseq_id, live_sets_id, transmaps_id;
  int src_iseq_id_found = -1, live_sets_id_found = -1, transmaps_id_found = -1, enum_candidate_id_found = -1;
  int num_results = 0;
  int row_code;
  RETCODE erc;

  query << "SELECT TOP 1 * FROM enum_candidates ORDER BY NEWID()\n";
  this->submit_query(query.str());
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&enum_candidate_id);
    dbbind(dbproc, 2, INTBIND, 0, (BYTE *)&src_iseq_id);
    dbbind(dbproc, 3, INTBIND, 0, (BYTE *)&live_sets_id);
    dbbind(dbproc, 4, INTBIND, 0, (BYTE *)&transmaps_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      enum_candidate_id_found = enum_candidate_id;
      src_iseq_id_found = src_iseq_id;
      live_sets_id_found = live_sets_id;
      transmaps_id_found = transmaps_id;
      num_results++;
    }
  }
  src_iseq_str = this->get_str_value_for_id("src_iseqs", "SRC_ISEQ_ID", src_iseq_id_found);
  live_sets_str = this->get_str_value_for_id("live_sets", "LIVE_SET_ID", live_sets_id_found);
  transmaps_str = this->get_str_value_for_id("transmaps", "TRANSMAPS_ID", transmaps_id_found);
}

size_t
db_t::count_results(string const &query_str)
{
  RETCODE erc;
  this->submit_query(query_str);
  int row_code;
  int iresultset;
  size_t num_results = 0;

  /*
   * Set up each result set with dbresults()
   * This is more commonly implemented as a while() loop, but we're counting the result sets.
   */
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      num_results++;
    }
  }
  return num_results;
}

bool
db_t::peep_entry_exists_for_enum_candidate(string const &src_iseq_str, string const &live_sets_str, string const &transmaps_str)
{
  db_id_t src_iseq_id = entry_exists("src_iseqs", "SRC_ISEQ_STR", src_iseq_str);
  ASSERT(src_iseq_id != DB_ID_INVALID);
  if (src_iseq_id == DB_ID_INVALID) {
    return false;
  }
  db_id_t live_sets_id = entry_exists("live_sets", "LIVE_SET_STR", live_sets_str);
  ASSERT(live_sets_id != DB_ID_INVALID);
  if (live_sets_id == DB_ID_INVALID) {
    return false;
  }
  db_id_t transmaps_id = entry_exists("transmaps", "TRANSMAPS_STR", transmaps_str);
  ASSERT(transmaps_id != DB_ID_INVALID);
  if (transmaps_id == DB_ID_INVALID) {
    return false;
  }
  db_id_t enum_candidate_id = DB_ID_INVALID;
  if (!enum_candidate_exists(src_iseq_id, live_sets_id, transmaps_id, enum_candidate_id)) {
    NOT_REACHED();
    return false;
  }
  stringstream query;
  query << "SELECT * FROM peep_entries WHERE ENUM_CANDIDATE_ID = " << enum_candidate_id << "\n";
  return count_results(query.str()) > 0;
}

vector<string>
db_t::get_itables_for_enum_candidate(string const &src_iseq_str, string const &live_sets_str, string const &transmaps_str)
{
  db_id_t src_iseqs_id = this->entry_exists("src_iseqs", "SRC_ISEQ_STR", src_iseq_str);
  ASSERT(src_iseqs_id != DB_ID_INVALID);
  db_id_t live_sets_id = this->entry_exists("live_sets", "LIVE_SET_STR", live_sets_str);
  ASSERT(live_sets_id != DB_ID_INVALID);
  db_id_t transmaps_id = this->entry_exists("transmaps", "TRANSMAPS_STR", transmaps_str);
  ASSERT(transmaps_id != DB_ID_INVALID);
  db_id_t enum_candidate_id = DB_ID_INVALID;
  bool exists = enum_candidate_exists(src_iseqs_id, live_sets_id, transmaps_id, enum_candidate_id);
  ASSERT(exists);
  list<db_id_t> itable_ids = get_itable_ids_for_enum_candidate(enum_candidate_id);
  vector<string> ret;
  for (auto itable_id : itable_ids) {
    string itable_position_str = get_enum_state_str_for_enum_candidate_and_itable(enum_candidate_id, itable_id);
    string itable_str = get_itable_str_for_id(itable_id);
    ret.push_back(string("max_vector: ") + itable_position_str + "\n==\nitable:\n" + itable_str);
  }
  return ret;
}

list<db_t::db_id_t>
db_t::get_itable_ids_for_enum_candidate(db_id_t enum_candidate_id)
{
  stringstream query;
  query << "SELECT ITABLE_ID FROM enum_states WHERE ENUM_CANDIDATE_ID=" << enum_candidate_id << "\n";
  RETCODE erc;
  this->submit_query(query.str());
  int row_code;
  int iresultset;
  db_id_t id = DB_ID_INVALID;
  list<db_id_t> ret;

  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, BIGINTBIND, 0, (BYTE *)&id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ret.push_back(id);
    }
  }
  return ret;
}

string
db_t::get_enum_state_str_for_enum_candidate_and_itable(db_id_t enum_candidate_id, db_id_t itable_id)
{
  stringstream query;
  RETCODE erc;
  int row_code, num_results = 0;
  int iresultset;
  char *str = new char[40960];
  ASSERT(str);
  ASSERT(enum_candidate_id != DB_ID_INVALID);
  ASSERT(itable_id != DB_ID_INVALID);
  query << "SELECT ENUM_STATE_STR FROM enum_states WHERE (ENUM_CANDIDATE_ID=" << enum_candidate_id << " AND ITABLE_ID=" << itable_id << ")\n";
  this->submit_query(query.str());
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, STRINGBIND, 0, (BYTE *)str);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      num_results++;
    }
  }
  if (num_results != 1) {
    cout << __func__ << " " << __LINE__ << ": num_results = " << num_results << endl;
    cout << "enum_candidate_id = " << enum_candidate_id << endl;
    cout << "itable_id = " << itable_id << endl;
  }
  ASSERT(num_results == 1);
  string ret = string(str);
  delete [] str;
  return ret;
}

string
db_t::get_itable_str_for_id(db_id_t itable_id)
{
  stringstream query;
  RETCODE erc;
  int row_code;
  int iresultset, num_results = 0;
  db_id_t itable_characteristics_id = DB_ID_INVALID;
  db_id_t dst_iseq_id = DB_ID_INVALID;

  query << "SELECT ITABLE_CHARACTERISTICS_ID FROM itables WHERE ITABLE_ID = " << itable_id << "\n";
  this->submit_query(query.str());
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&itable_characteristics_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      num_results++;
    }
  }
  string itable_characteristics = get_str_value_for_id("itable_characteristics", "ITABLE_CHARACTERISTICS_ID", itable_characteristics_id);
  query.str(string());
  query << "SELECT DST_ISEQ_ID FROM itable_dst_iseq_ids WHERE ITABLE_ID = " << itable_id << "\n";
  this->submit_query(query.str());
  list<db_id_t> dst_iseq_ids;
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&dst_iseq_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      dst_iseq_ids.push_back(dst_iseq_id);
    }
  }
  list<string> dst_iseq_strs;
  for (auto dst_iseq_id : dst_iseq_ids) {
    string dst_iseq_str = get_str_value_for_id("dst_iseqs", "DST_ISEQ_ID", dst_iseq_id);
    dst_iseq_strs.push_back(dst_iseq_str);
  }
  stringstream ss;
  ss << itable_characteristics << "entries (count " << dst_iseq_strs.size() << "):\n";
  for (auto dst_iseq_str : dst_iseq_strs) {
    ss << dst_iseq_str << "--\n";
  }
  return ss.str();
}

void
db_t::db_update_itable_ipos_for_enum_candidate(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str, pair<string, vector<string>> const &itable_string, string const &ipos_str)
{
  db_id_t src_iseqs_id = this->entry_exists("src_iseqs", "SRC_ISEQ_STR", src_iseq_str);
  ASSERT(src_iseqs_id != DB_ID_INVALID);
  db_id_t live_sets_id = this->entry_exists("live_sets", "LIVE_SET_STR", live_sets_str);
  ASSERT(live_sets_id != DB_ID_INVALID);
  db_id_t transmaps_id = this->entry_exists("transmaps", "TRANSMAPS_STR", transmaps_str);
  ASSERT(transmaps_id != DB_ID_INVALID);
  db_id_t enum_candidate_id = DB_ID_INVALID;
  bool exists = enum_candidate_exists(src_iseqs_id, live_sets_id, transmaps_id, enum_candidate_id);
  ASSERT(exists);

  string const &itable_characteristics = itable_string.first;
  vector<string> const &entries = itable_string.second;
  db_id_t itable_characteristics_id = this->entry_exists("itable_characteristics", "ITABLE_CHARACTERISTICS_STR", itable_characteristics);
  ASSERT(itable_characteristics_id != DB_ID_INVALID);
  list<db_id_t> dst_iseq_ids;
  for (const auto &entry : entries) {
    db_id_t dst_iseq_id = this->entry_exists("dst_iseqs", "DST_ISEQ_STR", entry);
    ASSERT(dst_iseq_id != DB_ID_INVALID);
    dst_iseq_ids.push_back(dst_iseq_id);
  }
  db_id_t itable_id = DB_ID_INVALID;
  //cout << __func__ << " " << __LINE__ << ": dst_iseq_ids.size() = " << dst_iseq_ids.size() << endl;
  bool itable_exists = this->itable_exists(itable_characteristics_id, dst_iseq_ids, itable_id);
  ASSERT(itable_exists);

  stringstream query;
  query << "UPDATE enum_states SET ENUM_STATE_STR='" << ipos_str << "' WHERE (ENUM_CANDIDATE_ID = " << enum_candidate_id << " AND ITABLE_ID = " << itable_id << ")\n";
  this->execute_query_no_results(query.str());
}

void
db_t::db_add_peep_entry(string const &src_iseq_str, string const &live_sets_str, string const &transmaps_str, string const &dst_iseq_str)
{
  db_id_t enum_candidate_id = this->add_enum_candidate_if_does_not_exist(src_iseq_str, live_sets_str, transmaps_str);
  db_id_t dst_iseq_id = insert_if_does_not_exist("dst_iseqs", "DST_ISEQ_STR", dst_iseq_str);
  string timestamp = get_cur_timestamp(as1, sizeof as1);
  stringstream query;
  query << "IF NOT EXISTS (SELECT * FROM peep_entries WHERE ENUM_CANDIDATE_ID = " << enum_candidate_id << " AND DST_ISEQ_ID = " << dst_iseq_id << ") BEGIN INSERT INTO peep_entries (ENUM_CANDIDATE_ID, DST_ISEQ_ID, TIMESTAMP)\n"
     "VALUES (" << enum_candidate_id << ", " << dst_iseq_id << ", '" << timestamp << "') END\n";
  this->execute_query_no_results(query.str());
}

string
db_t::get_enum_candidate_str_for_id(db_id_t enum_candidate_id)
{
  stringstream query;
  query << "SELECT SRC_ISEQ_ID,LIVE_SET_ID,TRANSMAPS_ID FROM enum_candidates WHERE enum_candidate_id = " << enum_candidate_id << "\n";
  this->submit_query(query.str());
  int num_results = 0;
  int row_code;
  RETCODE erc;
  db_id_t src_iseq_id = DB_ID_INVALID, live_sets_id = DB_ID_INVALID, transmaps_id = DB_ID_INVALID;

  for (int iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //we expect only one result set
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&src_iseq_id);
    dbbind(dbproc, 2, INTBIND, 0, (BYTE *)&live_sets_id);
    dbbind(dbproc, 3, INTBIND, 0, (BYTE *)&transmaps_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(num_results == 0); //we expect only one result
      num_results++;
    }
  }
  string src_iseq_str = this->get_str_value_for_id("src_iseqs", "SRC_ISEQ_ID", src_iseq_id);
  string live_sets_str = this->get_str_value_for_id("live_sets", "LIVE_SET_ID", live_sets_id);
  string transmaps_str = this->get_str_value_for_id("transmaps", "TRANSMAPS_ID", transmaps_id);
  return src_iseq_str + "--\n" + live_sets_str + "--\n" + transmaps_str;
}

void
db_t::db_download_peep_entries(std::ostream &oss)
{
  stringstream query;
  query << "SELECT ENUM_CANDIDATE_ID, DST_ISEQ_ID FROM peep_entries\n";
  this->submit_query(query.str());
  RETCODE erc;
  int row_code;
  int iresultset;
  list<pair<db_id_t, db_id_t>> peep_entry_ids;
  for (iresultset=1; (erc = dbresults(dbproc)) == SUCCEED; iresultset++) {
    ASSERT(iresultset == 1); //expect only one result set
    db_id_t enum_candidate_id = DB_ID_INVALID, dst_iseq_id = DB_ID_INVALID;
    dbbind(dbproc, 1, INTBIND, 0, (BYTE *)&enum_candidate_id);
    dbbind(dbproc, 2, INTBIND, 0, (BYTE *)&dst_iseq_id);
    while ((row_code = dbnextrow(dbproc)) == REG_ROW) {
      ASSERT(enum_candidate_id != DB_ID_INVALID);
      ASSERT(dst_iseq_id != DB_ID_INVALID);
      //cout << "enum_candidate_id = " << enum_candidate_id << endl;
      //cout << "dst_iseq_id = " << dst_iseq_id << endl;
      peep_entry_ids.push_back(make_pair(enum_candidate_id, dst_iseq_id));
    }
  }
  for (auto p : peep_entry_ids) {
    db_id_t enum_candidate_id = p.first;
    db_id_t dst_iseq_id = p.second;
    ASSERT(dst_iseq_id != DB_ID_INVALID);
    string enum_candidate_str = get_enum_candidate_str_for_id(enum_candidate_id);
    string dst_iseq_str = get_str_value_for_id("dst_iseqs", "DST_ISEQ_ID", dst_iseq_id);
    oss << "ENTRY:\n" << enum_candidate_str << "--\n" << dst_iseq_str << "==\n";
  }
}

void
db_t::begin_transaction(string const &name)
{
  stringstream query;
  query << "BEGIN TRANSACTION " << name << "\n";
  this->execute_query_no_results(query.str());
}

void
db_t::commit_transaction(string const &name)
{
  stringstream query;
  query << "COMMIT TRANSACTION " << name << "\n";
  this->execute_query_no_results(query.str());
}

/*void
db_t::pick_enum_candidate(std::string &src_iseq_str, std::string &live_sets_str, std::string &transmaps_str)
{
  size_t max_tries = 2000;
  size_t i;
  for (i = 0; i < max_tries; i++) {
    db_id_t enum_candidate_id;
    pick_random_enum_candidate(enum_candidate_id, src_iseq_str, live_sets_str, transmaps_str);
    if (!peep_entry_exists_for_enum_candidate(enum_candidate_id)) {
      break;
    }
  }
  ASSERT(i < max_tries);
}*/

/*
queries:
  1. choose enum candidate
    - pick src_iseq giving higher priority to src_iseqs that have fewest percentage of transmaps solved
    - given iseq, choose enum_candidate for src_iseq giving higher priority to transmap currently unsolved
  2. given enum candidate, choose itable giving higher priority to itable with lower enum_state and higher promise
  3. add peep entry
*/
