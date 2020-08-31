#ifndef __CFG_H
#define __CFG_H
#include "support/types.h"
#include <stdio.h>

struct input_section_t;
struct input_exec_t;
struct interval_set;
struct regset_t;
struct edge_t;
struct bbl_t;
struct chash;
struct rbtree;
class nextpc_t;
class symbol_t;

typedef struct cfg_edge_t {
  int dst;
  struct cfg_edge_t *next;
} cfg_edge_t;

#define SOURCE_NODE(n) ((n) - 2) //(in->num_bbls)
#define SINK_NODE(n) ((n) - 1) //(in->num_bbls + 1)

void add_cfg_edge(cfg_edge_t *table[], long src, long newdst);



void compute_dominators_helper(struct input_exec_t const *in, long *dominators,
    long num_bbls_aug, struct cfg_edge_t * const *fwd_edges,
    struct cfg_edge_t * const *bwd_edges, long start);
void free_cfg_edges(cfg_edge_t **edges, long n);

void build_cfg(struct input_exec_t *);
void compute_bbl_map(struct input_exec_t *in);
void analyze_cfg(struct input_exec_t *);
void cfg_print(FILE *fp, struct input_exec_t const *);

char const *lookup_symbol(struct input_exec_t const *in, src_ulong pc);
unsigned long pc2bbl(struct chash const *map, src_ulong pc);
unsigned long pc_find_bbl(struct input_exec_t const *in, src_ulong pc);
bool is_code_section(struct input_section_t const *section);
int is_executed_more_than(src_ulong nip, long long n);
bool pc_dominates(struct input_exec_t const *in, src_ulong pc1, src_ulong pc2);
bool pc_postdominates(struct input_exec_t const *in, src_ulong pc1, src_ulong pc2);
long pc_get_nextpcs(struct input_exec_t const *in, src_ulong pc, nextpc_t *nextpcs,
    double *nextpc_weights);
long pc_get_preds(struct input_exec_t const *in, src_ulong pc, src_ulong *preds);
bool pc_is_reachable(struct input_exec_t const *in, src_ulong pc);
long pc_get_next_edges_reducing_loops(struct input_exec_t const *in, src_ulong pc,
    struct edge_t *next_edges, double *next_edge_weights);
void bbl_free(struct bbl_t *bbl);
void free_bbl_map(struct input_exec_t *in);
void obtain_live_outs(struct input_exec_t const *in, struct regset_t *live_outs,
    nextpc_t const *nextpcs, long num_nextpcs);
char const *pc2function_name(struct input_exec_t const *in, src_ulong pc);
struct symbol_t const* pc_has_associated_symbol(struct input_exec_t const *in, src_ulong pc);
struct symbol_t const* pc_has_associated_symbol_of_func_or_notype(struct input_exec_t const *in, src_ulong pc);
src_ulong function_name2pc(struct input_exec_t const *in, char const *function_name);
src_ulong get_function_containing_pc(struct input_exec_t const *in, src_ulong pc);
void get_text_section(struct input_exec_t const *in, long *start_pc, long *end_pc);
//void compute_liveness(struct input_exec_t *in, int max_iter);
bool src_pc_belongs_to_plt_section(struct input_exec_t const *in, src_ulong pc);
int get_input_section_by_name(struct input_exec_t const *in, char const *name);
void bbl_leaders_add(struct rbtree *tree, src_ulong pc);

#endif
