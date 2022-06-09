#include <map>
#include <iostream>
#include "insn/src-insn.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/mman.h>
#include "support/timers.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "support/src-defs.h"
#include "support/globals.h"

#include "cutils/imap_elem.h"
#include "i386/cpu_consts.h"
#include "insn/rdefs.h"
//#include "ppc/execute.h"
#include "rewrite/static_translate.h"
#include "valtag/memslot_map.h"
#include "valtag/memslot_set.h"
#include "valtag/nextpc.h"

#include "config-host.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/bbl.h"
#include "insn/cfg.h"
#include "expr/memlabel.h"

#include "insn/jumptable.h"
#include "rewrite/peephole.h"
#include "superopt/rand_states.h"
#include "support/c_utils.h"
#include "valtag/regmap.h"
//#include "rewrite/memlabel_map.h"
//#include "ppc/regmap.h"
//#include "imm_map.h" 
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "valtag/transmap.h"
#include "support/debug.h"
#include "rewrite/transmap_set.h"
#include "cutils/enumerator.h"
#include "rewrite/transmap_genset.h"
#include "cutils/large_int.h"
#include "valtag/imm_vt_map.h"
//#include "gsupport/sym_exec.h"
#include "insn/live_ranges.h"

#include "i386/regs.h"
#include "x64/regs.h"

#include "valtag/elf/elf.h"
#include "cutils/rbtree.h"
#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "superopt/superopt.h"
#include "superopt/superoptdb.h"
//#include "rewrite/harvest.h"
#include "rewrite/symbol_map.h"
#include "valtag/pc_with_copy_id.h"
#include "gsupport/graph_symbol.h"
#include "insn/fun_type_info.h"
#include "cutils/pc_elem.h"

using namespace std;


static char const *constant_strings[] = { "$C", ",C", " C", "*C", ":C", "__reloc_", ".NEXTPC"};
static size_t num_constant_strings = sizeof constant_strings/sizeof constant_strings[0];


static bool
is_exvr_suffix(char const *ptr)
{
  return ptr[0] == 'b' || ptr[0] == 'B' || ptr[0] == 'w' || ptr[0] == 'd'
      || ptr[0] == 'q';
}

static bool
peep_preprocess_exvregs_suffix(char *line, char const *suffix,
    regmap_t const *regmap,
    char *(*exregname_suffix)(int, int, char const *, char *, size_t))
{
  int groupnum, exregnum, i, j;
  char const *pattern, *sptr;
  char *ptr, *start, *cur;
  size_t new_strlen;
  char new_str[64];

  pattern = "%exvr";
  cur = line;
  while (ptr = strstr(cur, pattern)) {
    start = ptr;
    ASSERT(strlen(pattern) == 5);
    cur = ptr + 5;
    if (   *(ptr - 1) != ' '
        && *(ptr - 1) != ','
        && *(ptr - 1) != '('
        && *(ptr - 1) != '*') {
      continue;
    }
    ptr += 5;
    if (*ptr < '0' || *ptr > '9') {
      continue;
    }
    groupnum = 0;
    while (*ptr >= '0' && *ptr <= '9') {
      groupnum = groupnum * 10 + ((*ptr) - '0');
      ptr++;
    }
    if (*ptr != '.') {
      continue;
    }
    ptr++;
    if (*ptr < '0' || *ptr > '9') {
      continue;
    }
    exregnum = 0;
    while (*ptr >= '0' && *ptr <= '9') {
      exregnum = exregnum * 10 + ((*ptr) - '0');
      ptr++;
    }
    ASSERT(exregnum >= 0 && exregnum < MAX_NUM_EXREGS(groupnum));
    new_str[0] = '\0';

    sptr = ptr;
    if (   (suffix[0] == '\0' && is_exvr_suffix(ptr))
        || (suffix[0] != '\0' && !strstart(ptr, suffix, &sptr))) {
      continue;
    }

    reg_identifier_t const &mapped_reg = regmap_get_elem(regmap, groupnum, exregnum);
    //cout << __func__ << " " << __LINE__ << ": mapped_reg = " << mapped_reg.reg_identifier_to_string() << endl;
    ASSERT(mapped_reg.reg_identifier_is_valid());
    if (mapped_reg.reg_identifier_is_unassigned()) {
      DYN_DBG2(CMN_ISEQ_FROM_STRING, "returning false, groupnum=%d, exregnum=%d.\n", groupnum,
          exregnum);
      return false;
    }
    //ASSERT(regmap->regmap_extable[groupnum][exregnum] >= 0);
    //cout << __func__ << " " << __LINE__ << ": mapped_reg = " << mapped_reg.reg_identifier_to_string() << endl;
    (*exregname_suffix)(groupnum, mapped_reg.reg_identifier_get_id(), suffix,
        new_str, sizeof new_str);
    new_strlen = strlen(new_str);
    
    memmove(start + new_strlen, sptr, strlen(sptr) + 1);
    memcpy(start, new_str, new_strlen);

    cur = start + new_strlen;
  }
  return true;
}

