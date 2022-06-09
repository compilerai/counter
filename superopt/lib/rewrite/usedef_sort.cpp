#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "support/c_utils.h"
#include "support/debug.h"
#include "config-host.h"
//#include "cpu.h"
#include "support/src-defs.h"
//#include "exec-all.h"
#include "rewrite/bbl.h"

#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "valtag/elf/elf.h"

#include "ppc/regs.h"
#include "valtag/memset.h"
#include "valtag/regset.h"
#include "insn/live_ranges.h"
#include "rewrite/translation_instance.h"
#include "cutils/interval_set.h"
#include "cutils/rbtree.h"
#include "cutils/pc_elem.h"
#include "cutils/addr_range.h"

static char as1[4096], as2[4096];
static char as3[4096], as4[4096];
static char udgraph_str1[4096];

typedef enum { BRANCH, MEM, REG_WW, REG_RW, REG_WR} edge_type_t;
typedef struct ud_edge_t {
  int src, dst;
  edge_type_t type;
  int regno;

  struct ud_edge_t *next;
} ud_edge_t;

static void
add_edge (ud_edge_t **list, ud_edge_t *edge)
{
  ud_edge_t *cur, *prev;

  for (prev=NULL, cur=*list;
      cur && ((cur->type > edge->type)
	|| (cur->type == edge->type && cur->regno > edge->regno));
      prev=cur,cur=cur->next);

  if (!prev) {
    edge->next = cur;
    *list = edge;
  } else {
    edge->next = cur;
    prev->next = edge;
  }
}

typedef struct udgraph_node_t {
  int *insns;
  int num_insns;

  ud_edge_t **successors;
  ud_edge_t **predecessors;/* these are just aliases to successors elements */

  regset_t use, def;
  memset_t memuse, memdef;

  /* nodes with higher weight have an affinity to appear lower in the order */
  unsigned weight;
} udgraph_node_t;

typedef struct udgraph_t {
  int n;
  udgraph_node_t *nodes; 
} udgraph_t;

static void
udgraph_node_init (udgraph_node_t *node, int n)
{
  node->insns = (int *)malloc(n*sizeof(n));
  assert (node->insns);
  node->num_insns = 0;

  regset_clear (&node->use);
  regset_clear (&node->def);

  memset_init (&node->memuse);
  memset_init (&node->memdef);

  node->successors = (ud_edge_t **)malloc(n*sizeof(ud_edge_t *));
  assert (node->successors);
  node->predecessors = (ud_edge_t **)malloc(n*sizeof(ud_edge_t *));
  assert (node->predecessors);

  memset(node->successors, 0x0, n*sizeof(ud_edge_t *));
  memset(node->predecessors, 0x0, n*sizeof(ud_edge_t *));
}

static void
udgraph_node_free (udgraph_node_t *node, int n)
{
  memset_free (&node->memuse);
  memset_free (&node->memdef);

  free(node->insns);
  int i;
  for (i = 0; i < n; i++) {
    ud_edge_t *edge, *next;
    for (edge = node->successors[i]; edge; edge=edge->next) {
      next = edge->next;
      free(edge);
    }
  }
  free (node->successors);
  free (node->predecessors);
}

static void
udgraph_node_copy (udgraph_node_t *dst, udgraph_node_t const *src)
{
  memcpy (dst, src, sizeof(udgraph_node_t));
}

