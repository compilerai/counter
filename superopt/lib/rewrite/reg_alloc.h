#ifndef __REG_ALLOC_H
#define __REG_ALLOC_H

struct translation_instance_t;

void solve_register_allocation(struct translation_instance_t const *ti,
    int *registers, int num_registers,
    int *states, int **transitions, int num_states,
    int (*register_use)(tb_t *tb, unsigned long pc),
    int (*register_def)(tb_t *tb, unsigned long pc));

int query_regalloc_state(translation_instance_t const *ti, unsigned long pc);
void free_regalloc_graph(translation_instance_t const *ti);


#endif
