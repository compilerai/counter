#pragma once

#include "support/types.h"

#include <set>
#include <string>

namespace eqspace {

using namespace std;

set<graph_loc_id_t> loc_set_from_string(string const &line);
string loc_set_to_string(set<graph_loc_id_t> const &loc_set);

}
