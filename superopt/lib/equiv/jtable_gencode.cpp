/* (c) Sorav Bansal. */
#include <map>
#include "equiv/jtable_gencode.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
//#include "lib/macros.h"
//#include "lib/utils.h"
#include "cutils/chash.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "support/debug.h"
#include "support/timers.h"
//#include "peep.h"
#include "i386/insntypes.h"
//#include "peeptab.h"
//#include "assignments.h"
#include "i386/disas.h"
#include "i386/opctable.h"
//#include "peep/jumptable1.h"
//#include "cpu_constraints.h"
//#include "peeptab_defs.h"
#include "valtag/regset.h"
#include "valtag/regmap.h"
#include "equiv/jtable.h"
#include "rewrite/dst-insn.h"
//#include "sys/vcpu.h"
//#include "bt_vcpu.h"
#include "config-host.h"
#include "support/c_utils.h"
#include "support/cmd_helper.h"
#include "valtag/memslot_map.h"

typedef enum { REGMAP_NAME_IREGMAP, REGMAP_NAME_JTABLE_REGMAP, REGMAP_NAME_NONE } regmap_name_t;

#define BINBUF_SIZE 409600
static char as1[409600];

static bool
is_identical_byte(uint8_t const * const *code, size_t ncodes, size_t bytenum)
{
  bool know_byteval = false;
  uint8_t byteval;
  size_t i;

  for (i = 0; i < ncodes; i++) {
    if (code[i]) {
      if (!know_byteval) {
        byteval = code[i][bytenum];
        know_byteval = true;
        continue;
      }
      ASSERT(know_byteval);
      if (code[i][bytenum] != byteval) {
        return false;
      }
    }
  }
  return true;
}

static size_t
output_regbit_code(char *buf, size_t buf_size, uint8_t const * const *binbufs,
    long num_binbufs, size_t binsize, size_t cur_offset, int bitnum, bool set)
{
  uint8_t _xor[binsize];
  unsigned int num_regbits;
  char *ptr = buf, *end = buf + buf_size;
  unsigned b;
  bool ret;
  int i;

  ret = is_power_of_two(num_binbufs , &num_regbits);
  ASSERT(ret);
  ASSERT((1 << num_regbits) == num_binbufs);
  ASSERT(bitnum >= 0 && bitnum < num_regbits);
  i = bitnum;
  for (b = 0; b < binsize; b++) {
    uint8_t const *code_set = NULL, *code_reset = NULL;
    int j;

    if (is_identical_byte(binbufs, num_binbufs, b)) {
      continue;
    }

    if (binbufs[0] && binbufs[1 << i]) {
      code_set = binbufs[1 << i];
      code_reset = binbufs[0];
    } else {
      for (j = 0; j < num_regbits; j++) {
        if (j != i && binbufs[1 << j] && binbufs[(1 << j) + (1 << i)]) {
          code_set = binbufs[(1 << j) + (1 << i)];
          code_reset = binbufs[1 << j];
          break;
        }
      }
      if (j == num_regbits) {
        /*printf("Warning: Code reset/set pair not found for bitnum %d. \n"
              "You should not be calling this function for this bitnum \n"
              "if you have a corresponding regcons in the rule.\n", i);*/
        //NOT_REACHED();
        return 0;
      }
    }
    bool set_greater_than_reset;
    ASSERT(code_set);       ASSERT(code_reset);
    if (code_set[b] >= code_reset[b]) {
      set_greater_than_reset = true;
    } else {
      set_greater_than_reset = false;
    }
    _xor[b] = code_set[b] ^ code_reset[b];
    //ASSERT((_xor[b] | code_set[b]) == code_set[b]);
    //ASSERT((_xor[b] | code_reset[b]) == code_set[b]);

    if (_xor[b]) {
      if (   (set && set_greater_than_reset)
          || (!set && !set_greater_than_reset)) {
        ptr += snprintf(ptr, end - ptr, "    *(bincode + %zd) |= 0x%hhx;\n", cur_offset + b, _xor[b]);
      } else {
        ptr += snprintf(ptr, end - ptr, "    *(bincode + %zd) &= ~0x%hhx;\n", cur_offset + b, _xor[b]);
      }
    }
  }
  ASSERT(ptr < end);
  return ptr - buf;
}

static void
output_regvar_code(FILE *fp, char const *regname, char const *limit,
    uint8_t const * const *binbufs,
    long num_binbufs, size_t binlen, long group, long reg,
    size_t cur_offset)
{
  int num_exregs, i;
  unsigned num_regbits;
  size_t bufsize = 409600;
  char *buf = new char[bufsize];
  bool ret;

  num_exregs = (group == I386_EXREG_GROUP_SEGS)?1:I386_NUM_EXREGS(group);
  ret = is_power_of_two(num_exregs, &num_regbits);
  ASSERT(ret);

  for (i = 0; i < num_regbits; i++) {
    ret = output_regbit_code(buf, bufsize, binbufs, num_binbufs, binlen, cur_offset, i, true);
    if (!ret) {
      continue;
    }

    fprintf(fp, "ASSERT(%s >= 0 && %s < %s);\n", regname, regname, limit);
    fprintf(fp, "if (%s & (1 << %d)) {\n", regname, i);
    fprintf(fp, "%s", buf);
    fprintf(fp, "} else {\n");
    ret = output_regbit_code(buf, bufsize, binbufs, num_binbufs, binlen, cur_offset, i, false);
    fprintf(fp, "%s", buf);
    fprintf(fp, "}\n");
  }
  delete [] buf;
}

static size_t
get_majority_non_null_element(size_t const *a, size_t alen)
{
  size_t value[alen], count[alen];
  size_t i, n, max, max_elem;
  int found;

  n = 0;
  for (i = 0; i < alen; i++) {
    if (a[i] == 0) {
      continue;
    }
    found = array_search(value, n, a[i]);
    if (found >= 0) {
      ASSERT(found < n);
      ASSERT(value[found] == a[i]);
      count[found]++;
      continue;
    }
    ASSERT(found == -1);
    value[n] = a[i];
    count[n] = 1;
    n++;
  }
  max = 0;
  max_elem = 0;
  for (i = 0; i < n; i++) {
    if (max < count[i]) {
      max = count[i];
      max_elem = value[i];
    }
  }
  return max_elem;
}

#if ARCH_DST == ARCH_I386
static void
i386_iseq_expand_constants_to_max_width(i386_insn_t *exec_iseq_const,
    i386_insn_t const *exec_iseq, size_t exec_iseq_len)
{
  operand_t const *orig_op;
  int i, j, max_op_size;
  operand_t *op;

  for (i = 0; i < exec_iseq_len; i++) {
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      op = &exec_iseq_const[i].op[j];
      orig_op = &exec_iseq[i].op[j];
      max_op_size = i386_max_operand_size(op, exec_iseq_const[i].opc, j - exec_iseq_const[i].num_implicit_ops);
      if (op->type == op_imm) {
        ASSERT(op->valtag.imm.tag == tag_const);
        ASSERT(orig_op->type == op_imm);
        if (orig_op->valtag.imm.tag == tag_var) {
          op->valtag.imm.val = 0x10101010 & ((1ULL << (max_op_size * BYTE_LEN)) - 1);
        }
      } else if (op->type == op_mem) {
        ASSERT(op->valtag.mem.disp.tag == tag_const);
        ASSERT(orig_op->type == op_mem);
        if (orig_op->valtag.mem.disp.tag == tag_var) {
          op->valtag.mem.disp.val = 0x10101010;
        }
      }
    }
  }
}

