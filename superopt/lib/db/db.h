#ifndef DB_H
#define DB_H

#include <sybfront.h>
#include <sybdb.h>
#include <string>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <list>

class db_t
{
private:
  DBPROCESS *dbproc;
  static int m_num_connections;
  typedef uint64_t db_id_t;
public:
  db_t();
  void begin_transaction(std::string const &name);
  void commit_transaction(std::string const &name);
  void execute_query(std::string const &query, std::stringstream &ret);
  void execute_query_no_results(std::string const &query);
  db_id_t add_enum_candidate_if_does_not_exist(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str);
  //void pick_enum_candidate(std::string &src_iseq_str, std::string &live_sets_str, std::string &transmaps_str);
  void populate_itables_for_enum_candidate(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str, std::vector<std::pair<std::string, std::vector<std::string>>> const &itable_strings, std::string const &enum_state_str);
  void pick_random_enum_candidate(/*db_id_t &enum_candidate_id, */std::string &src_iseq_str, std::string &live_sets_str, std::string &transmaps_str);
  bool peep_entry_exists_for_enum_candidate(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str);
  std::vector<std::string> get_itables_for_enum_candidate(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str);
  void db_update_itable_ipos_for_enum_candidate(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str, std::pair<std::string, std::vector<std::string>> const &itable_str, std::string const &ipos_str);
  void db_add_peep_entry(std::string const &src_iseq_str, std::string const &live_sets_str, std::string const &transmaps_str, std::string const &dst_iseq_str);
  void db_download_peep_entries(std::ostream &oss);
private:
  size_t count_results(std::string const &query);
  db_id_t insert_if_does_not_exist(char const *table, char const *field, std::string const &value);
  db_id_t entry_exists(char const *table, char const *field, std::string const &value);
  void submit_query(std::string const &query_str);
  std::string get_str_value_for_id(char const *table, char const *fieldname, db_id_t id);
  bool enum_candidate_exists(db_id_t src_iseq_id, db_id_t live_set_id, db_id_t transmaps_id, db_id_t &enum_candidate_id);
  void add_enum_state_mapping_if_does_not_exist(db_id_t enum_candidate_id, db_id_t itable_id, std::string const &enum_state_str);
  db_id_t add_itable(db_id_t characteristics_id, std::list<db_id_t> const &dst_iseq_ids);
  bool itable_exists(db_id_t characteristics_id, std::list<db_id_t> const &dst_iseq_ids, db_id_t &itable_id);
  static void output_id_list(std::stringstream &ss, std::list<db_id_t> const &ids);
  std::list<db_id_t> get_itable_ids_for_enum_candidate(db_id_t enum_candidate_id);
  std::string get_itable_str_for_id(db_id_t itable_id);
  std::string get_enum_state_str_for_enum_candidate_and_itable(db_id_t enum_candidate_id, db_id_t itable_id);
  std::string get_enum_candidate_str_for_id(db_id_t enum_candidate_id);
};

#endif