static void
usedef_register (udgraph_t *udgraph, int n, regset_t const *use,
    regset_t const *def, memset_t const *memuse,
    memset_t const *memdef, regset_t const *live_out0,
    int is_branch, int is_link)
{
      NOT_IMPLEMENTED();
/*
  int i;
  for (i = 0; i < n; i++) {
    ud_edge_t *edge = NULL;

    if (is_branch) {
      regset_t dlive0;
      regset_copy (&dlive0, live_out0);
      regset_intersect (&dlive0, &udgraph->nodes[i].def);

      if (   is_link
          || udgraph->nodes[i].memdef.num_memaccesses
          || regset_count(&dlive0)) {
        edge = (ud_edge_t *)malloc(sizeof(ud_edge_t));
        assert (edge);
        edge->type = BRANCH;
        edge->src = i;
        edge->dst = n;
        edge->regno = -1;
      }
    }

    if (edge) {
      add_edge(&udgraph->nodes[i].successors[n], edge);
    }

    edge = NULL;
    if (((memuse->num_memaccesses || memdef->num_memaccesses)
          && udgraph->nodes[i].memdef.num_memaccesses)
        || (memdef->num_memaccesses
          && (udgraph->nodes[i].memdef.num_memaccesses
            || udgraph->nodes[i].memuse.num_memaccesses)))
    {
      edge = (ud_edge_t *)malloc(sizeof(ud_edge_t));
      assert (edge);
      edge->type = MEM;
      edge->src = i;
      edge->dst = n;
      edge->regno = -1;
    }

    if (edge) {
      add_edge(&udgraph->nodes[i].successors[n], edge);
    }

    int r;
    for (r = 0; r < PPC_NUM_REGS; r++) {
      edge = NULL;

      if ((regset_belongs (use, r))
          && regset_belongs(&udgraph->nodes[i].def, r)) {
        edge = (ud_edge_t *)malloc(sizeof(ud_edge_t));
        assert (edge);
        edge->type = REG_WR;
        edge->src = i;
        edge->dst = n;
        edge->regno = r;
      } else if (regset_belongs (def, r)) {
        if (regset_belongs(&udgraph->nodes[i].def, r)) {
          edge = (ud_edge_t *)malloc(sizeof(ud_edge_t));
          assert (edge);
          edge->type = REG_WW;
          edge->src = i;
          edge->dst = n;
          edge->regno = r;
        } else if (regset_belongs(&udgraph->nodes[i].use, r)) {
          edge = (ud_edge_t *)malloc(sizeof(ud_edge_t));
          assert (edge);
          edge->type = REG_RW;
          edge->src = i;
          edge->dst = n;
          edge->regno = r;
        }
      }

      if (edge) {
        add_edge(&udgraph->nodes[i].successors[n], edge);
      }
    }
  }
  regset_copy (&udgraph->nodes[n].use, use);
  regset_copy (&udgraph->nodes[n].def, def);

  memset_copy (&udgraph->nodes[n].memuse, memuse);
  memset_copy (&udgraph->nodes[n].memdef, memdef);

  if (is_branch)
    udgraph->nodes[n].weight = 1;
  else
    udgraph->nodes[n].weight = 2;
*/
}

static int
matrix_multiply (int n, char a[n][n], char b[n][n])
{
  int any_change = 0;

  int i, j, k;
  for (i = 0; i < n; i++)
  {
    for (j = 0; j < n; j++)
    {
      if (a[i][j] > 1)
	continue;
      for (k = 0; k < n; k++)
      {
	if (k == j)
	  continue;

	if (a[i][k] && b[k][j])
	{
	  a[i][j] = 2;
	  any_change = 1;
	}
      }
    }
  }

  return any_change;
}

static void
compute_transitive_closure (int n, char trans[n][n])
{
  char edges[n][n];
  int i;
  for (i = 0; i < n; i++)
  {
    int j;
    for (j = 0; j < n; j++)
    {
      edges[i][j] = trans[i][j];
    }
  }

  int any_change = 1;

  while (any_change)
  {
    any_change = matrix_multiply (n, trans, edges);
  }
}

static void
normalize_udgraph (udgraph_t *udgraph)
{
  char trans[udgraph->n][udgraph->n];

  int i;
  for (i = 0; i < udgraph->n; i++)
  {
    int j;
    for (j = 0; j < udgraph->n; j++)
    {
      if (udgraph->nodes[i].successors[j])
	trans[i][j] = 1;
      else
	trans[i][j] = 0;
    }
  }

  compute_transitive_closure (udgraph->n, trans);

  for (i = 0; i < udgraph->n; i++)
  {
    int j;
    for (j = 0; j < udgraph->n; j++)
    {
      //dbg ("trans[%d][%d]=%d\n", i, j, trans[i][j]);
      if ((trans[i][j] > 1) && udgraph->nodes[i].successors[j])
      {
	//dbg ("pruning edge %d, %d\n", i, j);
	ud_edge_t *edge, *edge_next;
	for (edge = udgraph->nodes[i].successors[j]; edge; edge=edge_next)
	{
	  edge_next = edge->next;
	  free (edge);
	}
	udgraph->nodes[i].successors[j] = NULL;
      }
    }
  }

  for (i = 0; i < udgraph->n; i++)
  {
    int j;
    for (j = 0; j < udgraph->n; j++)
    {
      udgraph->nodes[j].predecessors[i] = udgraph->nodes[i].successors[j];
    }
  }
}

