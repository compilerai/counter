#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/utils.h"

#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace eqspace {

bool dyn_debug_flag = false;
static unordered_map<string,int> debug_class_levels;

void enable_dyn_debug()
{
  dyn_debug_flag = true;
}

void disable_dyn_debug()
{
  dyn_debug_flag = false;
}

void init_dyn_debug_from_string(string const& s)
{
  istringstream iss(s);
  string token;

  while(getline(iss, token, ',')) {
   	int lvl = 0;
    auto pos = token.find_first_of('=');
    if (pos == string::npos) {
    	lvl = 1;
    }
    else {
    	istringstream(token.substr(pos+1)) >> lvl;
    }
    if (lvl)
      debug_class_levels.insert(make_pair(token.substr(0, pos), lvl));
    else {
      CPP_DBG_EXEC(DYN_DEBUG, cout << __func__ << ':' << __LINE__ << " ignoring malformed input `" << token << "'" << endl);
    }
  }
  if (debug_class_levels.size())
    dyn_debug_flag = true;
}

void print_debug_class_levels()
{
  if (debug_class_levels.empty())
    return;

  cout << _FNLN_ << ": printing debug_class_levels:\n";
  for (auto const& dl : debug_class_levels) {
    cout << dl.first << " -> " << dl.second << endl;
  }
}

int get_debug_class_level(const char* dbgclass)
{
  auto const& itr = debug_class_levels.find(dbgclass);
  if (itr == debug_class_levels.end())
    return 0;
  return itr->second;
}

int get_debug_class_prefix_level(const char* dbgclass)
{
  for (auto iter = debug_class_levels.begin(); iter != debug_class_levels.end(); iter++) {
    if (string_has_prefix(dbgclass, iter->first)) {
      return iter->second;
    }
  }
  return 0;
}

void set_debug_class_level(const char* dbgclass, int lvl)
{
  debug_class_levels.insert(make_pair(dbgclass, lvl));
  if (lvl > 0) {
    dyn_debug_flag = true;
  }
}

}
