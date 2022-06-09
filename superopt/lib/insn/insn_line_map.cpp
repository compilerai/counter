#include <map>
#include "insn/insn_line_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include "support/debug.h"
#include "support/c_utils.h"

static void
insn_line_map_entry_copy(insn_line_map_entry_t *dst, insn_line_map_entry_t const *src)
{
  memcpy(dst, src, sizeof(insn_line_map_entry_t));
}

void
insn_line_map_copy(insn_line_map_t *dst, insn_line_map_t const *src)
{
  int i;
  ASSERT(dst);
  ASSERT(dst->num_entries == 0);
  ASSERT(dst->entries == NULL);
  if (!src) {
    return;
  }
  dst->num_entries = src->num_entries;
  dst->entries = (struct insn_line_map_entry_t *)malloc(dst->num_entries * sizeof(struct insn_line_map_entry_t));
  ASSERT(dst->entries);
  for (i = 0; i < src->num_entries; i++) {
    insn_line_map_entry_copy(&dst->entries[i], &src->entries[i]);
  }
}

void
fscanf_insn_line_map(FILE *fp, insn_line_map_t *map)
{
  struct insn_line_map_entry_t *e;
  size_t ret;

  ASSERT(map);
  ASSERT(map->num_entries == 0);
  ASSERT(!map->entries);
  ret = fscanf(fp, "insn_line_map %d entries\n", &map->num_entries);
  ASSERT(ret == 1);

  map->entries = (struct insn_line_map_entry_t *)malloc(map->num_entries * sizeof(struct insn_line_map_entry_t));
  ASSERT(map->entries);

  e = map->entries;
  while ((ret = fscanf(fp, ".i%ld: %*x %d %d %d %s\n", &e->insn_addr, &e->row, &e->col, &e->di, e->uri)) == 5) {
    e++;
    ASSERT(e <= map->entries + map->num_entries);
  }
  ASSERT(e == map->entries + map->num_entries);
}

static size_t
insn_line_map_entry_to_string(struct insn_line_map_entry_t const *e, char *buf, size_t size)
{
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  ptr += snprintf(ptr, end - ptr, ".i%ld: %lx %d %d %d %s\n", e->insn_addr, e->insn_addr, e->row, e->col, e->di, e->uri);
  ASSERT(ptr < end);
  return ptr - buf;
}

size_t
insn_line_map_to_string(struct insn_line_map_t const *map, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "insn_line_map %d entries\n", map->num_entries);
  ASSERT(ptr < end);
  for (i = 0; i < map->num_entries; i++) {
    ptr += insn_line_map_entry_to_string(&map->entries[i], ptr, end - ptr);
    /**ptr = '\n';
    ptr++;
    *ptr = '\0';*/
    ASSERT(ptr < end);
  }
  return ptr - buf;
}

void
insn_line_map_init(insn_line_map_t *map)
{
  map->num_entries = 0;
  map->entries = NULL;
}

void
insn_line_map_free(insn_line_map_t *map)
{
  free(map->entries);
  map->num_entries = 0;
  map->entries = NULL;
}

bool
insn_line_map_get_entry(insn_line_map_entry_t *out, insn_line_map_t const *haystack, long insn_addr)
{
  int i;
  for (i = 0; i < haystack->num_entries; i++) {
    if (haystack->entries[i].insn_addr == insn_addr) {
      insn_line_map_entry_copy(out, &haystack->entries[i]);
      return true;
    }
  }
  return false;
}

static long
insn_addr_distance(long addr1, long addr2)
{
  return ABS(addr1 - addr2);
}

void
insn_line_map_search_entry(insn_line_map_entry_t *out, insn_line_map_t const *haystack, long insn_addr)
{
  int i;
  out->insn_addr = 0;
  out->row = -1;
  out->col = -1;
  out->di = -1;
  out->uri[0] = '\0';
  for (i = 0; i < haystack->num_entries; i++) {
    DBG(INSN_LINE_MAP, "checking out->insn_addr %d and insn_addr %d against haystack[%d] entry %d\n", (int)out->insn_addr, (int)insn_addr, i, (int)haystack->entries[i].insn_addr);
    if (insn_addr_distance(haystack->entries[i].insn_addr, insn_addr) < insn_addr_distance(out->insn_addr, insn_addr)) {
    /*if (   haystack->entries[i].insn_addr <= insn_addr
        && haystack->entries[i].insn_addr >= out->insn_addr)*/
      insn_line_map_entry_copy(out, &haystack->entries[i]);
    }
  }
  DBG(INSN_LINE_MAP, "returning (%d,%d,%d) for %ld\n", (int)out->row, (int)out->col, (int)out->di, insn_addr);
}

static long
insn_line_map_entry_to_long(insn_line_map_entry_t const *a)
{
#define ROW_STEP (1L<<30)
#define COL_STEP (1L<<20)
  return a->row * ROW_STEP + a->col * COL_STEP + a->di;
}

long
insn_line_map_entry_diff(insn_line_map_entry_t const *a, insn_line_map_entry_t const *b)
{
  return insn_line_map_entry_to_long(a) - insn_line_map_entry_to_long(b);
}