static void
merge_nodes (udgraph_t *udgraph, int m, int n)
{
  assert (!udgraph->nodes[n].successors[m]);
  assert (!udgraph->nodes[m].successors[m]);
  assert (!udgraph->nodes[n].successors[n]);
  
  /* delete all successors m->n edges */
  ud_edge_t *cur, *next;
  for (cur=udgraph->nodes[m].successors[n]; cur; cur=next)
  {
    next = cur->next;
    free(cur);
  }
  udgraph->nodes[m].successors[n] = NULL;

  /*udgraph->nodes[m].weight = MAX(udgraph->nodes[m].weight,
      udgraph->nodes[n].weight);*/
  if (udgraph->nodes[m].weight < udgraph->nodes[n].weight) {
    udgraph->nodes[m].weight = udgraph->nodes[n].weight;
  }

  int i;
  for (i = 0; i < udgraph->n; i++)
  {
    ud_edge_t *ptr1 = udgraph->nodes[m].successors[i];
    ud_edge_t *ptr2 = udgraph->nodes[n].successors[i];
    ud_edge_t *ptr = NULL;

    while (ptr1 || ptr2)
    {
      if (ptr1)
      {
	if (!ptr2 || (ptr2->type < ptr1->type
	      || (ptr2->type == ptr1->type && ptr2->regno < ptr1->regno)))
	{
	  if (ptr)
	    ptr->next = ptr1;
	  else
	    udgraph->nodes[m].successors[i] = ptr1;
	  ptr1 = ptr1->next;
	}
	else
	{
	  assert (ptr2);
	  if (ptr)
	    ptr->next = ptr2;
	  else
	    udgraph->nodes[m].successors[i] = ptr2;
	  ptr2 = ptr2->next;
	}
      }
      else
      {
	assert (ptr2);
	if (ptr)
	  ptr->next = ptr2;
	else
	  udgraph->nodes[m].successors[i] = ptr2;
	ptr2 = ptr2->next;
      }
    }
  }

  for (i = 0; i < udgraph->nodes[n].num_insns; i++)
  {
    udgraph->nodes[m].insns[i+udgraph->nodes[m].num_insns] =
      udgraph->nodes[n].insns[i];
    DBG (UDGRAPH, "udgraph->nodes[%d].insns[%d]=%d\n", m,
	i+udgraph->nodes[m].num_insns, udgraph->nodes[n].insns[i]);
  }
  udgraph->nodes[m].num_insns += udgraph->nodes[n].num_insns;
  DBG (UDGRAPH, "udgraph->nodes[%d].num_insns=%d\n", m,
      udgraph->nodes[m].num_insns);

  regset_t use;
  regset_copy (&use, &udgraph->nodes[n].use);
  regset_diff (&use, &udgraph->nodes[m].def);
  regset_union (&use, &udgraph->nodes[m].use);
  regset_copy (&udgraph->nodes[m].use, &use);

  regset_union (&udgraph->nodes[m].def, &udgraph->nodes[n].def);

  memset_t memuse;
  memset_init (&memuse);
  memset_copy (&memuse, &udgraph->nodes[n].memuse);
  //memset_diff (&memuse, &udgraph->nodes[m].memdef);
  memset_union (&memuse, &udgraph->nodes[m].memuse);
  memset_copy (&udgraph->nodes[m].memuse, &memuse);
  memset_free (&memuse);

  memset_union (&udgraph->nodes[m].memdef, &udgraph->nodes[n].memdef);

  int positions[udgraph->n];
  int _m;// = MIN(m,n);
  int _n;// = MAX(m,n);
  if (m < n) {
    _m = m;
    _n = n;
  } else {
    _m = n;
    _n = m;
  }
  /* shift all instructions appropriately */
  for (i = 0; i < udgraph->n; i++)
  {
    if (i == _n)
    {
      positions[i] = _m;
      if (m == _n)
	udgraph_node_copy (&udgraph->nodes[positions[i]], &udgraph->nodes[i]);
    }
    else if (i > _n)
    {
      positions[i] = i-1;
      udgraph_node_copy (&udgraph->nodes[positions[i]], &udgraph->nodes[i]);
    }
    else
      positions[i] = i;
  }

  for (i = 0; i < udgraph->n-1; i++) {
    int j;
    for (j = 0; j < udgraph->n; j++) {
      DBG (UDGRAPH, "udgraph->nodes[%d].successors[%d]=%p\n", i, j,
          udgraph->nodes[i].successors[j]);
      ud_edge_t *edge;
      for (edge = udgraph->nodes[i].successors[j]; edge; edge=edge->next) {
        edge->src = positions[edge->src];
        edge->dst = positions[edge->dst];

        assert (edge->src < udgraph->n - 1);
        assert (edge->dst < udgraph->n - 1);
      }

      assert (positions[j] <= j);
      assert (positions[j] < udgraph->n - 1);
      if (positions[j] < j && !udgraph->nodes[i].successors[positions[j]]) {
        DBG (UDGRAPH, "setting nodes[%d].successors[%d] to %p\n", i,
            positions[j], udgraph->nodes[i].successors[j]);
        udgraph->nodes[i].successors[positions[j]] =
          udgraph->nodes[i].successors[j];

        DBG (UDGRAPH, "setting nodes[%d].successors[%d] to NULL\n", i, j);
        udgraph->nodes[i].successors[j] = NULL;
      }
    }
  }

  /*
  int j;
  for (i = 0; i < udgraph->n; i++)
    for (j = 0; j < udgraph->n; j++)
      DBG (UDGRAPH, "nodes[%d].successors[%d]=%p\n",i,j,udgraph->nodes[i].successors[j]);
      */

  udgraph->n--;

  /* reset all pred pointers */
  for (i = 0; i < udgraph->n; i++) {
    int j;
    for (j = 0; j < udgraph->n; j++) {
      if (i == j) {
        assert (!udgraph->nodes[i].successors[j]);
        udgraph->nodes[j].predecessors[i] = NULL;
        continue;
      }
      udgraph->nodes[j].predecessors[i] = udgraph->nodes[i].successors[j];
    }
  }
}

