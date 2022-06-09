#pragma once
//#include "rewrite/translation_instance.h"
#include <stdlib.h>

#define NEXTPC_FUNCTION_NAME_MAP_MAX_NUM_NEXTPCS 256
#define MAX_FUNCTION_NAME_LEN 256

typedef struct nextpc_function_name_map_entry_t {
  char name[MAX_FUNCTION_NAME_LEN];
} nextpc_function_name_map_entry_t;

typedef struct nextpc_function_name_map_t {
  int num_nextpcs;
  nextpc_function_name_map_entry_t table[NEXTPC_FUNCTION_NAME_MAP_MAX_NUM_NEXTPCS];
} nextpc_function_name_map_t;

void nextpc_function_name_map_init(nextpc_function_name_map_t *map);
void nextpc_function_name_map_from_string(nextpc_function_name_map_t *map, char const *buf);
char *nextpc_function_name_map_to_string(nextpc_function_name_map_t const *map, char *buf, size_t size);
//void nextpc_function_name_map_from_peep_comment(nextpc_function_name_map_t *map, char const *comment);
int nextpc_function_name_map_find_function_name(nextpc_function_name_map_t const *map, char const *function_name);
void nextpc_function_name_map_copy(nextpc_function_name_map_t *dst, nextpc_function_name_map_t const *src);
int nextpc_function_name_map_add(nextpc_function_name_map_t *map, char const *function_name);
