#ifndef TYPESTATE_H
#define TYPESTATE_H

#include <stdio.h>
#include <vector>
#include "i386/regs.h"
#include "ppc/regs.h"
#include "valtag/regset.h"
#include "exec/state.h"
#include "cutils/graph_node.h"
#include "valtag/state_elem_desc.h"

struct transmap_t;
struct state_t;
struct src_insn_t;
struct itable_t;
struct ts_tab_entry_cons_t;
struct ts_tab_entry_transfer_t;
struct state_elem_desc_t;
struct regset_t;
struct regmap_t;
struct itable_t;
struct dst_insn_t;
class rand_states_t;
class memslot_set_t;

#define DATATYPE_POLYMORPHIC 0b11111111
#define DATATYPE_BOOL        0b00001110
#define DATATYPE_SHCOUNT     0b00011100
#define DATATYPE_INT         0b00001100
#define DATATYPE_ADDR        0b00001001
#define DATATYPE_INTADDR     0b00001000
#define DATATYPE_CODE        0b00100000
#define DATATYPE_ONEBIT      0b01000000
#define DATATYPE_ROTCOUNT    0b10000000
#define DATATYPE_UDTYPE13    0b10100000
#define DATATYPE_BOTTOM      0b00000000

#define UNCOPYABLE 1
#define COPYABLE 0

//#define TYPEMOD_POLYMORPHIC  0b111  //equivalent to uninit. except allows usage
//                                    //as variable during type inference
//#define TYPEMOD_READ         0b101
//#define TYPEMOD_WRITE_CONST  0b011
//#define TYPEMOD_RW_CONST     0b001
//#define TYPEMOD_WRITE_ANY    0b010
//#define TYPEMOD_RW_ANY       0b000

//#define TYPEMOD_UNINIT       0b11
//#define TYPEMOD_READ         0b01
//#define TYPEMOD_WRITE        0b10
//#define TYPEMOD_READWRITE    0b00

//#define INDEX_SIZE_TYPE_POLYMORPHIC 0b11111111111 //used only during type inference
//#define INDEX_SIZE_TYPE_0_1         0b00000010001
//#define INDEX_SIZE_TYPE_0_2         0b00000010010
//#define INDEX_SIZE_TYPE_0_4         0b00000010100
//#define INDEX_SIZE_TYPE_0_8         0b00000011000
////#define INDEX_SIZE_TYPE_1_1         0b00000100001
//#define INDEX_SIZE_TYPE_1_2         0b00000100010
//#define INDEX_SIZE_TYPE_1_4         0b00000100100
//#define INDEX_SIZE_TYPE_1_8         0b00000101000
//#define INDEX_SIZE_TYPE_2_4         0b00001000100
//#define INDEX_SIZE_TYPE_2_8         0b00001001000
//#define INDEX_SIZE_TYPE_3_4         0b00010000100
//#define INDEX_SIZE_TYPE_3_8         0b00010001000
//#define INDEX_SIZE_TYPE_4_8         0b00100001000
//#define INDEX_SIZE_TYPE_5_8         0b01000001000
////#define INDEX_SIZE_TYPE_6_8         0b10000001000
//#define INDEX_SIZE_TYPE_6_8         0b10000001000
//#define INDEX_SIZE_TYPE_7_8         0b00000100001
//#define INDEX_SIZE_TYPE_ANY         0b00000000000

