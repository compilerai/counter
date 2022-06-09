#pragma once

#include <set>
#include <stdint.h>
#include <limits.h>

using namespace std;

class free_id_list
{
public:
  free_id_list() { m_free_ids.insert(make_pair(1, UINT_MAX)); }
  unsigned get_free_id(unsigned suggested_id = 0);
  void add_free_id(unsigned id);

private:
  set<pair<unsigned, unsigned>> m_free_ids; //set of id ranges
  //set<expr_id_t> m_used_ids;
  //set<expr_id_t> m_free_ids;
  //expr_id_t m_max_limit_used;
};


