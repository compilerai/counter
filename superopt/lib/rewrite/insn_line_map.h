#ifndef INSN_LINE_MAP_H
#define INSN_LINE_MAP_H

#include <stdio.h>
#include <stdbool.h>

#define MAX_URI_LEN 64

typedef struct insn_line_map_entry_t {
  unsigned long insn_addr;
  int row, col, di;
  char uri[MAX_URI_LEN];
} insn_line_map_entry_t;

typedef struct insn_line_map_t {
  int num_entries;
  struct insn_line_map_entry_t *entries;
} insn_line_map_t;

void insn_line_map_copy(insn_line_map_t *dst, insn_line_map_t const *src);
void fscanf_insn_line_map(FILE *fp, insn_line_map_t *dst);
size_t insn_line_map_to_string(struct insn_line_map_t const *dst, char *buf, size_t size);
void insn_line_map_init(insn_line_map_t *map);
void insn_line_map_free(insn_line_map_t *map);
bool insn_line_map_get_entry(insn_line_map_entry_t *out, insn_line_map_t const *haystack, long insn_addr);
void insn_line_map_search_entry(insn_line_map_entry_t *out, insn_line_map_t const *haystack, long insn_addr);
long insn_line_map_entry_diff(insn_line_map_entry_t const *a, insn_line_map_entry_t const *b);

#endif