//#define INDEX_SIZE_TYPE_POLYMORPHIC 0b111111 //used only during type inference
//#define INDEX_SIZE_TYPE_0_1         0b001110
//#define INDEX_SIZE_TYPE_0_2         0b001101
//#define INDEX_SIZE_TYPE_0_4         0b001011
//#define INDEX_SIZE_TYPE_0_8         0b000111
//#define INDEX_SIZE_TYPE_1_2         0b011100
//#define INDEX_SIZE_TYPE_1_4         0b011001
//#define INDEX_SIZE_TYPE_1_8         0b011010
//#define INDEX_SIZE_TYPE_2_4         0b010110
//#define INDEX_SIZE_TYPE_2_8         0b010101
//#define INDEX_SIZE_TYPE_3_4         0b010011
//#define INDEX_SIZE_TYPE_3_8         0b110001
//#define INDEX_SIZE_TYPE_4_8         0b110010
//#define INDEX_SIZE_TYPE_5_8         0b110100
//#define INDEX_SIZE_TYPE_6_8         0b111000
//#define INDEX_SIZE_TYPE_7_8         0b101100
//#define INDEX_SIZE_TYPE_ANY         0b000000

#define INDEX_SIZE_TYPE_POLYMORPHIC 0b1111111 //used only during type inference
#define INDEX_SIZE_TYPE_0_8         0b0011111
#define INDEX_SIZE_TYPE_0_4         0b0011110
#define INDEX_SIZE_TYPE_0_2         0b0011100
#define INDEX_SIZE_TYPE_0_1         0b0011000
#define INDEX_SIZE_TYPE_1_8         0b0101110
#define INDEX_SIZE_TYPE_1_4         0b0101100
#define INDEX_SIZE_TYPE_1_2         0b0101000
#define INDEX_SIZE_TYPE_2_8         0b0110100
#define INDEX_SIZE_TYPE_2_4         0b0110000
#define INDEX_SIZE_TYPE_3_8         0b1001100
#define INDEX_SIZE_TYPE_3_4         0b1001000
#define INDEX_SIZE_TYPE_4_8         0b1010000
#define INDEX_SIZE_TYPE_5_8         0b1100000
#define INDEX_SIZE_TYPE_6_8         0b1000001
#define INDEX_SIZE_TYPE_7_8         0b1000010
#define INDEX_SIZE_TYPE_ANY         0b0000000

#define TYPESTATE_ELEM_ID_MAX (-1)

//#define SIZE_TYPE_POLYMORPHIC 0b111 //used only during type inference
//#define SIZE_TYPE_1          0b001
//#define SIZE_TYPE_2          0b010
//#define SIZE_TYPE_4          0b100
//#define SIZE_TYPE_ANY        0b000

/*typedef enum tscons_rhs_t {
  TSCONS_RHS_VAR,
  TSCONS_RHS_CONST,
  TSCONS_RHS_MEET,
} tscons_rhs_t;*/

/*typedef enum tscons_lhs_kind_t {
  TSCONS_LHS_DATATYPE,
  TSCONS_LHS_TYPEMOD,
  TSCONS_LHS_INDEX_TYPE,
  TSCONS_LHS_SIZE_TYPE,
} tscons_lhs_kind_t;*/

typedef enum {
    USE_TYPES_NONE = 0,
    USE_TYPES_OPS_ONLY, //check that opcodes are applied to appropriate operands
    USE_TYPES_OPS_AND_GE_MODIFIED_BOTTOM,
    USE_TYPES_OPS_AND_GE_INIT_MEET_FINAL,
    USE_TYPES_OPS_AND_GE_FINAL,
        //check that the types of dst is always greater than equal to typestate_final
    //USE_TYPES_NO_FINGERPRINT,
    //USE_TYPES_OPS_AND_FINAL_AND_COMBO_RESTRICTIONS,
} use_types_t;

char *use_types_to_string(use_types_t use_types, char *buf, size_t size);
size_t use_types_from_string(use_types_t *use_types, char const *buf);

typedef struct typestate_elem_t {
  unsigned datatype:8;
  //unsigned typemod:3;
  unsigned index_size:7;
  unsigned copyable:1;
  //unsigned ts_unused:0;

  //uint8_t write_const_value;
  uint16_t polymorphic_datatype;
  //uint16_t polymorphic_typemod;
  //uint16_t polymorphic_index;
  //uint16_t polymorphic_size;
  void typestate_elem_convert_addr_datatype_to_intaddr_datatype();
  void typestate_elem_make_non_poly_copyable();
} typestate_elem_t;

