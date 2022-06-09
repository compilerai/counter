#pragma once
#include "valtag/symbol.h"

#define SYMBOL_MAP_MAX_NUM_SYMBOLS 256

typedef struct symbol_map_t {
  int num_symbols;
  symbol_t table[SYMBOL_MAP_MAX_NUM_SYMBOLS];
} symbol_map_t;

void symbol_map_init(symbol_map_t *map);
void symbol_map_from_string(symbol_map_t *map, char const *buf);
void symbol_map_to_string(symbol_map_t const *map, char *buf, size_t size);
void fscanf_symbol_map(FILE *fp, symbol_map_t *out);
void symbol_map_copy(symbol_map_t *dst, symbol_map_t const *src);
size_t symbol_map_get_symbol_size(symbol_map_t const *map, int symbol_id);
