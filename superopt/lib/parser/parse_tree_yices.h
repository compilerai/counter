#pragma once

#include <string>
#include <map>

#include "expr/expr.h"
#include "parser/parse_tree_common.h"

using namespace std;

map<string_ref,expr_ref> parse_yices_model(eqspace::context* ctx, const char* str);
expr_ref fn_expr_pair_list_to_ite(vector<pair<expr_ref,expr_ref>> const& exprlist);