static char *
udgraph_to_string (udgraph_t const *udgraph, char *buf, int buf_size)
{
  char *ptr = buf, *end = buf + buf_size;

  int i;
  for (i = 0; i < udgraph->n; i++) {
    ptr += snprintf (ptr, end-ptr, "%d [insns", i);
    int j;
    for (j = 0; j < udgraph->nodes[i].num_insns; j++) {
      ptr += snprintf (ptr, end-ptr, "%c%d", j?',':' ',
          udgraph->nodes[i].insns[j]);
    }
    ptr += snprintf (ptr, end - ptr, "] (use %s, def %s) depends on",
        regset_to_string(&udgraph->nodes[i].use, as1, sizeof as1),
        regset_to_string(&udgraph->nodes[i].def, as2, sizeof as2));
    for (j = 0; j < udgraph->n; j++) {
      if (udgraph->nodes[i].successors[j]) {
        ptr += snprintf (ptr, end-ptr, "%c%d[", j?' ':',', j);
        ud_edge_t *edge;
        for (edge = udgraph->nodes[i].successors[j]; edge; edge=edge->next) {
          if (edge->type == MEM)
            ptr += snprintf(ptr, end-ptr, "M%c", edge->next?',':']');
          else
            ptr += snprintf(ptr, end-ptr, "%d%c", edge->regno,
                edge->next?',':']');
        }
      }
    }

    ptr += snprintf (ptr, end - ptr, "; preds");
    for (j = 0; j < udgraph->n; j++) {
      if (udgraph->nodes[i].predecessors[j]) {
        ptr += snprintf (ptr, end-ptr, "%c%d[", j?' ':',', j);
        ud_edge_t *edge;
        for (edge = udgraph->nodes[i].predecessors[j]; edge; edge=edge->next) {
          if (edge->type == MEM)
            ptr += snprintf(ptr, end-ptr, "M%c", edge->next?',':']');
          else
            ptr += snprintf(ptr, end-ptr, "%d%c", edge->regno,
                edge->next?',':']');
        }
      }
    }

    ptr += snprintf(ptr, end - ptr, ". weight %d\n", udgraph->nodes[i].weight);
  }
  return buf;
}

