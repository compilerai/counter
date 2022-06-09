#pragma once

//#include "src-defs.h"
//#include "i386/regs.h"
//#include "ppc/regs.h"
#define CONSTS_DB_FILENAME SUPEROPTDBS_DIR "consts_db"

#define INSN_MEMTOUCH_OP_INDEX_IN_COMMENTS 256
#define COMMENT_MAX_OP_INDEX 65536

#define IMM_MAX_NUM_SCONSTANTS 4  //associated with imm_max_num_sconstants in ml/common.ml
#define NUM_CANON_CONSTANTS 0x100
#define MAX_NUM_SYMBOLS 0x100000
#define NUM_CANON_SYMBOLS 0x10000
#define NUM_CANON_DST_SYMBOL_ADDRS 0x10000
#define MAX_NUM_LOCALS 0x100
#define NUM_CANON_SECTIONS 0x1000
#define VCONSTANT_PLACEHOLDER 0x11
#define MAX_NEXTPC_CONSTANTS MAX_NUM_NEXTPCS
//#define SYMBOL_ID_DST_RODATA (NUM_CANON_SYMBOLS - 1)
//#define SYMBOL_ID_DST_RODATA_NAME "dst-rodata"
//#define SYMBOL_ID_DST_RODATA_SIZE (1<<20)  /* 1 MiB */
#define RODATA_SECTION_NAME ".rodata"

/* NEXTPCs should be signed 8-bit (<= 0x7f) to support jecxz
   instruction. Any change to EXT_JUMP_ID0 should also result in a change in
   ml/common.ml:ext_jmp_id0 */
#define NEXTPC(i) (i+1)
#define NEXTPC_INVALID NEXTPC(0x7e)

#define INSN_PC_INVALID 0x7fffffff

#define FCALL(i) (i+13)
#define FCALL_INVALID FCALL(123)

#define MAX_NUM_VCONSTANTS_PER_INSN 32
#define MAX_LINE_SIZE 10240

#define MAX_INSN_ID_CONSTANTS 4192
#define MAX_CALLEE_SAVE_REGCONSTS 4
#define INSN_ID_FALLTHROUGH (MAX_INSN_ID_CONSTANTS - 1)

#define MAX_NUM_REGS_EXREGS 32 /* maximum number of regs or exregs in a group. */

//#define MAX_NUM_REGS ((SRC_NUM_REGS < DST_NUM_REGS)?DST_NUM_REGS:SRC_NUM_REGS)
#define MAX_NUM_EXREG_GROUPS ((SRC_NUM_EXREG_GROUPS < DST_NUM_EXREG_GROUPS)?DST_NUM_EXREG_GROUPS:SRC_NUM_EXREG_GROUPS)
#define MAX_NUM_EXREGS(i) ((SRC_NUM_EXREGS(i) < DST_NUM_EXREGS(i))?DST_NUM_EXREGS(i):SRC_NUM_EXREGS(i))
#define MAX_EXREG_LEN(i) ((SRC_EXREG_LEN(i) < DST_EXREG_LEN(i))?DST_EXREG_LEN(i):SRC_EXREG_LEN(i))

//#define SI_MAX_FETCHLEN 65536
#define SI_MAX_FETCHLEN (&si_max_fetchlen)
#define FETCHLEN_MAX 16
#define MAX_NUM_NEXTPCS 256
#define NEXTPC_TYPE_ERROR MAX_NUM_NEXTPCS


#define array_search(haystack, n, needle) ({ \
  long __array_search_i, __array_search_ret = -1; \
  for (__array_search_i = 0; __array_search_i < (n); __array_search_i++) { \
    if ((haystack)[__array_search_i] == (needle)) {  \
      __array_search_ret = __array_search_i; \
    } \
  } \
  __array_search_ret; \
})

#define array_union(a, an, b, bn) ({ \
  int __array_union_i, __array_union_j, __array_union_new_an; \
  __array_union_new_an = (an); \
  for (__array_union_i = 0; __array_union_i < (bn); __array_union_i++) { \
    bool __array_union_found = false; \
    for (__array_union_j = 0; __array_union_j < (an); __array_union_j++) { \
      if ((a)[__array_union_j] == (b)[__array_union_i]) { \
        __array_union_found = true; \
        break; \
      } \
    } \
    if (!__array_union_found) { \
      (a)[__array_union_new_an++] = (b)[__array_union_i]; \
    } \
  } \
  __array_union_new_an; \
})

#define array_intersect(a, an, b, bn) ({ \
  long __array_intersect_i, __array_intersect_j, __array_intersect_new_an; \
  __array_intersect_new_an = 0; \
  for (__array_intersect_i = 0; __array_intersect_i < (bn); __array_intersect_i++) { \
    bool __array_intersect_found = false; \
    for (__array_intersect_j = __array_intersect_new_an; \
        __array_intersect_j < (an); \
        __array_intersect_j++) { \
      if ((a)[__array_intersect_j] == (b)[__array_intersect_i]) { \
        __array_intersect_found = true; \
        break; \
      } \
    } \
    if (__array_intersect_found) { \
      (a)[__array_intersect_new_an] = (b)[__array_intersect_i]; \
      __array_intersect_new_an++; \
    } \
  } \
  __array_intersect_new_an; \
})



#define array_scale(arr, n, scale) ({ \
  long __array_scale_i; \
  for (__array_scale_i = 0; __array_scale_i < (n); __array_scale_i++) { \
    (arr)[__array_scale_i] *= scale; \
  } \
})

#define array_max(haystack, n) ({ \
  long __array_max_i, __array_max_ret = -1; \
  for (__array_max_i = 0; __array_max_i < (n); __array_max_i++) { \
    if (   __array_max_ret == -1 \
        || (haystack)[__array_max_i] > (haystack)[__array_max_ret]) {  \
      __array_max_ret = __array_max_i; \
    } \
  } \
  __array_max_ret; \
})

#define array_min(haystack, n) ({ \
  long __array_min_i, __array_min_ret = -1; \
  for (__array_min_i = 0; __array_min_i < (n); __array_min_i++) { \
    if (   __array_min_ret == -1 \
        || (haystack)[__array_min_i] < (haystack)[__array_min_ret]) {  \
      __array_min_ret = __array_min_i; \
    } \
  } \
  __array_min_ret; \
})

#define array_all_equal(array, n, val) ({ \
  long __array_all_equal_i, __array_all_equal_ret = true; \
  for (__array_all_equal_i = 0; __array_all_equal_i < (n); __array_all_equal_i++) { \
    if (array[__array_all_equal_i] != val) { \
      __array_all_equal_ret = false; \
      break; \
    } \
  } \
  __array_all_equal_ret; \
})


#define ROUND

#define bool2str(x) ((x) ? "true" : "false")

#define stl(ptr, v) do { \
  *(dst_ulong *)(ptr) = tswapl(v); \
} while(0)