static size_t
i386_assemble_exec_iseq_using_regmap(uint8_t *buf, size_t buf_size,
    i386_insn_t const *exec_iseq, long exec_iseq_len, regmap_t const *dst_regmap,
    regmap_t const *src_regmap/*, int d*/, memslot_map_t const &memslot_map)
{
  i386_insn_t *exec_iseq_const = new i386_insn_t[exec_iseq_len];
  static imm_vt_map_t const_imm_map;
  static bool init = false;
  size_t ret;

  if (!init) {
    int i;
    imm_vt_map_init(&const_imm_map);
    for (i = 0; i < NUM_CANON_CONSTANTS; i++) {
      const_imm_map.table[i].tag = tag_const;
      const_imm_map.table[i].val = 0x10101010 + i;
    }
    const_imm_map.num_imms_used = NUM_CANON_CONSTANTS;
    init = true;
  }

  stopwatch_run(&i386_assemble_exec_iseq_using_regmap_timer);

  i386_iseq_copy(exec_iseq_const, exec_iseq, exec_iseq_len);
  i386_iseq_rename(exec_iseq_const, exec_iseq_len, dst_regmap, //use_regmap?&iregmap:&regmap,
      &const_imm_map, &memslot_map, src_regmap, NULL,
      //(!use_regmap && (d == JTABLE_EXEC_ISEQ_TYPE))?&iregmap:NULL,
      NULL, 0);
  //expand constants to max width to ensure regular assembly
  i386_iseq_expand_constants_to_max_width(exec_iseq_const, exec_iseq, exec_iseq_len);

  DBG3(JTABLE, "dst_regmap:\n%s\n",
      dst_regmap?regmap_to_string(dst_regmap, as1, sizeof as1):NULL);
  DBG3(JTABLE, "src_regmap:\n%s\n",
      src_regmap?regmap_to_string(src_regmap, as1, sizeof as1):NULL);
  DBG3(JTABLE, "exec_iseq_const:\n%s\n",
      i386_iseq_to_string(exec_iseq_const, exec_iseq_len, as1, sizeof as1));

  ret = i386_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, buf, buf_size,
      I386_AS_MODE_64_REGULAR);
  if (!(ret >= 0 && ret < buf_size)) {
    DBG(TMP, "exec_iseq_const:\n%s\n",
        i386_iseq_to_string(exec_iseq_const, exec_iseq_len, as1, sizeof as1));
  }
  ASSERT(ret >= 0 && ret < buf_size);
  stopwatch_stop(&i386_assemble_exec_iseq_using_regmap_timer);

  delete [] exec_iseq_const;
  return ret;
}
#elif ARCH_DST == ARCH_PPC
static size_t
ppc_assemble_exec_iseq_using_regmap(uint8_t *buf, size_t buf_size,
    ppc_insn_t const *exec_iseq, long exec_iseq_len, regmap_t const *dst_regmap,
    regmap_t const *src_regmap, int d)
{
  NOT_IMPLEMENTED();
}
#else
#error "unsupported dst arch"
#endif

static void
emit_code(FILE *ofp, uint8_t const *binbuf, size_t binlen/*, size_t cur_offset*/)
{
  fprintf(ofp, "uint8_t code[] = { %s };\nmemcpy(bincode" /*" + %zd"*/", code, %ld);\n",
      hex_string(binbuf, binlen, as1, sizeof as1)/*, cur_offset*/, binlen);
  fprintf(ofp, "binlen = %ld;\n", binlen);
}

static size_t
gen_code_for_exec_iseq(dst_insn_t const *exec_iseq, size_t exec_iseq_len, regcons_t const &regcons, fixed_reg_mapping_t const &fixed_reg_mapping, uint8_t *buf, size_t buf_size)
{
  regmap_t regmap;
  regmap_init(&regmap);
  bool ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_I386, NULL, fixed_reg_mapping);
  ASSERT(ret);
  bool alloced_binbuf = false;
  if (!buf) {
    buf = new uint8_t[BINBUF_SIZE];
    ASSERT(buf);
    buf_size = BINBUF_SIZE;
    alloced_binbuf = true;
  }

  size_t binlen = dst_assemble_exec_iseq_using_regmap(buf, buf_size,
      exec_iseq, exec_iseq_len, &regmap,
      /*use_regmap?NULL:*/&regmap, *memslot_map_t::memslot_map_for_src_env_state());
  ASSERT(binlen >= 0 && binlen < buf_size);
  if (alloced_binbuf) {
    delete [] buf;
  }
  return binlen;
}

static size_t
gen_code_for_exec_iseq(uint8_t *binbuf, size_t binbuf_size, dst_insn_t const *exec_iseq, size_t exec_iseq_len, regcons_t const &regcons, fixed_reg_mapping_t const &fixed_reg_mapping, long const insn_num_map[], long const insn_num_map_type[], long const insn_num_map_restore[], long const insn_num_map_data[], size_t iseq_len, regset_t const jtable_entry_temps[JTABLE_NUM_EXEC_ISEQS][ITABLE_ENTRY_ISEQ_MAX_LEN], long *bincode_map, long *exec_bincode_map)
{
  uint8_t *binptr = binbuf;
  uint8_t *binend = binbuf + binbuf_size;

  regcons_t regcons_all;
  regcons_set(&regcons_all);

  for (size_t i = 0; i < iseq_len; i++) {
    size_t n_index = insn_num_map[i];
    size_t t_index = insn_num_map_type[i];
    size_t r_index = insn_num_map_restore[i];
    size_t d_index = insn_num_map_data[i];
    size_t dindex = insn_num_map_data[i];
    size_t next_n_index = insn_num_map[i + 1];
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", binlen = " << (binptr - binbuf) << endl;
    bincode_map[i] = binptr - binbuf;
    regcons_t regcons_type;
    regcons_copy(&regcons_type, &regcons_all);
    regcons_unmark_dst_reg_for_regset(&regcons_type, jtable_entry_temps[JTABLE_EXEC_ISEQ_TYPE][i], I386_EXREG_GROUP_GPRS, R_ESP); //do not allow tmp regs to map to R_ESP; for typechecking code, we can have temporary registers that we do not want to map to R_ESP, but for other registers we are okay because it will help rename src-reg usages of the for C_EXVREG<n>

    //binptr += gen_code_for_exec_iseq(exec_iseq, exec_iseq_len, regcons, fixed_reg_mapping, binbuf, BINBUF_SIZE);
    binptr += gen_code_for_exec_iseq(&exec_iseq[n_index], t_index - n_index, regcons, fixed_reg_mapping, binptr, binend - binptr);
    binptr += gen_code_for_exec_iseq(&exec_iseq[t_index], r_index - t_index, regcons_type, fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end_var_to_var(), binptr, binend - binptr);
    binptr += gen_code_for_exec_iseq(&exec_iseq[r_index], d_index - r_index, regcons, fixed_reg_mapping, binptr, binend - binptr);
    binptr += gen_code_for_exec_iseq(&exec_iseq[d_index], next_n_index - d_index, regcons, fixed_reg_mapping, binptr, binend - binptr);
    exec_bincode_map[next_n_index] = binptr - binbuf; //exec_bincode_map entries are only defined for next_n_index indices
  }
  bincode_map[iseq_len] = binptr - binbuf;
  exec_bincode_map[exec_iseq_len] = binptr - binbuf;
  return binptr - binbuf;
}

