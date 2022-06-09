#pragma once

#include <memory>
#include <vector>
#include <list>
#include <set>
#include <utility>

namespace eqspace {

using namespace std;

class counter_example_t;
class sprel_map_t;

class state;
class context;
class expr_sort;
class expr;
struct expr_compare;

using sort_ref = shared_ptr<expr_sort>;
using expr_ref = shared_ptr<expr>;
using expr_vector = vector<expr_ref>;
using expr_list = list<expr_ref>;
using expr_set = set<expr_ref>;
using expr_pair = pair<expr_ref,expr_ref>;

struct consts_struct_t;
class tfg;

}
