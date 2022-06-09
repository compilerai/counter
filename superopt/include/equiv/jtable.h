#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "valtag/regmap.h"
#include "valtag/regcons.h"
#include "superopt/superopt.h"
#include "i386/insntypes.h"
#include "insn/src-insn.h"
#include "cutils/chash.h"
#include "sym_exec/sym_exec.h"
#include "eq/quota.h"
#include "valtag/memslot_set.h"

struct itable_t;
struct itable_position_t;
struct valtag_t;
struct typestate_t;
class memslot_map_t;

//#define MAX_INSN_BINSIZE 256
#define MAX_INSN_BINSIZE 40960
#define MAX_TYPECHECK_BINSIZE 256
#define MAX_HEADER_BINSIZE 1024
#define MAX_FOOTER_BINSIZE MAX_HEADER_BINSIZE
#define MAX_SAVE_RESTORE_REGS_BINSIZE 256

#define JTABLE_EXEC_ISEQ_DATA 0
#define JTABLE_EXEC_ISEQ_TYPE 1
#define JTABLE_NUM_EXEC_ISEQS 2

typedef struct jtable_entry_t {
  dst_insn_t *exec_iseq[JTABLE_NUM_EXEC_ISEQS][ITABLE_ENTRY_ISEQ_MAX_LEN]; //used only during jtable_init
  long exec_iseq_len[JTABLE_NUM_EXEC_ISEQS][ITABLE_ENTRY_ISEQ_MAX_LEN]; //used only during jtable_init
  long itable_entry_iseq_len;
  regcons_t regcons;
  regset_t use, def;
  memslot_set_t memslot_use, memslot_def;
  regset_t jtable_entry_temps[JTABLE_NUM_EXEC_ISEQS][ITABLE_ENTRY_ISEQ_MAX_LEN];
  //regset_t ctemps; //these regs do not need to be renamed using regmap
} jtable_entry_t;

typedef struct cached_bincode_elem_t {
  regmap_t regmap;
  uint8_t *bincode;
  size_t binsize;

  struct myhash_elem h_elem;
} cached_bincode_elem_t;

typedef struct cached_bincode_arr_elem_t {
  regmap_t regmap;
  uint8_t *bincode[NEXTPC_TYPE_ERROR + 1];
  size_t binsize[NEXTPC_TYPE_ERROR + 1];

  struct myhash_elem h_elem;
} cached_bincode_arr_elem_t;

typedef struct cached_codes_for_regmaps_t {
  struct chash header;
  struct chash footer;
  struct chash save_regs;
  struct chash restore_regs;
} cached_codes_for_regmaps_t;

typedef struct jtable_t {
  struct itable_t const *itab;
  jtable_entry_t *jentries;

  regmap_t cur_regmap;

  regset_t cur_use[ENUM_MAX_LEN], cur_def[ENUM_MAX_LEN];
  memslot_set_t cur_memslot_use[ENUM_MAX_LEN], cur_memslot_def[ENUM_MAX_LEN];
  regcons_t cur_regcons[ENUM_MAX_LEN];

  //sym_exec se;
  quota qt;

  size_t num_states;
  uint8_t **insn_bincode[ENUM_MAX_LEN]/*[NUM_FP_STATES]*/;
  size_t *insn_binsize[ENUM_MAX_LEN]/*[NUM_FP_STATES]*/;

  struct cached_codes_for_regmaps_t cached_codes_for_regmaps;

  uint8_t *header_bincode;
  size_t header_binsize;

  uint8_t *footer_bincode[NEXTPC_TYPE_ERROR + 1];
  size_t footer_binsize[NEXTPC_TYPE_ERROR + 1];

  //uint8_t *type_error_footer_bincode;
  //size_t type_error_footer_binsize;

  uint8_t *save_regs_bincode;
  size_t save_regs_binsize;

  uint8_t *restore_regs_bincode;
  size_t restore_regs_binsize;

  size_t (*gencode)(int entrynum, uint8_t *, size_t,
      //int const regmap_extable[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)],
      map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &iregmap_extable,
      map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &jtable_regmap_extable,
      int const exreg_sequence[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)],
      int const exreg_sequence_len[MAX_NUM_EXREG_GROUPS],
      int32_t const *vconst_map,
      int32_t const *vconst_sequence,
      long vconst_sequence_len,
      long const *memloc_sequence,
      long memloc_sequence_len,
      memslot_map_t const *memslot_map,
      long const *, long, uint8_t const * const footer_bincode[ENUM_MAX_LEN],
      int32_t (*imm_vt_map_rename_vconst_fn)(struct valtag_t const *, int32_t,
          valtag_t const *, long, nextpc_t const *, int,
           map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const *)/*,
      int d, size_t insn_num*/);

  size_t (*gen_typecheck_code)(int entrynum, uint8_t *, size_t,
      //int const regmap_extable[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)],
      map<exreg_group_id_t, map<exreg_id_t, exreg_id_t>> const &regmap_extable,
      int32_t const *, long,
      long const *, long, uint8_t const * const footer_bincode[ENUM_MAX_LEN],
      int32_t (*imm_map_rename_vconst_fn)(struct valtag_t const *, int32_t,
          int32_t const *, long));

  void *gencode_lib_handle;

  fixed_reg_mapping_t const &jtable_get_fixed_reg_mapping() const { return itab->get_fixed_reg_mapping(); }
} jtable_t;

void jtable_init(jtable_t *jtable, struct itable_t const *itable,
    char const *jtable_cur_regmap_filename, size_t num_states);
void jtable_read(jtable_t *jtable, struct itable_t const *itable,
    char const *jtab_filename, char const *jtable_cur_regmap_filename,
    size_t num_states);
long jtable_construct_executable_binary(jtable_t *jtable,
    struct itable_position_t const *ipos, imm_vt_map_t const *imm_map,
    fingerprintdb_live_out_t const &fingerprintdb_live_out,
    long next, int fp_state_num);
    //bool use_types
size_t jtable_get_executable_binary(uint8_t *buf, size_t size, jtable_t const *jtable,
    struct itable_position_t const *ipos, int fp_state_num);
void jtable_free(jtable_t *jtable);
void jtable_print(FILE *fp, jtable_t const *jtable);
long jtable_construct_executable_binary_for_all_states(
    jtable_t *jtable, struct itable_position_t const *ipos,
    rand_states_t const &rand_states,
    fingerprintdb_live_out_t const &fingerprintdb_live_out,
    long next);

