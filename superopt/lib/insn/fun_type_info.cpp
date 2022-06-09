#include "insn/fun_type_info.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "support/debug.h"

static bool
fscanf_sym_type_info(FILE *fp, sym_type_info_t *sti)
{
  long max_alloc = 4096;
  char *buf;
  int ret;

  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);

  ret = fscanf(fp, " %zu", &sti->typesize);
  ASSERT(ret == 1);
  ret = fscanf(fp, " \"%[^\"]\"", buf);
  if (ret == 0) {
    return false;
  }
  ASSERT(ret == 1);
  ASSERT(strlen(buf) < max_alloc);
  sti->type_name = (char *)malloc((strlen(buf) + 1) * sizeof(char));
  ASSERT(sti->type_name);
  strncpy(sti->type_name, buf, strlen(buf) + 1);
  ret = fscanf(fp, " \"%[^\"]\"", buf);
  ASSERT(ret == 1);
  ASSERT(strlen(buf) < max_alloc);
  sti->name = (char *)malloc((strlen(buf) + 1) * sizeof(char));
  ASSERT(sti->name);
  strncpy(sti->name, buf, strlen(buf) + 1);
  ret = fscanf(fp, " }");
  ASSERT(ret == 0);
  free(buf);
  return true;
}

static void
sym_type_info_copy_shallow(sym_type_info_t *dst, sym_type_info_t const *src)
{
  memcpy(dst, src, sizeof(sym_type_info_t));
}

static void
fscanf_sym_type_info_list(FILE *fp, sym_type_info_t **ls, size_t *lsnum)
{
  bool retb;
  int ret;

  *ls = NULL;
  *lsnum = 0;
  do {
    sym_type_info_t sti;
    char chr;

    ret = fscanf(fp, " %c", &chr);
    ASSERT(ret == 1);
    if (chr == ':') {
      break;
    }
    ASSERT(chr == '{');
    retb = fscanf_sym_type_info(fp, &sti);
    if (!retb) {
      break;
    }
    ASSERT(retb);
    *ls = (sym_type_info_t *)realloc(*ls,
        (*lsnum + 1) * sizeof(sym_type_info_t));
    ASSERT(*ls);
    sym_type_info_copy_shallow(&((*ls)[*lsnum]), &sti);
    (*lsnum)++;
  } while(1);
}

bool
fscanf_fun_type_info(fun_type_info_t *fti, FILE *fp)
{
  long max_alloc = 4096;
  char *buf;
  int ret;

  if (feof(fp)) {
    return false;
  }
  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);
  ret = fscanf(fp, " FUNCTION%*d: %s", buf);
  if (ret != 1) {
    return false;
  }
  ASSERT(ret == 1);
  ASSERT(strlen(buf) < max_alloc);
  fti->ret.name = (char *)malloc((strlen(buf) + 1) * sizeof(char));
  ASSERT(fti->ret.name);
  strncpy(fti->ret.name, buf, strlen(buf) + 1);
  ret = fscanf(fp, " %zu", &fti->ret.typesize);
  ASSERT(ret == 1);
  ret = fscanf(fp, " \"%[^\"]\"", buf);
  ASSERT(ret == 1);
  ASSERT(strlen(buf) < max_alloc);
  fti->ret.type_name = (char *)malloc((strlen(buf) + 1) * sizeof(char));
  ASSERT(fti->ret.type_name);
  strncpy(fti->ret.type_name, buf, strlen(buf) + 1);
  ret = fscanf(fp, " :");
  ASSERT(ret == 0);

  free(buf);

  fscanf_sym_type_info_list(fp, &fti->params, &fti->num_params);
  fscanf(fp, "%*[^\n]\n");
  //fscanf_sym_type_info_list(fp, &fti->vars, &fti->num_vars);
  return true;
}

void
fun_type_info_copy_shallow(fun_type_info_t *dst, fun_type_info_t const *src)
{
  memcpy(dst, src, sizeof(fun_type_info_t));
}

static void
sym_type_info_free(sym_type_info_t *sti)
{
  ASSERT(sti->name);
  free(sti->name);
  ASSERT(sti->type_name);
  free(sti->type_name);
}

void
fun_type_info_free(fun_type_info_t *fti)
{
  size_t i;

  sym_type_info_free(&fti->ret);
  for (i = 0; i < fti->num_params; i++) {
    sym_type_info_free(&fti->params[i]);
  }
  free(fti->params);
}

static void
sym_type_info_print(FILE *fp, sym_type_info_t const *sti)
{
  fprintf(fp, " {%zu \"%s\" \"%s\"}", sti->typesize, sti->type_name, sti->name);
}

void
fun_type_info_print(FILE *fp, fun_type_info_t const *fti)
{
  size_t i;
  fprintf(fp, "%s %zu \"%s\" :", fti->ret.name, fti->ret.typesize, fti->ret.type_name);
  for (i = 0; i < fti->num_params; i++) {
    sym_type_info_print(fp, &fti->params[i]);
  }
  fprintf(fp, " :\n");
}