static void
merge_udgraph (udgraph_t *udgraph)
{
  for(;;) {
    int merge_candidate_src = -1;
    int merge_candidate_dst = -1;
    int merge_gain = 0;

    int i;
    for (i = 0; i < udgraph->n; i++) {
      int j;
      for (j = 0; j < udgraph->n; j++) {
        ud_edge_t *pred = udgraph->nodes[i].predecessors[j];
        if (!pred)
          continue;

        int gain = pred->type*100 + pred->regno;
        if (gain > merge_gain) {
          merge_candidate_src = j;
          merge_candidate_dst = i;
          merge_gain = gain;
        }
      }

      for (j = 0; j < udgraph->n; j++) {
        int gain = 0;
        ud_edge_t *succ;

        for (succ = udgraph->nodes[i].successors[j]; succ; succ=succ->next) {
          gain += succ->type*100 + succ->regno;
        }

        //dbg ("%d,%d merge_gain %d\n", i, j, merge_gain);
        if (gain > merge_gain) {
          merge_candidate_src = i;
          merge_candidate_dst = j;
          merge_gain = gain;
        }
      }

    }

    if (merge_candidate_src == -1)
      break;

    assert (merge_candidate_dst != -1);

    DBG (UDGRAPH, "calling merge_nodes %d,%d\n", merge_candidate_src,
        merge_candidate_dst);
    merge_nodes (udgraph, merge_candidate_src, merge_candidate_dst);
    normalize_udgraph (udgraph);

    DBG (UDGRAPH, "printing udgraph after merging nodes %d,%d\n%s\n",
        merge_candidate_src, merge_candidate_dst,
        udgraph_to_string (udgraph, udgraph_str1, sizeof udgraph_str1));
  }
}


static void
udgraph_compute_sort_order (int *order, int n, udgraph_t const *udgraph)
{
  int i, num_insns=0;
  char outputted_nodes[udgraph->n];
  int num_outputted_nodes = 0;
  int num_iter = 0;
  memset(outputted_nodes, 0x0, sizeof outputted_nodes);

  while (num_outputted_nodes < udgraph->n) {
    assert (num_iter < udgraph->n);
    num_iter++;

    unsigned cur_output_weight = (unsigned)-1;
    int cur_output_node = -1;
    for (i = 0; i < udgraph->n; i++) {
      if (outputted_nodes[i])
        continue;

      int ready = 1;
      int j;
      for (j = 0; j < udgraph->n; j++) {
        if (udgraph->nodes[i].predecessors[j] && !outputted_nodes[j]) {
          ready = 0;
          break;
        }
      }
      if (!ready)
        continue;

      if (udgraph->nodes[i].weight < cur_output_weight) {
        cur_output_weight = udgraph->nodes[i].weight;
        cur_output_node = i;
      }
    }

    assert (cur_output_node >= 0);
    i = cur_output_node;
    outputted_nodes[i] = 1;
    num_outputted_nodes++;
    int j;
    for (j = 0; j < udgraph->nodes[i].num_insns; j++) {
      order[num_insns] = udgraph->nodes[i].insns[j];
      num_insns++;
    }
  }
  assert (num_insns == n);
}

static void udgraph_free (udgraph_t *udgraph)
{
  int i;
  for (i = 0; i < udgraph->n; i++)
  {
    udgraph_node_free (&udgraph->nodes[i], udgraph->n);
  }
  free(udgraph->nodes);
}

