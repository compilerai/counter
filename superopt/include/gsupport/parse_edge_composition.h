#pragma once
#include "gsupport/graph_ec.h"

namespace eqspace {

template<typename T_PC, typename T_E>
graph_edge_composition_ref<T_PC,T_E>
parse_edge_composition(const char* str);

}
