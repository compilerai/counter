#pragma once

#include <memory>
#include <vector>
#include <list>

namespace eqspace {

using namespace std;

class counter_example_t;
class sprel_map_t;

class state;
class context;
class expr_sort;
class expr;
struct expr_compare;

typedef shared_ptr<expr_sort> sort_ref;
typedef shared_ptr<expr> expr_ref;
typedef vector<expr_ref> expr_vector;
typedef list<expr_ref> expr_list;

struct consts_struct_t;
class tfg;

}