static size_t
emit_code_for_exec_iseq_and_also_infer_pcrels(FILE *ofp, dst_insn_t *exec_iseq, size_t exec_iseq_len, regcons_t const &regcons, fixed_reg_mapping_t const &fixed_reg_mapping, long const insn_num_map[], long const insn_num_map_type[], long const insn_num_map_restore[], long const insn_num_map_data[], size_t iseq_len, regset_t const jtable_entry_temps[JTABLE_NUM_EXEC_ISEQS][ITABLE_ENTRY_ISEQ_MAX_LEN])
{
  uint8_t *binbuf, *binptr, *binend;
  binbuf = new uint8_t[BINBUF_SIZE];
  ASSERT(binbuf);

  long bincode_map[iseq_len + 1];
  long exec_bincode_map[exec_iseq_len + 1];
  for (size_t i = 0; i < exec_iseq_len + 1; i++) {
    exec_bincode_map[i] = -1;
  }
  size_t binlen;
  for (size_t iter = 0; iter < 2; iter++) { //do this twice, so by the second iteration, we would have renamed the abs_inums using bincode_map
    binlen = gen_code_for_exec_iseq(binbuf, BINBUF_SIZE, exec_iseq, exec_iseq_len, regcons, fixed_reg_mapping, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, iseq_len, jtable_entry_temps, bincode_map, exec_bincode_map);
    if (iter == 0) {
      //cout << __func__ << " " << __LINE__ << ": before renaming sboxed abs inums, exec_iseq =\n" << dst_iseq_to_string(exec_iseq, exec_iseq_len, as1, sizeof as1) << endl;
      dst_iseq_rename_sboxed_abs_inums_using_map(exec_iseq, exec_iseq_len, bincode_map, iseq_len + 1); //renames abs_inums to their actual bin offsets
      //cout << __func__ << " " << __LINE__ << ": after renaming sboxed abs inums, exec_iseq =\n" << dst_iseq_to_string(exec_iseq, exec_iseq_len, as1, sizeof as1) << endl;
      dst_iseq_convert_sboxed_abs_inums_to_binpcrel_inums(exec_iseq, exec_iseq_len, exec_bincode_map); //convert them to pcrels using bincode_map as pc_map
      //cout << __func__ << " " << __LINE__ << ": after converting sboxed abs inums to pcrel inums, exec_iseq =\n" << dst_iseq_to_string(exec_iseq, exec_iseq_len, as1, sizeof as1) << endl;
    }
  }

  ASSERT(ofp);
  emit_code(ofp, binbuf, binlen);
  delete [] binbuf;
  return binlen;
}



static size_t
jtable_entry_output_fixup_code_for_regs_using_regmap_or_sequence(FILE *ofp,
    dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop,
    regcons_t const &regcons, regset_t const &temps, int d,
    size_t inum, fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    regmap_name_t regmap_name, size_t cur_offset)
{
  long i, j, g, binlen, prev_binlen;
  regmap_t regmap, iregmap;
  regset_t use, def, touch;
  uint8_t **binbufs;
  //bool emitted_code;
  //size_t *binlens;
  long num_exregs;
  bool ret;

  dst_insn_t const *exec_iseq = exec_iseq_start;
  size_t exec_iseq_len = exec_iseq_stop - exec_iseq_start;

  binlen = gen_code_for_exec_iseq(exec_iseq, exec_iseq_len, regcons, dst_fixed_reg_mapping, NULL, 0);

  //ASSERT(e->exec_iseq[d][inum]);
  //regset_copy(&touch, &e->use);
  //regset_union(&touch, &e->def);

  //dst64_iseq_get_usedef_regs(e->exec_iseq[d][inum], e->exec_iseq_len[d][inum], &use, &def);
  dst64_iseq_get_usedef_regs(exec_iseq, exec_iseq_len, &use, &def);
  regset_copy(&touch, &use);
  regset_union(&touch, &def);

  dst_iseq_get_used_vconst_regs(exec_iseq, exec_iseq_len, &use);
  regset_union(&touch, &use);

  //emitted_code = false;
  fprintf(ofp, "/*dst_iseq[%zd]:\n%s", inum, dst_iseq_to_string(exec_iseq, exec_iseq_len, as1, sizeof as1));
  fprintf(ofp, "touch: %s*/\n", regset_to_string(&touch, as1, sizeof as1));
  for (g = 0; g < I386_NUM_EXREG_GROUPS; g++) {
    num_exregs = (g == I386_EXREG_GROUP_SEGS)?1:I386_NUM_EXREGS(g); //regset_count_exregs(&touch, g);
    for (i = 0; i < num_exregs; i++) {
      if (!regset_belongs_ex(&touch, g, i)) {
        fprintf(ofp, "//ignoring exreg[%ld][%ld] because it does not belong to touch;\n",
            g, i);
        continue;
      }
      if (regset_belongs_ex(&temps, g, i)) {
        fprintf(ofp, "//ignoring exreg[%ld][%ld] because it belongs to temps;\n",
            g, i);
        continue;
      }
      if (dst_fixed_reg_mapping.var_reg_maps_to_fixed_reg(g, i)) {
        fprintf(ofp, "//ignoring exreg[%ld][%ld] because it represents fixed reg;\n",
            g, i);
        continue;
      }
      stopwatch_run(&jtable_entry_output_fixup_code_for_non_temp_regs_timer);
      binbufs = (uint8_t **)malloc(I386_NUM_EXREGS(g) * sizeof(uint8_t *));
      ASSERT(binbufs);
      //binlens = (size_t *)malloc(I386_NUM_EXREGS(g) * sizeof(size_t));
      //ASSERT(binlens);

      regmap_init(&regmap);
      ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_I386, NULL, dst_fixed_reg_mapping);
      ASSERT(ret);

      stopwatch_run(&jtable_entry_output_fixup_code_gen_binbufs_timer);
      prev_binlen = -1;
      for (j = 0; j < I386_NUM_EXREGS(g); j++) {
        regmap_t src_regmap;
        regmap_copy(&iregmap, &regmap);

        //iregmap.regmap_extable[g][i] = j;
        regmap_set_elem(&iregmap, g, i, j);
        regmap_copy(&src_regmap, &iregmap);
        src_regmap.regmap_add_fixed_reg_mapping(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end_var_to_var()); //ensure that the fixed regs are not renamed in src_regmap (which is used for d == 1, i.e., !use_regmap)

        binbufs[j] = NULL;
        //binlens[j] = 0;

        if (d == 0 && !regmap_satisfies_regcons(&iregmap, &regcons)) {
          continue;
        }
        //regcons_unmark_dst_reg(&regcons, g, j);
        //ASSERT(regmap.extable[g][i] == j);
        binbufs[j] = (uint8_t *)malloc(BINBUF_SIZE * sizeof(uint8_t));
        ASSERT(binbufs[j]);


        DBG4(JTABLE, "group %d, reg %d, variant %d, iregmap:\n%s\n", (int)g, (int)i,
            (int)j, regmap_to_string(&iregmap, as1, sizeof as1));
        DBG2(JTABLE, "exec_iseq:\n%s\n",
            dst_iseq_to_string(exec_iseq, exec_iseq_len, as1, sizeof as1));

        binlen = dst_assemble_exec_iseq_using_regmap(binbufs[j], BINBUF_SIZE,
            exec_iseq, exec_iseq_len, (regmap_name == REGMAP_NAME_NONE) ? &regmap : &iregmap,
            (regmap_name == REGMAP_NAME_NONE)?&src_regmap:NULL/*, d*/, *memslot_map_t::memslot_map_for_src_env_state());
            
        ASSERT(binlen >= 0);
        ASSERT(prev_binlen == -1 || binlen == prev_binlen);
        prev_binlen = binlen;
      }
      stopwatch_stop(&jtable_entry_output_fixup_code_gen_binbufs_timer);

      //binlen = get_majority_non_null_element(binlens, I386_NUM_EXREGS(g));

      for (j = 0; j < I386_NUM_EXREGS(g); j++) {
        if (!binbufs[j]) {
          continue;
        }
        /*DBG2(JTABLE, "binbufs[%ld]:\n%s\n", (long)binlens[j],
            hex_string(binbufs[j], binlens[j], as1, sizeof as1));*/
        /*if (binlen == -1) {
          binlen = binlens[j];
        }
        ASSERT(binlens[j] == binlen);*/
        //printf("binlens[%ld] = %zd, binlen = %zd\n", j, binlens[j], binlen);
        /*if (binlens[j] != binlen) {
          free(binbufs[j]);
          binlens[j] = 0;
          binbufs[j] = NULL;
          continue;
        }*/
        //if (!emitted_code) {
        //  emit_code(ofp, binbufs[j], binlen, cur_offset);
        //  emitted_code = true;
        //}
      }
      //ASSERT(binlen != -1);

      char regname[256], limit[64];
      if (regmap_name == REGMAP_NAME_IREGMAP) {
        //fprintf(ofp, "if (iregmap_extable.count(%ld) == 0) { printf(\"%%s() %%d: iregmap_extable.count(%ld) == 0\\n\", __func__, __LINE__); NOT_REACHED(); }\n", (long)g, (long)g);
        //fprintf(ofp, "if (iregmap_extable.at(%ld).count(%ld) == 0) { printf(\"%%s() %%d: error: iregmap_extable.count(%ld).at(%ld) == 0\\n\", __func__, __LINE__); NOT_REACHED(); }\n", (long)g, (long)i, (long)g, (long)i);
        //snprintf(regname, sizeof regname, "regmap_extable.at(%ld).at(exreg_sequence[%ld][%ld]).reg_identifier_get_id()", (long)g, (long)g, (long)i);
        snprintf(regname, sizeof regname, "iregmap_extable.at(%ld).at(%ld).reg_identifier_get_id()", (long)g, (long)i);
        snprintf(limit, sizeof limit, "%d", I386_NUM_EXREGS(g));
        //fprintf(ofp, "ASSERT(%s >= 0 && %s < %d);\n", regname, regname,
        //    I386_NUM_EXREGS(group));
      } else if (regmap_name == REGMAP_NAME_JTABLE_REGMAP) {
        snprintf(regname, sizeof regname, "jtable_regmap_extable.at(%ld).at(%ld).reg_identifier_get_id()", (long)g, (long)i);
        snprintf(limit, sizeof limit, "%d", I386_NUM_EXREGS(g));
      } else {
        ASSERT(regmap_name == REGMAP_NAME_NONE);
        snprintf(regname, sizeof regname, "exreg_sequence[%ld][%ld]", (long)g, (long)i);
        //snprintf(limit, sizeof limit, "exreg_sequence_len[%ld]", (long)g);
        snprintf(limit, sizeof limit, "%d", I386_NUM_EXREGS(g));
        //fprintf(ofp, "ASSERT(%s >= 0 && %s < exreg_sequence_len[%ld]);\n",
        //    regname, regname, group, reg);
      }

      stopwatch_run(&jtable_entry_output_regvar_code_timer);
      fprintf(ofp, "//regvar code for exreg[%ld][%ld]\n", g, i);
      fprintf(ofp, "//");
      for (j = 0; j < I386_NUM_EXREGS(g); j++) {
        fprintf(ofp, ",binbufs[%ld]=%p", j, binbufs[j]);
      }
      fprintf(ofp, "\n");
      output_regvar_code(ofp, regname, limit, (uint8_t const **)binbufs, I386_NUM_EXREGS(g),
          binlen, g, i, cur_offset);
      stopwatch_stop(&jtable_entry_output_regvar_code_timer);

      for (j = 0; j < I386_NUM_EXREGS(g); j++) {
        if (binbufs[j]) {
          free(binbufs[j]);
          binbufs[j] = NULL;
        }
      }
      free(binbufs);
      //free(binlens);
      binbufs = NULL;
      stopwatch_stop(&jtable_entry_output_fixup_code_for_non_temp_regs_timer);
    }
  }
  //if (!emitted_code) {
  //  uint8_t *binbuf;

  //  //ASSERT(d == JTABLE_EXEC_ISEQ_TYPE);
  //  regmap_init(&regmap);
  //  ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_I386, NULL);
  //  ASSERT(ret);
  //  regmap_copy(&iregmap, &regmap);
  //  //ASSERT(use_regmap);
  //  binbuf = (uint8_t *)malloc(BINBUF_SIZE * sizeof(uint8_t));
  //  ASSERT(binbuf);
  //  binlen = dst_assemble_exec_iseq_using_regmap(binbuf, BINBUF_SIZE,
  //      exec_iseq, exec_iseq_len, &regmap,
  //      use_regmap?NULL:&iregmap/*, d*/);

  //  emit_code(ofp, binbuf, binlen, cur_offset);
  //  emitted_code = true;
  //  free(binbuf);
  //}
  //ASSERT(emitted_code);
  //cout << __func__ << " " << __LINE__ << ": returning " << binlen << endl;
  return binlen;
}