bool
peep_preprocess_exvregs(char *line, regmap_t const *regmap,
    char *(*exregname_suffix)(int, int, char const *, char *, size_t))
{
  char const *suffixes[] = { "b", "B", "w", "d", "q", "" };
  //long nbytes[] = {1, 1, 2, 4, 8};
  long i;

  for (i = 0; i < sizeof suffixes/sizeof suffixes[0]; i++) {
    if (!peep_preprocess_exvregs_suffix(line, suffixes[i], regmap,
        exregname_suffix)) {
      DYN_DBG2(CMN_ISEQ_FROM_STRING, "peep_preprocess_exvregs_suffix returned false for suffix "
          "%s.\n", suffixes[i]);
      return false;
    }
  }
  return true;
}

void
peep_preprocess_scan_label(char *line, char const *label, long val)
{
  char label_str[32];
  char *ptr;

  snprintf (label_str, sizeof label_str, ".%s", label);
  if (ptr = strstr(line, label_str)) {
    char label_val[32];
    snprintf(label_val, sizeof label_val, "0x%lx", val);
    unsigned label_str_len = strlen(label_str);
    unsigned label_val_len = strlen(label_val);
    memmove(ptr + label_val_len, ptr + label_str_len, strlen(ptr + label_str_len) + 1);
    memcpy(ptr, label_val, label_val_len);
  }
}



void
peep_preprocess_all_constants(translation_instance_t *ti, char *line,
    vconstants_t *vconstants, int num_vconstants)
{
  char *cur, *op_end;
  long i;

  for (i = 0; i < num_vconstants; i++) {
    vconstants[i].valid = 0;
  }

  i = 0;

  cur = line;
  /* this code assumes that there can be at most one vconstant in one operand.
     it also assumes that operands are separated by commas.
     operands may have depth=1 bracketing; brackets may contain commas but
     do not contain any constants.
     it increments 'i' with the number of operands seen. inv_rename should
     also obey this order and count. */
  while (   (op_end = strchr(cur + 1, ','))
         || (op_end = strchr(cur + 1, '\n'))
         || (op_end = strchr(cur + 1, '\0'))) {
    if (   *op_end == ','
        && strchr(cur + 1, '(') < op_end
        && strchr(cur + 1, ')') > op_end) {
      op_end = strchr(cur + 1, ')');
      ASSERT(op_end);
    }
    //char opstr[512];
    size_t oplen;
    char *start;
    //memcpy(opstr, cur, op_end - cur);
    //opstr[op_end - cur] = '\0';
    DYN_DBG3(PEEP_TAB, "op_end=%s, i = %ld, &vconstants[i]=%p.\n", op_end, i, &vconstants[i]);
    //DYN_DBG2(PEEP_TAB, "Looking at '%s'\n", opstr);
    vconstants[i].valid = 0;
    if (   (start = (char *)strstr_arr(cur, constant_strings, num_constant_strings))
        && start < op_end
        && (oplen = parse_vconstant(start, &vconstants[i], ti).second)) {
      ASSERT(oplen > 0);
      ASSERT(vconstants[i].valid);
      DYN_DBG2(PEEP_TAB, "start = %s.", start);
      char replacement[256];
      snprintf(replacement, sizeof replacement, "0x%x ", vconstants[i].placeholder);
      DYN_DBG2(PEEP_TAB, "found vconstant at %ld, oplen = %zd, "
          "vconstants[i].placeholder = %ld\n", i, oplen,
          (long)vconstants[i].placeholder);
      memmove(start + 1 + strlen(replacement), start + oplen, strlen(start + oplen) + 1);
      memcpy(start + 1, replacement, strlen(replacement));
      //memcpy(cur, opstr, strlen(opstr));
      //op_end = cur + strlen(opstr);
      op_end = start + 1 + strlen(replacement);
    }
    if (*op_end == '\0') {
      DYN_DBG3(PEEP_TAB, "breaking because op_end = %s.\n", op_end);
      break;
    }
    DYN_DBG3(PEEP_TAB, "cur = %s.\n", cur);
    //cur += strlen(opstr);
    cur = op_end;
    i++;
    DYN_DBG3(PEEP_TAB, "cur = %s.\n", cur);
    if (*cur == '\0') {
      break;
    }
  }
}