static void
usedef_sort_order (int *order, bbl_t *bbl,
    regset_t const *bbl_live_out0, regset_t const *bbl_live_out1,
    live_ranges_t const *bbl_live_ranges0,
    live_ranges_t const *bbl_live_ranges1)
{
  NOT_IMPLEMENTED();
#if 0
  udgraph_t udgraph_store;
  udgraph_t *udgraph = &udgraph_store;
  udgraph->nodes = NULL;
  udgraph->n = 0;

  int n = tb->size >> 2;
  int i;
  target_ulong ptr;

  /*
  printf("Before udsort:\n");
  for (i = 0; i < n; i++) {
    src_insn_t insn;
    //target_disas(stdout, tb->pc, tb->size, 0);
    src_insn_fetch(&insn, tb->pc + i * 4);
    src_insn_to_string(&insn, as1, sizeof as1);
    printf("%s", as1);
    //printf(" %d", order[i]);
  }
  printf("bbl_live_out0 = %s\n", bbl_live_out0?
      regset_to_string(bbl_live_out0, as1, sizeof as1):NULL);
  printf("bbl_live_out1 = %s\n", bbl_live_out1?
      regset_to_string(bbl_live_out1, as1, sizeof as1):NULL);
  printf("bbl_live_ranges0 = %s\n", bbl_live_out0?
      live_ranges_to_string_full(bbl_live_ranges0, bbl_live_out0,
	as1, sizeof as1):NULL);
  printf("bbl_live_ranges1 = %s\n", bbl_live_out1?
      live_ranges_to_string_full(bbl_live_ranges1, bbl_live_out1, as1,
	sizeof as1):NULL);
  */

  
  udgraph->n = n;
  udgraph->nodes = (udgraph_node_t *)malloc(n*sizeof(udgraph_node_t));
  assert (udgraph->nodes);

  for (i = 0; i < n; i++) {
    udgraph_node_init (&udgraph->nodes[i], n);
    udgraph->nodes[i].insns[0] = i;
    udgraph->nodes[i].num_insns = 1;
  }

  DBG (UDGRAPH, "bbl_live_out0: %s\nbbl_live_out1: %s\n"
      "bbl_live_ranges0: %s\nbbl_live_ranges1: %s\n",
      bbl_live_out0?regset_to_string(bbl_live_out0, as1, sizeof as1):"(null)",
      bbl_live_out1?regset_to_string(bbl_live_out1, as2, sizeof as2):"(null)",
      bbl_live_out0?
       live_ranges_to_string(bbl_live_ranges0, bbl_live_out0,
         as3, sizeof as3):"(null)",
      bbl_live_out1?
       live_ranges_to_string(bbl_live_ranges1, bbl_live_out1, as4,
         sizeof as4):"(null)");

  for (i = 0, ptr = tb->pc; ptr < tb->pc + tb->size; i++, ptr+=4) {
    regset_t use, def;
    memset_t memuse, memdef;
    src_insn_t insn;

    regset_clear(&use);    regset_clear(&def);
    memset_init (&memuse);     memset_clear(&memuse);
    memset_init (&memdef);     memset_clear(&memdef);

    bool fetch = src_insn_fetch(&insn, ptr, NULL);
    if (fetch) {
      src_insn_get_usedef(&insn, &use, &def, &memuse, &memdef);
    }

    /*
    regset_t tuse, tdef;
    memset_t tmemuse, tmemdef;
    memset_init (&tmemuse);
    memset_init (&tmemdef);
    uint32_t opcode = ldl_input(ptr);
    get_ppc_usedef (opcode, &tuse, &tdef, &tmemuse, &tmemdef);
    assert(ppc_regset_equals(&use, &tuse));
    assert(ppc_regset_equals(&def, &tdef));
    assert(memset_equals(&memdef, &tmemdef));
    assert(memset_equals(&memuse, &tmemuse));
    memset_free (&tmemuse);
    memset_free (&tmemdef);
    */


    //src_insn_t src_insn;
    //int disas = ppc_disassemble (&ppc_insn, (char *)&opcode);

    NOT_IMPLEMENTED();
#if 0
    usedef_register(udgraph, i, &use, &def, &memuse, &memdef,
        ((ptr + 4) == (tb->pc + tb->size))?bbl_live_out0:&tb->live_regs[i],
        fetch?src_insn_is_branch(&insn):0,
        tb->next_possible_eip[2]?1:0);
#endif

    regset_t dlive0;
    regset_copy (&dlive0,
        bbl_live_out1?bbl_live_out1:bbl_live_out0?
        bbl_live_out0:regset_empty());
    regset_intersect (&dlive0, &def);

    int j;
    for (j = 0; j < PPC_NUM_REGS; j++) {
      if (regset_belongs(&dlive0, j)) {
        live_ranges_t const *live_ranges_out;
        live_ranges_out = bbl_live_ranges1?bbl_live_ranges1:bbl_live_ranges0?
          bbl_live_ranges0:live_ranges_zero();
        DBG (UDGRAPH, "Adding %d to udgraph->nodes[%d].weight %d->%d\n",
            live_ranges_out->table[j], i, udgraph->nodes[i].weight,
            udgraph->nodes[i].weight+live_ranges_out->table[j]);
        udgraph->nodes[i].weight += live_ranges_out->table[j];
      }

      if (j>=PPC_REG_CRFN(0) && j<PPC_REG_CRFN(8)) {
        //add an extra bit for the flags
        udgraph->nodes[i].weight++;
      }
    }

    memset_free (&memuse);
    memset_free (&memdef);
  }


  normalize_udgraph (udgraph);

  DBG(UDGRAPH, "%lx: printing udgraph before call to merge_udgraph\n%s\n",
      tb->pc, udgraph_to_string(udgraph, udgraph_str1, sizeof udgraph_str1));

  merge_udgraph (udgraph);

  udgraph_compute_sort_order (order, n, udgraph);

  /*
  printf("After udsort:\n");
  for (i = 0; i < n; i++) {
    src_insn_t insn;
    src_insn_fetch(&insn, tb->pc+order[i] * 4);
    src_insn_to_string(&insn, as1, sizeof as1);
    printf("%s", as1);
    //target_disas(tb->pc+order[i]*4, 4, 0);
  }
  */

  udgraph_free (udgraph);
#endif
}

