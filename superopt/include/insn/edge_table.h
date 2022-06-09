#ifndef __EDGE_TABLE_H
#define __EDGE_TABLE_H
#include "support/types.h"

struct edge_table_t;
typedef struct edge_table_t edge_table_t;

void edge_table_init(edge_table_t  **table, int size);
void edge_register(edge_table_t *table, src_ulong nip1,
    src_ulong nip2, long long num_occur);

long long edge_table_query(edge_table_t const *table, src_ulong nip1,
    src_ulong nip2);

void edge_table_dump(FILE *fp, edge_table_t const *table, char const *dir);
void edge_table_read(FILE *fp, edge_table_t *table);

void *edge_table_find_next(edge_table_t const *table, void *iter,
    src_ulong nip1, src_ulong *nip2, long long *num_occur);

void edge_table_free (edge_table_t *table);

unsigned long long exec_frequency(unsigned long nip);

#endif
