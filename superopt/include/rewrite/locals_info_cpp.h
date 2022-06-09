#ifndef LOCALS_INFO_CPP_H
#define LOCALS_INFO_CPP_H
#include "expr/expr.h"

//struct cspace::locals_info_t *locals_info_new_using_context(struct context *ctx);
void locals_info_add(struct cspace::locals_info_t *li, unsigned dwarf_map_index, string name, size_t size, expr_ref expr);
void locals_info_clear(struct cspace::locals_info_t *li);
bool locals_info_get_expr(expr_ref &e, struct cspace::locals_info_t const *li, unsigned dwarf_map_index);

#endif