typedef struct tscons_t {
  int var;
  typestate_elem_t max_type;
} tscons_t;

typedef struct typestate_constraints_t {
  int num_constraints, cons_size;
  tscons_t *cons;
} typestate_constraints_t;


#define MAX_NUM_TE_PER_ENTRY 32

typedef struct typestate_t {
  //long type_mismatching_insn_index;
  //int last_executed_insn;
  typestate_elem_t exregs[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)][MAX_NUM_TE_PER_ENTRY];
  typestate_elem_t memlocs[MAX_NUM_MEMLOCS][MAX_NUM_TE_PER_ENTRY];
  typestate_elem_t stack[STATE_MAX_STACK_SIZE];
  typestate_elem_t memory[MEMORY_SIZE + MEMORY_PADSIZE];

  typestate_elem_t vconsts[NUM_CANON_CONSTANTS][MAX_NUM_TE_PER_ENTRY];

  void typestate_apply_to_all_typestate_elems(std::function<void (typestate_elem_t &)> f);
  void typestate_convert_addr_datatype_to_intaddr_datatype();
  void typestate_make_non_poly_copyable();
  void typestate_set_poly_bytes_in_non_poly_regs_to_bottom();
  static void typestate_reg_if_non_poly_set_poly_bytes_to_bottom(typestate_elem_t *tsreg);
} typestate_t;

void typestate_elem_init(typestate_elem_t *ts, long vnum);
void typestate_elem_copy(typestate_elem_t *dst, typestate_elem_t const *src);
void typestate_elem_meet(typestate_elem_t *dst, typestate_elem_t const *src);
int get_index_size_type(int index, int size);
bool is_index_size_type(typestate_elem_t const *e, int *index, int *size);

void typestate_init(typestate_t *ts);
void typestate_init_to_top(typestate_t *ts, size_t num_states);
bool typestate_elem_is_top(typestate_elem_t const *ts);
//void typestate_init_using_itable(typestate_t *ts_initial, typestate_t *ts_final,
//    struct itable_t const *itable);
void typestate_copy(typestate_t *dst, typestate_t const *src);
void typestates_copy(typestate_t *dst, typestate_t const *src, size_t num_states);
bool typestates_equal(typestate_t const *a, typestate_t const *b);
bool typestates_equal_n(typestate_t const *a, typestate_t const *b, int n);
//void typestate_free(typestate_t *ts);
void typestate_meet(typestate_t *dst, typestate_t const *src);
void typestate_rename_using_transmap(typestate_t *state, struct transmap_t const *tmap);
void typestate_rename_using_transmap_inverse(typestate_t *state, struct transmap_t const *tmap);

void typestate_constraints_init(typestate_constraints_t *tscons);
void typestate_constraints_add(typestate_constraints_t *tscons, tscons_t *newcons);
void typestate_constraints_free(typestate_constraints_t *tscons);
void typestate_constraints_solve(typestate_t *out_initial, typestate_t *out_final,
    typestate_t const *in_initial, typestate_t const *in_final,
    typestate_constraints_t const *tscons);

char *typestate_elem_to_string(typestate_elem_t const *ts, char *buf, size_t size);
size_t typestate_elem_from_string(typestate_elem_t *ts, char const *buf);


//void typestate_from_stream(typestate_t *ts, FILE *fp);

char *typestate_to_string(typestate_t const *ts, char *buf, size_t size);

char *typestate_elem_array_to_string(typestate_elem_t const *in, long len, char *buf,
    size_t size);
size_t typestate_elem_array_from_string(typestate_elem_t *out, long *len, long size, char const *buf);

/*void src_insn_collect_typestate_constraints(typestate_constraints_t *tscons,
    struct src_insn_t const *insn, regset_t const *insn_live_out,
    struct state_t const *state, struct typestate_t *varstate);*/

