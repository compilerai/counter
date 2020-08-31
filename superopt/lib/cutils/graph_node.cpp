#include "graph_node.h"
#include <support/debug.h>
#include "stdint.h"
#include <stdlib.h>
#include <stdio.h>
#include "support/c_utils.h"

bool
graph_node_add_outgoing_edge(struct graph_node *node, struct graph_node *dst)
{
  int i;

  for (i = 0; i < node->num_edges; i++) {
    if (node->edges[i].dst == dst) {
      return false;
    }
  }
  node->edges[node->num_edges].dst = dst;
  node->num_edges++;
  ASSERT(node->num_edges <= GRAPH_NODE_MAX_NUM_EDGES);
  return true;
}

bool
graph_node_remove_outgoing_edge(struct graph_node *node, struct graph_node *dst)
{
  int i;
  for (i = 0; i < node->num_edges; i++) {
    if (node->edges[i].dst == dst) {
      break;
    }
  }
  if (i == node->num_edges) {
    return false;
  }
  for (i++; i < node->num_edges; i++) {
    ASSERT(i > 0);
    memcpy(&node->edges[i - 1], &node->edges[i], sizeof(graph_edge));
  }
  node->num_edges--;
  return true;
}

void
graph_node_remove_all_outgoing_edges(struct graph_node *n)
{
  n->num_edges = 0;
}


/* returns if a cycle originating at ROOT contains ORIG */
static bool
graph_node_detect_cycle_rec(struct graph_node const *root,
    struct graph_node const *orig)
{
  int i;
  for (i = 0; i < root->num_edges; i++) {
    if (root->edges[i].dst == orig) {
      return true;
    }
    if (graph_node_detect_cycle_rec(root->edges[i].dst, orig)) {
      return true;
    }
  }
  return false;
}

/* detect if a cycle containing root exists, and if so return all elems */
bool
graph_node_detect_cycle(struct graph_node const *root)
{
  return graph_node_detect_cycle_rec(root, root);
}

bool
graph_node_topsort(struct graph_node sorted_nodes[] /*output*/,
    struct graph_node const *root)
{
  NOT_IMPLEMENTED();
}

graph_edge_iter const *
graph_node_edges_start(struct graph_node const *node)
{
  return &node->edges[0];
}

graph_edge_iter const *
graph_node_edges_next(graph_node const *node, graph_edge_iter const *iter)
{
  if (iter - node->edges == node->num_edges - 1) {
    return NULL;
  }
  return iter + 1;
}

struct graph_node *
graph_edge_iter_get_dst_node(graph_edge_iter const *iter)
{
  return (graph_node *)iter->dst;
}

void
graph_node_init(struct graph_node *node)
{
  node->num_edges = 0;
}

int
graph_node_num_outgoing_edges(struct graph_node const *n)
{
  return n->num_edges;
}

char *
graph_edges_to_string(struct graph_node const *n, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = ptr + size;

  for (i = 0; i < n->num_edges; i++) {
    ptr += snprintf(ptr, end - ptr, " %p", n->edges[i].dst);
  }
  *ptr++ = '\n';
  *ptr = '\0';
  return buf;
}
