#ifndef MEMLABEL_MAP_H
#define MEMLABEL_MAP_H

#include "expr/memlabel_obj.h"

namespace eqspace {

typedef map<mlvarname_t,memlabel_ref> graph_memlabel_map_t;
set<memlabel_ref> get_memlabel_set_from_mlvarnames(graph_memlabel_map_t const &memlabel_map, set<mlvarname_t> const &mlvars);

void memlabel_map_union(graph_memlabel_map_t &dst, graph_memlabel_map_t const &src);
void memlabel_map_intersect(graph_memlabel_map_t &dst, graph_memlabel_map_t const &src);
void memlabel_map_get_relevant_memlabels(graph_memlabel_map_t const &m, vector<memlabel_ref> &out);

string memlabel_map_to_string(graph_memlabel_map_t const &ml_map, string const& suffix_string);
void expr_set_memlabels_to_top(expr_ref const& e, graph_memlabel_map_t& memlabel_map);
void graph_memlabel_map_union(graph_memlabel_map_t& dst, graph_memlabel_map_t const& src);
//string read_memlabel_map(istream& in, context* ctx, string line, graph_memlabel_map_t &memlabel_map);

}

#endif