void types_init(void);
size_t gen_typechecking_code(struct dst_insn_t *out, size_t out_size, dst_insn_t const *in,
    /*regset_t *ctemps, */size_t const num_regs_used[I386_NUM_EXREG_GROUPS],
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    use_types_t use_types);
void typestate_set_modified_to_bottom(typestate_t *dst, typestate_t const *src);

void ts_tab_entry_constraints_print(FILE *logfile,
    struct ts_tab_entry_cons_t const *cons);
void ts_tab_entry_transfers_print(FILE *logfile,
    struct ts_tab_entry_transfer_t const *trans);

void typestate_set_regs_to_bottom(typestate_t *dst, struct regset_t const *regs);
void typestate_set_temp_stack_slots_to_bottom(typestate_t *typestate, size_t num_temp_stack_slots);
void typestate_set_memslots_to_bottom(typestate_t *dst, memslot_set_t const *memslots);

bool is_identity_type_transfer(struct ts_tab_entry_transfer_t const *trans);
void typestate_constraints_print(FILE *logfile,
    struct typestate_constraints_t const *tscons);
void src_iseq_compute_typestate_using_transmaps(struct typestate_t *typestate_initial,
    struct typestate_t *typestate_final, struct src_insn_t const *src_iseq,
    long src_iseq_len,
    struct regset_t const *live_out, struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmaps, long num_live_out, bool mem_live_out,
    rand_states_t const &rand_states);

size_t typestate_from_string(typestate_t *ts, char const *buf);
//int get_index_size_type(int index, int size);
bool typestate_elem_arrays_are_untouched(typestate_elem_t const *arr_initial,
    typestate_elem_t const *arr_final, size_t size);

void typestate_canonicalize(typestate_t *ts_initial, typestate_t *ts_final,
    struct regmap_t *regmap);

void itable_prune_insns_using_min_type(struct itable_t *itab,
    typestate_elem_t const *mintype);

#define INDEX_POLYMORPHIC -1
#define SIZE_POLYMORPHIC -1
#define INDEX_ANY -2
#define SIZE_ANY -2

typedef struct ts_tab_entry_cons_t {
  state_elem_desc_t elem;
  //long max_type_len;
  typestate_elem_t max_type[MAX_NUM_TE_PER_ENTRY];
  struct ts_tab_entry_cons_t *next;
} ts_tab_entry_cons_t;

typedef struct ts_tab_entry_transfer_elem_t {
  bool state_elem;
  struct /*union*/ {
    state_elem_desc_t desc;
    typestate_elem_t type;
  } e;
} ts_tab_entry_transfer_elem_t;

typedef struct ts_tab_entry_transfer_t { //transfer from [0] to [1]
  state_elem_desc_t dst;
  ts_tab_entry_transfer_elem_t elems[MAX_NUM_TE_PER_ENTRY];
  int num_elems;
  //struct ts_tab_entry_transfer_t *must_happen_after;
  struct graph_node must_happen_after;
  struct ts_tab_entry_transfer_t *next;
} ts_tab_entry_transfer_t;

size_t typestate_from_file(typestate_t *typestate_initial, typestate_t *typestate_final,
    size_t num_states, char const *filename);
void typestates_meet(typestate_t *dst, typestate_t const *src, int n);
void typestates_set_modified_to_bottom(typestate_t *dst, typestate_t const *src, int n);
void typestates_convert_addr_datatype_to_intaddr_datatype(typestate_t *ts, int n);
void typestates_make_non_poly_copyable(typestate_t *ts, int n);
void typestates_set_poly_bytes_in_non_poly_regs_to_bottom(typestate_t *ts, int n);
void typestates_set_stack_regnum_to_bottom(typestate_t *ts_init, typestate_t *ts_fin, size_t num_states, fixed_reg_mapping_t const &fixed_reg_mapping);
void typestate_copy_to_type_env(typestate_t const *typestate, long addr);

#endif
