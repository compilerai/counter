#ifndef __INSNTYPES_H
#define __INSNTYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "i386/opctable.h"
#include "i386/cpu_consts.h"
//#include "imm_map.h"
#include "support/src_tag.h"
#include "valtag/valtag.h"
#include "valtag/memvt.h"
//#include "rewrite/memvt_list.h"

//#define NUM_CRS  5
#define MAX_OPERAND_STRSIZE 32

#define FLAG_CF 0x00000001
#define FLAG_PF 0x00000008
#define FLAG_AF 0x00000010
#define FLAG_ZF 0x00000040
#define FLAG_SF 0x00000080
#define FLAG_DF 0x00000400
#define FLAG_OF 0x00000800

/* Flags stored in PREFIXES.  */
#define I386_PREFIX_REPZ 1
#define I386_PREFIX_REPNZ 2
#define I386_PREFIX_LOCK 4
#define PREFIX_CS 8
#define PREFIX_SS 0x10
#define PREFIX_DS 0x20
#define PREFIX_ES 0x40
#define PREFIX_FS 0x80
#define PREFIX_GS 0x100
#define PREFIX_DATA 0x200
#define PREFIX_ADDR 0x400
#define PREFIX_FWAIT 0x800

//#define VREG0_HIGH_8BIT 64

#define PREFIX_BYTE_SEG(i) (((i) == R_ES)?0x26:((i) == R_CS)?0x2e:((i) == R_SS)?0x36:((i) == R_DS)?0x3e:((i) == R_FS)?0x64:((i) == R_GS)?0x65:-1)

#ifdef __cplusplus
#include <iostream>
class regmap_t;
class imm_vt_map_t;
class nextpc_map_t;
class src_iseq_match_renamings_t;
#endif

typedef struct operand_t {
  opertype_t type;
  unsigned size:7;    //in bytes
  unsigned rex_used:1;

  /* Hold either variables or constants. */

  union {
    memvt_t mem;
    valtag_t seg;
    valtag_t reg;
    valtag_t imm;
    valtag_t pcrel;
    valtag_t cr;
    valtag_t db;
    valtag_t tr;
    valtag_t mmx;
    valtag_t xmm;
    valtag_t ymm;
    valtag_t d3now;
    valtag_t prefix;
    valtag_t st;
    //valtag_t flags_unsigned;
    //valtag_t flags_signed;
    valtag_t flags_other;
    valtag_t flags_eq;
    valtag_t flags_ne;
    valtag_t flags_ult;
    valtag_t flags_slt;
    valtag_t flags_ugt;
    valtag_t flags_sgt;
    valtag_t flags_ule;
    valtag_t flags_sle;
    valtag_t flags_uge;
    valtag_t flags_sge;
    valtag_t st_top;
    valtag_t fp_data_reg;
    valtag_t fp_control_reg_rm;
    valtag_t fp_control_reg_other;
    //valtag_t fp_status_reg;
    valtag_t fp_tag_reg;
    valtag_t fp_last_instr_pointer;
    valtag_t fp_last_operand_pointer;
    valtag_t fp_opcode;
    valtag_t mxcsr_rm;
    valtag_t fp_status_reg_c0;
    valtag_t fp_status_reg_c1;
    valtag_t fp_status_reg_c2;
    valtag_t fp_status_reg_c3;
    valtag_t fp_status_reg_other;
  } valtag;

  /*union {
    unsigned all:4;
    operand_mem_tag_t mem;
    unsigned seg:4;
    unsigned reg:4;
    unsigned imm:4;
    unsigned pcrel:4;
    unsigned cr:4;
    unsigned db:4;
    unsigned tr:4;
    unsigned mmx:4;
    unsigned xmm:4;
    unsigned d3now:4;
    unsigned prefix:4;
    unsigned st:4;
    unsigned flags_unsigned:4;
    unsigned flags_signed:4;
    unsigned st_top:4;
  } tag;*/
#ifdef __cplusplus
  void serialize(ostream& os) const;
  void deserialize(istream& is);
#endif
} operand_t;


typedef struct i386_insn_t {
  opc_t opc;
  operand_t op[I386_MAX_NUM_OPERANDS];
  int num_implicit_ops;

  //memvt_list_t memtouch;
#ifdef __cplusplus
  long insn_hash_func_after_canon(long hash_size) const;
  bool insn_matches_template(i386_insn_t const &templ, src_iseq_match_renamings_t &src_iseq_match_renamings/*regmap_t &regmap, imm_vt_map_t &imm_vt_map, nextpc_map_t &nextpc_map*/, bool var) const;
  static bool uses_serialization();
  void serialize(ostream& os) const;
  void deserialize(istream& is);
#endif
} i386_insn_t;

//void print_insn(i386_insn_t const *insn);
//void println_insn(i386_insn_t const *insn);

#ifdef __cplusplus
void serialize_opc_t(ostream& os, opc_t opc);
opc_t deserialize_opc_t(istream& is);
#endif

/* Operand functions. */
bool operands_equal(operand_t const *op1, operand_t const *op2);
#ifdef __cplusplus
extern "C"
#endif
int default_seg(int base);

extern int i386_base_reg;
extern int i386_available_regs[];
extern int i386_num_available_regs;
//extern int i386_available_xmm_regs[];
//extern int i386_num_available_xmm_regs;

//extern int i386_base_reg;
//extern int i386_available_regs[];
//extern int i386_num_available_regs;
//extern int i386_available_xmm_regs[];
//extern int i386_num_available_xmm_regs;

//typedef uint32_t target_ulong;
#endif
