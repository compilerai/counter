#ifndef CODEGEN_H
#define CODEGEN_H

#include "expr/expr.h"

void codegen(map<string, shared_ptr<tfg>> const &function_tfg_map, string const &output_filename, vector<string> const& disable_dst_insn_folding_for_functions, context *ctx);

#endif /*codegen.h*/