static size_t
jtable_gen_save_restore_vregs_insns(regcons_t const &regcons, regset_t const &use, regset_t const &def, regset_t const &temps, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs, dst_insn_t *buf, size_t size, bool is_save)
{
  //regmap_t regmap;
  //bool ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_I386, NULL);
  //ASSERT(ret);
  transmap_t tmap_no_regs, tmap_id;
  tmap_id = dst_transmap_default(dst_fixed_reg_mapping);
  transmap_intersect_with_regset(&tmap_id, &itable_dst_relevant_regs);
  transmap_init_env_dst(&tmap_no_regs, SRC_ENV_ADDR);
  //cout << __func__ << " " << __LINE__ << ": tmap_id =\n" << transmap_to_string(&tmap_id, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": tmap_no_regs =\n" << transmap_to_string(&tmap_no_regs, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": itable_dst_relevant_regs =\n" << regset_to_string(&itable_dst_relevant_regs, as1, sizeof as1) << endl;
  //regmap_t iregmap;
  //regmap_copy(&iregmap, &regmap);
  //transmap_rename_using_dst_regmap(&tmap_id, &iregmap, NULL);
  size_t n_insns;
  if (is_save) {
    n_insns = transmap_translation_insns(&tmap_id, &tmap_no_regs, NULL, buf, size, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);
  } else {
    n_insns = transmap_translation_insns(&tmap_no_regs, &tmap_id, NULL, buf, size, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);
  }
  //cout << __func__ << " " << __LINE__ << ": buf =\n" << dst_iseq_to_string(buf, n_insns, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_fixed_reg_mapping =\n" << dst_fixed_reg_mapping.fixed_reg_mapping_to_string() << endl;
  dst_iseq_rename_cregs_to_vregs(buf, n_insns);
  //cout << __func__ << " " << __LINE__ << ": after rename cregs to vregs, buf =\n" << dst_iseq_to_string(buf, n_insns, as1, sizeof as1) << endl;
  return n_insns;
}

static size_t
jtable_gen_save_vregs_insns(regcons_t const &regcons, regset_t const &use, regset_t const &def, regset_t const &temps, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs, dst_insn_t *buf, size_t size)
{
  return jtable_gen_save_restore_vregs_insns(regcons, use, def, temps, dst_fixed_reg_mapping, itable_dst_relevant_regs, buf, size, true);
}


static size_t
jtable_gen_restore_vregs_insns(regcons_t const &regcons, regset_t const &use, regset_t const &def, regset_t const &temps, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs, dst_insn_t *buf, size_t size)
{
  return jtable_gen_save_restore_vregs_insns(regcons, use, def, temps, dst_fixed_reg_mapping, itable_dst_relevant_regs, buf, size, false);
}

