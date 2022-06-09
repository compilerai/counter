#ifndef GRAPH_NODE_H
#define GRAPH_NODE_H
#include "stdbool.h"
#include "stddef.h"

#define GRAPH_NODE_MAX_NUM_EDGES 16
struct graph_node;

typedef struct graph_edge {
  struct graph_node *dst;
} graph_edge;

typedef struct graph_node {
  struct graph_edge edges[GRAPH_NODE_MAX_NUM_EDGES];
  int num_edges;
} graph_node;

typedef struct graph_edge graph_edge_iter;

/* Converts pointer to graphnode element GRAPHNODE_ELEM into a pointer to
 * the structure that GRAPHNODE_ELEM is embedded inside.  Supply the
 * name of the outer structure STRUCT and the member name MEMBER
 * of the hash element.  See list.h for an example. */
#define graph_node_entry(GRAPHNODE_ELEM, STRUCT, MEMBER)                           \
  ((STRUCT *) ((uint8_t *) &(GRAPHNODE_ELEM)->edges                           \
    - offsetof (STRUCT, MEMBER.edges)))

bool graph_node_add_outgoing_edge(struct graph_node *node, struct graph_node *dst);
bool graph_node_remove_outgoing_edge(struct graph_node *node, struct graph_node *dst);
bool graph_node_detect_cycle(
    struct graph_node const *root);
bool graph_node_topsort(struct graph_node sorted_nodes[] /*output*/,
    struct graph_node const *root);
graph_edge_iter const *graph_node_edges_start(struct graph_node const *node);
graph_edge_iter const *graph_node_edges_next(graph_node const *node,
    graph_edge_iter const *iter);
struct graph_node *graph_edge_iter_get_dst_node(graph_edge_iter const *iter);
void graph_node_remove_all_outgoing_edges(struct graph_node *n);
void graph_node_init(struct graph_node *node);
int graph_node_num_outgoing_edges(struct graph_node const *n);
char *graph_edges_to_string(struct graph_node const *n, char *buf, size_t size);

#endif /* lib/graph_node.h */
