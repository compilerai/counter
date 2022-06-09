#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/debug.h"
#include "support/c_utils.h"
#include "rewrite/symbol_map.h"

void
symbol_map_init(symbol_map_t *map)
{
  map->num_symbols = 0;
}

void
symbol_map_from_string(symbol_map_t *map, char const *buf)
{
  char const *ptr, *remaining, *endline;
  int symbol_num, i;
  char tmpbuf[512];

  ptr = buf;
  i = 0;
  map->num_symbols = 0;
  while (strstart(ptr, "C_SYMBOL", &remaining)) {
    char const *colon;
    colon = strchr(ptr, ':');
    ASSERT(colon);
    ASSERT(colon - remaining < sizeof tmpbuf);
    memcpy(tmpbuf, remaining, colon- remaining);
    ASSERT(tmpbuf[colon - remaining - 1] == ' ');
    tmpbuf[colon - remaining - 1] == '\0'; // XXX = intended?
    symbol_num = atoi(tmpbuf);
    ASSERT(symbol_num == i);
    i++;
    ASSERT(colon[1] == ' ');
    endline = strchr(colon, '\n');
    ASSERT(endline - colon <= sizeof map->table[i].name);
    memcpy(map->table[i].name, colon + 1, endline - colon - 1);
    map->table[i].name[endline - colon - 1] = '\0';
    map->num_symbols++;
  }
}

void
symbol_map_to_string(symbol_map_t const *map, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;
  *ptr = '\0';
  for (i = 0; i < map->num_symbols; i++) {
    ptr += snprintf(ptr, end - ptr, "C_SYMBOL%d : %s\n", i, map->table[i].name);
    ASSERT(ptr < end);
  }
}

/*
void
fscanf_symbol_map(FILE *fp, symbol_map_t *out)
{
  long ret, max_alloc;
  char *tmap_string;

  max_alloc = 40960;
  tmap_string = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(tmap_string);
  ret = fscanf_till_next_peephole_section(fp, tmap_string, max_alloc);
  symbol_map_from_string(out, tmap_string);
  free(tmap_string);
}*/

void
fscanf_symbol_map(FILE *fp, symbol_map_t *map)
{
#define MAX_SYMBOL_NAME_SIZE 512
  char *symbol_name;
  int symbol_num;
  long symbol_val, symbol_size;
  symbol_name = (char *)malloc(MAX_SYMBOL_NAME_SIZE * sizeof(char));
  ASSERT(symbol_name);
  symbol_map_init(map);
  while (fscanf(fp, "C_SYMBOL%d : %s : %lx : %ld\n", &symbol_num, symbol_name, &symbol_val, &symbol_size) == 4) {
    ASSERT(symbol_num >= map->num_symbols);
    int i;
    for (i = map->num_symbols; i < symbol_num; i++) {
      strncpy(map->table[i].name, "SYMBOL_DOES_NOT_EXIST", sizeof map->table[i].name);
      map->table[i].value = 0;
      map->table[i].size = 0;
    }
    map->num_symbols = symbol_num;
    ASSERT(map->num_symbols < SYMBOL_MAP_MAX_NUM_SYMBOLS);
    strncpy(map->table[map->num_symbols].name, symbol_name, sizeof map->table[map->num_symbols].name);
    map->table[map->num_symbols].value = symbol_val;
    map->table[map->num_symbols].size = symbol_size;
    map->num_symbols++;
    //printf("found symbol %d: %s\n", symbol_num, symbol_name);
  }
}

void
symbol_map_copy(symbol_map_t *dst, symbol_map_t const *src)
{
  memcpy(dst, src, sizeof(symbol_map_t));
}

size_t
symbol_map_get_symbol_size(symbol_map_t const *map, int symbol_id)
{
  if (symbol_id >= 0 && symbol_id < map->num_symbols) {
    return map->table[symbol_id].size;
  } else {
    return 0;
  }
}
