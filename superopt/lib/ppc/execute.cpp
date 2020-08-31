#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
//#include "cpu.h"
#include "support/debug.h"
#include "config-host.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/jumptable.h"
//#include "strtab.h"
//#include "gas/disas.h"

#include "valtag/elf/elf.h"

#include "rewrite/static_translate.h"
//#include "dbg_functions.h"

#include "support/c_utils.h"
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"

//#include "live_ranges.h"
#include "ppc/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"
//#include "ppc/regmap.h"

#include "rewrite/transmap_set.h"

#include "rewrite/edge_table.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"

#include "support/timers.h"
#include "ppc/execute.h"

static char as1[4096], as2[4096], as3[4096];

//#define JUMP_HANDLER_PEEP 1
//#define JUMP_HANDLER_QEMU 0

/*int
ppc_setup_executable_code(struct tb_t *tb, uint8_t const *code, int codesize)
{
  uint8_t *tc_end = tb->tc_ptr + tb->tc_size;
  int num_insns = codesize >> 2;
  src_regset_t live_regs[num_insns+1];
  uint16_t jmp_offsets[2];
  uint16_t jmp_insn_offsets[2];
  uint16_t edge_offsets[2];
  uint16_t edge_select_offsets[2];
  int i;
  uint16_t edge_select_offset = (uint16_t)-1, jmp_offset0 = (uint16_t)-1;
  transmap_t const *out_tmap;

  for (i = 0; i < num_insns + 1; i++) {
    src_regset_copy(&live_regs[i], src_regset_all());
  }
  edge_offsets[1] = (uint16_t)-1;
  tb->tb_jmp_insn_offset = jmp_insn_offsets;
  tb->tb_edge_offset = edge_offsets;
  tb->tb_edge_select_offset = edge_select_offsets;
  tb->tb_jmp_offset = jmp_offsets;
  tb->tb_edge_select_offset[0] = (uint16_t)-1;
  tb->tb_jmp_offset[1] = (uint16_t)-1;
  tb->num_successors = 2; // conservatively assume that there are 2 successors

  //env->singlestep_enabled = 1;

  save_callee_save_registers();

  // mov $0x20000000, %ebp
  *tb->tc_ptr++ = 0xbd; *(uint32_t *)tb->tc_ptr = SRC_ENV_ADDR; tb->tc_ptr+=4;

  for (i = 0; i < codesize>>2; i++) {
    target_ulong next_possible_eips[3];

    tb->pc = (uint32_t)code + i*4;
    tb->size = codesize - i*4;
    tb->live_regs = &live_regs[i];
    cpu_ppc_gen_code(&ti, tb, next_possible_eips);

    if (tb->tb_edge_select_offset[0] != (uint16_t)-1) {
      assert(edge_select_offset == (uint16_t)-1);
      edge_select_offset = tb->tb_edge_select_offset[0];
    }
    DBG(SRC_EXEC, "tb->pc=%x, opcode=%x, next_possible_eips=%x,%x,%x, "
        "edge_offsets=%x,%x, jmp_offsets=%x,%x, codesize=%x\n", tb->pc,
        ldl_input(tb->pc), next_possible_eips[0], next_possible_eips[1],
        next_possible_eips[2], tb->tb_edge_offset[0], tb->tb_edge_offset[1],
        tb->tb_jmp_offset[0], tb->tb_jmp_offset[1], tb->codesize);

    // qemu sets tb_jmp_offset[1] only if there was a jump 
    if (next_possible_eips[0] != tb->pc + tb->size) {
      assert(jmp_offset0 == (uint16_t)-1);
      jmp_offset0 = tb->tb_jmp_offset[0];
    }
    if (tb->tb_edge_offset[1] == (uint16_t)-1) {
      if (   next_possible_eips[0] == tb->pc + tb->size
          || next_possible_eips[1] == tb->pc + tb->size) {
        tb->tc_ptr = tb->tc_start + tb->tb_edge_offset[0];
      } else {
        // unconditional jump 
        tb->tc_ptr = tb->tc_start + tb->codesize;
      }
    } else {
      tb->tc_ptr = tb->tc_start + tb->tb_edge_offset[1];
    }
  }
  out_tmap = transmap_no_regs();

  add_jump_handling_code(JUMP_HANDLER_QEMU);
  tb->codesize = tb->tc_ptr - tb->tc_start;
  
  tb->pc = (uint32_t)code;
  tb->size = codesize;
  
  DBG_EXEC(SRC_EXEC,
      uint32_t opcode = ldl_input(tb->pc);
      fprintf(stdout, "IN:\n");
      snprint_insn_ppc(as1, sizeof as1, tb->pc, (uint8_t *)&opcode, tb->size, -1);
      fprintf(stdout, "%s", as1);
      fprintf(stdout, "OUT:\n");
      sprint_iseq_i386(tb->tc_start, tb->codesize, 0, as1, sizeof as1);
      fprintf(stdout, "%s", as1);
      fflush(stdout);
      );

  return 0;
}*/

