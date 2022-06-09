#pragma once

#include "expr/state.h"
#include "expr/expr.h"
#include "expr/context.h"

#include "gsupport/graph_ec.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/graph_full_pathset.h"

namespace eqspace {

//expr_ref tfg_edge_composition_get_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, context* ctx, bool simplified = false);
//expr_ref tfg_edge_composition_get_unsubstituted_edge_cond(shared_ptr<tfg_edge_composition_t> const &src_ec, context* ctx, bool simplified = false);
//state tfg_edge_composition_get_to_state(shared_ptr<tfg_edge_composition_t> const &src_ec, context* ctx, bool simplified = false);
//unordered_set<expr_ref> tfg_edge_composition_get_assumes(shared_ptr<tfg_edge_composition_t> const &src_ec, context* ctx);
shared_ptr<tfg_edge_composition_t> tfg_edge_composition_create_from_path(list<shared_ptr<tfg_edge const>> const &path);
set<allocsite_t> get_alloca_ptrs_in_path(shared_ptr<graph_full_pathset_t<pc,tfg_edge> const> const& src_path);
set<allocsite_t> get_allocas_in_path(shared_ptr<graph_full_pathset_t<pc,tfg_edge> const> const& src_path);
set<allocsite_t> get_deallocs_in_path(shared_ptr<graph_full_pathset_t<pc, tfg_edge> const> const& src_path);


}
