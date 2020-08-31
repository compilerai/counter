#pragma once

#include <string>
#include <map>
#include <vector>
#include <stack>

#include "expr/expr.h"
#include "parser/parse_tree_common.h"

using namespace std;

map<string_ref,expr_ref> parse_boolector_model(eqspace::context* ctx, const char* str);