static size_t
jtable_entry_gen_exec_iseq(jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs, dst_insn_t *exec_iseq, size_t exec_iseq_size, long *insn_num_map, long *insn_num_map_type, long *insn_num_map_restore, long *insn_num_map_data)
{
  dst_insn_t *ptr = exec_iseq;
  dst_insn_t *end = exec_iseq + exec_iseq_size;

  for (size_t i = 0; i < e->itable_entry_iseq_len; i++) {
    insn_num_map[i] = ptr - exec_iseq;
    ptr += jtable_gen_save_vregs_insns(e->regcons, e->use, e->def, e->jtable_entry_temps[JTABLE_EXEC_ISEQ_DATA][i], dst_fixed_reg_mapping, itable_dst_relevant_regs, ptr, end - ptr);
    insn_num_map_type[i] = ptr - exec_iseq;
    dst_iseq_copy(ptr, e->exec_iseq[JTABLE_EXEC_ISEQ_TYPE][i], e->exec_iseq_len[JTABLE_EXEC_ISEQ_TYPE][i]);
    ptr += e->exec_iseq_len[JTABLE_EXEC_ISEQ_TYPE][i];
    ASSERT(ptr < end);
    insn_num_map_restore[i] = ptr - exec_iseq;
    ptr += jtable_gen_restore_vregs_insns(e->regcons, e->use, e->def, e->jtable_entry_temps[JTABLE_EXEC_ISEQ_DATA][i], dst_fixed_reg_mapping, itable_dst_relevant_regs, ptr, end - ptr);
    ASSERT(ptr < end);
    insn_num_map_data[i] = ptr - exec_iseq;
    dst_iseq_copy(ptr, e->exec_iseq[JTABLE_EXEC_ISEQ_DATA][i], e->exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][i]);
    ptr += e->exec_iseq_len[JTABLE_EXEC_ISEQ_DATA][i];
    ASSERT(ptr < end);
  }
  insn_num_map[e->itable_entry_iseq_len] = ptr - exec_iseq;

  //dst_iseq_rename_sboxed_abs_inums_using_map(exec_iseq, ptr - exec_iseq, insn_num_map, e->itable_entry_iseq_len + 1);
  //dst_iseq_convert_sboxed_abs_inums_to_pcrel_inums(exec_iseq, ptr - exec_iseq);
  CPP_DBG_EXEC2(JTABLE, cout << __func__ << " " << __LINE__ << ": exec_iseq =\n" << dst_iseq_to_string(exec_iseq, ptr - exec_iseq, as1, sizeof as1) << endl);
 
  return ptr - exec_iseq;
}

static size_t
jtable_entry_output_fixup_code_for_regs(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[])
{
  size_t binlen = 0;
  for (size_t i = 0; i < e->itable_entry_iseq_len; i++) {
    size_t n_index = insn_num_map[i];
    size_t t_index = insn_num_map_type[i];
    size_t r_index = insn_num_map_restore[i];
    size_t d_index = insn_num_map_data[i];
    size_t dindex = insn_num_map_data[i];
    size_t next_n_index = insn_num_map[i + 1];
    regcons_t regcons_all;
    regcons_set(&regcons_all);
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", binlen = " << binlen << endl;
    binlen += jtable_entry_output_fixup_code_for_regs_using_regmap_or_sequence(ofp, &exec_iseq[n_index], &exec_iseq[t_index], regcons_all, *regset_empty(), JTABLE_EXEC_ISEQ_DATA, i, dst_fixed_reg_mapping, REGMAP_NAME_JTABLE_REGMAP, binlen);
    binlen += jtable_entry_output_fixup_code_for_regs_using_regmap_or_sequence(ofp, &exec_iseq[t_index], &exec_iseq[r_index], e->regcons, e->jtable_entry_temps[JTABLE_EXEC_ISEQ_TYPE][i], JTABLE_EXEC_ISEQ_TYPE, i, dst_fixed_reg_mapping, REGMAP_NAME_NONE, binlen);
    binlen += jtable_entry_output_fixup_code_for_regs_using_regmap_or_sequence(ofp, &exec_iseq[r_index], &exec_iseq[d_index], regcons_all, *regset_empty(), JTABLE_EXEC_ISEQ_DATA, i, dst_fixed_reg_mapping, REGMAP_NAME_JTABLE_REGMAP, binlen);
    binlen += jtable_entry_output_fixup_code_for_regs_using_regmap_or_sequence(ofp, &exec_iseq[d_index], &exec_iseq[next_n_index], e->regcons, e->jtable_entry_temps[JTABLE_EXEC_ISEQ_DATA][i], JTABLE_EXEC_ISEQ_DATA, i, dst_fixed_reg_mapping, REGMAP_NAME_IREGMAP, binlen);
  }
  return binlen;
}

static void
binbufs_diff(uint8_t const *a, uint8_t const *b, size_t size, size_t *start, size_t *len)
{
  bool diff_started, diff_finished;
  size_t i;

  diff_started = false;
  diff_finished = false;
  for (i = 0; i < size; i++) {
    if (!diff_started && a[i] != b[i]) {
      ASSERT(!diff_finished);
      diff_started = true;
      *start = i;
    } else if (diff_started && a[i] == b[i]) {
      diff_started = false;
      diff_finished = true;
      *len = i - *start;
    }
  }
  if (diff_started) {
    ASSERT(!diff_finished);
    diff_started = false;
    diff_finished = true;
    *len = i - *start;
  }
}

static void
output_vconst_var_code(FILE *ofp, size_t start, size_t len, valtag_t const *vt)
{
  ASSERT(vt->tag == tag_var);
  fprintf(ofp, "{\n");
  fprintf(ofp, "static valtag_t vt; %s;\n", imm_valtag_to_string(vt, "vt", as1, sizeof as1));
  fprintf(ofp, "ASSERT(vt.val < vconst_map_len);\n");
  fprintf(ofp, "int32_t curval = vconst_map[vt.val];\n");
  fprintf(ofp, "int32_t val = (*imm_map_rename_vconst_fn)(&vt, curval, vconst_map, " //XXX:vconst_map is of type int32_t *, but the function takes valtag_t *! FIXME
      "vconst_map_len, NULL, 0, NULL);\n");
  DBG_EXEC3(JTABLE,
      fprintf(ofp, "printf(\"%%s() %%d: val = 0x%%x\\n\", __func__, __LINE__, val);\n"););
  switch (len) {
    case 1:
      fprintf(ofp, "*(uint8_t *)(bincode + %zd) = (uint8_t)val;\n", start);
      break;
    case 2:
      fprintf(ofp, "*(uint16_t *)(bincode + %zd) = (uint16_t)val;\n", start);
      break;
    case 4:
      fprintf(ofp, "*(uint32_t *)(bincode + %zd) = (uint32_t)val;\n", start);
      break;
    default: NOT_REACHED();
  }
  fprintf(ofp, "}\n");
}

static void
output_vconst_num_code(FILE *ofp, size_t start, size_t len, valtag_t const *vt)
{
  ASSERT(vt->tag == tag_imm_vconst_num);
  fprintf(ofp, "{\n");
  fprintf(ofp, "static valtag_t vt; %s;\n", imm_valtag_to_string(vt, "vt", as1, sizeof as1));
  fprintf(ofp, "ASSERT(vt.val >= 0 && vt.val < vconst_map_len);\n");
  fprintf(ofp, "int32_t val = vconst_sequence[vt.val];\n");
  fprintf(ofp, "ASSERT(val >= 0);\n");
  DBG_EXEC3(JTABLE,
      fprintf(ofp, "printf(\"%%s() %%d: val = 0x%%x\\n\", __func__, __LINE__, val);\n"););
  ASSERT(len == 4);
  fprintf(ofp, "*(uint32_t *)(bincode + %zd) = (uint32_t)val;\n", start);
  fprintf(ofp, "}\n");
}

static void
output_memlocs_code(FILE *ofp, size_t start, size_t len, valtag_t const *vt)
{
  ASSERT(vt->tag == tag_imm_symbol);
  fprintf(ofp, "{\n");
  fprintf(ofp, "static valtag_t vt; %s;\n", imm_valtag_to_string(vt, "vt", as1, sizeof as1));
  fprintf(ofp, "ASSERT(vt.val >= 0 && vt.val < memloc_sequence_len);\n");
  fprintf(ofp, "int32_t val = memloc_sequence[vt.val];\n");
  fprintf(ofp, "ASSERT(val >= 0);\n");
  fprintf(ofp, "ASSERT(memslot_map->memslot_map_belongs(val));\n");
  fprintf(ofp, "ASSERT(!memslot_map->memslot_map_denotes_constant(val));\n");
  fprintf(ofp, "valtag_t const &vt2 = memslot_map->memslot_map_get_mapping(val);\n");
  fprintf(ofp, "ASSERT(vt2.tag == tag_const);\n");
  fprintf(ofp, "val = vt2.val;\n");
  DBG_EXEC3(JTABLE,
      fprintf(ofp, "printf(\"%%s() %%d: val = 0x%%x\\n\", __func__, __LINE__, val);\n"););
  ASSERT(len = 4);
  fprintf(ofp, "*(uint32_t *)(bincode + %zd) = (uint32_t)val;\n", start);
  fprintf(ofp, "}\n");
}

