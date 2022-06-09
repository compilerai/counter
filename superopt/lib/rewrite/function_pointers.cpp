#include "rewrite/function_pointers.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "support/debug.h"

struct fp_entry {
  char const *name;
  void *pointer;
};

#define MAX_TABLE_SIZE 64
static struct fp_entry table[MAX_TABLE_SIZE];
static int table_size = 0;

void *
function_pointer_get(char const *name)
{
  int i;
  for (i = 0; i < table_size; i++) {
    if (!strcmp(name, table[i].name)) {
      return table[i].pointer;
    }
  }
  NOT_REACHED();
  return NULL;
}

void
function_pointer_register(char const *name, void *pointer)
{
  table[table_size].name = strdup(name);
  table[table_size].pointer = pointer;
  table_size++;
}
