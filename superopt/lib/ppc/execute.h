#ifndef __PPC_EXECUTE_H
#define __PPC_EXECUTE_H
#include <stdint.h>
#include "ppc/regs.h"
//#include "imm_map.h"
#include "exec/state.h"
#include "equiv/equiv.h"

struct regset_t;

//#if SRC == ARCH_PPC
#define EA_REG 27
//#define MEM_BASE_REG 28     /* the register where memory base addr is stored */
#define savedEA_REG 29

//struct CPUPPCState;
//#endif
struct transmap_t;

int init_execution_environment(void);

/*long
ppc_setup_executable_code(uint8_t *exec_code, size_t exec_codesize, uint8_t const *code,
    size_t codesize, uint16_t jmp_offset0, struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmap, long num_live_out);*/

/*struct CPUPPCState;
int
ppc_execute(struct CPUPPCState *env, ppc_state_t const *state,
            uint8_t const *code, int codesize, ppc_state_t *outstate);

int
x86_execute(struct CPUPPCState *env, ppc_state_t const *state,
            uint8_t const *code, int codesize, uint16_t jmp_offset0,
            uint16_t edge_select_offset,
            struct transmap_t const *in_tmap, struct transmap_t const *out_tmap,
            ppc_state_t *outstate);

int
ppc_states_equivalent(ppc_state_t const *state1,
    		      ppc_state_t const *state2,
    		      struct regset_t const *live_regs0,
              struct regset_t const *live_regs1);

char *ppc_state_to_string(ppc_state_t const *state, char *buf, int size);
void ppc_state_rename_using_regmap(struct ppc_state_t *state,
    struct src_regmap_t const *regmap);
void ppc_state_inv_rename_using_regmap(struct ppc_state_t *state,
    struct src_regmap_t const *regmap);*/
long ppc_setup_executable_code(uint8_t *exec_code, size_t exec_codesize, uint8_t const *code,
    size_t codesize, uint16_t jmp_offset0, struct transmap_t const *in_tmap,
    struct transmap_t const *out_tmap, long num_live_out);

#endif
