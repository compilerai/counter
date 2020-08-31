#ifndef PEEP_TYPE_CONS_H
#define PEEP_TYPE_CONS_H
#include "valtag/state_elem_desc.h"

#define POOL_ID_STACK 1
#define POOL_ID_NONSTACK 2

typedef enum peep_datatype_mempool_t {
  PEEP_DATATYPE_MEM_POOL_KNOWN_OFFSET,
  PEEP_DATATYPE_MEM_POOL_UNKNOWN_OFFSET,
  PEEP_DATATYPE_MEM_POOL_ANY,
} peep_datatype_mempool_t;

typedef struct peep_type_cons_elem_t {
  state_elem_desc_t loc;
  peep_datatype_mempool_t peep_datatype_mempool;
  int pool_id;
  int offset;
} peep_type_cons_elem_t;

typedef struct peep_type_cons_t {
  int num_elems, elems_size;
  peep_type_cons_elem_t *elems;
} peep_type_cons_t;

void peep_type_cons_init(peep_type_cons_t *peep_type_cons);

void peep_type_cons_add(peep_type_cons_t *peep_type_cons,
    peep_type_cons_elem_t const *new_elem);

char *peep_type_cons_to_string(peep_type_cons_t const *peep_type_cons, char *buf,
    size_t size);

size_t peep_type_cons_from_string(peep_type_cons_t *peep_type_cons, char const *buf,
    size_t size);

bool peep_type_cons_less(peep_type_cons_t const *a, peep_type_cons_t const *b);

#endif /* peep_type_cons.h */