#define gen_ppc_zero_EA() do { \
  /* li EA, 0 */ \
  uint32_t opcode = (0x38 << 24) | (EA_REG << 21) | 0x0; \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define gen_ppc_add_EA_reg(reg) do { \
  /* add EA, EA, reg */ \
  uint32_t opcode =   (0x7c << 24) | (EA_REG << 21) | (EA_REG << 16) \
                    | (reg << 11)  | (0x214); \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define gen_ppc_add_EA_simm(simm) do { \
  /* addi EA, EA, simm */ \
  uint32_t opcode =   (0x38 << 24) | (EA_REG << 21) | (EA_REG << 16) \
                    | (simm & 0xffff); \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define gen_ppc_save_EA() do { \
  /* mr savedEA_REG, EA_REG */ \
  uint32_t opcode =   (0x7c << 24) | (EA_REG << 21) | (savedEA_REG << 16) \
                    | (EA_REG << 11) | 0x378; \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define gen_ppc_mov_savedEA_reg(reg) do { \
  /* mr reg, savedEA_REG */ \
  uint32_t opcode =   (0x7c << 24) | (savedEA_REG << 21) | (reg << 16) \
                    | (savedEA_REG << 11) | 0x378; \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define MEM_SANDBOX_MASK (MEMORY_SIZE - 1)

#if 0
#define gen_ppc_sandbox_EA() do { \
  /* andi. EA, EA, MEM_SANDBOX_MASK */ \
  uint32_t opcode =   (0x70 << 24) | (EA_REG << 21) | (EA_REG << 16) \
                    | MEM_SANDBOX_MASK; \
  stl(&ppc_code[i], opcode); i+=4; \
  /* add EA, EA, MEM_BASE_REG */ \
  opcode =   (0x7c << 24) | (EA_REG << 21) | (EA_REG << 16) \
             | (MEM_BASE_REG << 11)  | (0x214); \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)
#endif

#if 0
  /* addis EA, EA, ((base >> 16) & 0xffff) */ \
  opcode =   (0x3c << 24) | (EA_REG << 21) | (EA_REG <<16) \
           | (( base >> 16) & 0xffff); \
  stl(&ppc_code[i], opcode); i+=4; \
  /* addi EA, EA, (base & 0xffff) */ \
  opcode =   (0x38 << 24) | (EA_REG << 21) | (EA_REG << 16) \
           | (base & 0xffff); \
	   */
#endif

#define gen_ppc_load_reg(areg, opc1, opc3, dreg) do { \
  uint32_t opcode; \
  if (opc1 == 0x1f) { \
    /* convert l[bdw]x to l[bdw] */ \
    opcode = (0x20 << 26) | opc3 << 26 | (dreg << 21) | (areg << 16) | 0x0; \
  } else { \
    /* use one of l[bdw] */ \
    opcode = (opc1 << 26) | (dreg << 21) | (areg << 16) | 0x0; \
  } \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

#define gen_ppc_store_reg(areg, opc1, opc3, sreg) do { \
  uint32_t opcode; \
  if (opc1 == 0x1f) { \
    /* convert st[bdw]x to l[bdw] */ \
    opcode = (0x90 << 24) | opc3 << 26 | (sreg << 21) | (areg << 16) | 0x0; \
  } else { \
    opcode = (opc1 << 26) | (sreg << 21) | (areg << 16) | 0x0; \
  } \
  stl(&ppc_code[i], opcode); i+=4; \
} while (0)

static void
gen_ppc_sandboxed_code(ppc_insn_t const *insn, uint8_t *ppc_code, int *cur)
{
  int i = *cur;
  int is_load = 0, is_store = 0, update_with_EA = 0;

  if (   insn->opc1 == 0x22    /* lbz */
      || insn->opc1 == 0x2a    /* lha */
      || insn->opc1 == 0x28    /* lhz */
      || insn->opc1 == 0x20) { /* lwz */
    is_load = 1;
    gen_ppc_zero_EA();
    if (insn->valtag.rA.val != 0) {
      ASSERT(insn->valtag.rA.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rA.val);
    }
    ASSERT(insn->valtag.imm.simm.tag == tag_const);
    gen_ppc_add_EA_simm(insn->valtag.imm.simm.val);
  }

  if (   insn->opc1 == 0x23    /* lbzu */
      || insn->opc1 == 0x2b    /* lhau */
      || insn->opc1 == 0x29    /* lhzu */
      || insn->opc1 == 0x21) { /* lwzu */
    is_load = 1;
    gen_ppc_zero_EA();
    ASSERT(insn->valtag.rA.tag == tag_const);
    gen_ppc_add_EA_reg(insn->valtag.rA.val);
    ASSERT(insn->valtag.imm.simm.tag == tag_const);
    gen_ppc_add_EA_simm(insn->valtag.imm.simm.val);
    gen_ppc_save_EA();
    update_with_EA = 1;
  }

  if (insn->opc1 == 0x1f && insn->opc2 == 0x17) {
    if (   insn->opc3 == 0x2    /* lbzx */
        || insn->opc3 == 0xa    /* lhax */
        || insn->opc3 == 0x8    /* lhzx */
        || insn->opc3 == 0x0) { /* lwzx */
      is_load = 1;
      gen_ppc_zero_EA();
      if (insn->valtag.rA.val != 0) {
        ASSERT(insn->valtag.rA.tag == tag_const);
        gen_ppc_add_EA_reg(insn->valtag.rA.val);
      }
      ASSERT(insn->valtag.rB.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rB.val);
    }
    if (   insn->opc3 == 0x3    /* lbzux */
        || insn->opc3 == 0xb    /* lhaux */
        || insn->opc3 == 0x9    /* lhzux */
        || insn->opc3 == 0x1) { /* lwzux */
      is_load = 1;
      gen_ppc_zero_EA();
      ASSERT(insn->valtag.rA.tag == tag_const);
      ASSERT(insn->valtag.rB.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rA.val);
      gen_ppc_add_EA_reg(insn->valtag.rB.val);
      gen_ppc_save_EA();
      update_with_EA = 1;
    }
  }

  if (   insn->opc1 == 0x26    /* stb */
      || insn->opc1 == 0x2c    /* sth */
      || insn->opc1 == 0x24) { /* stw */
    is_store = 1;
    gen_ppc_zero_EA();
    if (insn->valtag.rA.val != 0) {
      ASSERT(insn->valtag.rA.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rA.val);
    }
    ASSERT(insn->valtag.imm.simm.tag == tag_const);
    gen_ppc_add_EA_simm(insn->valtag.imm.simm.val);
  }
  if (   insn->opc1 == 0x27    /* stbu */
      || insn->opc1 == 0x2d    /* sthu */
      || insn->opc1 == 0x25) { /* stwu */
    is_store = 1;
    gen_ppc_zero_EA();
    ASSERT(insn->valtag.rA.tag == tag_const);
    gen_ppc_add_EA_reg(insn->valtag.rA.val);
    ASSERT(insn->valtag.imm.simm.tag == tag_const);
    gen_ppc_add_EA_simm(insn->valtag.imm.simm.val);
    gen_ppc_save_EA();
    update_with_EA = 1;
  }

  if (insn->opc1 == 0x1f && insn->opc2 == 0x17) {
    if (   insn->opc3 == 0x6    /* stbx */
        || insn->opc3 == 0xc    /* sthx */
        || insn->opc3 == 0x4) { /* stwx */
      is_store = 1;
      gen_ppc_zero_EA();
      if (insn->valtag.rA.val != 0) {
        ASSERT(insn->valtag.rA.tag == tag_const);
        gen_ppc_add_EA_reg(insn->valtag.rA.val);
      }
      ASSERT(insn->valtag.rB.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rB.val);
      
    }
    if (   insn->opc3 == 0x7    /* stbux */
        || insn->opc3 == 0xd    /* sthux */
        || insn->opc3 == 0x5) { /* stwux */
      is_store = 1;
      gen_ppc_zero_EA();
      ASSERT(insn->valtag.rA.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rA.val);
      ASSERT(insn->valtag.rB.tag == tag_const);
      gen_ppc_add_EA_reg(insn->valtag.rB.val);
      gen_ppc_save_EA();
      update_with_EA = 1;
    }
  }

  if (is_load) {
    NOT_IMPLEMENTED();
    //gen_ppc_sandbox_EA(); //XXX
    //ASSERT(insn->valtag.rSD.rD.tag == tag_const);
    //gen_ppc_load_reg(EA_REG, insn->opc1, insn->opc3, insn->valtag.rSD.rD.val);
  } else if (is_store) {
    NOT_IMPLEMENTED();
    //gen_ppc_sandbox_EA(); //XXX
    //ASSERT(insn->valtag.rSD.rS.tag == tag_const);
    //gen_ppc_store_reg(EA_REG, insn->opc1, insn->opc3, insn->valtag.rSD.rS.val);
  }
  
  if (is_load || is_store) {
    if (update_with_EA) {
      ASSERT(insn->valtag.rA.tag == tag_const);
      gen_ppc_mov_savedEA_reg(insn->valtag.rA.val);
    }
  } else {
    ppc_insn_assemble(insn, &ppc_code[i], 4);
    i+=4;
  }
  *cur = i;
}

static void
ppc_generate_code(ppc_insn_t const *iseq, int ppc_iseq_len, imm_vt_map_t const *imm_map,
    uint8_t *ppc_code, int *ppc_codesize, uint16_t *jmp_offset0)
{
  NOT_IMPLEMENTED();
/*
  int max_ppc_codesize, i;
	ppc_insn_t insn;

  ASSERT(jmp_offset0);
  ASSERT(ppc_code);
  ASSERT(ppc_codesize);

  *jmp_offset0 = (uint16_t)-1;
  max_ppc_codesize = *ppc_codesize;
  *ppc_codesize = 0;
  for (i = 0; i < ppc_iseq_len; i++) {
    ppc_insn_init(&insn);
    ppc_insn_copy(&insn, &iseq[i]);
    ppc_insn_rename_constants(&insn, imm_map, NULL// XXX //, 0, ppc_v2c_regmap(), NULL);
    gen_ppc_sandboxed_code(&insn, ppc_code, ppc_codesize);
    assert(*ppc_codesize <= max_ppc_codesize);
  }
*/
}

/*void
ppc_state_rename_using_regmap(struct ppc_state_t *state,
    src_regmap_t const *regmap)
{
  struct ppc_state_t in_state;
  int i;
  memcpy(&in_state.regs, state->regs, sizeof(in_state.regs));
  for (i = 0; i < PPC_NUM_REGS; i++) {
    if (regmap->table[i] != -1) {
      ASSERT(regmap->table[i] >= 0 && regmap->table[i] < PPC_NUM_REGS);
      state->regs[regmap->table[i]] = in_state.regs[i];
    }
  }
}

void
ppc_state_inv_rename_using_regmap(struct ppc_state_t *state,
    struct src_regmap_t const *regmap)
{
  struct ppc_state_t in_state;
  int i;
  memcpy(&in_state.regs, state->regs, sizeof(in_state.regs));
  for (i = 0; i < PPC_NUM_REGS; i++) {
    if (regmap->table[i] != -1) {
      ASSERT(regmap->table[i] >= 0 && regmap->table[i] < PPC_NUM_REGS);
      state->regs[i] = in_state.regs[regmap->table[i]];
    }
  }
}*/
