#include "ptfg/function_signature.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <list>
#include "support/debug.h"
#include "support/utils.h"

using namespace std;

namespace eqspace {

string
function_signature_t::function_signature_to_string_for_eq() const
{
  stringstream ss;
  size_t n = 0;
  ss << m_return_type << " (";
  ASSERT(m_argument_types.size() == m_argument_names.size());
  for (list<string>::const_iterator iter_t = m_argument_types.begin(), iter_n = m_argument_names.begin();
       iter_t != m_argument_types.end() && iter_n != m_argument_names.end();
       iter_t++, iter_n++) {
    if (n) {
      ss << ", ";
    }
    ss << *iter_t << " " << *iter_n;
    n++;
  }
  if (m_is_vararg) {
    if (n) {
      ss << ", ";
    }
    ss << "...";
  }
  ss << ")";
  return ss.str();
}

pair<string, string>
function_signature_t::get_type_and_name(string const& s)
{
  vector<string> svec = splitOnChar(s, ' ');
  if (svec.size() <= 1) {
    cout << __func__ << " " << __LINE__ << ": svec.size() = " << svec.size() << endl;
    cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
    for (auto const& sv : svec) {
      cout << " " << sv;
    }
    cout << endl;
  }
  ASSERT(svec.size() > 1);
  string name = svec.at(svec.size() - 1);
  string type;
  for (size_t i = 0; i < svec.size() - 1; i++) {
    type += svec.at(i) + " ";
  }
  return make_pair(type, name);
}

function_signature_t
function_signature_t::read_from_string(string const& line)
{
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  size_t lparen = line.find('(');
  ASSERT(lparen != string::npos);
  string return_type = line.substr(0, lparen);
  trim(return_type);
  size_t last = lparen;
  size_t comma = line.find(',', last);
  size_t rparen = line.find(')', last);
  bool is_vararg = false;
  list<string> types, names;
  while (comma != string::npos) {
    ASSERT(comma < rparen);
    string arg_type_and_name = line.substr(last + 1, comma - (last + 1));
    trim(arg_type_and_name);
    pair<string, string> pr = get_type_and_name(arg_type_and_name);
    types.push_back(pr.first);
    names.push_back(pr.second);
    last = comma;
    comma = line.find(',', last + 1);
  }
  if (last + 1 < rparen) {
    string arg_type_and_name = line.substr(last + 1, rparen - (last + 1));
    trim(arg_type_and_name);
    if (arg_type_and_name == "...") {
      is_vararg = true;
    } else {
      pair<string, string> pr = get_type_and_name(arg_type_and_name);
      types.push_back(pr.first);
      names.push_back(pr.second);
    }
  }
  return function_signature_t(return_type, types, names, is_vararg);
}

}
