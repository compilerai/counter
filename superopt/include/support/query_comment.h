#pragma once
//#include "support/debug.h"
#include "support/consts.h"
#include "support/utils.h"
#include <string>
#include <iostream>
#include <sstream>
#include <map>

namespace eqspace
{
using namespace std;

class query_comment
{
public:
  query_comment() : query_comment("") { }
  query_comment(string const &c)
  {
    size_t len = min(c.length(), (size_t)G_MAX_QUERY_COMMENT_SIZE);
    m_comment = c.substr(0, len);
    sanitize_str_for_file_name(m_comment);
    m_count = query_comment_get_count(m_comment);
  }

  std::string to_string() const
  {
    stringstream ss;
    //ss << "time: " << m_time / 1e6 << endl;
    //ss << "provable_type: " << m_check_for_provable << endl;
    ss << m_comment << "." << m_count;
    //ss << m_comment << endl;
    return ss.str();
  }

 

  //string m_comment;
  //bool m_check_for_provable;
  //long long m_time;
private:
  static unsigned long query_comment_get_count(std::string const &c)
  {
    if (max_count_for_type_map.count(c) == 0) {
      max_count_for_type_map.insert(make_pair(c, 0));
    } else {
      max_count_for_type_map[c]++;
    }
    return max_count_for_type_map.at(c);
  }

  static map<std::string, unsigned long> max_count_for_type_map;

  std::string m_comment;
  unsigned long m_count;
};

}