static void
output_memloc_nums_code(FILE *ofp, size_t start, size_t len, valtag_t const *vt)
{
  ASSERT(vt->tag == tag_imm_symbol_num);
  fprintf(ofp, "{\n");
  fprintf(ofp, "static valtag_t vt; %s;\n", imm_valtag_to_string(vt, "vt", as1, sizeof as1));
  fprintf(ofp, "ASSERT(vt.val >= 0 && vt.val < memloc_sequence_len);\n");
  fprintf(ofp, "int32_t val = memloc_sequence[vt.val];\n");
  fprintf(ofp, "ASSERT(val >= 0);\n");
  //fprintf(ofp, "ASSERT(memslot_map->memslot_map_belongs(val));\n");
  //fprintf(ofp, "ASSERT(!memslot_map->memslot_map_denotes_constant(val));\n");
  //fprintf(ofp, "valtag_t const &vt2 = memslot_map->memslot_map_get_mapping(val);\n");
  //fprintf(ofp, "ASSERT(vt2.tag == tag_const);\n");
  //fprintf(ofp, "val = vt2.val - SRC_ENV_ADDR;\n");
  DBG_EXEC3(JTABLE,
      fprintf(ofp, "printf(\"%%s() %%d: val = 0x%%x\\n\", __func__, __LINE__, val);\n"););
  ASSERT(len = 4);
  fprintf(ofp, "*(uint32_t *)(bincode + %zd) = (uint32_t)val;\n", start);
  fprintf(ofp, "}\n");
}



/*static size_t
jtable_entry_output_fixup_code_for_vconsts_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset)
{
  size_t binbuf_size = 4096;
  uint8_t binbuf0[binbuf_size], binbuf1[binbuf_size];
  size_t exec_iseq_len = exec_iseq_stop - exec_iseq_start;
  dst_insn_t *exec_iseq_const = new dst_insn_t[exec_iseq_len];
  long i, j, binlen0, binlen1;
  size_t start, len;
  valtag_t const *vtv;
  operand_t *op;
  valtag_t *vt;

  size_t binlen = gen_code_for_exec_iseq(exec_iseq_start, exec_iseq_len, regcons, dst_fixed_reg_mapping, NULL, 0);

  dst_iseq_copy(exec_iseq_const, exec_iseq_start, exec_iseq_len);
  bool renamed = dst_iseq_rename_regs_and_transmaps_after_inferring_regcons(exec_iseq_const,
      exec_iseq_len, NULL, NULL, 0, NULL, NULL, fixed_reg_mapping_t());
  ASSERT(renamed);

#if ARCH_DST == ARCH_I386
  for (i = 0; i < exec_iseq_len; i++) {
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      int max_op_size;

      op = &exec_iseq_const[i].op[j];
      if (op->type == op_imm) {
        vt = &op->valtag.imm;
        vtv = &exec_iseq_start[i].op[j].valtag.imm;
      } else if (op->type == op_mem) {
        vt = &op->valtag.mem.disp;
        vtv = &exec_iseq_start[i].op[j].valtag.mem.disp;
        //vt = &op->valtag.mem.disp;
      } else {
        continue;
      }
      if (vtv->tag != tag_var) {
        continue;
      }
      max_op_size = i386_max_operand_size(op, exec_iseq_const[i].opc, j - exec_iseq_const[i].num_implicit_ops);

      vt->tag = tag_const;
      vt->val = 0x10101010;

      ASSERT(max_op_size >= 0);
      if (max_op_size < 4) {
        vt->val &= (1ULL << (max_op_size * BYTE_LEN)) - 1;
      }

      binlen0 = dst_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, binbuf0,
          binbuf_size, I386_AS_MODE_64_REGULAR);

      vt->val = 0x02020202;
      ASSERT(max_op_size >= 0);
      if (max_op_size < 4) {
        vt->val &= (1ULL << (max_op_size * BYTE_LEN)) - 1;
      }

      binlen1 = dst_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, binbuf1,
          binbuf_size, I386_AS_MODE_64_REGULAR);
      if (binlen0 != binlen1 || binlen0 == 0) {
        cout << __func__ << " " << __LINE__ << ": exec_iseq_const:\n" << dst_iseq_to_string(exec_iseq_const, exec_iseq_len, as1, sizeof as1) << endl;
        printf("binlens mismatch\n");
        printf("binbuf0: %s.\n", hex_string(binbuf0, binlen0, as1, sizeof as1));
        printf("binbuf1: %s.\n", hex_string(binbuf1, binlen0, as1, sizeof as1));
      }
      ASSERT(binlen0 == binlen1);
      ASSERT(binlen0 == binlen);
      ASSERT(binlen0 != 0);
      DBG_EXEC4(JTABLE, printf("binbuf0: %s.\n", hex_string(binbuf0, binlen0, as1, sizeof as1)));
      DBG_EXEC4(JTABLE, printf("binbuf1: %s.\n", hex_string(binbuf1, binlen0, as1, sizeof as1)));

      binbufs_diff(binbuf0, binbuf1, binlen0, &start, &len);
      DBG4(JTABLE, "binbuf = %s; start = %ld, len = %ld\n",
          hex_string(binbuf0, binlen0, as1, sizeof as1), start, len);

      output_vconst_var_code(ofp, cur_offset + start, len, vtv);
    }
  }
#else
  NOT_IMPLEMENTED();
#endif
  delete [] exec_iseq_const;
  return binlen;
}*/

static size_t
jtable_entry_output_fixup_code_for_tag_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset, src_tag_t tag, std::function<int (operand_t const *, opc_t, int)> max_operand_size_f, std::function<void (FILE *, size_t, size_t, valtag_t const *)> output_vconst_code_f)
{
  size_t binbuf_size = 40960;
  uint8_t *binbuf0 = new uint8_t[binbuf_size];
  uint8_t *binbuf1 = new uint8_t[binbuf_size];
  dst_insn_t const *exec_iseq = exec_iseq_start;
  size_t exec_iseq_len = exec_iseq_stop - exec_iseq_start;
  dst_insn_t *exec_iseq_const = new dst_insn_t[exec_iseq_len];
  long i, j, binlen0, binlen1;
  size_t start, len;
  valtag_t const *vtv;
  operand_t *op;
  valtag_t *vt;

  size_t binlen = gen_code_for_exec_iseq(exec_iseq_start, exec_iseq_len, regcons, dst_fixed_reg_mapping, NULL, 0);

  dst_iseq_copy(exec_iseq_const, exec_iseq, exec_iseq_len);
  bool renamed = dst_iseq_rename_regs_and_transmaps_after_inferring_regcons(exec_iseq_const,
      exec_iseq_len, NULL, NULL, 0, NULL, NULL, fixed_reg_mapping_t());
  ASSERT(renamed);

#if ARCH_DST == ARCH_I386
  for (i = 0; i < exec_iseq_len; i++) {
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      op = &exec_iseq_const[i].op[j];
      if (op->type == op_imm) {
        vt = &op->valtag.imm;
        vtv = &exec_iseq[i].op[j].valtag.imm;
      } else if (op->type == op_mem) {
        vt = &op->valtag.mem.disp;
        vtv = &exec_iseq[i].op[j].valtag.mem.disp;
        //vt = &op->valtag.mem.disp;
      } else {
        continue;
      }
      if (vtv->tag != tag/*tag_imm_vconst_num*/) {
        continue;
      }
      int max_op_size = max_operand_size_f(op, exec_iseq_const[i].opc, j - exec_iseq_const[i].num_implicit_ops);

      vt->tag = tag_const;
      vt->val = 0x10101010;
      ASSERT(max_op_size >= 0);

      if (max_op_size < 4) {
        vt->val &= (1ULL << (max_op_size * BYTE_LEN)) - 1;
      }
      binlen0 = dst_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, binbuf0,
          binbuf_size, I386_AS_MODE_64_REGULAR);

      vt->val = 0x02020202;
      ASSERT(max_op_size >= 0);
      if (max_op_size < 4) {
        vt->val &= (1ULL << (max_op_size * BYTE_LEN)) - 1;
      }
      binlen1 = dst_iseq_assemble(NULL, exec_iseq_const, exec_iseq_len, binbuf1,
          binbuf_size, I386_AS_MODE_64_REGULAR);
      if (binlen0 != binlen1) {
        printf("binlens mismatch\n");
        printf("binbuf0: %s.\n", hex_string(binbuf0, binlen0, as1, sizeof as1));
        printf("binbuf1: %s.\n", hex_string(binbuf1, binlen0, as1, sizeof as1));
      }
      ASSERT(binlen0 == binlen1);
      ASSERT(binlen0 == binlen);
      DBG_EXEC4(JTABLE, printf("binbuf0: %s.\n", hex_string(binbuf0, binlen0, as1, sizeof as1)));
      DBG_EXEC4(JTABLE, printf("binbuf1: %s.\n", hex_string(binbuf1, binlen0, as1, sizeof as1)));

      binbufs_diff(binbuf0, binbuf1, binlen0, &start, &len);
      DBG4(JTABLE, "binbuf = %s; start = %ld, len = %ld\n",
          hex_string(binbuf0, binlen0, as1, sizeof as1), start, len);

      output_vconst_code_f(ofp, cur_offset + start, len, vtv);
      //output_vconst_num_code(ofp, cur_offset + start, len, vtv);
    }
  }
