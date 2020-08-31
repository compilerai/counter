#include "translation_instance.h"
#include "reg_alloc.h"

typedef struct regalloc_graph_tb_t
{
  int n;
  regalloc_possibilities_t *regalloc_possibilities;
  regalloc_possibilities_elem_t *regalloc_decisions;
};

typedef struct regalloc_graph_t
{
  translation_instance_t const *ti;
  regalloc_graph_tb_t *tbs;
} regalloc_graph_t;

void solve_regalloc(translation_instance_t const *ti,
    int num_values,
    int *states, int **transitions, int num_states,
    int (*mark_register_use)(unsigned long pc),
    int (*mark_register_def)(unsigned long pc))
{
  int i;
  for (i = 0; i < num_registers; i++)
  {
  }
}
