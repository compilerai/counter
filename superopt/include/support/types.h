#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef __cplusplus
#include <string>
#include <vector>
#include <map>
#include "support/string_ref.h"
#include "support/config-all.h"
#endif

typedef int32_t target_long;
typedef uint32_t target_ulong;

typedef long double floatx80_t;
typedef long double float128_t; //TODO: Use __float128 in GCC; not supported in Clang
typedef float128_t float_max_t;

/*typedef uint32_t bfd_vma;
typedef int32_t bfd_signed_vma;
typedef uint8_t bfd_byte;*/

#define TARGET_LONG_BITS 32

typedef enum iseq_fetch_return_val_t {
  ISEQ_FETCH_SUCCESS = 0,
  ISEQ_FETCH_FAILED_NOT_HARVESTABLE,
  ISEQ_FETCH_FAILED_OTHER,
} iseq_fetch_return_val_t;


typedef enum {
  I386_AS_MODE_32 = 32,
  I386_AS_MODE_64 = 64,
  I386_AS_MODE_64_REGULAR = 65,
} i386_as_mode_t;

#ifndef ARCH_SRC
#error "ARCH_SRC not defined"
#endif

#if ARCH_SRC == ARCH_I386

#define SRC_LONG_BITS 32
#define SRC_WORDS_BIGENDIAN 0
//#define SRC_INSN_ALIGN 1

#elif ARCH_SRC == ARCH_X64

#define SRC_LONG_BITS 64
#define SRC_WORDS_BIGENDIAN 0

#elif ARCH_SRC == ARCH_PPC

#define SRC_LONG_BITS 32
#define SRC_WORDS_BIGENDIAN 1
//#define SRC_INSN_ALIGN 4

#elif ARCH_SRC == ARCH_ETFG

#define SRC_LONG_BITS DST_LONG_BITS
#define SRC_WORDS_BIGENDIAN DST_WORDS_BIGENDIAN
//#define SRC_INSN_ALIGN 1

#else
#error Unrecognized src arch
#endif

#ifndef ARCH_DST
#error "ARCH_DST not defined"
#endif

#if ARCH_DST == ARCH_X64

#define DST_LONG_BITS 64
#define DST_WORDS_BIGENDIAN 0

#elif ARCH_DST == ARCH_I386

#define DST_LONG_BITS 32
#define DST_WORDS_BIGENDIAN 0

#elif ARCH_DST == ARCH_PPC

#define DST_LONG_BITS 32
#define DST_WORDS_BIGENDIAN 0

#else
#error Unrecognized dst arch
#endif

#define SRC_LONG_SIZE (SRC_LONG_BITS / 8)
#define DST_LONG_SIZE (DST_LONG_BITS / 8)

/* src_ulong is the type of a src address */
#if SRC_LONG_SIZE == 4
typedef int32_t src_long;
typedef uint32_t src_ulong;
#define SRC_FMT_lx "%08x"
#elif SRC_LONG_SIZE == 8
typedef int64_t src_long;
typedef uint64_t src_ulong;
#define SRC_FMT_lx "%016" PRIx64
#else
#error SRC_LONG_SIZE undefined
#endif

/* dst_ulong is the type of a dst address */
#if DST_LONG_SIZE == 4
typedef int32_t dst_long;
typedef uint32_t dst_ulong;
#define DST_FMT_lx "%08x"
#elif DST_LONG_SIZE == 8
typedef int64_t dst_long;
typedef uint64_t dst_ulong;
#define DST_FMT_lx "%016" PRIx64
#else
#error DST_LONG_SIZE undefined
#endif

typedef enum {
  IMM_UNMODIFIED = 0,
  IMM_TIMES_SC2_PLUS_SC1_UMASK_SC0,
  IMM_TIMES_SC2_PLUS_SC1_MASK_SC0,
  IMM_SIGN_EXT_SC0,
  IMM_LSHIFT_SC0,
  IMM_ASHIFT_SC0,
  IMM_PLUS_SC0_TIMES_C0,
  IMM_LSHIFT_SC0_PLUS_MASKED_C0,
  IMM_LSHIFT_SC0_PLUS_UMASKED_C0,
  IMM_TIMES_SC2_PLUS_SC1_MASKTILL_SC0,
  IMM_TIMES_SC2_PLUS_SC1_MASKFROM_SC0,
  IMM_TIMES_SC3_PLUS_SC2_MASKTILL_C0_TIMES_SC1_PLUS_SC0,
  IMM_MOD_INVALID
} imm_modifier_t;