#else
  NOT_IMPLEMENTED();
#endif
  delete [] exec_iseq_const;
  delete [] binbuf0;
  delete [] binbuf1;
  return binlen;
}

static size_t
jtable_entry_output_fixup_code_for_vconsts_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset)
{
  return jtable_entry_output_fixup_code_for_tag_for_single_exec_iseq(ofp, exec_iseq_start, exec_iseq_stop, regcons, dst_fixed_reg_mapping, cur_offset, tag_var, i386_max_operand_size, output_vconst_var_code);
}

static int max_operand_size_f_ret4(operand_t const *, opc_t, int)
{
  return 4;
};


static size_t
jtable_entry_output_fixup_code_for_vconst_nums_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset)
{
  return jtable_entry_output_fixup_code_for_tag_for_single_exec_iseq(ofp, exec_iseq_start, exec_iseq_stop, regcons, dst_fixed_reg_mapping, cur_offset, tag_imm_vconst_num, max_operand_size_f_ret4, output_vconst_num_code);
}

static size_t
jtable_entry_output_fixup_code_for_memlocs_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset)
{
  return jtable_entry_output_fixup_code_for_tag_for_single_exec_iseq(ofp, exec_iseq_start, exec_iseq_stop, regcons, dst_fixed_reg_mapping, cur_offset, tag_imm_symbol, max_operand_size_f_ret4, output_memlocs_code);
}

static size_t
jtable_entry_output_fixup_code_for_memloc_nums_for_single_exec_iseq(FILE *ofp, dst_insn_t const *exec_iseq_start, dst_insn_t const *exec_iseq_stop, regcons_t const &regcons, fixed_reg_mapping_t const &dst_fixed_reg_mapping, size_t cur_offset)
{
  return jtable_entry_output_fixup_code_for_tag_for_single_exec_iseq(ofp, exec_iseq_start, exec_iseq_stop, regcons, dst_fixed_reg_mapping, cur_offset, tag_imm_symbol_num, max_operand_size_f_ret4, output_memloc_nums_code);
}

static size_t
jtable_entry_output_fixup_code_for_vconsts_and_vconst_nums(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[], size_t (*single_exec_iseq_fn)(FILE *, dst_insn_t const *, dst_insn_t const *, regcons_t const &, fixed_reg_mapping_t const &, size_t))
{
  size_t binlen = 0;
  for (size_t i = 0; i < e->itable_entry_iseq_len; i++) {
    size_t n_index = insn_num_map[i];
    size_t t_index = insn_num_map_type[i];
    size_t r_index = insn_num_map_restore[i];
    size_t d_index = insn_num_map_data[i];
    size_t next_n_index = insn_num_map[i + 1];
    binlen += (*single_exec_iseq_fn)(ofp, &exec_iseq[n_index], &exec_iseq[t_index], e->regcons, dst_fixed_reg_mapping, binlen);
    binlen += (*single_exec_iseq_fn)(ofp, &exec_iseq[t_index], &exec_iseq[r_index], e->regcons, dst_fixed_reg_mapping, binlen);
    binlen += (*single_exec_iseq_fn)(ofp, &exec_iseq[r_index], &exec_iseq[d_index], e->regcons, dst_fixed_reg_mapping, binlen);
    binlen += (*single_exec_iseq_fn)(ofp, &exec_iseq[d_index], &exec_iseq[next_n_index], e->regcons, dst_fixed_reg_mapping, binlen);
  }
  return binlen;
}

static size_t
jtable_entry_output_fixup_code_for_vconsts(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[])
{
  return jtable_entry_output_fixup_code_for_vconsts_and_vconst_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, &jtable_entry_output_fixup_code_for_vconsts_for_single_exec_iseq);
}

static size_t
jtable_entry_output_fixup_code_for_vconst_nums(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[])
{
  return jtable_entry_output_fixup_code_for_vconsts_and_vconst_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, &jtable_entry_output_fixup_code_for_vconst_nums_for_single_exec_iseq);
}

static size_t
jtable_entry_output_fixup_code_for_memlocs(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[])
{
  return jtable_entry_output_fixup_code_for_vconsts_and_vconst_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, &jtable_entry_output_fixup_code_for_memlocs_for_single_exec_iseq);
}

static size_t
jtable_entry_output_fixup_code_for_memloc_nums(FILE *ofp, jtable_entry_t const *e, fixed_reg_mapping_t const &dst_fixed_reg_mapping, dst_insn_t const *exec_iseq, size_t exec_iseq_len, long const insn_num_map[], long const insn_num_map_type[], long insn_num_map_restore[], long const insn_num_map_data[])
{
  return jtable_entry_output_fixup_code_for_vconsts_and_vconst_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, &jtable_entry_output_fixup_code_for_memloc_nums_for_single_exec_iseq);
}



static void
jtable_entry_output_fixup_code_for_nextpcs(FILE *ofp, dst_insn_t const *exec_iseq, size_t exec_iseq_len)
{
}

