#pragma once
#include <memory>
#include <map>
#include "support/string_ref.h"
#include "expr/expr.h"

namespace eqspace {
class tfg;
class pc;
class context;
class sym_exec;
class graph_arg_regs_t;
}

class dst_insn_t;
class transmap_t;
class fixed_reg_mapping_t;
 
using namespace std;

dshared_ptr<tfg>
dst_iseq_get_preprocessed_tfg(dst_insn_t const *dst_iseq, long dst_iseq_len, transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_live_out, graph_arg_regs_t const &argument_regs, map<pc, map<string_ref, expr_ref>> const &return_reg_map, fixed_reg_mapping_t const &fixed_reg_mapping, context *ctx, sym_exec &se, string const& name, string const& fname);