void
usedef_sort(struct input_exec_t *in)
{
#if ARCH_SRC == ARCH_PPC
#if 0
  //src_ulong start_ip;
  int iter;

  //while (!any_change)  //XXX
  for (iter = 0; iter < 2; iter++) {
    build_cfg(in);
    analyze_cfg(in);

    int i;
    for (i = 0; i < in->nb_tbs; i++) {
      tb_t *tb = &in->tbs[i];
      int order[tb->size>>2];
      int j;

      /*
          XXX: do not do this, as it changes the live_ranges values all
          over for different peep slices. causes problems with fix-transmaps
          if (   !(belongs_to_peep_program_slice(tb->pc)
              && belongs_to_peep_program_slice(tb->pc+tb->size-4))) {
            continue;
          }
      */
      usedef_sort_order (order, tb,
          (tb->num_successors>0)?&in->tbs[tb->succ[0]].live_regs[0]:NULL,
          (tb->num_successors>1)?&in->tbs[tb->succ[1]].live_regs[0]:NULL,
          (tb->num_successors>0)?&in->tbs[tb->succ[0]].live_ranges[0]:NULL,
          (tb->num_successors>1)?&in->tbs[tb->succ[1]].live_ranges[0]:NULL);

      for (j = 0; j < (tb->size >> 2); j++) {
        order[j] = j;
      }

      uint8_t new_buf[tb->size];
      uint8_t *new_ptr = new_buf;
      uint8_t opcode[SRC_MAX_ILEN];
      for (j = 0; j < (tb->size>>2); j++) {
        src_insn_t insn;
        bool fetch = src_insn_fetch(&insn, tb_insn_address(tb, order[j]), NULL);
        tb_fetch_insn(tb, order[j], opcode);
        //printf("0x%x: read: 0x%x\n", tb->pc + order[j]*4, *(uint32_t *)opcode);
#if ARCH_SRC == ARCH_PPC
        if (fetch) {
          if (insn.tag.BD != tag_invalid) {
            insn.val.BD += (order[j] - j)<<2;
            src_insn_get_bincode(&insn, opcode);
          } else if (insn.tag.LI != tag_invalid) {
            insn.val.LI += (order[j] - j)<<2;
            src_insn_get_bincode(&insn, opcode);
          }
        }
        //printf("0x%x: writ: 0x%x\n", tb->pc + order[j]*4, *(uint32_t *)opcode);
        memcpy(new_ptr, opcode, 4);
        new_ptr += 4;
#elif ARCH_SRC == ARCH_I386
        NOT_IMPLEMENTED();
#endif
      }

      new_ptr = new_buf;
      for (j = 0; j < (tb->size>>2); j++) {
        uint32_t opcode = *(uint32_t *)new_ptr;
        stl_input(tb->pc + (j<<2), opcode);
        /*
        uint32_t old_opcode = ldl_input(tb->pc + (j << 2));
        if (opcode != old_opcode) {
          printf("%x: %x replacing %x\n", tb->pc + (j << 2), opcode, old_opcode);
          stl_input(tb->pc + (j<<2), old_opcode);
        }
        */
        new_ptr += 4;
      }
    }
    free_cfg(in);
  }
  //in->entry = start_ip;
#endif
#endif
}