static void
jtable_entry_output_gencode(FILE *ofp, jtable_entry_t const *e/*, int d*/, fixed_reg_mapping_t const &dst_fixed_reg_mapping, regset_t const &itable_dst_relevant_regs)
{
  long max_exec_iseq_len = 128;
  for (size_t i = 0; i < JTABLE_NUM_EXEC_ISEQS; i++) {
    for (size_t l = 0; l < e->itable_entry_iseq_len; l++) {
      max_exec_iseq_len += e->exec_iseq_len[i][l] + 64; //the extra 64 is for transmap translation insns
    }
  }
  dst_insn_t *exec_iseq = new dst_insn_t[max_exec_iseq_len];
  long *insn_num_map = new long[e->itable_entry_iseq_len + 1];
  long *insn_num_map_type = new long[e->itable_entry_iseq_len];
  long *insn_num_map_restore = new long[e->itable_entry_iseq_len];
  long *insn_num_map_data = new long[e->itable_entry_iseq_len];

  size_t exec_iseq_len = jtable_entry_gen_exec_iseq(e, dst_fixed_reg_mapping, itable_dst_relevant_regs, exec_iseq, max_exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);

  size_t emitted_binlen = emit_code_for_exec_iseq_and_also_infer_pcrels(ofp, exec_iseq, exec_iseq_len, e->regcons, dst_fixed_reg_mapping, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data, e->itable_entry_iseq_len, e->jtable_entry_temps);


  stopwatch_run(&jtable_entry_output_fixup_code_for_regs_timer);
  size_t reg_binlen = jtable_entry_output_fixup_code_for_regs(ofp, e, /*d, */dst_fixed_reg_mapping, itable_dst_relevant_regs, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);
  if (reg_binlen != emitted_binlen) {
    cout << __func__ << " " << __LINE__ << ": reg_binlen = " << reg_binlen << ", emitted_binlen = " << emitted_binlen << endl;
  }
  ASSERT(reg_binlen == emitted_binlen);
  stopwatch_stop(&jtable_entry_output_fixup_code_for_regs_timer);

  stopwatch_run(&jtable_entry_output_fixup_code_for_vconsts_timer);
  size_t vconsts_binlen = jtable_entry_output_fixup_code_for_vconsts(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);
  ASSERT(vconsts_binlen == emitted_binlen);
  stopwatch_stop(&jtable_entry_output_fixup_code_for_vconsts_timer);

  size_t vconst_nums_binlen = jtable_entry_output_fixup_code_for_vconst_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);
  ASSERT(vconst_nums_binlen == emitted_binlen);

  size_t memlocs_binlen = jtable_entry_output_fixup_code_for_memlocs(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);
  ASSERT(memlocs_binlen == emitted_binlen);

  size_t memloc_nums_binlen = jtable_entry_output_fixup_code_for_memloc_nums(ofp, e, dst_fixed_reg_mapping, exec_iseq, exec_iseq_len, insn_num_map, insn_num_map_type, insn_num_map_restore, insn_num_map_data);
  ASSERT(memloc_nums_binlen == emitted_binlen);

  jtable_entry_output_fixup_code_for_nextpcs(ofp, exec_iseq, exec_iseq_len);

  delete [] exec_iseq;
  delete [] insn_num_map;
  delete [] insn_num_map_type;
  delete [] insn_num_map_restore;
  delete [] insn_num_map_data;
}

void
jtable_init_gencode_functions(char *outfile, size_t outfile_size,
    jtable_entry_t const *jentries, long num_entries, fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    regset_t const &itable_dst_relevant_regs)
{
  char srcfile[strlen(outfile) + 32];
  FILE *ofp;
  long i;
  int d;

  snprintf(srcfile, sizeof srcfile, "%s.c", outfile);
  ofp = fopen(srcfile, "w");
  ASSERT(ofp);

  fprintf(ofp, "#include <stdlib.h>\n"
      "#include <stdint.h>\n"
      "#include <string.h>\n"
      "#include <stdio.h>\n"
      "#include \"rewrite/config-all.h\"\n"
      "#include \"support/types.h\"\n"
      "#include \"rewrite/imm_vt_map.h\"\n"
      "#include \"rewrite/memslot_map.h\"\n"
      "#include \"rewrite/reg_identifier.h\"\n"
      "#include \"rewrite/valtag.h\"\n"
      "#include \"support/debug.h\"\n"
      "using namespace std;\n"
  );

  fprintf(ofp, "#define print_args() do { \\\n"
      "printf(\"entrynum = %%ld.\\n\", entrynum);\\\n"
      "printf(\"iregmap_extable:\\n\");\\\n"
      "int i, j;\\\n"
      "for (const auto &g : iregmap_extable) {\\\n"
      "printf(\"group %%d:\\n\", g.first);\\\n"
      "for (const auto &r : g.second) {\\\n"
      "printf(\"  %%d : %%d\\n\", r.first.reg_identifier_get_id(), r.second.reg_identifier_get_id());\\\n"
      "}\\\n"
      "}\\\n"
      "printf(\"jtable_regmap_extable:\\n\");\\\n"
      "for (const auto &g : jtable_regmap_extable) {\\\n"
      "printf(\"group %%d:\\n\", g.first);\\\n"
      "for (const auto &r : g.second) {\\\n"
      "printf(\"  %%d : %%d\\n\", r.first.reg_identifier_get_id(), r.second.reg_identifier_get_id());\\\n"
      "}\\\n"
      "}\\\n"
      "printf(\"vconsts:\\n\");\\\n"
      "for (i = 0; i < vconst_map_len; i++) {\\\n"
      "printf(\"  %%d : 0x%%x\\n\", i, (int)vconst_map[i]);\\\n"
      "}\\\n"
      "printf(\"nextpcs:\\n\");\\\n"
      "for (i = 0; i < nextpc_sequence_len; i++) {\\\n"
      "printf(\"  %%d : %%d\\n\", i, (int)nextpc_sequence[i]);\\\n"
      "}\\\n"
      "} while(0);\n");


  fprintf(ofp,
      "extern \"C\"\n"
      "size_t jtable_gencode_function(long entrynum, uint8_t *bincode, "
      "size_t bincode_size, "
      //"int const regmap_extable[%d][%d], "
      "std::map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &iregmap_extable, "
      "std::map<exreg_group_id_t, map<reg_identifier_t, reg_identifier_t>> const &jtable_regmap_extable, "
      "int const exreg_sequence[%d][%d], "
      "int const exreg_sequence_len[%d], "
      "int32_t const *vconst_map, "
      "int32_t const *vconst_sequence, "
      "long vconst_map_len, "
      "long const *memloc_sequence, "
      "long memloc_sequence_len, "
      "memslot_map_t const *memslot_map, "
      "long const *nextpc_sequence, long nextpc_sequence_len, "
      "uint8_t const * const footer_bincode[%d], "
      "int32_t (*imm_map_rename_vconst_fn)(valtag_t const *, int32_t, "
      "int32_t const *, long, nextpc_t const *, int, int const extable[%d][%d])"
      //", int d"
      ") {\n",
      //MAX_NUM_EXREG_GROUPS, MAX_NUM_EXREGS(0),
      MAX_NUM_EXREG_GROUPS, MAX_NUM_EXREGS(0),
      MAX_NUM_EXREG_GROUPS, ENUM_MAX_LEN, MAX_NUM_EXREG_GROUPS, MAX_NUM_EXREGS(0));

  DBG_EXEC3(JTABLE, fprintf(ofp, "print_args();\n"););
  fprintf(ofp, "size_t binlen = 0;\n");

  //for (d = 0; d < JTABLE_NUM_EXEC_ISEQS; d++) {
  //  fprintf(ofp, "if (d == %d) {\n", d);
    fprintf(ofp, "switch (entrynum) {\n\n");

    for (i = 0; i < num_entries; i++) {
      DBG3(JTABLE, "Emitting gencode for entry %ld\n", i);
      fprintf(ofp, "case %ld: {\n", i);
      jtable_entry_output_gencode(ofp, &jentries[i]/*, d*/, dst_fixed_reg_mapping, itable_dst_relevant_regs);
      fprintf(ofp, "break; }\n");
    }

    fprintf(ofp, "}\n");
    //fprintf(ofp, "}\n");
  //}
  fprintf(ofp, "return binlen;\n");
  fprintf(ofp, "}\n");

  fclose(ofp);

  static char command[4096];
  snprintf(command, sizeof command, "g++ -std=c++11 -O2 -DARCH_SRC=%d -DARCH_DST=%d -I%s/lib -I%s "
      "-shared -fPIC -Wl,-soname,%s.so %s.c -o %s.so", ARCH_SRC, ARCH_DST, SRC_DIR, ABUILD_DIR,
      outfile, outfile, outfile);
  cout << __func__ << " " << __LINE__ << ": calling " << command << endl;
  cmd_helper_issue_command(command);
  cout << __func__ << " " << __LINE__ << ": done " << command << endl;

  strncat(outfile, ".so", outfile_size);
}