bool
peep_preprocess_using_regmap(translation_instance_t *ti, char *line,
    regmap_t const *regmap, vconstants_t *vconstants,
    char * (*exregname_suffix_fn)(int, int, char const *, char *, size_t))
{
  autostop_timer func_timer(__func__);
  if (!peep_preprocess_exvregs(line, regmap, exregname_suffix_fn)) {
    DYN_DBG(CMN_ISEQ_FROM_STRING, "peep_preprocess_exvregs returned false, line: %s\n", line);
    return false;
  }

  DYN_DBG(CMN_ISEQ_FROM_STRING, "before peep_preprocess_all_constants, line: %s\n", line);
  peep_preprocess_all_constants(ti, line, vconstants, MAX_NUM_VCONSTANTS_PER_INSN);
  peep_preprocess_scan_label(line, "JUMPTABLE_ADDR", SRC_ENV_JUMPTABLE_ADDR);
  peep_preprocess_scan_label(line, "JUMPTABLE_INTERVAL", SRC_ENV_JUMPTABLE_INTERVAL);
  peep_preprocess_scan_label(line, "SCRATCH0", SRC_ENV_SCRATCH0);

  DYN_DBG(CMN_ISEQ_FROM_STRING, "after processing, line: %s\n", line);
  return true;
}


//the following function, when implemented, does not belong here
bool
peep_entry_exists_for_prefix_insns(peep_table_t const *peep_table,
    src_insn_t const *iseq, long iseq_len)
{
  return true; //XXX : this is missing out on important sequences, so returning true currently (till this is debugged)
  /* Organize the peeptab as a trie of the instructions, and check if the prefix
     of the proposed instruction sequence exists. Using rbtree currently. */

  peep_entry_t const *upper_bound;
  struct rbtree_elem const *found;
  peep_entry_t needle;
  long i;

  if (!peep_table) {
    return true;
  }

  needle.src_iseq = (src_insn_t *)iseq;
  needle.src_iseq_len = iseq_len;

  found = rbtree_upper_bound(&peep_table->rbtree, &needle.rb_elem);
  if (!found) {
    return false;
  }
  upper_bound = rbtree_entry(found, peep_entry_t, rb_elem);
  //ASSERT(peep_entry_src_iseq_less(&needle.rb_elem, &upper_bound->rb_elem, NULL));
  for (i = 0; i < iseq_len; i++) {
    if (src_insn_less(&iseq[i], &upper_bound->src_iseq[i])) {
      return false;
    }
    ASSERT(!src_insn_less(&upper_bound->src_iseq[i], &iseq[i]));
  }
  return true;
}

