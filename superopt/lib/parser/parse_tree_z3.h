#pragma once

#include <string>
#include <map>

#include "expr/expr.h"
#include "parser/parse_tree_common.h"

using namespace std;

map<string_ref,expr_ref> parse_z3_model(eqspace::context* ctx, const char* str);
