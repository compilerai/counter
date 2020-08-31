#ifndef SYMBOL_H
#define SYMBOL_H

#include "support/types.h"
#ifndef ELFIO_HPP
#include "valtag/elf/elf.h"
#endif

#define MAX_SYMNAME_LEN 256

typedef struct symbol_t {
  char name[MAX_SYMNAME_LEN];
  Elf64_Addr value;
  Elf_Xword size;
  unsigned char bind;
  unsigned char type;
  Elf_Half shndx;
  unsigned char other;
} symbol_t;

#endif