#define stq(ptr, v) do { \
  *(uint64_t *)(ptr) = tswapq(v); \
} while(0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#endif

/*
#define _hex_bad 99

#define hex_value(c) ({ \
  int ret; \
  if ((c) >= 'a' && (c) <= 'f') ret = (c) - 'a' + 10; \
  else if ((c) >= 'A' && (c) <= 'F') ret = (c) - 'A' + 10; \
  else if ((c) >= '0' && (c) <= '9') ret = (c) - '0'; \
  else ret = _hex_bad; \
  ret; \
})

#define hex_p(c) (hex_value(c) != _hex_bad)
*/

#define MAKE_MASK(n) (((n) == 64)?0xffffffffffffffffULL:(1ULL << (n)) - 1)

#define SRC_ENV_ADDR 0x7d000000
#define SRC_ENV_SIZE (1 << 20)
#define SRC_ENV_PREFIX_SPACE 0x5000 // needed because qemufb uses this space
#define NEXTPC_CONST_PREFIX "nextpc_const."
#define G_MEM_SYMBOL "mem"
#define G_ALLOC_SYMBOL "alloc"
#define G_POISON_SYMBOL "poison"
#define G_FPSTACK_SYMBOL "fpstack"
#define G_STACK_SYMBOL "esp"
#define G_LOCAL_KEYWORD "local"
#define G_LOCAL_SIZE_KEYWORD "local_size"
#define G_HEAP_ALLOC_KEYWORD "hpalloc"
#define G_UNINIT_NONCE_KEYWORD "uninit_nonce"
#define G_NOTSTACK_KEYWORD "notstack"
#define G_HEAP_KEYWORD "heap"
#define G_RODATA_KEYWORD "rodata"
#define G_ARG_KEYWORD "arg"
#define G_DUMMY_KEYWORD "dummy"
#define G_MEMLABEL_PREFIX "memlabel-"
#define G_MEMLABEL_TOP_SYMBOL "top"
#define G_MEMLABEL_LB_SUFFIX "_begin"
#define G_MEMLABEL_UB_SUFFIX "_end"
#define G_MEMLABEL_IS_ABSENT_SUFFIX "_is-absent"
#define G_ALLOCA_SEEN_SUFFIX "_alloca_seen"
#define G_SYMBOL_KEYWORD "symbol"
#define G_DST_SYMBOL_ADDR_KEYWORD "dst_symbol_addr"
#define G_SECTION_KEYWORD "section"
//#define G_VAR_KEYWORD "var"
#define G_INPUT_KEYWORD "input"
#define G_SOLVER_STACK_MEM_NAME "commonMEM.stack9000.mem"
#define G_SOLVER_NONSTACK_MEM_NAME "commonMEM.nonstack.mem"
#define G_SOLVER_FCALL_MEM_NAME "fcallMEM.mem"
#define G_SOLVER_DST_MEM_NAME G_INPUT_KEYWORD "." G_DST_PREFIX "." G_MEM_SYMBOL
#define G_SOLVER_POINTSTO_UF_NAME "pointsTo"
#define G_SOLVER_MEM_ALLOC_TMP_VARNAME "solver-mem-alloc"
#define G_SOLVER_MEM_TMP_VARNAME "solver-mem"
#define G_BELONGS_TO_SAME_REGION_TMP_VARNAME "belongs-to-freshvar"
#define G_SOLVER_MAE_BOUNDED_VARNAME "mae-bounded-var!"
#define G_SOLVER_MAEE_BOUNDED_VARNAME "maee-bounded-var!"
#define G_SOLVER_ISM_BOUNDED_VARNAME "ism-bounded-var!"
#define G_SOLVER_ICM_BOUNDED_VARNAME "icm-bounded-var!"
#define G_SOLVER_IPCM_BOUNDED_VARNAME "ipcm-bounded-var!"
#define G_SOLVER_ISSM_BOUNDED_VARNAME "issm-bounded-var!"
#define G_SOLVER_MIA_BOUNDED_VARNAME "mia-bounded-var!"
#define G_SOLVER_ALC_BOUNDED_VARNAME "alc-bounded-var!"
#define G_SOLVER_US_BOUNDED_VARNAME "us-bounded-var!"
#define G_SOLVER_MEMMASK_BOUNDED_VARNAME "memmask-bounded-var!"
#define G_SOLVER_MEMSPLICE_BOUNDED_VARNAME "memsplice-bounded-var!"
#define G_SOLVER_RAM_BOUNDED_VARNAME "region-agrees-with-ml-bounded-var!"
#define G_SOLVER_BTSR_BOUNDED_VARNAME "belongs-to-same-region-bounded-var!"
#define G_SOLVER_ALLOCA_PTR_UF_NAME "alloca_ptr"

#define G_SOLVER_FPA_TO_IEEE_BV_UF_NAME "fpa_to_ieee_bv_uf"
#define G_SOLVER_MEMLABEL_MAX_ID_BITS 8 // 8 bits (max. 256 memlabels) ought to be enough?

//#define G_IO_SYMBOL "io"

#define G_CONTAINS_FLOAT_OP_SYMBOL "contains_float_op"
#define G_CONTAINS_UNSUPPORTED_OP_SYMBOL "contains_unsupported_op"

#define SMT_SOLVER_TMP_FILES "/tmp/smt-solver-tmp-files"
#define SMT_QUERY_FILENAME_PREFIX "smt_query"
#define SMT_MODEL_FILENAME_PREFIX "smt_model"
#define IS_PROVABLE_FILENAME_PREFIX "is_provable"
#define IS_EXPR_EQUAL_USING_LHS_SET_FILENAME_PREFIX "is_expr_equal_using_lhs_set"
#define PROVE_FILENAME_PREFIX "prove"
#define PROVE_QD_FILENAME_PREFIX "proveqd"
#define DECIDE_HOARE_TRIPLE_FILENAME_PREFIX "decide_hoare_triple"
#define TFG_UNROLL_TILL_NO_REPEATED_PCS_FILENAME_PREFIX "tfg_unroll_till_no_repeat"
#define CG_CREATE_SRC_TFG_SSA_FOR_NEW_EC_FILENAME_PREFIX "cg_create_src_tfg_ssa_for_new_ec"
#define UPDATE_INVARIANT_STATE_OVER_EDGE_FILENAME_PREFIX "update_invariant_state_over_edge"
#define INFER_INVARIANTS_XFER_OVER_PATH_FILENAME_PREFIX "infer_invariants_xfer_over_path"
#define CHECK_WFCONDS_ON_EDGE_FILENAME_PREFIX "check_wfconds_on_edge"
#define ADD_CORR_GRAPH_EDGE_FILENAME_PREFIX "add_corr_graph_edge"
#define GRAPH_PROPAGATE_CES_ACROSS_NEW_EDGE_FILENAME_PREFIX "graph_propagate_ces_across_new_edge"
#define INFEASIBLE_CG_DUMP_PREFIX "infeasible_cg_dump"
#define CORR_GRAPH_ADD_CORRELATION_FILENAME_PREFIX "corr_graph_add_correlation"
#define LDR_DECOMPOSE_FILENAME_PREFIX "ldr_decompose"

//#define G_FUNCTION_CALLEE_ACCESSIBLE_REGION_KEYWORD G_HEAP_KEYWORD
#define G_FUNCTION_CALL_REGNUM_EAX 1
#define G_FUNCTION_CALL_REGNUM_ECX 2
#define G_FUNCTION_CALL_REGNUM_EDX 3
#define G_FUNCTION_CALL_REGNUM_RETVAL(r) (1000+(r))

#define USE_LOOP_REPRESENTATION_FOR_POPCNT_OPCODES true
#define REGMASK_ARRAY_SIZE 8

#define MEMLABEL_INVALID_INTVAL -1
#define MEMLABEL_TOP_INTVAL (MEMLABEL_INVALID_INTVAL + 1)
#define MEMLABEL_MEM_INTVAL (MEMLABEL_TOP_INTVAL + 1)
#define MEMLABEL_STACK_INTVAL (MEMLABEL_MEM_INTVAL + 1)
#define MEMLABEL_NOTSTACK_INTVAL (MEMLABEL_STACK_INTVAL + 1)
#define MEMLABEL_HEAP_INTVAL (MEMLABEL_NOTSTACK_INTVAL + 1)
#define MEMLABEL_GLOBAL_INTVAL_START (MEMLABEL_HEAP_INTVAL + 1)
#define MEMLABEL_LOCAL_INTVAL_START (MEMLABEL_GLOBAL_INTVAL_START + MEMLABEL_MAX_GLOBALS)
#define MEMLABEL_VAR_INTVAL_START (MEMLABEL_LOCAL_INTVAL_START + MEMLABEL_MAX_LOCALS)
#define MEMLABEL_MAX_GLOBALS (0x10000 - 2)
#define MEMLABEL_MAX_LOCALS  MEMLABEL_MAX_GLOBALS
#define MEMLABEL_MAX_VARS    MEMLABEL_MAX_GLOBALS

//#define OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE 1
//#define OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE 2
//#define OP_FUNCTION_CALL_ARGNUM_MEM 3
//#define OP_FUNCTION_CALL_ARGNUM_MEM_ALLOC 4
//#define OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST 5
//#define OP_FUNCTION_CALL_ARGNUM_REGNUM 6
//#define OP_FUNCTION_CALL_ARGNUM_FARG0 (OP_FUNCTION_CALL_ARGNUM_REGNUM + 1)
//#define FUNCTION_CALL_NEXTPC_CONST_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_NEXTPC_CONST - 1)
//#define FUNCTION_CALL_REGNUM_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_REGNUM - 1)
//#define FUNCTION_CALL_MEMLABEL_CALLEE_READABLE_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE - 1)
//#define FUNCTION_CALL_MEMLABEL_CALLEE_WRITEABLE_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE - 1)
//#define FUNCTION_CALL_MEM_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_MEM - 1)
//#define FUNCTION_CALL_MEM_ALLOC_ARG_INDEX (OP_FUNCTION_CALL_ARGNUM_MEM_ALLOC - 1)

#define OP_FUNCTION_CALL_OUTPUT_EAX_REGNUM 1
#define OP_FUNCTION_CALL_OUTPUT_EDX_REGNUM 3

#define BYTE_SIZE 8
#define PRED_DEPENDENCY_KEYWORD "pred_dependency_keyword"
#define FUNCTION_CALL_MAX_ARGS 64
#define MEMSIZE_MAX ((size_t)-1)

#define G_REGULAR_REG_NAME "exreg."
#define G_INVISIBLE_REG_NAME "invisible_reg."
#define G_HIDDEN_REG_NAME "hidden_reg."

#define G_TFG_INPUT_IDENTIFIER "tfginput"
#define G_TFG_OUTPUT_IDENTIFIER "tfgoutput"
#define G_TFG_LOC_IDENTIFIER "tfgloc"
#define G_TFG_ADDR_IDENTIFIER "tfgaddr"
#define G_TFG_PRED_IDENTIFIER "tfgpred"
#define G_EDGE_COND_IDENTIFIER "econd"
#define G_SELFCALL_IDENTIFIER "SELFCALL"
#define G_LLVM_DBG_DECLARE_FUNCTION "llvm.dbg.declare"
#define G_LLVM_DBG_VALUE_FUNCTION "llvm.dbg.value"
#define G_LLVM_STACKSAVE_FUNCTION "llvm.stacksave"
#define G_LLVM_STACKRESTORE_FUNCTION "llvm.stackrestore"
#define G_LLVM_LIFETIME_FUNCTION_PREFIX "llvm.lifetime."
#define G_LLVM_MEMCPY_FUNCTION "llvm.memcpy"
#define G_LLVM_MEMSET_FUNCTION "llvm.memset"
#define G_LLVM_VA_START_FUNCTION "llvm.va_start"
#define G_LLVM_VA_COPY_FUNCTION "llvm.va_copy"
#define G_LLVM_VA_END_FUNCTION "llvm.va_end"
#define Z3_MEMLABELS_ENUMERATION_SORT_NAME "memlabels"
#define Z3_MEMLABELS_UI_FUNCTION "memlabels_uif"
#define Z3_MEMMASK_UI_FUNCTION "memmask_uif"
#define Z3_MEMJOIN_UI_FUNCTION "memjoin_uif"

#define LLVM_FUNCTION_NAME_PREFIX "#"
#define LLVM_GLOBAL_VARNAME_PREFIX "@"

#define HBYTE_LEN 4
#define BYTE_LEN 8
#define WORD_LEN 16
#define DWORD_LEN 32
#define QWORD_LEN 64
#define DQWORD_LEN 128
#define QQWORD_LEN 256
#define FPWORD_LEN 79
#define WORD48_LEN 48
#define WORD11_LEN 11
#define WORD2_LEN 2
#define WORD3_LEN 3
#define COUNT_WORD_LEN WORD_LEN

#define DEFAULT_FUNCTION_ARGUMENT_PTR_NBYTES 1
#define DEFAULT_FUNCTION_ARGUMENT_PTR_BIGENDIAN false

#define DEFAULT_RETURN_VALUE_PTR_NBYTES DEFAULT_FUNCTION_ARGUMENT_PTR_NBYTES
#define DEFAULT_RETURN_VALUE_PTR_BIGENDIAN DEFAULT_FUNCTION_ARGUMENT_PTR_BIGENDIAN

//#define DEFAULT_QUERY_TIMEOUT_SPREL_MEET 600
//#define DEFAULT_QUERY_TIMEOUT_REMOVE_SELECT 600

#define RODATA_SYMBOL_SIZE 0 //nbytes

#define G_LLVM_HIDDEN_REGISTER_NAME G_LLVM_PREFIX "-%hidden-reg"
#define G_LLVM_RETURN_REGISTER_NAME G_LLVM_PREFIX "-%ret-reg"
//#define Z3_IO_BVLEN 32

#define G_LLVM_PREFIX "llvm"
#define G_DST_PREFIX "dst"

#define UNINIT_KEYWORD "UNINIT.keyword"

//#define TFG_LOC_ID_EDGECOND 999999

#define SPREL_ASSUME_COMMENT_PREFIX "sprel_assume"
#define UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "undef-behaviour"

//#define UNDEF_BEHAVIOUR_ASSUME_BITCAST_AFTERCAST_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-bitcast-aftercast-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_BITCAST_SRC_EXPR_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-bitcast-src-expr-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_INTPTR_AFTERCAST_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-intptr-aftercast-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_INTPTR_SRC_EXPR_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-intptr-src-expr-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_TYPE_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-type-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_SWITCH_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-switch-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_RET_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-ret-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_ALLOCA_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-alloca-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_STORE_ADDR_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-store-addr-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_STORE_DATA_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-store-data-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_LOAD_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-load-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_FCALL_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-fcall-langtype-assume"
//#define UNDEF_BEHAVIOUR_ASSUME_ARG_ISLANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-arg-langtype-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_SRC UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-align-memcpy-src-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_DST UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-align-memcpy-dst-assume"
#define UNDEF_BEHAVIOUR_ASSUME_IS_MEMACCESS_LANGTYPE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-memaccess-langtype-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ALIGN_ISLANGALIGNED UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-align-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ISGEPOFFSET UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-gepoffset-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ISINDEXFORSIZE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-indexforsize-assume"
#define UNDEF_BEHAVIOUR_ASSUME_DEREFERENCE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-dereference-assume"
#define UNDEF_BEHAVIOUR_ASSUME_ISSHIFTCOUNT UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-shiftcount-assume"
#define UNDEF_BEHAVIOUR_ASSUME_DIVBYZERO UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-divbyzero-assume"
#define UNDEF_BEHAVIOUR_ASSUME_DIV_NO_OVERFLOW UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-div-no-overflow-assume"
#define UNDEF_BEHAVIOUR_ASSUME_BELONGS_TO_VARIABLE_SPACE UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-belongs-to-variable-space"
#define UNDEF_BEHAVIOUR_ASSUME_REGION_AGREES_WITH_MEMLABEL UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-region-agrees-with-memlabel-assume"
#define UNDEF_BEHAVIOUR_ASSUME_BELONGS_TO_SAME_REGION UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX "-belongs-to-same-region-assume"

#define GEPOFFSET_KEYWORD "gepoffset"
#define GEPTOTALOFFSET_KEYWORD "total_offset"

#define UNINIT_MEM_EQ_PREFIX "uninit-memeq-"
#define GUESS_PREFIX "guess-"
#define PRECOND_PREFIX "precond-"
#define PRED_LINEAR_PREFIX "linear"
#define PRED_MEM_PREFIX GUESS_PREFIX

#define WFCOND_FCALL_SP_IS_ALIGNED "-fcall-sp-is-aligned"
#define WFCOND_SP_BELOW_ISP "-sp-below-isp"
#define WFCOND_SP_IN_STACK "-sp-in-stack"
#define WFCOND_DST_EDGECOND_IMPLIES_SRC_EDGECOND "-dst-edgecond-implies-src-edgecond"
#define WFCOND_ALLOCA_REGION_WAS_STACK "-alloca-region-was-stack"
#define WFCOND_ALLOCA_ADDR_NEQ_ZERO "-alloca-addr-neq-zero"
#define WFCOND_ALLOCA_ADDR_NO_OVERFLOW "-alloca-addr-no-overflow"
#define WFCOND_ALLOCA_ADDR_IS_ALIGNED "-alloca-addr-is-aligned"
#define WFCOND_DEALLOC_SP_ABOVE_UB "-dealloc-sp-above-ub"
#define WFCOND_DEALLOC_REGION_WAS_ML "-dealloc-region-was-ml"
// all wfcond comment strings
#define ALL_WFCONDS { WFCOND_FCALL_SP_IS_ALIGNED, WFCOND_SP_BELOW_ISP, WFCOND_SP_IN_STACK, WFCOND_DST_EDGECOND_IMPLIES_SRC_EDGECOND, WFCOND_ALLOCA_REGION_WAS_STACK, WFCOND_ALLOCA_ADDR_NEQ_ZERO, WFCOND_ALLOCA_ADDR_NO_OVERFLOW, WFCOND_ALLOCA_ADDR_IS_ALIGNED, WFCOND_DEALLOC_SP_ABOVE_UB, WFCOND_DEALLOC_REGION_WAS_ML }

#define MAX_SCALAR_SIZE_IN_BYTES 0 // memmask -> select conversion threshold; must be zero if dst tfg alias analysis is coarse-grained

#define BIT_LEN 1

#define LLVM_INTERMEDIATE_VALUE_KEYWORD "intermediate"

#define LLVM_MEM_SYMBOL G_LLVM_PREFIX "-" G_MEM_SYMBOL
//#define LLVM_IO_SYMBOL G_LLVM_PREFIX "-" G_IO_SYMBOL
#define LLVM_CONTAINS_FLOAT_OP_SYMBOL G_LLVM_PREFIX "-" G_CONTAINS_FLOAT_OP_SYMBOL
#define LLVM_CONTAINS_UNSUPPORTED_OP_SYMBOL G_LLVM_PREFIX "-" G_CONTAINS_UNSUPPORTED_OP_SYMBOL
//#define LLVM_STATE_INDIR_TARGET_ADDR_DUMMY_KEY_ID G_LLVM_PREFIX "-" G_STATE_INDIR_TARGET_ADDR_DUMMY_KEY_ID
#define LLVM_STATE_INDIR_TARGET_KEY_ID G_LLVM_PREFIX "-" G_STATE_INDIR_TARGET_KEY_ID
#define LLVM_CALLEE_SAVE_REGNAME G_LLVM_PREFIX "-callee-save"
#define MAX_STACK_SIZE 0xffffff

#define PC_INDEX_STR_START  "0"
#define PC_SUBINDEX_START 0
#define PC_SUBINDEX_FIRST_INSN_IN_BB 1 //cannot be 0 because it conflicts with pc::start()

//SUBSUBINDEX ordering constraints
//1. intermediate_val should be after switch_intermediate: so that if the switch branch is jumping to a phi node, the order of nodes is correct.
//2. intermediate_val should be before fcall_start: so that the intermediate value computations for fcall arguments are ordered before the actual call
#define PC_SUBSUBINDEX_BASIC_BLOCK_ENTRY 0
#define PC_SUBSUBINDEX_DEFAULT 1
#define PC_SUBSUBINDEX_SWITCH_INTERMEDIATE (2 + 1)
#define PC_SUBSUBINDEX_MAX_SWITCH_INTERMEDIATES 100000
#define PC_SUBSUBINDEX_INTERMEDIATE_VAL(n) (PC_SUBSUBINDEX_SWITCH_INTERMEDIATE + PC_SUBSUBINDEX_MAX_SWITCH_INTERMEDIATES + (n))
#define PC_SUBSUBINDEX_MAX_INTERMEDIATE_VALS 100000
#define PC_SUBSUBINDEX_FCALL_START PC_SUBSUBINDEX_INTERMEDIATE_VAL(PC_SUBSUBINDEX_MAX_INTERMEDIATE_VALS)
#define PC_SUBSUBINDEX_FCALL_MIDDLE(n) (PC_SUBSUBINDEX_FCALL_START + 1 + (n))
#define PC_SUBSUBINDEX_MAX_FCALL_MIDDLE_NODES 100000
#define PC_SUBSUBINDEX_FCALL_END PC_SUBSUBINDEX_FCALL_MIDDLE(PC_SUBSUBINDEX_MAX_FCALL_MIDDLE_NODES)
#define PC_SUBSUBINDEX_INTRINSIC_START (PC_SUBSUBINDEX_FCALL_END + 1)
#define PC_SUBSUBINDEX_INTRINSIC_MIDDLE(n) (PC_SUBSUBINDEX_INTRINSIC_START + 1 + (n))
#define PC_SUBSUBINDEX_MAX_INTRINSIC_MIDDLE_NODES 100000
#define PC_SUBSUBINDEX_INTRINSIC_END PC_SUBSUBINDEX_INTRINSIC_MIDDLE(PC_SUBSUBINDEX_MAX_INTRINSIC_MIDDLE_NODES)
#define PC_SUBSUBINDEX_SP_VERSION (PC_SUBSUBINDEX_INTRINSIC_END+1)
#define PC_SUBSUBINDEX_GHOSTVAR_ITERVAR(n) (PC_SUBSUBINDEX_SP_VERSION + 1 + (n))
#define PC_SUBSUBINDEX_MAX_GHOSTVAR_ITERVARS 100000
#define PC_SUBSUBINDEX_DEALLOC_EDGES(n) (PC_SUBSUBINDEX_GHOSTVAR_ITERVAR(PC_SUBSUBINDEX_MAX_GHOSTVAR_ITERVARS) + 1 + n)
#define PC_SUBSUBINDEX_MAX_DEALLOC_EDGES 1000
#define PC_SUBSUBINDEX_VARARG (PC_SUBSUBINDEX_DEALLOC_EDGES(PC_SUBSUBINDEX_MAX_DEALLOC_EDGES))
#define PC_SUBSUBINDEX_ARGNUM(n) (PC_SUBSUBINDEX_VARARG + 1 + (n))
#define PC_SUBSUBINDEX_ARGNUM_MAX 1000
#define PC_SUBSUBINDEX_ALLOCA_INTERMEDIATE(n) (PC_SUBSUBINDEX_ARGNUM(PC_SUBSUBINDEX_ARGNUM_MAX) + (n))
#define PC_SUBSUBINDEX_ALLOCA_INTERMEDIATE_MAX 100
#define PC_SUBSUBINDEX_DEALLOC_INTERMEDIATE(n) (PC_SUBSUBINDEX_ALLOCA_INTERMEDIATE(PC_SUBSUBINDEX_ALLOCA_INTERMEDIATE_MAX) + (n))
#define PC_SUBSUBINDEX_DEALLOC_INTERMEDIATE_MAX 100

#define PC_SUBSUBINDEX_SSA_PHI_OFFSET 100000
#define PC_SUBSUBINDEX_SSA_PHI (PC_SUBSUBINDEX_DEALLOC_INTERMEDIATE(PC_SUBSUBINDEX_DEALLOC_INTERMEDIATE_MAX) + PC_SUBSUBINDEX_SSA_PHI_OFFSET)
#define PC_SUBSUBINDEX_SSA_PHINUM(n) (PC_SUBSUBINDEX_SSA_PHI + 1 + (n))
#define PC_SUBSUBINDEX_SSA_PHINUM_MAX 1000

#define GUESSING_MAX_LOOKAHEAD 8
#define CHECKING_MAX_DEPTH 4

#define SYMBOL_ID_INVALID ((symbol_id_t)-1)

#define MEMZERO_MEMVAR_NAME "memzero_memvar"
#define MEMZERO_MEMALLOC_VAR_NAME "memzero_memallocvar"

#define FUNCTION_NAME_FIELD_IDENTIFIER "=FunctionName:"
#define TFG_SSA_IDENTIFIER "=TFG_SSA"
#define TFG_SSA_DONE_IDENTIFIER "=TFG_SSA_done"
#define TFG_IDENTIFIER "=TFG"
#define TFG_DONE_IDENTIFIER "=TFGdone"
#define TFG_LLVM_IDENTIFIER "=TFG_LLVM"
#define TFG_LLVM_DONE_IDENTIFIER "=TFG_LLVM_done"
#define PROOF_FILE_RESULT_PREFIX "=result: "

#define ETFG_SECTION_START_ADDR 0
#define ETFG_SECTION_TYPE SHT_PROGBITS
#define ETFG_TEXT_SECTION_FLAGS SHF_EXECINSTR
#define ETFG_SYMBOLS_SECTION_FLAGS (SHF_ALLOC | SHF_WRITE)
#define ETFG_SECTION_SIZE 0
#define ETFG_SECTION_INFO 0
#define ETFG_SECTION_ALIGN 0x1
#define ETFG_SECTION_ENTRY_SIZE 1
#define INPUT_EXEC_TYPE_ETFG ET_EXEC

#define ETFG_MAX_INSNS_PER_FUNCTION 0x100000

#define SMT_HELPER_PROCESS_MSG_SIZE 1024
#define QD_HELPER_PROCESS_SEND_SIZE 1024
#define QD_HELPER_PROCESS_RECV_SIZE 10
//#define SAGE_HELPER_PROCESS_MSG_SIZE 1024

#define MAX_UNROLL 1

#define TMAP_LOC_NONE 0
#define TMAP_LOC_SYMBOL 1
#define TMAP_LOC_EXREG(i) (20 + (i))
#define TMAP_MAX_NUM_LOCS 3

#define ETFG_FBGEN_TARGET_REGNAME "%trgt"

#define G_MEMLABEL_VAR_PREFIX "mlvar."
#define G_MEMLABEL_VAR_PREFIX_FOR_FCALL "mlcall."
//#define PRED_ISMEMLABEL_LOC_COMMENT_PREFIX "ismemlabel-loc."
//#define PRED_ISMEMLABEL_ADDR_COMMENT_PREFIX "ismemlabel-addr."

#define ETFG_ISEQ_TFG_NAME "etfg_iseq_tfg"
#define REG_EXPR_PREFIX "reg"

#define INSN_MAX_NEXTPC_COUNT 8

//#define G_STATE_INDIR_TARGET_ADDR_DUMMY_KEY_ID "indir_tgt_addr_dummy"
#define G_STATE_INDIR_TARGET_KEY_ID "indir_tgt"

#define I386_FASTCALL_NUM_REGPARAMS 2
#define I386_FASTCALL_REGPARAM(r) (((r) == 0) ? R_ECX : ((r) == 1) ? R_EDX : -1/*not-reached*/)
#define I386_FASTCALL_NUM_RETREGS 2
#define I386_FASTCALL_RETREG(r) (((r) == 0) ? R_EAX: ((r) == 1) ? R_EDX : -1/*not-reached*/)

#define I386_FASTCALL_NUM_REGPARAMS_EXT 7
#define I386_FASTCALL_REGPARAM_EXT(r) (((r) <= 1) ? I386_FASTCALL_REGPARAM(r) : ((r) == 2) ? R_EBX : ((r) == 3) ? R_EBP : ((r) == 4) ? R_ESI : ((r) == 5) ? R_EDI : R_EAX)

#define X64_FASTCALL_NUM_REGPARAMS 2
#define X64_FASTCALL_REGPARAM(r) (((r) == 0) ? R_ECX : ((r) == 1) ? R_EDX : -1/*not-reached*/)
#define X64_FASTCALL_NUM_RETREGS 2
#define X64_FASTCALL_RETREG(r) (((r) == 0) ? R_EAX: ((r) == 1) ? R_EDX : -1/*not-reached*/)

#define X64_FASTCALL_NUM_REGPARAMS_EXT 7
#define X64_FASTCALL_REGPARAM_EXT(r) (((r) <= 1) ? I386_FASTCALL_REGPARAM(r) : ((r) == 2) ? R_EBX : ((r) == 3) ? R_EBP : ((r) == 4) ? R_ESI : ((r) == 5) ? R_EDI : R_EAX)


#define ETFG_CANON_CONST0 1234
#define ETFG_CANON_CONST1 2345

#define ETFG_SYMBOLS_START_ADDR 0x10000000

#define MYTEXT_SECTION_NAME ".text"
#define MYDATA_SECTION_NAME ".data"

#define REG_ASSIGNED_TO_CONST_PREFIX "REGCONST_"
#define LLVM_NUM_CALLEE_SAVE_REGS 4
#define I386_NUM_CALLEE_SAVE_REGS 4

#define MAX_CALLEE_SAVE_CONSTANTS I386_NUM_CALLEE_SAVE_REGS

#define PEEPTAB_MEM_CONSTANT(i) (0x9368+8*(i)) //the constant should be such that it is not expected to appear oherwise in the peep tab. also different constants should be separated by at least 8 bytes
#define PEEPTAB_MAX_MEM_CONSTANTS 64
#define PEEPTAB_MEM_CONSTANT_GET_ORIG(v) (((v) - 0x9368)/8)

#define CANON_CONSTANT_PREFIX "const."
#define RANDOM_CONSTANT_FOR_SYMBOL_ID(symbol_id) (9999 + 13*(symbol_id))
#define RANDOM_CONSTANT_FOR_LOCAL_ID(local_id) (5236 + 13*(local_id))

#define LLVM_SWITCH_TMPVAR_PREFIX "%switch.tmpvar."

//#define SRC_INPUT_ARG_NAME_SUFFIX "_ARG"
#define LLVM_METHOD_ARG_PREFIX "llvm-method-arg."

#define G_SRC_KEYWORD "src"
#define G_DST_KEYWORD "dst"
#define G_SSA_KEYWORD "ssa"
#define ITER_VAR_KEYWORD "itervar"
#define GHOST_VAR_KEYWORD "ghostvar"

#define EPSILON "epsilon"
#define DYN_DEBUG_STATE_END_OF_STATE_MARKER "--"

#define TX_TEXT_START_ADDR 0x8000000
#define TX_TEXT_START_PTR ((uint8_t *)0x700000000000 + TX_TEXT_START_ADDR)
#define CODE_GEN_BUFFER_SIZE     (1<<26)

//#define LLVM_BASICBLOCK_COUNTING_IDX_START 0x100000
#define LLVM_UNDEF_VALUE_NAME G_LLVM_PREFIX "-undef-value"
#define UNDEF_VARIABLE_NAME "undef-variable"

#define PHI_NODE_TMPVAR_SUFFIX ".phi.tmpvar"

#define TYPE_PROBE_NAME "type_probe"
#define MEMVAR_NAME_PREFIX "memvar."

#define DEFAULT_USEDEF_TAB_HASH_SIZE 65537
#define DEFAULT_TS_TAB_HASH_SIZE 65537

#define TRANSMAP_REG_ID_UNASSIGNED 65535

#define MAX_REGS_PER_ITABLE_ENTRY 5
#define MAX_VCONSTS_PER_ITABLE_ENTRY 4
#define MAX_MEMLOCS_PER_ITABLE_ENTRY 4
#define MAX_NEXTPCS_PER_ITABLE_ENTRY 1

#define PEEPTAB_MEM_PREFIX "MEMLOC"

#define MAX_NUM_CANON_CONSTANTS_IN_SRC_ISEQ 4
#define NUM_IMM_MAPS 4
//#define NUM_STATES 76
#define NUM_STATES 4
#define MAX_CODE_LEN 4096

//#define NUM_FP_STATES NUM_STATES
#define MAX_COMMENT_SIZE 40960
#define MAX_NUM_MEMLOCS 16
#define STATE_MAX_STACK_SIZE 16 //could be increased

#define SRC_ENV_STATE_STACK_BOTTOM (SRC_ENV_ADDR + offsetof(state_t, stack[0]))
#define SRC_ENV_STATE_STACK_TOP (SRC_ENV_ADDR + offsetof(state_t, stack[STATE_MAX_STACK_SIZE >> 2]))
#define FINGERPRINT_TYPE_ERROR ((uint64_t)-1)

#define PRIORITIZE_ENUM_CANDIDATES_LIVE_EQ_TOUCH 1
#define PRIORITIZE_ENUM_CANDIDATES_PRUNE_TMAP_FLAG_MAPPINGS_SYNTACTICALLY 2
#define PRIORITIZE_ENUM_CANDIDATES_PRUNE_TMAP_FLAG_MAPPINGS_READ_EXCEPT_CBRANCH 3

//#define DEFAULT_MIN_THRESH_STATES_PER_SRC_ISEQ 64
#define MAX_NUM_FPS 1024
#define DB_ID_INVALID 0
#define MAX_NUM_DST_INSNS_PER_TRANSLATION_UNIT 8192
#define MAX_EXEC_NEXTPCS (256+1) //+1 for NEXTPC_TYPE_ERROR
#define SI_MAX_NUM_NEXTPCS 16

#define UNRESOLVED_SYMBOL 1  /* must be non-zero */

#define PC_UNDEFINED     0x7fffffff
#define PC_JUMPTABLE     0x7ffffffe
#define PC_NEXTPC_CONST(nextpc_num) (0x7fff0000 + nextpc_num) //used only for etfg iseqs where fcalls name nextpc_consts (and not addresses)
#define PC_NEXTPC_CONSTS_MAX_COUNT (PC_JUMPTABLE - PC_NEXTPC_CONST(0))
#define PC_IS_NEXTPC_CONST(p) ((p) >= PC_NEXTPC_CONST(0) && (p) < PC_JUMPTABLE)
#define PC_NEXTPC_CONST_GET_NEXTPC_ID(nextpc_const) ((nextpc_const) - 0x7fff0000)

#define BBL_MAX_NUM_NEXTPCS 8
#define BBL_MAX_NUM_INSNS 8192

//#define COST_INFINITY ((long)(1ULL<<62))
#define COST_INFINITY ((long)0x7fffffffffffffffULL)
#define EQ_MAPPING_BREAK_COST_FLOW_WEIGHT 0x1000000ULL

#define LLVM_FCALL_ARG_COPY_PREFIX G_LLVM_PREFIX "-fcall_arg_copy."
#define NUM_COLORS_INFINITY 999999

#define PEEP_MATCH_COST_FALLTHROUGH_NEXTPC_NOT_MAPPED_IDENTICALLY 2

#define ETFG_MAIN_FUNCTION_IDENTIFIER "main"
#define ETFG_MAIN_IN_FLOW (1ULL << 24)
#define ETFG_FUNCTION_IN_FLOW (1ULL << 22)
#define PRED_WEIGHT_JUMPTABLE_PC 1
#define MAX_TAIL_PATH_LENGTH_FOR_OPENING_DIAMOND_CFG_STRUCTURES 8
#define COMPUTE_FLOW_ON_CFG_ITER 100
#define COMPUTE_FLOW_NUM_ITER_FOR_FUNCTION_ENTRIES 16

#define MIR_PHI_NODE_OPCODE "PHI"
#define MIR_RET_NODE_OPCODE "RET"
#define MIR_FIXED_STACK_SLOT_PREFIX "%fixed-stack."
#define NEXTPC_ID_INVALID -1
#define LLVM_FIELDNUM_PREFIX "field"
#define GRAPH_LOC_ID_INVALID (-1)
#define EXPR_UNDEF_VALUE_STR "undef"

#define BVSOLVE_DEDUP_THRESHOLD 10
#define SOLVER_MEMLABEL_SORT_BVSIZE 8
#define SOLVER_ROUNDING_MODE_SORT_BVSIZE 3
#define G_MAX_QUERY_COMMENT_SIZE 150 /* keep this sufficiently less than Linux filename limit i.e. 256 */
//#define G_MEMLABEL_STACK_SIZE (1<<28) /* 256 MiB ought to be enough for stack */
#define G_MEMLABEL_DEFAULT_SIZE (1 << 20)  //1MiB
#define UNALIASED_MEMSLOT_FRESH_VAR_PREFIX "unaliased_memslot."

#define FREE_VAR_IDX_PREFIX "free_var_idx."
#define ALIGNMENT_FOR_FUNCTION_SYMBOL 1
#define ALIGNMENT_FOR_TRANSMAP_MAPPING_TO_SYMBOL 1
#define ALIGNMENT_FOR_RODATA_SYMBOL 1
#define ALIGNMENT_FOR_VARARG_LOCAL 4

#define ADDR_DIFF_INDICATOR_VAR_PREFIX "addr_diff_indicator_var."
#define SIMPLIFY_FIXED_POINT_SYNTACTIC_CONVERGENCE_MAX_ITERS 16
#define SIMPLIFY_FIXED_POINT_CONVERGENCE_MAX_ITERS 1024
#define HEAP_UNALLOCATED_START_ADDR "heap_unallocated_start_addr"
#define HEAP_UNALLOCATED_END_ADDR "heap_unallocated_end_addr"
#define QCC_BUILD_MAX_TRIES 10

#define QCC_MARKER_PREFIX "marker"
#define PC_BVSORT_LEN DWORD_LEN
#define FUNCTION_CALL_IDENTIFIER_PREFIX "func.call"
#define G_DST_SYMBOL_PREFIX "dst-symbol-"
#define HEAP_CONSTRAINT_EXPR_HOLE_VARNAME "heap_constraint_expr_hole_var"

#define ETFG_SYMBOL_ID_FUNCTION_START 10000
#define ETFG_SYMBOL_ID_NEXTPC_INDEX_START 20000
#define ETFG_SYMBOL_NEXTPC_PREFIX "nextpc"
#define DST_INSN_FOLDING_FILENAME_PREFIX "dst_insn_folding"
#define LLVM_LOCKSTEP_DEFAULT_VOID_VAL (-1)

#define STACK_SIZE_VARNAME "stack.size"
#define RODATA_SIZE_VARNAME "rodata.size"
#define ARRAY_CONSTANT_STRING_RAC_PREFIX "RAC"

#define LOOKUP_CACHE_DEFAULT_SIZE (1<<16) /* 64K ought to be enough for a cache */

#define QUERY_DECOMPOSE_TIMED_OUT_QUERIES_THRESHOLD 1 /* selected arbitrarily */
#define QUERY_DECOMPOSE_QUERIES_THRESHOLD 16000 /* selected arbitrarily */


#define SP_VERSION_REGISTER_LABEL_PREFIX G_DST_PREFIX ".sp="
#define SP_VERSION_REGISTER_LABEL_SUFFIX "=sp"
#define FNAME_GLOBAL_SPACE "GLOBAL"
#define CONSTEXPR_KEYWORD "constexpr"

#define VARARG_LOCAL_VARNAME "--vararg--"

#define SEMANTICAA_LOCSIZE_UNKNOWN ((uint64_t)-1)

#define EQUIVALENCE_FOUND_MESSAGE "Equivalence proof found. The programs are equivalent"
#define EQUIVALENCE_NOT_FOUND_MESSAGE "Equivalence proof not found after an exhaustive search"
#define EQUIVALENCE_TIMER_NAME "Time taken for equivalence check"
#define SMT_QUERY_TIMER_NAME "query:smt"

#define XML_PRINTING_SYMBOL_PREFIX "@"
#define XML_PRINTING_LOCAL_PREFIX "#"

//src_env start
#define TYPE_ENV_INITIAL_ADDR (SRC_ENV_ADDR + 0x1000000)
#define TYPE_ENV_INITIAL_SIZE (1 << 20)

#define TYPE_ENV_FINAL_ADDR   (TYPE_ENV_INITIAL_ADDR + 0x800000)
#define TYPE_ENV_FINAL_SIZE TYPE_ENV_INITIAL_SIZE

#define TYPE_ENV_SCRATCH_ADDR (TYPE_ENV_FINAL_ADDR + 0x800000)
#define TYPE_ENV_SCRATCH_SIZE SRC_ENV_SIZE
#define TYPE_ENV_TMP_ADDR (TYPE_ENV_SCRATCH_ADDR + TYPE_ENV_SCRATCH_SIZE - 32)
#define TYPE_ENV_ERR_ADDR (TYPE_ENV_SCRATCH_ADDR + TYPE_ENV_SCRATCH_SIZE - 28)

#define SRC_ENV_NEXTPC_NUM (&(((state_t *)SRC_ENV_ADDR)->nextpc_num))
//#define SRC_ENV_STACK_SIZE 256
//#define SRC_ENV_STACK_TOP (SRC_ENV_ADDR + SRC_ENV_SIZE)
//#define SRC_ENV_STACK_BOTTOM (SRC_ENV_STACK_TOP - SRC_ENV_STACK_SIZE)
//#define SRC_ENV_STACK_BOTTOM (SRC_ENV_STACK_TOP - SRC_ENV_STACK_SIZE)
#define SRC_ENV_END (SRC_ENV_ADDR + SRC_ENV_SIZE)
#define SRC_ENV_TMPREG_STORE (SRC_ENV_END - 272)
#define SRC_ENV_MEMBASE (SRC_ENV_END - 264)
#define SRC_ENV_EXCEPTION (SRC_ENV_END - 256)
#define SRC_ENV_SCRATCH0 (SRC_ENV_END - 252)
#define SRC_ENV_SCRATCH(n) (SRC_ENV_SCRATCH0 + (n)*8)
#define SRC_TMP_STACK (SRC_ENV_END - 128) //this has nothing to do with SRC_ENV_STACK; this is just for tmp storage used by our own logic, not user accessible.
#define TMP_STORAGE SRC_TMP_STACK

#define EXCEPTION_FPE 123

#define QEMU_ENV_REG(env, regnum) (dst_ulong)(long)((void *)(env) + (regnum) * sizeof(src_ulong))
/*#define QEMU_ENV_SET_REG(env, regnum, val) do { \
    *(src_ulong *)((void *)(env) + (regnum) * sizeof(src_ulong)) = (val); \
} while(0)*/
#define QEMU_ENV_EXCEPTION_INDEX(env) 0x80 // assume, only exceptions are syscalls
#define QEMU_ENV_PC(env) *(src_ulong *)((void *)(env) + 8 * sizeof(src_ulong))
#define QEMU_ENV_CRF(env, crfn) (dst_ulong)(long)((void *)(env) + 34 * sizeof(src_ulong) + (crfn) * sizeof(uint32_t))
#define QEMU_ENV_XER_SO(env) (dst_ulong)(long)((void *)(env) + 34 * sizeof(src_ulong) + 8 * sizeof(uint32_t) + 1 * sizeof(src_ulong))
#define QEMU_ENV_JUMPTARGET(env) ((dst_ulong)(long)env + 0x268)

#if SRC_FALLBACK == FALLBACK_QEMU
#define SRC_ENV_JUMPTARGET QEMU_ENV_JUMPTARGET((void *)SRC_ENV_ADDR)
#elif (SRC_FALLBACK == FALLBACK_ID || SRC_FALLBACK == FALLBACK_ETFG)
#define SRC_ENV_JUMPTARGET (SRC_ENV_ADDR + 512)
#endif

#if ARCH_SRC == ARCH_I386

#define R_SYSCALL_ARG0 R_EAX
#define R_SYSCALL_ARG1 R_EBX
#define R_SYSCALL_ARG2 R_ECX
#define R_SYSCALL_ARG3 R_EDX
#define R_SYSCALL_ARG4 R_ESI
#define R_SYSCALL_ARG5 R_EDI
#define R_SYSCALL_ARG6 R_EBP
#define R_SYSCALL_RETVAL R_EAX

#elif ARCH_SRC == ARCH_PPC

#define R_SYSCALL_ARG0 0
#define R_SYSCALL_ARG1 3
#define R_SYSCALL_ARG2 4
#define R_SYSCALL_ARG3 5
#define R_SYSCALL_ARG4 6
#define R_SYSCALL_ARG5 7
#define R_SYSCALL_ARG6 8
#define R_SYSCALL_RETVAL 3

#endif
//src_env stop

#define SETUP_BUF_SIZE 0
#define START_VMA ((uint8_t *)(TX_TEXT_START_PTR - SETUP_BUF_SIZE))

#define PREDICATE_MERGE_THRESHOLD 64
#define CG_PREDICATE_MERGE_THRESHOLD 8
#define RELOCATABLE_MEMLABELS_CHECK_FILENAME_PREFIX "cg.check_relocatable_memlabels"

#define LINENAME_PREFIX "line "
#define RIPREL_VARNAME "riprel"

#define CG_WITH_FAILCOND_DST_NODE_WEIGHT 10
#define CG_WITH_FAILCOND_SRC_RANK_WEIGHT 100
#define CG_WITH_FAILCOND_DST_RANK_WEIGHT 1000
#define CG_WITH_FAILCOND_CORRECT_NODE_WEIGHT 10000
#define CG_WITH_FAILCOND_ABSTRACT_ASSERTION_WEIGHT 100000

#define LAMBDA_VARNAME_PREFIX "Lambda"
#define COUNTER_EXAMPLE_MAX_CONTAINER_SIZE 64

#define EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN -1
#define CUR_ROUNDING_MODE_VARNAME "cur_rounding_mode"

#define ROUNDING_MODE_CONSTANT_PREFIX "ROUND_"

#define REPLACE_DONOTSIMPLIFY_USING_SOLVER_EXPRESSIONS_FREE_VAR_PREFIX "replace_donotsimplify_using_solver_expressions."

#define FPSTACK_ADDRLEN 3

#define I386_ROUNDING_MODE_ENCODING_FOR_ROUND_TO_NEAREST 0

#define WORD_FP_EBITS_LEN 5
#define WORD_FP_SBITS_LEN 11

#define DWORD_FP_EBITS_LEN 8
#define DWORD_FP_SBITS_LEN 24

#define QWORD_FP_EBITS_LEN 11
#define QWORD_FP_SBITS_LEN 53

#define FPWORD_FP_EBITS_LEN 15
#define FPWORD_FP_SBITS_LEN 64

#define DQWORD_FP_EBITS_LEN 15
#define DQWORD_FP_SBITS_LEN 113

#define FLOATX80_INTEGER_BIT_INDEX 63
#define BV_PRED_ARITY_THRESHOLD 14

#define G_MEMALLOC_SSA_VARNAME_SUFFIX "memalloc.ssa.var"

#define G_LOCAL_ALLOC_COUNT_VARNAME "local_alloc_count"
//#define G_LOCAL_ALLOC_COUNT_SSA_VARNAME "local_alloc_count.ssa"

#define COMMENT_WORD_LEN DWORD_LEN
#define COMMENT_MAX_DIGIT_VAL 10000

//#define ALLOCSITE_PC_VARARG_LOCAL_INDEX "0"
//#define ALLOCSITE_PC_SUBINDEX 0
//#define ALLOCSITE_PC_SUBSUBINDEX 0

#define UNSTABLE_CG_DUMP_PREFIX "unstable_cg."

#define DUMMY_ARG_ADDR_VARNAME "dummy-arg-addr"

#define DEFAULT_BIGENDIAN_VALUE false

#define ALLOCSITE_BEGIN_CHAR '='
#define ALLOCSITE_END_CHAR '='

#define ALLOCA_PTR_FN_NAME "alloca_ptr"

#define DYN_DEBUG_CMDLINE_NAME "dyn-debug"
#define DYN_DEBUG_CMDLINE_PREFIX DYN_DEBUG_CMDLINE_NAME "="

#define ALLOCA_SIZE_FN_NAME "alloca_size"
#define REGION_SIZE_SMALL_THRESHOLD ((int)(QWORD_LEN/BYTE_LEN))

#define UPDATE_DST_EDGE_FOR_LOCAL_ALLOCATIONS_AND_DEALLOCATIONS_DUMP_PREFIX "update_dst_edge_for_local_allocations_and_deallocations"

// (isp+4) is aligned by 16 (ref. i386 ABI, $2.2.2) -- also 16 in amd64
#define STACK_ALIGNMENT_AT_FCALL_ENTRY 16

#define DST_NAME_FOR_ASSEMBLY "dst"

#define DUMMY_FNAME "dummy_fname"

#define ALLOCSTACK_BEGIN_CHAR '/'
#define ALLOCSTACK_END_CHAR '/'
#define ALLOCSTACK_FUNC_ALLOCSITE_SEPARATOR "__x__"
#define ALLOCSTACK_SEPARATOR "___y___"
#define ALLOCSTACK_ANY_PREFIX_SYMBOL "*"

#define CALL_CONTEXT_BEGIN_CHAR '{'
#define CALL_CONTEXT_END_CHAR '}'
#define CALL_CONTEXT_ENTRY_SEPARATOR ":"
#define CALL_CONTEXT_SEPARATOR ";"
#define CALL_CONTEXT_ANY_PREFIX_SYMBOL "*"



#define DEFAULT_ENTRY_FNAME "main"
#define FTMAP_ENTRY_IDENTIFIER "=FunctionTFGMap Entry:"

#define PMEM_PREFIX "pmem_"

#define LOCAL_ALLOC_CALLEE_NAME "local_allocFN"
#define HEAPALLOC_CALLEE_NAME "heapallocFN"

#define LIBC_MALLOC_FNAME "malloc"
#define LIBC_FREE_FNAME "free"
#define CPP_NEW_FNAME "_Znwm"
#define CPP_DELETE_FNAME "_ZdlPv"

#define CALL_CONTEXT_NULL_FUNCTION_NAME "null-function"

//#define CLONED_PC_PREFIX "cloned"
#define CLONE_COUNT_NONE 0
#define CLONE_COUNT_ONE 1
//#define CLONE_COUNT_TWO 2

#define QD_PROCESS_CANCEL_MESSAGE "0x420x43"
#define QD_PROCESS_CANCEL_RECEIPT_MESSAGE "9gxabcd1" //needs to be unique enough that it does not appear otherwise in the qd-process response (e.g., in counterexamples)

#define PROVE_QD_POLL_INTERVAL_IN_MILLISECONDS 10
#define BV_SOLVE_VAR_IDX_FOR_CONSTANT_TERM (INT_MIN)

#define RAND_VALS_CE_NAME_PREFIX "rand_vals."
#define WEAKENING_POINT_PREFIX "weakening-point."
#define PC_EMPTY_CLONE_PREFIX "emptyclone"
#define PC_CLONE_PREFIX "cloned"

#define SRC_TFG_SSA_START_LOC_ID 0
#define DST_TFG_SSA_START_LOC_ID 1000

#define FTMAP_CALL_GRAPH_PC_SEPARATOR ":"

#define CALL_CONTEXT_DEPTH_ZERO 0
#define FTMAP_CALL_GRAPH_START_FNAME "call_graph_start_fname"

#define TFG_SSA_CONSTRUCT_FROM_NON_SSA_TFG "tfg_ssa_construct_from_non_ssa_tfg"

#define PRED_COMMENT_EXIT_BOOLBV "exit.boolbv"
#define PRED_COMMENT_EXIT_MEMEQ "exit.memeq"
#define PRED_COMMENT_EXIT_SP_PRESERVED "exit.sp-preserved"
#define PRED_COMMENT_GUESS_MEMALLOCS_EQ "mem-allocs-equality"
#define PRED_COMMENT_GUESS_MEM_NONSTACK_EQ "mem-nonstack"
#define PRED_COMMENT_GUESS_MEM_LOCAL_EQ "memeq-local-"
#define PRED_COMMENT_DST_EDGECOND_PROVES_FALSE "dst_edge_composition_proves_false"
#define PRED_COMMENT_FALSE_PREDICATE "false-predicate"
#define PRED_COMMENT_TRUE_PREDICATE "true-predicate"
#define PRED_COMMENT_INEQ_UPPER_BOUND_UNSIGNED "ub-unsigned"
#define PRED_COMMENT_INEQ_LOWER_BOUND_SIGNED "lb-signed"
#define PRED_COMMENT_INEQ_UPPER_BOUND_SIGNED "ub-signed"
#define PRED_COMMENT_INEQ_LOWER_BOUND_UNSIGNED "lb-unsigned"
#define PRED_COMMENT_HOUDINI_GUESS_SUFFIX "-houdini-guess"

#define EXPR_GROUP_NAME_REGION_AGREES_WITH_MEMLABEL "expr-group-region_agrees_with_memlabel"
#define EXPR_GROUP_NAME_NONARG_LOCALS_ISCONTIGUOUS "nonarg-locals-iscontiguous"
#define EXPR_GROUP_NAME_NONARG_LOCALS_IS_PROBABLY_CONTIGUOUS "nonarg-locals-isprobably-contiguous"
#define EXPR_GROUP_NAME_VARIABLY_SIZED_ALLOCA_SP_VERSIONS "variably-sized-alloca-sp_versions"
#define EXPR_GROUP_NAME_LOCALS_LB_UB_AND_SP_VERSIONS "locals-lb-ub-and-sp_versions"
#define EXPR_GROUP_NAME_LOCALS_UB_UPPER_BOUND "locals-ub-upper-bound"
#define EXPR_GROUP_NAME_ABSTRACT_MEMLABEL_ASSERTS "abstract-memlabel-asserts"
#define EXPR_GROUP_NAME_LOCAL_SIZES_ARE_EQUAL_SUFFIX "-local-sizes-are-equal"
#define EXPR_GROUP_NAME_MEMLABEL_IS_ABSENT "memlabel-is-absent"
#define EXPR_GROUP_NAME_SP_IN_STACK "sp-in-stack"
#define EXPR_GROUP_NAME_SP_BELOW_ISP "sp-below-isp"

#define PRED_COMMENT_GUESS_HOUDINI_REGION_AGREES_WITH_MEMLABEL EXPR_GROUP_NAME_REGION_AGREES_WITH_MEMLABEL PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_NONARG_LOCALS_ISCONTIGUOUS EXPR_GROUP_NAME_NONARG_LOCALS_ISCONTIGUOUS PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_NONARG_LOCALS_IS_PROBABLY_CONTIGUOUS EXPR_GROUP_NAME_NONARG_LOCALS_IS_PROBABLY_CONTIGUOUS PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_VARIABLY_SIZED_ALLOCA_SP_VERSIONS EXPR_GROUP_NAME_VARIABLY_SIZED_ALLOCA_SP_VERSIONS PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_LOCALS_LB_UB_AND_SP_VERSIONS EXPR_GROUP_NAME_LOCALS_LB_UB_AND_SP_VERSIONS PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_LOCALS_UB_UPPER_BOUND EXPR_GROUP_NAME_LOCALS_UB_UPPER_BOUND PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_ABSTRACT_MEMLABEL_ASSERTS EXPR_GROUP_NAME_ABSTRACT_MEMLABEL_ASSERTS PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_LOCAL_SIZES_ARE_EQUAL_SUFFIX EXPR_GROUP_NAME_LOCAL_SIZES_ARE_EQUAL_SUFFIX PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_MEMLABEL_IS_ABSENT EXPR_GROUP_NAME_MEMLABEL_IS_ABSENT PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_SP_IN_STACK EXPR_GROUP_NAME_SP_IN_STACK PRED_COMMENT_HOUDINI_GUESS_SUFFIX
#define PRED_COMMENT_GUESS_HOUDINI_SP_BELOW_ISP EXPR_GROUP_NAME_SP_BELOW_ISP PRED_COMMENT_HOUDINI_GUESS_SUFFIX
