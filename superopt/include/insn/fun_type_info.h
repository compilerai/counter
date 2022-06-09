#ifndef FUN_TYPE_INFO_H
#define FUN_TYPE_INFO_H
#include <stdbool.h>
#include <stdio.h>

typedef struct sym_type_info_t {
  char *name;
  char *type_name;
  size_t typesize;
} sym_type_info_t;

typedef struct fun_type_info_t {
  sym_type_info_t ret;
  sym_type_info_t *params;
  size_t num_params;
} fun_type_info_t;

bool fscanf_fun_type_info(fun_type_info_t *fti, FILE *fp);
void fun_type_info_copy_shallow(fun_type_info_t *dst, fun_type_info_t const *src);
void fun_type_info_free(fun_type_info_t *fti);
void fun_type_info_print(FILE *fp, fun_type_info_t const *fti);

#endif
