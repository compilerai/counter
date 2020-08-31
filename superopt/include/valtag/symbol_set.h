#ifndef SYMBOL_SET_H
#define SYMBOL_SET_H
#include "support/types.h"
#include "valtag/symbol.h"
//#include "translation_instance.h"
#include "support/consts.h"
#include "support/src_tag.h"

//#define MAX_NUM_SYMBOLS 40960
typedef struct symbol_set_t {
  long symbol_num[MAX_NUM_SYMBOLS];
  bool is_dynamic[MAX_NUM_SYMBOLS];
  symbol_t symbol[MAX_NUM_SYMBOLS];
  size_t n_elems;
} symbol_set_t;

void symbol_set_init(symbol_set_t *set);
void symbol_set_add(symbol_set_t *set, bool is_dynamic,
    long symbol_num, char const *name, long symbol_val,
    long symbol_size, long symbol_bind, long symbol_type, long symbol_shndx);
long symbol_set_search_symbol_num(symbol_set_t const *set, char const *name);
char const *symbol_set_get_symbol_name(symbol_set_t const *set, long symbol_num);
symbol_t const *symbol_set_get_symbol(symbol_set_t const *set, long symbol_num);
void symbol_set_free(symbol_set_t *set);
long symbol_get_id_from_valtag(src_tag_t tag, long val);
void symbol_get_valtag_from_id(src_tag_t *tag, long *val, long id);

#endif
