#pragma once

#include <string>
#include <map>

#include "expr/expr.h"
#include "parser/parse_tree_common.h"
#include "parser/parsing_common.h"

using namespace std;

map<string_ref,expr_ref> parse_z3_model(eqspace::context* ctx, const char* str);
map<string_ref,expr_ref> parse_z3_model_neo(eqspace::context* ctx, string const& str, bool debug = false);

