#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <support/debug.h>
#include <support/debug.h>
#include "support/dyn_debug.h"
#include "support/mytimer.h"

#include "i386/cpu_consts.h"
#include "i386/opctable.h"

//using namespace eqspace;

#define MAX_DISAS_ENTRIES 8192
#define MAX_SIZEFLAGS 4
#define MAX_MODES 4
#define MAX_OPCS 40960
#define REX_MAX 15

static opc_t opctable[MAX_DISAS_ENTRIES][MAX_MODES][REX_MAX + 1][MAX_SIZEFLAGS];

static char nametable[MAX_OPCS][16];
static size_t nametable_size = 0;

static int sizeflagtable[MAX_SIZEFLAGS];
static int sizeflagtable_size = 0;

static i386_as_mode_t modetable[MAX_MODES];
static int modetable_size = 0;

static int find_sizeflag_num(int sizeflag);

extern "C" void
opctable_init(void)
{
  autostop_timer func_timer(__func__);
  int i;
  for (i = 0; i < MAX_DISAS_ENTRIES; i++) {
    int j;
    for (j = 0; j < MAX_MODES; j++) {
      int r;
      for (r = 0; r <= REX_MAX; r++) {
        int k;
        for (k = 0; k < MAX_SIZEFLAGS; k++) {
          opctable[i][j][r][k] = opc_inval;
        }
      }
    }
  }
  snprintf(nametable[nametable_size], sizeof nametable[nametable_size],"(bad)");
  nametable_size++;
  snprintf(nametable[nametable_size], sizeof nametable[nametable_size],"sfence");
  nametable_size++;
  snprintf(nametable[nametable_size], sizeof nametable[nametable_size],"crc32l");
  nametable_size++;
  snprintf(nametable[nametable_size], sizeof nametable[nametable_size],"crc32w");
  nametable_size++;
  snprintf(nametable[nametable_size], sizeof nametable[nametable_size],"crc32b");
  nametable_size++;
}

static opc_t
opc_nametable_find_insert(char const *name, bool insert)
{
  unsigned i;
  for (i = 0; i < nametable_size; i++) {
    if (!strcmp(nametable[i], name)) {
      return (opc_t)i;
    }
  }
  if (insert) {
    return (opc_t)nametable_size;
  }
  NOT_REACHED();
}

extern "C" opc_t
opc_nametable_find(char const *name)
{
  return opc_nametable_find_insert(name, false);
}

size_t
opctable_list_all(char *opcodes[], size_t opcodes_size)
{
  unsigned i;

  ASSERT(opcodes_size >= 0);
  ASSERT(nametable_size <= opcodes_size);
  for (i = 0; i < nametable_size; i++) {
    opcodes[i] = strdup(nametable[i]);
  }
  return nametable_size;
}

static int
find_mode_num(i386_as_mode_t mode)
{
  int mode_num = modetable_size;
  int i;
  for (i = 0; i < modetable_size; i++) {
    if (modetable[i] == mode) {
      mode_num = i;
      break;
    }
  }
  ASSERT(modetable_size <= MAX_MODES);
  ASSERT(mode_num < MAX_MODES);
  return mode_num;
}

extern "C" void
opctable_insert(char const *name, int dp_num, i386_as_mode_t mode, int rex, int sizeflag)
{
  int sizeflag_num, mode_num;
  opc_t opc;

  DYN_DBG2(i386_disas, "name=%s.\n", name);
  opc = opc_nametable_find_insert(name, true);
  DYN_DBG2(i386_disas, "opc=%d.\n", (int)opc);

  ASSERT(opc < MAX_OPCS);
  sizeflag_num = find_sizeflag_num(sizeflag);
  mode_num = find_mode_num(mode);

  ASSERT(sizeflag_num < MAX_SIZEFLAGS);
  ASSERT(mode_num < MAX_MODES);
  ASSERT(dp_num < MAX_DISAS_ENTRIES);
  ASSERT(rex >= 0 && rex <= REX_MAX);

  opctable[dp_num][mode_num][rex][sizeflag_num] = opc;

  if (opc == (int)nametable_size) {
    snprintf(nametable[opc], sizeof nametable[opc], "%s", name);
    nametable_size++;
  }
  if (sizeflag_num == sizeflagtable_size) {
    //printf("Adding %d to sizeflagtable.\n", sizeflag);
    sizeflagtable[sizeflag_num] = sizeflag;
    sizeflagtable_size++;
  }
  if (mode_num == modetable_size) {
    modetable[mode_num] = mode;
    modetable_size++;
  }
}

void
opctable_print(FILE *fp)
{
  int dp_num;
  for (dp_num = 0; dp_num < MAX_DISAS_ENTRIES; dp_num++) {
    int mode;
    for (mode = 0; mode < MAX_MODES; mode++) {
      for (int rex = 0; rex <= REX_MAX; rex++) {
        int sizeflag;
        for (sizeflag = 0; sizeflag < MAX_SIZEFLAGS; sizeflag++) {
          opc_t opc;
          opc = opctable[dp_num][mode][rex][sizeflag];
          fprintf(fp, "(%#x,%d,%d,%d)->%#x->%s\n", dp_num, mode, rex, sizeflag, opc,
              nametable[opc]);
        }
      }
    }
  }
}

extern "C" opc_t
opctable_find(int dp_num, i386_as_mode_t mode, int rex, int sizeflag)
{
  if (mode == I386_AS_MODE_64_REGULAR) {
    mode = I386_AS_MODE_64;
  }
  int sizeflagnum = find_sizeflag_num(sizeflag);
  ASSERT(sizeflagnum < sizeflagtable_size);
  int mode_num = find_mode_num(mode);
  ASSERT(mode_num < modetable_size);
  ASSERT(rex >= 0 && rex <= REX_MAX);

  if (opctable[dp_num][mode_num][rex][sizeflagnum] == opc_inval) {
    //printf("Invalid opcode found at eip=%#x.\n", vcpu.eip);
    return -1;
  }
  return opctable[dp_num][mode_num][rex][sizeflagnum];
}

extern "C" char const *
opctable_name(opc_t opc)
{
  if (!(opc >= 0 && opc < nametable_size)) {
    cout << __func__ << " " << __LINE__ << ": opc = " << opc << ", nametable_size = " << nametable_size << endl;
    NOT_REACHED();
    return nullptr;
  }
  return nametable[opc];
}

static int
find_sizeflag_num(int sizeflag)
{
  int i;
  int sizeflag_num = sizeflagtable_size;
  for (i = 0; i < sizeflagtable_size; i++) {
    if (sizeflagtable[i] == sizeflag) {
      sizeflag_num = i;
      break;
    }
  }
  ASSERT(sizeflagtable_size <= MAX_SIZEFLAGS);
  ASSERT(sizeflag_num < MAX_SIZEFLAGS);
  return sizeflag_num;
}


