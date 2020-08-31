#include "graph/graph_loc_id.h"

#include <cstdlib>
#include <sstream>

namespace eqspace {

using namespace std;

set<graph_loc_id_t> loc_set_from_string(string const &line)
{
  char const *line_c = line.c_str();
  char const *ptr = line_c;
  set<graph_loc_id_t> ret;
  while (*ptr != '\0') {
    char *remaining;
    graph_loc_id_t loc_id = strtol(ptr, &remaining, 0);
    ASSERT(remaining);
    if (remaining == ptr) {
      printf("line_c = %s, ptr = %s\n", line_c, ptr);
    }
    ASSERT(remaining != ptr);
    ret.insert(loc_id);
    ptr = remaining;
    if (*ptr == ',') {
      ptr++;
    }
    while (*ptr == ' ') {
      ptr++;
    }
  }
  return ret;
}

string loc_set_to_string(set<graph_loc_id_t> const &loc_set)
{
  stringstream ss;
  bool first = true;
  for (auto loc_id : loc_set) {
    if (!first) {
      ss << ", ";
    }
    ss << loc_id;
    first = false;
  }
  return ss.str();
}

}