typedef enum reg_type
{
  reg_type_exvreg = 0,
  reg_type_const,
  reg_type_symbol,
  reg_type_dst_symbol_addr,
  reg_type_local,
  reg_type_nextpc_const,
  reg_type_insn_id_const,
  reg_type_function_call_identifier_regout,
  reg_type_function_call_identifier_memout,
  reg_type_function_call_identifier_regout_unknown_args,
  reg_type_function_call_identifier_memout_unknown_args,
  //reg_type_function_call_identifier_io_out,
  reg_type_nextpc_indir_target_const,
  reg_type_input_stack_pointer_const,
  reg_type_input_memory_const,
  reg_type_retaddr_const,
  reg_type_callee_save_const,
  //reg_type_memzero,
  reg_type_memory,
  reg_type_io,
  reg_type_stack,
  reg_type_src_env,
  reg_type_contains_float_op,
  reg_type_contains_unsupported_op,
  reg_type_addr_ref,
  reg_type_section,
} reg_type;

typedef enum {
  invalid = 0,
  op_reg,
  op_seg,
  op_mem,
  op_imm,
  op_pcrel,
  op_cr,
  op_db,
  op_tr,
  op_mmx,
  op_xmm,
  op_ymm,
  op_d3now,
  op_prefix,
  op_st,
  //op_flags_unsigned,
  //op_flags_signed,
  op_flags_other,
  op_flags_eq,
  op_flags_ne,
  op_flags_ult,
  op_flags_slt,
  op_flags_ugt,
  op_flags_sgt,
  op_flags_ule,
  op_flags_sle,
  op_flags_uge,
  op_flags_sge,
  op_st_top,
  op_fp_data_reg,
  op_fp_control_reg_rm,
  op_fp_control_reg_other,
  //op_fp_status_reg,
  op_fp_tag_reg,
  op_fp_last_instr_pointer,
  op_fp_last_operand_pointer,
  op_fp_opcode,
  op_mxcsr_rm,
  op_fp_status_reg_c0,
  op_fp_status_reg_c1,
  op_fp_status_reg_c2,
  op_fp_status_reg_c3,
  op_fp_status_reg_other,
} opertype_t;



struct locals_info_t;
typedef int addr_id_t;
typedef int graph_arg_id_t;
typedef int graph_loc_id_t; //-1 represents error; so need a signed type
typedef unsigned expr_id_t;
typedef unsigned scev_id_t;
typedef unsigned constant_value_id_t;
typedef unsigned array_constant_id_t;
//typedef int cs_addr_ref_id_t;
typedef int nextpc_id_t;
typedef size_t relevant_memlabel_idx_t;
typedef int bv_solve_var_idx_t;
typedef size_t argnum_t;
typedef int constant_id_t;
typedef long bbl_index_t;
typedef long copy_id_t;
typedef size_t version_number_t;
typedef int path_depth_t;
typedef int path_delta_t;
typedef int path_mu_t;
typedef int paths_weight_t;
typedef int variable_id_t;
typedef variable_id_t symbol_id_t;
//typedef variable_id_t local_id_t;
typedef size_t variable_size_t;
typedef bool variable_constness_t;
typedef int exreg_group_id_t;
typedef int exreg_id_t;
typedef size_t offset_t;
#ifdef __cplusplus
using mlvarname_t = string_ref;
//using mlvarname_t = string;
class transmap_t;
typedef size_t align_t;
typedef std::pair<transmap_t, std::vector<transmap_t>> in_out_tmaps_t;
//typedef std::map<symbol_id_t,std::tuple<std::string_ref,size_t,bool>> graph_symbol_map_t;
using bcfile_t = string;
using asmfile_t = string;
using objfile_t = string;
using proof_file_t = string;
using harvest_file_t = string;
using harvest_log_t = string;
using section_idx_t = int;
using clone_prefix_t = std::string;
#endif
typedef uint8_t transmap_loc_id_t;
typedef long cost_t;
typedef unsigned long long flow_t;
typedef long memlabel_group_id_t;
typedef int clone_count_t;


// NOTE: the first index must be 0
typedef enum { SOLVER_ID_Z3 = 0, SOLVER_ID_Z3v487, SOLVER_ID_YICES, SOLVER_ID_CVC4, SOLVER_ID_BOOLECTOR } solver_id_t;

typedef enum { llvm_insn_other, llvm_insn_farg, llvm_insn_bblentry, llvm_insn_phinode, llvm_insn_alloca, llvm_insn_ret, llvm_insn_call, llvm_insn_invoke, llvm_insn_load, llvm_insn_store } llvm_insn_type_t;

enum xfer_dir_t { forward, backward };

#ifdef __cplusplus
using TypeID = enum { VoidTyID = 0, IntegerTyID, PointerTyID };
#endif
