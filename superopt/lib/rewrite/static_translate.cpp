#include <stdarg.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <elfio/elfio.hpp>

using namespace std;
using namespace ELFIO;

#include "eq/parse_input_eq_file.h"
#include "sym_exec/sym_exec.h"
//#include "rewrite/harvest.h"
#include "support/debug.h"
#include "support/cmd_helper.h"
#include "support/cartesian.h"
#include "config-host.h"
#include "superopt/fallback.h"
#include "rewrite/bbl.h"
#include "insn/cfg.h"
#include "rewrite/pred.h"
#include "insn/jumptable.h"
#include "gas/disas.h"

#include "rewrite/static_translate.h"
#include "superopt/src_env.h"

#include "cutils/pc_elem.h"
#include "cutils/addr_range.h"
#include "support/c_utils.h"
#include "valtag/transmap.h"
#include "rewrite/peephole.h"
#include "cutils/chash.h"
#include "cutils/imap_elem.h"

#include "insn/live_ranges.h"
#include "ppc/regs.h"
#include "insn/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "i386/disas.h"
//#include "regmap.h"
#include "valtag/regcons.h"
#include "valtag/nextpc.h"

#include "rewrite/transmap_set.h"
#include "ppc/disas.h"
#include "cutils/rbtree.h"
#include "cutils/interval_set.h"
#include "rewrite/dwarf.h"
#include "insn/edge_table.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"

#include "support/timers.h"
#include "valtag/ldst_input.h"
#include "rewrite/analyze_stack.h"
#include "rewrite/unroll_loops.h"
//#include "insn_cache.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "valtag/regcount.h"
#include "insn/fun_type_info.h"
#include "rewrite/txinsn.h"
#include "rewrite/dst_insn_folding_opts.h"
#include "rewrite/tail_calls_set.h"

extern FILE *logfile;

#define TLS_SECTION_ADDR 0x00009000

#include "expr/expr.h"
#include "graph/graph.h"
#include "tfg/tfg_llvm.h"
#include "rewrite/cfg_pc.h"
#include "gsupport/rodata_map.h"
#include "rewrite/alloc_constraints.h"

//static uint8_t code_gen_buffer[CODE_GEN_BUFFER_SIZE] __attribute__((aligned (32)));
//#define SETUP_BUF_SIZE ((sizeof setup_instructions) + 32)
//uint8_t *code_gen_start = NULL;

/*static void dfs_find_bbl_leaders(input_exec_t *in, src_ulong nip,
    struct interval_set *translated_addrs);
static void compute_flow(input_exec_t *in);
static void compute_liveness(input_exec_t *in);
static void compute_live_ranges(input_exec_t *in, int max_iter);
static void tbs_output_debug_info(translation_instance_t *ti);*/

#define TARGET_READ_UINT32(data) ({ \
  char const *_ptr = (char *)(data); \
  char bswap_ptr[4]; \
  bswap_ptr[0] = _ptr[3]; \
  bswap_ptr[1] = _ptr[2]; \
  bswap_ptr[2] = _ptr[1]; \
  bswap_ptr[3] = _ptr[0]; \
  *(uint32_t *)bswap_ptr; \
})
#define TARGET_READ_UINT16(data) ({ \
  char const *_ptr = (data); \
  char bswap_ptr[2]; \
  bswap_ptr[0] = _ptr[1]; \
  bswap_ptr[1] = _ptr[0]; \
  *(uint16_t *)bswap_ptr; \
})
#define TARGET_READ_UINT8(data) ({ \
  char const *_ptr = (data); \
  *(uint8_t *)_ptr; \
})

#define TARGET_READ_INT8(data) ({ \
  char const *_ptr = (data); \
  *(int8_t *)_ptr; \
})

#define READ_LEB128(data) ({ \
  char const *_ptr = (data); \
  int32_t _ret = 0; \
  int _nbytes = 0; \
  do { \
    _ret += ((*_ptr) & 0x7f)<<(7*_nbytes); \
    _nbytes++; \
  } while ((*(_ptr++))&0x80); \
  if (7*_nbytes < 32) { \
    /* sign extend */ \
    _ret = (_ret << (32 - 7*_nbytes)) >> (32 - 7*_nbytes); \
  } \
  (data) += _nbytes; \
  _ret;\
})

static char as1[4096000];
static char as2[40960];
static char as3[4096];
static char as4[4096];
uint8_t bin1[4096];

int preserve_instruction_order = 0;
int aggressive_assumptions_about_flags = 1;
//bool use_fallback_translation_only = false;
int static_link_flag = 0;
char const *superoptdb_server = NULL;

/*#ifndef NDEBUG
namespace eqspace {
template<> long long node<cfg_pc>::m_num_allocated_node = 0;
template<> long long edge<cfg_pc>::m_num_allocated_edge = 0;
template<> long long node<pc>::m_num_allocated_node = 0;
template<> long long edge<pc>::m_num_allocated_edge = 0;
template<> long long node<int_pc>::m_num_allocated_node = 0;
template<> long long edge<int_pc>::m_num_allocated_edge = 0;
template<> long long node<pcpair>::m_num_allocated_node = 0;
template<> long long edge<pcpair>::m_num_allocated_edge = 0;
}
#endif*/


/* instructions to set up the executable binary */
unsigned char setup_instructions[] = {
#if ARCH_SRC == ARCH_PPC
  0xb9, 0x01, 0x00, 0x00, 0x00,       /* mov    $0x1,%ecx */
  0x89, 0xe2,                         /* mov    %esp,%edx */
  0x8b, 0x02,                         /* mov    (%edx),%eax */
  0x0f, 0xc8,                         /* bswap  %eax */
  0x89, 0x02,                         /* mov    %eax,(%edx) */
  0x83, 0xc2, 0x04,                      /* add    $0x4,%edx */
  0x85, 0xc9,                         /* test   %ecx,%ecx */
  0x89, 0xc1,                         /* mov    %eax,%ecx */
  0x75, 0xf1,                         /* jne    12*/
  0x85, 0xc0,                         /* test   %eax,%eax */
  0x75, 0xed,                         /* jne    12 */
  //0x83, 0xc2, 0x04,                     /* add    $0x4,%edx */

  0x89, 0xe0,                         /* mov    %esp,%eax */
  0xbb, 0x00, 0x00, 0x00, 0x00,              /* mov    $0x0,%ebx */
  0x83, 0xc0, 0x04,                      /* add    $0x4,%eax */
  0x8b, 0x08,                         /* mov    (%eax),%ecx */
  0x39, 0xd1,                         /* cmp    %edx,%ecx */
  0x7e, 0x06,                         /* jle    3e <.IGNORE> */
  0x39, 0xd9,                         /* cmp    %ebx,%ecx */
  0x7d, 0x02,                         /* jge    3e <.IGNORE> */
  0x89, 0xcb,                         /* mov    %ecx,%ebx */
  0x39, 0xd0,                         /* cmp    %edx,%eax */
  0x7e, 0xed,                         /* jle    2f <.IGNORE-0xf> */

  //0x29, 0xd3,                         /* sub    %edx,%ebx */
  //0x83, 0xfb, 0x24,                      /* cmp    $0x24,%ebx */
  //0x7e, 0x39,                         /* jle    73 <.ERROR> */
  0xc7, 0x42, 0xf8, 0x00, 0x00, 0x00, 0x13, /* movl   $0x13000000,0xfffffff8(%edx) */
  0xc7, 0x42, 0xfc, 0x00, 0x00, 0x00, 0x20, /* movl   $0x20000000,0xfffffffc(%edx) */
  0xc7, 0x02, 0x00, 0x00, 0x00, 0x14,       /* movl   $0x14000000,(%edx) */
  0xc7, 0x42, 0x04, 0x00, 0x00, 0x00, 0x20, /* movl   $0x20000000,0x4(%edx) */
  0xc7, 0x42, 0x08, 0x00, 0x00, 0x00, 0x15, /* movl   $0x15000000,0x8(%edx) */
  0xc7, 0x42, 0x0c, 0x00, 0x00, 0x00, 0x00, /* movl   $0x0,0xc(%edx) */
  0xc7, 0x42, 0x10, 0x00, 0x00, 0x00, 0x00, /* movl   $0x0,0x10(%edx) */
  0xc7, 0x42, 0x14, 0x00, 0x00, 0x00, 0x00, /* movl   $0x0,0x14(%edx) */
  0xeb, 0x05,                         /* jmp    87 <.DONE> */
  
  0xa3, 0x00, 0x00, 0x00, 0x00,             /* .ERROR: mov    %eax,0x0 */

#endif
  0x31, 0xc0,                         /* .DONE: xor    %eax,%eax */
  0x31, 0xc9,                         /* xor    %ecx,%ecx */
  0x31, 0xd2,                         /* xor    %edx,%edx */
  0x31, 0xdb,                         /* xor    %ebx,%ebx */
};


#define MAX_TEXTSIZE    (CODE_GEN_BUFFER_SIZE)

#define MAX_JUMPTABLE_STUB_CODESIZE (1ULL<<23)

#define PAGE_SIZE (1<<12)
#define SRC_ID_STRING "ARCH_SRC"

static void
output_exec_init(output_exec_t *out, char const *name)
{
  out->num_helper_sections = 0;
  out->name = name;

  out->generated_relocs = NULL;
  out->generated_relocs_size = 0;
  out->num_generated_relocs = 0;

  out->generated_symbols = NULL;
  out->generated_symbols_size = 0;
  out->num_generated_symbols = 0;
}



#if 0
void
tb_alloc_offsets(tb_t *tb, int n)
{
  tb->tb_jmp_offset = n?(long *)malloc(n * sizeof(long)):NULL;
  tb->tb_edge_select_offset = (n > 1)?
    (long *)malloc((n - 1) * sizeof(long)):NULL;
  tb->tb_edge_offset = n?(long *)malloc(n * sizeof(long)):NULL;
  tb->tb_jmp_insn_offset = n?(long *)malloc(n * sizeof(long)):NULL;
  ASSERT(!n || tb->tb_jmp_offset);
  ASSERT(!n || tb->tb_edge_offset);
  ASSERT(!n || tb->tb_jmp_insn_offset);
  int i;
  for (i = 0; i < n; i++) {
    tb->tb_jmp_offset[i] = -1;
    tb->tb_edge_offset[i] = -1;
    tb->tb_jmp_insn_offset[i] = -1;
  }
}

void
tb_free_offsets(tb_t *tb)
{
  free(tb->tb_jmp_offset);
  free(tb->tb_edge_select_offset);
  free(tb->tb_edge_offset);
  free(tb->tb_jmp_insn_offset);
}
#endif

static void
dyn_entry_read_string (elfio &pReader,
    char *ret_name, int ret_name_size, Elf32_Sword tag, Elf32_Word value)
{
  int i;
  ret_name[0] = '\0';
  switch (tag) {
    case DT_NEEDED:
      for (i = 0; i < pReader.sections.size(); i++) {
        section *sec;
        sec = pReader.sections[i];
        if (sec->get_name() == ".dynstr") {
          char const *strings = sec->get_data();
          ASSERT(sec->get_size() > value);
          strncpy(ret_name, strings + value, ret_name_size);
          ASSERT(strlen(ret_name) < ret_name_size);
          return;
        }
      }
      ASSERT(0);
      break;
    default:
      break;
  }
}


static void
init_helper_section(elfio &pELFO, section *pHelperSections[], int i,
    char const *name, int type, int flags)
{
  pHelperSections[i] = pELFO.sections.add(name);
  pHelperSections[i]->set_type(type);
  pHelperSections[i]->set_flags(flags);
  pHelperSections[i]->set_addr_align(0x1);
  pHelperSections[i]->set_info(0);
  pHelperSections[i]->set_entry_size(0);
}

static void
read_env_section(output_exec_t *out)
{
  static char env[SRC_ENV_SIZE + SRC_ENV_PREFIX_SPACE];
  memset(&env, 0x0, sizeof env);  /* fill it with zeros */
  out->env_section.name = (char *)".src_env";
  out->env_section.data = (char *)&env;
  out->env_section.size = sizeof env;
  out->env_section.addr = SRC_ENV_ADDR - SRC_ENV_PREFIX_SPACE;
}

static void
init_env_section(output_exec_t *out, elfio &pELFO, section **pEnvSection)
{
  section *ret;

  ret = pELFO.sections.add( ".src_env");
  ret->set_type(SHT_PROGBITS);
  ret->set_flags(SHF_ALLOC | SHF_WRITE);
  ret->set_addr_align(0x1);
  ret->set_info(0);
  ret->set_entry_size(0);
  *pEnvSection = ret;
}

 
static void
init_symtab(elfio &pELFO, string_section_accessor **pStrWriter,
    symbol_section_accessor **pSymWriter, section **pSymSec, unsigned char exec_type)
{
  section *symsec;

  // Create string table section
  section* pStrSec = pELFO.sections.add(".strtab");
  ASSERT(pStrSec);
  pStrSec->set_type(SHT_STRTAB);
  pStrSec->set_flags(0);
  pStrSec->set_info(0);
  pStrSec->set_addr_align(1);
  pStrSec->set_entry_size(0);
  // Create string table writer
  *pStrWriter = new string_section_accessor(pStrSec);
  //pELFO->CreateSectionWriter( IELFO::ELFO_STRING, pStrSec, (void**)pStrWriter );

  // Create symbol table section
  symsec = pELFO.sections.add(".symtab");
  ASSERT(symsec);
  symsec->set_type(SHT_SYMTAB);
  symsec->set_flags(0);
  //symsec->set_info(ELF_ST_INFO(STB_GLOBAL, STT_FUNC)); //XXX: was ELF_ST_INFO(STT_NOTYPE, STT_FUNC) for ET_EXEC, written a fix in the following if-then-else but not tested on object files
  if (exec_type == ET_EXEC) {
    symsec->set_info(ELF_ST_INFO(STB_LOCAL, STT_FUNC));
  } else {
    symsec->set_info(ELF_ST_INFO(STB_GLOBAL, STT_FUNC));
  }
  symsec->set_addr_align(4);
  symsec->set_entry_size(sizeof(ELFIO::Elf32_Sym));
  symsec->set_link(pStrSec->get_index());
  *pSymSec = symsec;

  //pStrSec->Release();

  *pSymWriter = new symbol_section_accessor(pELFO, symsec);
  //pELFO->CreateSectionWriter (IELFO::ELFO_SYMBOL, *pSymSec,
      //(void **)pSymWriter);

  (*pSymWriter)->add_symbol(**pStrWriter, ".dummy", 0,
          0, 0, 0, 0, 0); //add zero entry: the first entry in any symbol table is zero
}

static bool
input_exec_contains_plt_section(struct input_exec_t const *in)
{
  int i;
  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, ".plt")) {
      return true;
    }
  }
  return false;
}

#if 0
static void
mirror_section_into_output_exec(output_exec_t *out, elfio &pELFO, struct input_exec_t const *in, char const *section_name)
{
  input_section_t const *isec;
  int isec_index;

  isec_index = get_input_section_by_name(in, section_name);
  if (isec_index < 0) {
    return;
  }
  ASSERT(isec_index >= 0 && isec_index < in->num_input_sections);
  isec = &in->input_sections[isec_index];

  DBG(STAT_TRANS, "mirroring dynamic section %s address 0x%lx size %ld\n", section_name, (long)isec->addr, (long)isec->size);

  section *sec = pELFO.sections.add(section_name);
  sec->set_address(isec->addr);
  sec->set_type(isec->type);
  sec->set_flags(isec->flags);
  sec->set_size(isec->size);
  sec->set_info(isec->info);
  sec->set_addr_align(isec->align);
  sec->set_entry_size(isec->entry_size);
  sec->set_data(isec->data, isec->size);
/*
  // Create string table section
  section *pDynStrSec = pELFO.sections.add(".dynstr");
  pDynStrSec->set_type(SHT_STRTAB);
  pDynStrSec->set_flags(SHF_ALLOC);
  pDynStrSec->set_info(STT_NOTYPE);
  pDynStrSec->set_addr_align(1);
  pDynStrSec->set_entry_size(0);
  // Create string table writer
  pDynStrWriter = new string_section_accessor(pDynStrSec);
  //pELFO->CreateSectionWriter(IELFO::ELFO_STRING,pDynStrSec,
  //    (void**)pDynStrWriter );

  // Create symbol table section
  section* pDynSymSec = pELFO.sections.add(".dynsym");
  pDynSymSec->set_type(SHT_DYNSYM);
  pDynSymSec->set_flags(SHF_ALLOC);
  pDynSymSec->set_info(STT_FUNC);
  pDynSymSec->set_addr_align(4);
  pDynSymSec->set_entry_size(sizeof(ELFIO::Elf32_Sym));
  pDynSymSec->set_link(pDynStrSec->get_index());

  // Create dynamic section
  section *pDynamicSec = pELFO.sections.add(".ynamic");
  pDynamicSec->set_type(SHT_PROGBITS);
  pDynamicSec->set_flags(0);
  pDynamicSec->set_info(2);
  pDynamicSec->set_addr_align(4);
  pDynamicSec->set_entry_size(sizeof(ELFIO::Elf32_Dyn));
  pDynamicSec->set_link(pDynStrSec->get_index());

  //pDynStrSec->Release();

  pDynSymWriter = new symbol_section_accessor(pELFO, pDynSymSec);
  //pELFO->CreateSectionWriter (IELFO::ELFO_SYMBOL, pDynSymSec,
  //    (void **)pDynSymWriter);

  pDynamicWriter = new dynamic_section_accessor(pELFO, pDynamicSec);
  //pELFO->CreateSectionWriter (IELFO::ELFO_DYNAMIC, pDynamicSec,
  //    (void **)pDynamicWriter);
*/

  // Create interpreter section
  /*
  IELFOSection* pInterpSec = pELFO->AddSection( ".interp",
      SHT_PROGBITS,
      SHF_ALLOC,
      0,
      1,
      1 );


  char *dyn_loader = "/lib/ld-linux.so.2";
  pInterpSec->SetData(dyn_loader, strlen(dyn_loader)+1);
      */
}
#endif

static void
init_dynamic_sections(output_exec_t *out, elfio &pELFO, struct input_exec_t const *in)
{
/*
  if (!input_exec_contains_plt_section(in)) {
    return;
  }

  dynamic_section_accessor *pDynamicWriter;
  string_section_accessor *pDynStrWriter;
  symbol_section_accessor *pDynSymWriter;

  pDynStrWriter = NULL;
  pDynSymWriter = NULL;
  pDynamicWriter = NULL;

  mirror_section_into_output_exec(out, pELFO, in, ".dynstr");
  mirror_section_into_output_exec(out, pELFO, in, ".dynsym");
  mirror_section_into_output_exec(out, pELFO, in, ".dynamic");
  mirror_section_into_output_exec(out, pELFO, in, ".got");
  mirror_section_into_output_exec(out, pELFO, in, ".got.plt");
*/
}


#define DYNSYM_SEC_INDEX -2

static int
get_input_section_index(input_exec_t const *in, int orig_index)
{
  for (int i = 0; i < in->num_input_sections; i++) {
    if (in->input_sections[i].orig_index == orig_index)
      return i;
  }
  if (orig_index == SHN_UNDEF) {
    return DYNSYM_SEC_INDEX;
  }
  return -1;
}

static void
append_src_id_to_symbols(input_exec_t *in)
{
#if ARCH_SRC == ARCH_PPC
  //int i;
  for (auto &s : in->symbol_map) {
    int sec_index;

    sec_index = get_input_section_index(in, s.second.shndx);
    DYN_DBG(stat_trans, "symbol %s, %llx, shndx %hu, sec_index %d\n",
        s.second.name, (long long)s.second.value, s.second.shndx,
        sec_index);

    if (sec_index == -1) {
      continue;
    }
    DYN_DBG(stat_trans, "input_section[sec_index] %s\n",
        in->input_sections[sec_index].name);
    /*
       if (in->symbols[i].type != STT_FUNC)
       continue;
       */
    if (((sizeof s.second.name) - strlen(s.second.name))
        < strlen(SRC_ID_STRING)) {
      printf("Buffer too small to append SRC_ID_STRING %s. buffer size %d, "
          "cur_string len %d, %s\n", SRC_ID_STRING,
          sizeof s.second.name, strlen(s.second.name),
          s.second.name);
      ASSERT(0);
    }
    char *ptr = s.second.name;
    char *end = s.second.name + strlen(s.second.name) + 1;
    while (*ptr != '\0' && *ptr != '@') {
      ptr++;
    }
    memmove(ptr+3, ptr, end-ptr);
    char const *idptr = SRC_ID_STRING;
    while (*idptr != '\0') {
      *ptr++ = *idptr++;
    }
  }
#endif
}

static Elf32_Word
get_helper_section_addr(translation_instance_t *ti, char const *name)
{
  for (int i = 0; i < ti->out->num_helper_sections; i++) {
    if (!strcmp(ti->out->helper_sections[i].name, name)) {
      return ti->out->helper_sections[i].addr;
    }
  }
  return 0;
}

static void
convert_libname(char *libname, int size)
{
  char *ptr = libname;
  int shift = strlen(SRC_ID_STRING);
  char *last_fslash = libname;
  while (*ptr) {
    if (*ptr == '/') {
      last_fslash = ptr;
    }
    ptr++;
  }
  ASSERT(ptr + shift - libname <= size);
  while (ptr > last_fslash) {
    *(ptr + shift) = *ptr;
    ptr--;
  }
  ptr = last_fslash+1;
  char *id = (char *)SRC_ID_STRING;
  while (*id) {
    *ptr++ = *id++;
  }
}

static void
tcodes_add_mapping(struct chash *tcodes, src_ulong pc, uint8_t *tcode)
{
  struct imap_elem_t needle, *e;
  struct myhash_elem *found;

  needle.pc = pc;
  found = myhash_find(tcodes, &needle.h_elem);
  if (found) {
    return;
  }
  e = (struct imap_elem_t *)malloc(sizeof(imap_elem_t));
  ASSERT(e);
  e->pc = pc;
  e->val = (long)tcode;
  myhash_insert(tcodes, &e->h_elem);
}

/*static void
get_tcodes(struct chash const *tcodes, uint8_t const **next_tcodes,
    src_ulong const *nextpcs, long num_nextpcs)
{
  long i;

  for (i = 0; i < num_nextpcs; i++) {
    if (!PC_IS_NEXTPC_CONST(nextpcs[i]) && tcodes) {
      next_tcodes[i] = tcodes_query(tcodes, nextpcs[i]);
      DBG(CONNECT_ENDPOINTS, "nextpcs[%ld] = %lx, next_tcodes[%ld] = %p\n", i, (long)nextpcs[i], i, next_tcodes[i]);
    } else {
      next_tcodes[i] = (uint8_t const *)(long)nextpcs[i];
    }
  }
}*/

/*static void
get_tinsns(map<src_ulong, dst_insn_t const *> const *tinsns_map, txinsn_t *next_tinsns,
    src_ulong const *nextpcs, long num_nextpcs)
{
  for (long i = 0; i < num_nextpcs; i++) {
    if (!PC_IS_NEXTPC_CONST(nextpcs[i]) && tinsns_map) {
      //next_tcodes[i] = tcodes_query(tcodes, nextpcs[i]);
      dst_insn_t const *next_tinsn = tinsns_map->count(nextpcs[i]) ? tinsns_map->at(nextpcs[i]) : NULL;
      next_tinsns[i] = txinsn_t(tag_insn_ptr, (long)next_tinsn);
      DBG(CONNECT_ENDPOINTS, "nextpcs[%ld] = %lx, next_tinsns[%ld] = %p\n", i, (long)nextpcs[i], i, next_tinsns[i].txinsn_get_val());
    } else {
      next_tinsns[i] = txinsn_t(tag_var, (long)nextpcs[i]);
    }
  }
}*/



//#define MYTEXT_START 0x5048000U

static int
find_psection_by_name(section *pSections[], size_t num_pSections, char const *name)
{
  size_t i;
  for (i = 0; i < num_pSections; i++) {
    if (!strcmp(pSections[i]->get_name().c_str(), name)) {
      return pSections[i]->get_index();
    }
  }
  return -1;
}

static char const *
txed_isection_name(input_exec_t const *in, char const *name, char *buf, size_t size)
{
#if ARCH_SRC == ARCH_ETFG
  return name;
#endif
  if (   in->fresh
      && in->type == ET_EXEC
      //&& strcmp(name, ".got")
      && strcmp(name, ".got.plt")
      //&& strcmp(name, ".plt")
      && strcmp(name, ".dynamic")
      //&& strcmp(name, ".hash")
      && strcmp(name, ".gnu.hash")
      && strcmp(name, ".gnu.version")
      && strcmp(name, ".gnu.version_r")
      && strcmp(name, ".interp")
      && strcmp(name, ".dynstr")
      && strcmp(name, ".dynsym")) {
    snprintf(buf, size, ".txed%s", name);
    ASSERT(strlen(buf) < size);
    return buf;
  } else {
    return name;
  }
}

static long
psections_get_dynstr_addr(input_exec_t const *in, section *pSections[], long num_pSections)
{
  long i;
  for (i = 0; i < num_pSections; i++) {
    if (pSections[i]->get_name() != txed_isection_name(in, ".dynstr", as1, sizeof as1)) {
      continue;
    }
    return pSections[i]->get_address();
  }
  NOT_REACHED();
}

static void
output_dynamic_section(elfio &pELFO, input_exec_t const *in, struct chash const *tcodes, section *pSections[], long num_pSections)
{
  //input_section_t const *isec;
  //int isec_index;

  /*isec_index = get_input_section_by_name(in, ".dynamic");
  if (isec_index < 0) {
    return;
  }
  ASSERT(isec_index >= 0 && isec_index < in->num_input_sections);
  isec = &in->input_sections[isec_index];*/

  section *sec = pELFO.sections.add(".dynamic");
  //sec->set_address(isec->addr);
  sec->set_type(in->dynamic_section.type);
  sec->set_flags(in->dynamic_section.flags);
  sec->set_size(in->dynamic_section.size);
  sec->set_info(in->dynamic_section.info);
  sec->set_addr_align(in->dynamic_section.align);
  sec->set_entry_size(in->dynamic_section.entry_size);
  int dynstr_sec_index = find_psection_by_name(pSections, num_pSections, txed_isection_name(in, ".dynstr", as1, sizeof as1));
  ASSERT(dynstr_sec_index >= 0);
  sec->set_link(dynstr_sec_index);

  char data[in->dynamic_section.size];
  memcpy(data, in->dynamic_section.data, in->dynamic_section.size);
  for (char *ptr = &data[4]; ptr - data < in->dynamic_section.size; ptr += 8) {
    uint32_t new_val;
    Elf_Xword val;

    DYN_DBG(stat_trans, "ptr = %p, *(uint32_t *)ptr = %x\n", ptr, *(uint32_t *)ptr);
    val = *(uint32_t *)ptr;
    if (!get_translated_addr_using_tcodes(&new_val, tcodes, val)) {
      new_val = val;
    }
    /*if (*(uint32_t *)(ptr - 4) == DT_STRTAB) {
      new_val = psections_get_dynstr_addr(in, pSections, num_pSections);
      DBG(TMP, "new_val = %ld\n", (long)new_val);
    }*/
    *(uint32_t *)ptr = new_val;
  }
  sec->set_data(data, in->dynamic_section.size);

/*
  dynamic_section_accessor dynamic(pELFO, sec);

  for (int i = 0; i < in->num_dyn_entries; i++) {
    uint8_t const *txed_val;
    Elf_Xword val;

    val = in->dyn_entries[i].value;
    txed_val = tcodes_query(tcodes, val);
    DBG(TMP, "val = 0x%lx, txed_val = %p\n", (long)val, txed_val);
    if (!txed_val) {
      txed_val = (uint8_t const *)val;
    }

    DBG(TMP, "val = 0x%lx, txed_val = %p\n", (long)val, txed_val);
    DBG(TMP, "Outputting dynamic section entry %d: %lx (val %lx)\n", i, (long)txed_val, val);
    //dynamic.add_entry(in->dyn_entries[i].tag, (Elf_Xword &)txed_val);
  }
*/
}

#if 0
static void
fill_dynamic_section (IELFOStringWriter *pDynStrWriter,
    IELFODynamicWriter *pDynamicWriter, translation_instance_t *ti)
{
  output_exec_t *out = ti->out;

  for (int i = 0; i < out->num_dyn_entries; i++)
  {
    if (out->dyn_entries[i].tag == DT_NEEDED)
    {
      convert_libname(out->dyn_entries[i].name,sizeof out->dyn_entries[i].name);
      Elf32_Word off = pDynStrWriter->AddString (out->dyn_entries[i].name);
      pDynamicWriter->AddEntry (out->dyn_entries[i].tag, off);
    } else if (out->dyn_entries[i].tag == DT_PLTGOT) {
      Elf32_Word addr = get_helper_section_addr (ti, ".pltgot");
      ASSERT(addr != 0);
      pDynamicWriter->AddEntry (out->dyn_entries[i].tag, addr);
    } else if (out->dyn_entries[i].tag == DT_STRTAB) {
      /*
      Elf32_Word addr = get_helper_section_addr (ti, ".dynstr");
      ASSERT(addr != 0);
      pDynamicWriter->AddEntry (out->dyn_entries[i].tag, addr);
      */
    } else if (out->dyn_entries[i].tag == DT_SYMTAB) {
      /*
      Elf32_Word addr = get_helper_section_addr (ti, ".dynsym");
      ASSERT(addr != 0);
      pDynamicWriter->AddEntry (out->dyn_entries[i].tag, addr);
      */
    }
    else
    {
      pDynamicWriter->AddEntry (out->dyn_entries[i].tag,
    out->dyn_entries[i].value);
    }
  }
}

static void
fill_dynsym (IELFOStringWriter *pDynStrWriter, IELFOSymbolTable *pDynSymWriter,
    IELFOSection *pTextSec, IELFOSection **pSections,
    translation_instance_t *ti)
{
  output_exec_t *out = ti->out;
  for (int i = 0; i < out->num_symbols; i++)
  {
    int sec_index = get_input_section_index(out, out->symbols[i].shndx);
    DBG (STAT_TRANS, "symbol %s, %lx, shndx %hu, sec_index %d\n",
  out->symbols[i].name, out->symbols[i].value, out->symbols[i].shndx,
  sec_index);

    if (sec_index != DYNSYM_SEC_INDEX)
    {
      continue;
    }

    DBG (STAT_TRANS, "symbol %s, %lx, shndx %hu, sec_index %d\n",
  out->symbols[i].name, out->symbols[i].value, out->symbols[i].shndx,
  sec_index);

    pDynSymWriter->add_symbol( pDynStrWriter,
  out->symbols[i].name,
  out->symbols[i].value,
  out->symbols[i].size,
  (out->symbols[i].bind == STB_GLOBAL)?STB_WEAK:out->symbols[i].bind,
  out->symbols[i].type,
  0,
  SHN_UNDEF);
  }
}
#endif

static bool
ti_symbol_exists(translation_instance_t *ti, char *name, size_t name_size,
    long val, char const *section_name)
{
  long i, section_name_index;
  output_exec_t *out;

  out = ti->out;

  section_name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings,
      &ti->reloc_strings_size, section_name);

  for (i = 0; i < out->num_generated_symbols; i++) {
    if (   out->generated_symbols[i].symbol_value == val
        && out->generated_symbols[i].section_name_index == section_name_index) {
      strncpy(name, &ti->reloc_strings[out->generated_symbols[i].symbol_index],
          name_size);
      ASSERT(strlen(name) < name_size);
      return true;
    }
  }

  return false;
}

static void
ti_gen_symbol(translation_instance_t *ti, char const *name, long val,
    long size, unsigned char bind, unsigned char type, char const *section_name,
    unsigned char other)
{
  long i, name_index, section_name_index;
  input_exec_t const *in;
  output_exec_t *out;

  if (*name == '\0') {
    return;
  }

  out = ti->out;
  in = ti->in;
  name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings,
      &ti->reloc_strings_size, name);
  if (!strcmp(section_name, "null")) {
    section_name_index = -1;
  } else {
    section_name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings,
        &ti->reloc_strings_size, section_name);
  }

  /*char name2[strlen(name) + 64];
  if (   in->fresh
      && name_index == section_name_index) {
    snprintf(name2, sizeof name2, "%s.sec_start", name);
    name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings,
        &ti->reloc_strings_size, name2);
  } else {
    snprintf(name2, sizeof name2, "%s", section_name);
  }*/

  /*for (i = 0; i < out->num_generated_symbols; i++) {
    ASSERT(out->generated_symbols[i].symbol_index != name_index
        || out->generated_symbols[i].symbol_value != val
        || out->generated_symbols[i].section_name_index != section_name_index);
  }*/
  i = out->num_generated_symbols++;
  if (out->num_generated_symbols > out->generated_symbols_size) {
    out->generated_symbols_size = (out->generated_symbols_size < 16)?16:
      2 * out->generated_symbols_size;
    out->generated_symbols = (generated_symbol_t *)realloc(out->generated_symbols,
        out->generated_symbols_size * sizeof(generated_symbol_t));
    ASSERT(out->generated_symbols);
  }
  DYN_DBG(stat_trans, "Adding symbol[%ld] %s to %ld, section %s\n", i, name, name_index,
      section_name);
  ASSERT(i < out->generated_symbols_size);
  out->generated_symbols[i].symbol_index = name_index;
  out->generated_symbols[i].symbol_value = val;
  out->generated_symbols[i].size = size;
  out->generated_symbols[i].bind = bind;
  out->generated_symbols[i].type = type;
  out->generated_symbols[i].section_name_index = section_name_index;
  out->generated_symbols[i].other = other;
}

static long
get_txed_code_address(input_exec_t const *in, long addr, int *offset)
{
  long val;
  int off;
 
  for (off = 0; off < 16; off++) {
    val = pc2index(in, addr - off);
    if (val >= 0 && val < in->num_si) {
      long symval;
      symval = (long)in->si[val].tc_ptr - (long)TX_TEXT_START_PTR/*START_VMA*/;
      *offset = off;
      return symval;
    }
  }
  return INSN_PC_INVALID;
}

static void
generate_symbols_using_input_exec(translation_instance_t *ti)
{
  input_exec_t const *in = ti->in;
  //output_exec_t *out = ti->out;
  /*Elf32_Word nSymIndex;
  nSymIndex = pSymWriter.add_symbol(pStrWriter,
      "mytext_start",
      0x0,
      0,
      STB_WEAK,
      STT_NOTYPE,
      0,
      pTextSec->get_index());*/

  ti_gen_symbol(ti, MYTEXT_SECTION_NAME, 0, 0/*pTextSec->get_size()*/, STB_LOCAL, STT_SECTION, MYTEXT_SECTION_NAME, 0);  //order of symbol generation matter! see https://blogs.oracle.com/solaris/inside-elf-symbol-tables-v2


  for (const auto &s : in->symbol_map) {
    /*ASSERT(!ti_symbol_exists(ti, in->symbols[i].name, sizeof in->symbols[i].name,
         in->symbols[i].value, ".mytext"));*/
    int sec_index = get_input_section_index(in, s.second.shndx);
    DYN_DBG(stat_trans, "symbol %s, %lx, shndx %hu, sec_index %d\n",
        s.second.name, (long)s.second.value, s.second.shndx,
        sec_index);

    DYN_DBG(stat_trans, "input_section[%d]=%s\n",
        sec_index, (sec_index >= 0)?in->input_sections[sec_index].name:"null");
    if (sec_index == DYNSYM_SEC_INDEX) {
      continue;
    }
    // if (in->symbols[i].type != STT_FUNC) continue;
    if (   sec_index >= 0
        && is_code_section(&in->input_sections[sec_index])) {
      long txed_val;
      int offset;
      txed_val = get_txed_code_address(in, s.second.value, &offset);
      if (txed_val != INSN_PC_INVALID && offset == 0) {
        DYN_DBG(stat_trans, "addng symbol %s to value %lx.\n", s.second.name, txed_val);

        ti_gen_symbol(ti, s.second.name, txed_val, s.second.size, s.second.bind, s.second.type, MYTEXT_SECTION_NAME, s.second.other);
        /*pSymWriter.add_symbol(pStrWriter,
            in->symbols[i].name,
            (dst_ulong)((long)in->si[val].tc_ptr - (long)START_VMA),
            in->symbols[i].size,
            (in->symbols[i].bind == STB_GLOBAL)?STB_WEAK:in->symbols[i].bind,
            in->symbols[i].type,
            0,
            pTextSec->get_index());*/
      } else {
        DYN_DEBUG(stat_trans, printf("Warning: symbol '%s' value %lx not found in code region\n",
            s.second.name, (long)s.second.value));
      }
    } else {
      /*int j, psec_index = -1;
      for (j = 0; j < num_pSections; j++) {
        if (pSections[j]->get_name()
            == txed_isection_name(in, in->input_sections[sec_index].name,
              as1, sizeof as1)) {
          psec_index = j;
          break;
        }
      }*/
      //if (psec_index != -1) {
        /*pSymWriter.add_symbol(pStrWriter,
            in->symbols[i].name,
            (sec_index == DYNSYM_SEC_INDEX)?in->symbols[i].value:
                in->symbols[i].value - in->input_sections[sec_index].addr,
            in->symbols[i].size,
            (in->symbols[i].bind == STB_GLOBAL)?STB_WEAK:in->symbols[i].bind,
            in->symbols[i].type,
            0,
            (sec_index==DYNSYM_SEC_INDEX)?
            SHN_UNDEF:pSections[psec_index]->get_index());*/
        long symval;
        symval = (sec_index == DYNSYM_SEC_INDEX)?s.second.value:
                s.second.value - in->input_sections[sec_index].addr;

        DYN_DBG(stat_trans, "addng symbol %s to value %lx.\n", s.second.name, symval);
        ti_gen_symbol(ti, s.second.name, symval, s.second.size,
           s.second.bind,
           s.second.type,
           (sec_index == -1)?"null":txed_isection_name(in, in->input_sections[sec_index].name, as1, sizeof as1),
           s.second.other);
      //}
    }
  }
}

/*static void
fill_symtab(string_section_accessor &pStrWriter, symbol_section_accessor &pSymWriter,
    section *pTextSec, section **pSections, int num_pSections,
    uint8_t const *code_gen_buffer, translation_instance_t *ti)
{
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;
  Elf32_Word nSymIndex;
  nSymIndex = pSymWriter.add_symbol(pStrWriter,
      "mytext_start",
      0x0,
      0,
      STB_WEAK,
      STT_NOTYPE,
      0,
      pTextSec->get_index());

  for (int i = 0; i < in->num_symbols; i++) {
    int sec_index = get_input_section_index(in, in->symbols[i].shndx);
    DBG(STAT_TRANS, "symbol %s, %lx, shndx %hu, sec_index %d\n",
        in->symbols[i].name, (long)in->symbols[i].value, in->symbols[i].shndx,
        sec_index);

    if (sec_index < 0) {
      continue;
    }
    DBG (STAT_TRANS, "input_section[%d]=%s\n",
        sec_index, in->input_sections[sec_index].name);
    // if (in->symbols[i].type != STT_FUNC) continue;
    if (is_code_section(&in->input_sections[sec_index])) {
      long val;

      val = pc2index(in, in->symbols[i].value);

      if (val >= 0 && val < in->num_si) {
        pSymWriter.add_symbol(pStrWriter,
            in->symbols[i].name,
            (dst_ulong)((long)in->si[val].tc_ptr - (long)START_VMA),
            in->symbols[i].size,
            (in->symbols[i].bind == STB_GLOBAL)?STB_WEAK:in->symbols[i].bind,
            in->symbols[i].type,
            0,
            pTextSec->get_index());
      } else {
        printf("Warning: symbol '%s' value %lx not found in code region\n",
            in->symbols[i].name, (long)in->symbols[i].value);
      }
    } else {
      int j, psec_index = -1;
      for (j = 0; j < num_pSections; j++) {
        if (pSections[j]->get_name()
            == txed_isection_name(in, in->input_sections[sec_index].name,
              as1, sizeof as1)) {
          psec_index = j;
          break;
        }
      }
      if (psec_index != -1) {
        pSymWriter.add_symbol(pStrWriter,
            in->symbols[i].name,
            (sec_index == DYNSYM_SEC_INDEX)?in->symbols[i].value:
                in->symbols[i].value - in->input_sections[sec_index].addr,
            in->symbols[i].size,
            (in->symbols[i].bind == STB_GLOBAL)?STB_WEAK:in->symbols[i].bind,
            in->symbols[i].type,
            0,
            (sec_index==DYNSYM_SEC_INDEX)?
            SHN_UNDEF:pSections[psec_index]->get_index());
      }
    }
  }
}*/

static void
init_helper_sections(output_exec_t *out, elfio &pELFO, section *pHelperSections[])
{
  for (int i = 0; i < out->num_helper_sections; i++) {
    init_helper_section(pELFO, pHelperSections, i,
        out->helper_sections[i].name, out->helper_sections[i].type,
        out->helper_sections[i].flags);
  }
}

static int
init_input_sections(input_exec_t const *in, elfio &pELFO, section *pSections[], int pSection_to_input_section_map[])
{
  int ret = 0;

  for (int i = 0; i < in->num_input_sections; i++) {
    //bool retain_original_name_and_flags = false;
    /*ASSERT(   strcmp(in->input_sections[i].name, ".plt")     //plt sections should not be in input_sections array
           && strcmp(in->input_sections[i].name, ".rel.plt")
           && strcmp(in->input_sections[i].name, ".got.plt"));*/

    /*if (strstart(in->input_sections[i].name, ".rel.", NULL)) {
      //retain_original_name_and_flags = true;
      continue;
    }*/
    if (!in->fresh) {
      if (   !strcmp(in->input_sections[i].name, ".jumptable_stub_code")
          || !strcmp(in->input_sections[i].name, MYTEXT_SECTION_NAME)
          || !strcmp(in->input_sections[i].name, ".srctext")
          || !strcmp(in->input_sections[i].name, ".plt")
          || !strcmp(in->input_sections[i].name, ".eh_frame")
          || !strcmp(in->input_sections[i].name, ".dbg_functions")) {
        continue;
      }
    }
#if ARCH_SRC == ARCH_ETFG
    if (   !strcmp(in->input_sections[i].name, MYTEXT_SECTION_NAME)
        || !strcmp(in->input_sections[i].name, ".srctext")) {
      continue;
    }
#endif
    DYN_DBG(stat_trans, "Outputting input section %s.\n", in->input_sections[i].name);
    Elf32_Word type = in->input_sections[i].type;
    char name[strlen(in->input_sections[i].name) + 8];
    Elf32_Word flags;
    flags = in->input_sections[i].flags;
    /*if (retain_original_name_and_flags) {
      snprintf(name, sizeof name, "%s", in->input_sections[i].name);
    } else {*/
      snprintf(name, sizeof name, "%s",
          txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1));
      flags = flags & ~SHF_EXECINSTR;
    //}
    pSections[ret] = pELFO.sections.add(name);
    pSections[ret]->set_type(type);
    pSections[ret]->set_flags(flags);
    pSections[ret]->set_info(in->input_sections[i].info);
    pSections[ret]->set_addr_align(in->input_sections[i].align);
    pSections[ret]->set_entry_size(in->input_sections[i].entry_size);
    pSection_to_input_section_map[ret] = i;
    ret++;
  }
  return ret;
}

static section * 
find_jumptable_stubcode_section(section **pHelperSections, int n)
{
  for (int i = 0; i < n; i++) {
    if (pHelperSections[i]->get_name() == ".jumptable_stub_code") {
      return pHelperSections[i];
    }
  }
  return NULL;
}

static void
ti_get_symbol_value(translation_instance_t const *ti, char const *name,
    dst_ulong *symbol_value, dst_ulong *section_name_index)
{
  output_exec_t const *out;
  long name_index, i;

  out = ti->out;
  name_index = add_or_find_reloc_symbol_name((char **)&ti->reloc_strings,
      (long *)&ti->reloc_strings_size, name);

  for (i = 0; i < out->num_generated_symbols; i++) {
    if (out->generated_symbols[i].symbol_index == name_index) {
      *symbol_value = out->generated_symbols[i].symbol_value;
      if (section_name_index) {
        *section_name_index = out->generated_symbols[i].section_name_index;
      }
      return;
    }
  }
  NOT_REACHED();
}

static int
convert_input_section_index_to_psection_index(int shndx, input_exec_t const *in,
    section * const pSections[], size_t num_pSections)
{
  char const *name, *remaining;
  size_t i;
  ASSERT(shndx >= 0 && shndx < in->num_input_sections);
  for (i = 0; i < num_pSections; i++) {
    name = in->input_sections[shndx].name;
    if (pSections[i]->get_name() == name) {
      return i;
    }
    if (   strstart(name, ".rel", &remaining)
        && (pSections[i]->get_name() == remaining)) {
      return i;
    }
  }
  return -1;
}

static void
dump_text_section(section *pTextSec, uint8_t const *code_gen_buffer, size_t codesize)
{
  static char text_store[MAX_TEXTSIZE];
  char *text = text_store + PAGE_SIZE;
  char *tptr = text;

#if 0
  memcpy(tptr, setup_instructions, sizeof setup_instructions);
  tptr += sizeof setup_instructions;

  /* The following three instructions place env in $ebp and $esp in env->gpr[1], before
     setting up our local stack in $esp. */
  // mov $SRC_ENV_ADDR, %ebp
  //tptr += mov_src_env_addr_to_ebp(tptr, tend - tptr);
#if SRC_FALLBACK == FALLBACK_QEMU
  *tptr++ = 0xbd;  *(uint32_t *)tptr = SRC_ENV_ADDR;  tptr += 4;
  // mov    %esp,stack_regnum*4(%ebp)[r1]
  *tptr++ = 0x89;  *tptr++ = 0x65;  *tptr++ = SRC_STACK_REGNUM * 4;
  // mov $SRC_STACK_POINTER, %esp
  *tptr++ = 0xbc;  *(uint32_t *)tptr = SRC_TMP_STACK;  tptr += 4;
#endif

  ASSERT(tptr - text <= SETUP_BUF_SIZE);
  while (tptr - text < SETUP_BUF_SIZE) {
    *tptr++ = 0x90;
  }
#endif
  memcpy(tptr, code_gen_buffer, codesize);
  tptr += codesize;

  pTextSec->set_data(text, tptr - text);
}


static void
init_reloc_section(input_exec_t *in, elfio &pELFO, section const *pTextSec,
    section const *pSymSec, relocation_section_accessor **pRelWriter)
{
  char *secname;
  size_t secname_len;

  secname_len = pTextSec->get_name().size() + 8;
  secname = new char[secname_len];
  ASSERT(secname);
  snprintf(secname, secname_len, ".rel%s", pTextSec->get_name().c_str());

  section *pRelSec = pELFO.sections.add(secname);
  pRelSec->set_type(SHT_REL);
  pRelSec->set_flags(0);
  pRelSec->set_info(pTextSec->get_index());
  pRelSec->set_addr_align(4);
  pRelSec->set_entry_size(sizeof(ELFIO::Elf32_Rel));
  delete[] secname;

  pRelSec->set_link(pSymSec->get_index());

  *pRelWriter = new relocation_section_accessor(pELFO, pRelSec);
}



static void
dump_elf(elfio &pELFO, translation_instance_t *ti, char const *target, uint8_t const *code_gen_buffer, size_t codesize,
    struct chash const *tcodes)
{
  section *pTextSec = NULL;
  int *pSection_to_input_section_map;
  section *pHelperSections[MAX_HELPER_SECTIONS];
  section **pSections;
  int num_pSections;
  section *pEnvSection = NULL, *pSymSec;
  string_section_accessor *pStrWriter;
  symbol_section_accessor *pSymWriter;
  relocation_section_accessor *pRelWriter, *pJTRelWriter;
  //dynamic_section_accessor *pDynamicWriter;
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;

  pSection_to_input_section_map = (int *)malloc(MAX_PSECTIONS * sizeof(int));
  ASSERT(pSection_to_input_section_map);
  pSections = (section **)malloc(MAX_PSECTIONS * sizeof(section *));
  ASSERT(pSections);

  num_pSections = init_input_sections(ti->in, pELFO, pSections, pSection_to_input_section_map);
  init_helper_sections(ti->out, pELFO, pHelperSections);
  if (in->fresh) {
    init_env_section(ti->out, pELFO, &pEnvSection);
  }
  init_symtab(pELFO, &pStrWriter, &pSymWriter, &pSymSec, in->type);
  if (in->type == ET_EXEC && in->has_dynamic_section) {
    init_dynamic_sections(ti->out, pELFO,/* &pDynStrWriter, &pDynSymWriter,
        &pDynamicWriter,*/ in);
    output_dynamic_section(pELFO, in, tcodes, pSections, num_pSections);
  }
  pTextSec = pELFO.sections.add(".TXtext"/*MYTEXT_SECTION_NAME*/);
  ASSERT(pTextSec);
  pTextSec->set_type(SHT_PROGBITS);
  pTextSec->set_flags(SHF_ALLOC | SHF_EXECINSTR);
  pTextSec->set_info(0);
  pTextSec->set_addr_align(0x1);
  pTextSec->set_entry_size(0);
  DBG(DUMP_ELF, "text section index = %d\n", pTextSec->get_index());

  init_reloc_section(ti->in, pELFO, pTextSec, pSymSec, &pRelWriter);

  section *pJTStubcodeSec = find_jumptable_stubcode_section(pHelperSections,
      out->num_helper_sections);
  if (pJTStubcodeSec) {
    init_reloc_section(ti->in, pELFO, pJTStubcodeSec, pSymSec, &pJTRelWriter);
  }

  for (int i = 0; i < num_pSections; i++) {
    int j;
    DBG(INVOKE_LINKER, "looking at pSection %d: '%s'\n", i,
        pSections[i]->get_name().c_str());
    j = pSection_to_input_section_map[i];
    /*for (j = 0; j < in->num_input_sections; j++) {
      if (pSections[i]->get_name()
          == txed_isection_name(in, in->input_sections[j].name, as1, sizeof as1)) {
        break;
      }
    }*/
    ASSERT(j >= 0 && j < in->num_input_sections);
    DBG(INVOKE_LINKER, "setting data for input section '%s'\n",
        in->input_sections[j].name);
    pSections[i]->set_data(in->input_sections[j].data, in->input_sections[j].size);
  }

  for (int i = 0; i < out->num_helper_sections; i++) {
    if (out->helper_sections[i].size == 0) {
      continue;
    }
    pHelperSections[i]->set_data(out->helper_sections[i].data,
        out->helper_sections[i].size);
  }

  if (pEnvSection) {
    pEnvSection->set_data(out->env_section.data, out->env_section.size);
  }

  dump_text_section(pTextSec, code_gen_buffer, codesize);

  /*fill_symtab(*pStrWriter, *pSymWriter, pTextSec, pSections, num_pSections,
      code_gen_buffer, ti);*/
  /* fill_dynsym (pDynStrWriter, pDynSymWriter, pTextSec, pSections, ti);
     fill_dynamic_section (pDynStrWriter, pDynamicWriter, ti); */

  //add relocation entries for dbg functions
  struct chash reloc_symbols;
  long mytext_name_index;

  myhash_init(&reloc_symbols, imap_hash_func, imap_equal_func, NULL);
  mytext_name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings, 
      &ti->reloc_strings_size, MYTEXT_SECTION_NAME);
  for (long i = 0; i < out->num_generated_symbols; i++) {
    struct imap_elem_t *imap_elem;
    unsigned char bind, type, other;
    char const *section_name;
    Elf32_Word nSymIndex;
    int sn_str_index;
    long val, size;
    char *name;

    name = ti->reloc_strings + out->generated_symbols[i].symbol_index;
    sn_str_index = out->generated_symbols[i].section_name_index;
    val = out->generated_symbols[i].symbol_value;
    size = out->generated_symbols[i].size;
    bind = out->generated_symbols[i].bind;
    type = out->generated_symbols[i].type;
    other = out->generated_symbols[i].other;

    DYN_DBG(stat_trans, "adding symbol %s to val %ld, sn_str_index %d, size %ld, bind %x\n", name,
        (long)out->generated_symbols[i].symbol_value, sn_str_index, size, (unsigned)bind);
    long shndx;
    if (sn_str_index == -1) {
      if (val != 0) {
        shndx = SHN_COMMON;
      } else {
        shndx = SHN_ABS;
      }
    } else {
      ASSERT(sn_str_index >= 0 && sn_str_index < ti->reloc_strings_size);
      section_name = &ti->reloc_strings[sn_str_index];
      //ASSERT(strcmp(name, section_name));
      shndx = find_psection_by_name(pSections, num_pSections, section_name);
      if (   shndx == -1
          && !strcmp(section_name, ".text")) {
        shndx = pTextSec->get_index();
      }
      ASSERT(in->type != ET_REL || shndx >= 0);
      if (shndx < 0) {
        printf("shndx %ld for section %s\n", shndx, section_name);
        shndx = SHN_UNDEF;
      }
      DYN_DBG(stat_trans, "shndx %ld for section_name %s.\n", shndx, section_name);
    }
    nSymIndex = pSymWriter->add_symbol(*pStrWriter, name, val, size, bind, type,
        other, shndx);
    imap_elem = (struct imap_elem_t *)malloc(sizeof *imap_elem);
    ASSERT(imap_elem);
    imap_elem->pc = out->generated_symbols[i].symbol_index;
    imap_elem->val = nSymIndex;
    myhash_insert(&reloc_symbols, &imap_elem->h_elem);
  }
  CPP_DBG_EXEC(RELOC_STRINGS, cout << __func__ << " " << __LINE__ << ": num_generated_relocs = " << out->num_generated_relocs << endl);
  for (int i = 0; i < out->num_generated_relocs; i++) {
    struct myhash_elem *found;
    struct imap_elem_t needle;
    CPP_DBG_EXEC2(RELOC_STRINGS,
        cout << __func__ << " " << __LINE__ << ": mytext_name_index = " << mytext_name_index << endl;
        cout << __func__ << " " << __LINE__ << ": out->generated_relocs[" << i << "].section_name_index = " << out->generated_relocs[i].section_name_index << endl;
    );
    if (out->generated_relocs[i].section_name_index != mytext_name_index) {
      continue;
    }
    uint32_t index = out->generated_relocs[i].symbol_index;
    Elf32_Word nSymIndex;
    needle.pc = index;
    found = myhash_find(&reloc_symbols, &needle.h_elem);
    if (found) {
      nSymIndex = chash_entry(found, imap_elem_t, h_elem)->val;
    } else {
      struct imap_elem_t *imap_elem;
      char const *name;
  
      name = ti->reloc_strings + index;
      if (strstart(name, G_LLVM_MEMCPY_FUNCTION, NULL)) {
        name = "memcpy";
      } else if (strstart(name, G_LLVM_MEMSET_FUNCTION, NULL)) {
        name = "memset";
      }
      DYN_DBG(stat_trans, "adding symbol with GLOBAL, NOTYPE %s\n", name);
      nSymIndex = pSymWriter->add_symbol(*pStrWriter, name, 0,
          0, STB_GLOBAL, STT_NOTYPE, 0, SHN_UNDEF);
      imap_elem = (struct imap_elem_t *)malloc(sizeof *imap_elem);
      ASSERT(imap_elem);
      imap_elem->pc = out->generated_relocs[i].symbol_index;
      imap_elem->val = nSymIndex;
      myhash_insert(&reloc_symbols, &imap_elem->h_elem);
    }

    dst_ulong offset;
    offset = out->generated_relocs[i].symbol_loc;
    DBG(RELOC_STRINGS, "symbol_loc=%lx, code_gen_buffer=%p\n",
        (long)out->generated_relocs[i].symbol_loc, code_gen_buffer);
    pRelWriter->add_entry(offset, nSymIndex, (unsigned char)out->generated_relocs[i].type);
  }
  myhash_destroy(&reloc_symbols, imap_elem_free);
  delete pRelWriter;

  //add relocation entries for jumptable stub code
  long jtstub_name_index;
  jtstub_name_index = add_or_find_reloc_symbol_name(&ti->reloc_strings, 
      &ti->reloc_strings_size, ".jumptable_stub_code");
  for (int i = 0; i < out->num_generated_relocs; i++) {
    if (out->generated_relocs[i].section_name_index != jtstub_name_index) {
      continue;
    }
    ASSERT(in->fresh);
    uint32_t index = out->generated_relocs[i].symbol_index;
    char *name = ti->reloc_strings + index;
    dst_ulong symbol_value;
    Elf32_Word nSymIndex;

    ti_get_symbol_value(ti, name, &symbol_value, NULL);
    DYN_DBG(stat_trans, "adding symbol %s\n", name);
    nSymIndex = pSymWriter->add_symbol(*pStrWriter, name,
        symbol_value,
        0, STB_WEAK, STT_FUNC, 0, pTextSec->get_index());
    uint32_t offset = out->generated_relocs[i].symbol_loc;
    pJTRelWriter->add_entry(offset, nSymIndex, (unsigned char)R_386_PC32);
  }
  if (pJTStubcodeSec) {
    delete pJTRelWriter;
  }

  /*for (int i = 0; i < out->num_helper_sections; i++) {
    pHelperSections[i]->Release(); pHelperSections[i] = NULL;
  }

  if (pEnvSection) {
    pEnvSection->Release(); pEnvSection = NULL;
  }

  for (int i = 0; i < num_pSections; i++) {
    if (pSections[i]) {
      pSections[i]->Release(); pSections[i] = NULL;
    }
  }*/


  DBG(DUMP_ELF, "%s", "reached end");

  delete pStrWriter;
  delete pSymWriter;
  /*if (pDynStrWriter) {
    delete pDynStrWriter;
  }
  if (pDynSymWriter) {
    delete pDynSymWriter;
  }
  if (pDynamicWriter) {
    delete pDynamicWriter;
  }*/

  // Set the program entry point
  //pELFO->SetEntry(START_VMA);
  if (!pELFO.save(target)) {
    printf("Save failed.\n");
    exit(1);
  }
  free(pSection_to_input_section_map);
  free(pSections);
  //pELFO->Release(); pELFO = NULL;
}

static bool
reloc_strings_contain_dbg_function(char const *reloc_strings, int size)
{
  char const *ptr;
  for (ptr = reloc_strings; *ptr; ptr += strlen(ptr) + 1) {
    ASSERT(ptr - reloc_strings < size);
    DYN_DBG3(stat_trans, "ptr (%p) = %s. ptr - reloc_strings = %zd, size = %d\n",
        ptr, ptr, ptr - reloc_strings, size);
    if (strstart(ptr, "__dbg_", NULL)) {
      return true;
    }
  }
  return false;
}

static void
etfg_invoke_linker(input_exec_t *in, output_exec_t *out, translation_instance_t const *ti,
    char const *target)
{
  static char command[2048];
  //char const *ldscript = ABUILD_DIR "/ld.script";
  char ldscript[1024];
  snprintf(ldscript, sizeof ldscript, "%s.ld.script", out->name);
  //FILE *ldscriptfp = fopen (ldscript, "w");
  //ASSERT(ldscriptfp);
  ofstream outfp(ldscript);
  ASSERT(outfp.is_open());

  char const *input_ldscript = SRC_DIR "/misc/etfg.ld.script";
  ifstream infp(input_ldscript);
  ASSERT(infp.is_open());

  string line;
  string marker = "  .ctors";
  while (getline(infp, line)) {
    if (line.substr(0, marker.size()) == marker) {
      for (int i = 0; i < in->num_input_sections; i++) {
        if (!strcmp(txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1), ".srctext")) {
          continue;
        }
        DBG(INVOKE_LINKER, "adding input_section: %s\n",
            txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1));
        unsigned long sec_addr;
        sec_addr = (unsigned long)in->input_sections[i].addr;
        snprintf(command, sizeof command, "  . = 0x%lx;\n", sec_addr);
        outfp << command;
        snprintf(command, sizeof command, "  %s : { %s(%s) }\n",
            txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1), target,
            txed_isection_name(in, in->input_sections[i].name, as2, sizeof as2));
        outfp << command;
      }

      DBG(INVOKE_LINKER, "adding env section at %lx: %s\n", (unsigned long)out->env_section.addr,
          out->env_section.name);
      //env section is needed because the txed code may use SRC_ENV_SCRATCH
      snprintf(command, sizeof command, "  . = 0x%lx;\n", (unsigned long)out->env_section.addr);
      outfp << command;
      snprintf(command, sizeof command, "  %s : { %s(%s) }\n",
          out->env_section.name, target, out->env_section.name);
      outfp << command;

      for (int i = 0; i < out->num_helper_sections; i++) {
        DBG(INVOKE_LINKER, "adding helper section at %lx: %s\n",
            (unsigned long)out->helper_sections[i].addr, out->helper_sections[i].name);
        snprintf(command, sizeof command, "  . = 0x%lx;\n", (unsigned long)out->helper_sections[i].addr);
        outfp << command;
        snprintf(command, sizeof command, "  %s : { %s(%s) }\n",
            out->helper_sections[i].name, target, out->helper_sections[i].name);
        outfp << command;
      }
    }
    outfp << line << endl;
  }
  outfp.close();

  snprintf(command, sizeof command, I386_LD_EXEC " -L /usr/lib32 -L " ABUILD_DIR " -m elf_i386 --dynamic-linker /lib/ld-linux.so.2 /usr/lib32/crt1.o /usr/lib32/crti.o /usr/lib32/crtn.o %s -lc -lm -lcrypt -lmymalloc -T %s -o %s", target, ldscript, out->name);
  cmd_helper_issue_command(command);
}

void
invoke_linker(input_exec_t *in, output_exec_t *out, translation_instance_t const *ti,
    char const *target)
{
  //char *ldscript = (char *)ABUILD_DIR "/ld.script";
  char ldscript[1024];
  snprintf(ldscript, sizeof ldscript, "%s.ld.script", out->name);
  //= (char *)ABUILD_DIR "/ld.script";
  FILE *ldscriptfp = fopen (ldscript, "w");
  ASSERT(ldscriptfp);

  DBG(INVOKE_LINKER, "in->fresh=%d\n", in->fresh);
#if ARCH_SRC == ARCH_I386
  fprintf(ldscriptfp, "ENTRY(start_vma)\n");
#endif
  fprintf(ldscriptfp, "SECTIONS\n{\n");
  fprintf(ldscriptfp, "/* Read-only sections, merged into text segment: */\n");
  fprintf(ldscriptfp, "/*PROVIDE (__executable_start = 0x08048000); . = 0x08048000 + SIZEOF_HEADERS;*/\n");
  //fprintf(ldscriptfp, "PROVIDE (__executable_start = 0x08048000); . = 0x08048000 + 1024;\n");
  //fprintf(ldscriptfp, "PROVIDE (__executable_start = 0x06000000); . = 0x06000000 + 0x400;\n");
  //fprintf(ldscriptfp, "PROVIDE (__executable_start = 0x%x); . = 0x%x + 0x400;\n",
  //    MYTEXT_START, MYTEXT_START);
  fprintf(ldscriptfp, "PROVIDE (__executable_start = 0x%x); . = 0x%x + 0x400;\n",
      TX_TEXT_START_ADDR, TX_TEXT_START_ADDR);
  fprintf(ldscriptfp, "start_vma = .;\n");
  fprintf(ldscriptfp, "%s : { %s(%s) }\n", MYTEXT_SECTION_NAME, target, MYTEXT_SECTION_NAME);
  fprintf(ldscriptfp, ".text           :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "*(EXCLUDE_FILE (%s) .text .stub .text.* .gnu.linkonce.t.*)\n", target);
  fprintf(ldscriptfp, "KEEP (*( EXCLUDE_FILE (%s) .text.*personality*))\n", target);
  fprintf(ldscriptfp, "/* .gnu.warning sections are handled specially by elf32.em.  */\n");
  //fprintf (ldscriptfp, "*(.gnu.warning)\n");
  fprintf(ldscriptfp, "} =0x90909090\n");
  if (   in->fresh
      || reloc_strings_contain_dbg_function(ti->reloc_strings,
             ti->reloc_strings_size)) {
    DBG(INVOKE_LINKER, "%s", "adding dbg_functions\n");
#if ARCH_SRC == ARCH_PPC
    fprintf(ldscriptfp, ".dbg_functions : { " ABUILD_DIR "/dbg_functions.o }\n");
#endif
  }
  fprintf(ldscriptfp, ".interp         : { *(EXCLUDE_FILE(%s) .interp) }\n", target);
  fprintf(ldscriptfp, ".hash           : { *(EXCLUDE_FILE(%s) .hash) }\n", target);
  //fprintf(ldscriptfp, ".dynsym         : { *(EXCLUDE_FILE(%s) .dynsym) }\n", target);
  //fprintf(ldscriptfp, ".dynstr         : { *(EXCLUDE_FILE(%s) .dynstr) }\n", target);
  //fprintf(ldscriptfp, ".gnu.version    : { *(EXCLUDE_FILE(%s) .gnu.version) }\n", target);
  //fprintf(ldscriptfp, ".gnu.version_d  : { *(EXCLUDE_FILE(%s) .gnu.version_d) }\n", target);
  //fprintf(ldscriptfp, ".gnu.version_r  : { *(EXCLUDE_FILE(%s) .gnu.version_r) }\n", target);
  fprintf(ldscriptfp, ".rel.dyn        :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.init)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.text .rel.text.* .rel.gnu.linkonce.t.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.fini)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.data.rel.ro*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.data .rel.data.* .rel.gnu.linkonce.d.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.ctors)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.dtors)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.got)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)\n", target);
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ".rela.dyn       :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.init)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.text .rela.text.* .rela.gnu.linkonce.t.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.fini)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.data .rela.data.* .rela.gnu.linkonce.d.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.ctors)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.dtors)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.got)\n", target);
  fprintf(ldscriptfp, "*(EXCLUDE_FILE(%s) .rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)\n", target);
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ".rel.plt        : { *(EXCLUDE_FILE(%s) .rel.plt) }\n", target);
  fprintf(ldscriptfp, ".rela.plt       : { *(EXCLUDE_FILE(%s) .rela.plt) }\n", target);
  fprintf(ldscriptfp, ".init           :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "KEEP (*(EXCLUDE_FILE (%s) .init))\n", target);
  fprintf(ldscriptfp, "} =0x90909090\n");
  fprintf(ldscriptfp, ".plt            : { *(EXCLUDE_FILE(%s) .plt) }\n", target);
  fprintf(ldscriptfp, ".fini           :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "KEEP (*(EXCLUDE_FILE (%s) .fini))\n", target);
  fprintf(ldscriptfp, "} =0x90909090\n");
  fprintf(ldscriptfp, "PROVIDE (__etext = .);\n");
  fprintf(ldscriptfp, "PROVIDE (_etext = .);\n");
  fprintf(ldscriptfp, "PROVIDE (etext = .);\n");
#if 1 //ARCH_SRC == ARCH_PPC
  fprintf(ldscriptfp, ".rodata         : { *(EXCLUDE_FILE(%s) .rodata .rodata.* .gnu.linkonce.r.*) }\n", target);
  fprintf(ldscriptfp, ".rodata1        : { *(.rodata1) }\n");
  fprintf(ldscriptfp, ".eh_frame_hdr : { *(.eh_frame_hdr) }\n");
  fprintf(ldscriptfp, ".eh_frame       : ONLY_IF_RO { KEEP (*(.eh_frame)) }\n");
  fprintf(ldscriptfp, ".gcc_except_table   : ONLY_IF_RO { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) }\n");
#elif ARCH_SRC == ARCH_ARM
  fprintf(ldscriptfp, ".rodata         : { *(EXCLUDE_FILE(%s) .rodata .rodata.* .gnu.linkonce.r.*) }\n", target);
  fprintf(ldscriptfp, ".rodata1        : { *(.rodata1) }\n");
  fprintf(ldscriptfp, ".eh_frame_hdr : { *(.eh_frame_hdr) }\n");
  fprintf(ldscriptfp, ".eh_frame       : ONLY_IF_RO { KEEP (*(.eh_frame)) }\n");
  fprintf(ldscriptfp, ".gcc_except_table   : ONLY_IF_RO { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) }\n");
#endif
  fprintf(ldscriptfp, "/* Adjust the address for the data segment.  We want to adjust up to\n");
  fprintf(ldscriptfp, "the same address within the page on the next page up.  */\n");
  fprintf(ldscriptfp, ". = ALIGN (0x1000) - ((0x1000 - .) & (0x1000 - 1)); . = DATA_SEGMENT_ALIGN (0x1000, 0x1000);\n");
  fprintf(ldscriptfp, "/* Exception handling  */\n");
  //fprintf(ldscriptfp, ".eh_frame       : ONLY_IF_RW { KEEP (*(.eh_frame)) }\n");
  //fprintf(ldscriptfp, ".gcc_except_table   : ONLY_IF_RW { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) }\n");
  fprintf(ldscriptfp, "/* Thread Local Storage sections  */\n");
  fprintf(ldscriptfp, ".tdata    : { *(.tdata .tdata.* .gnu.linkonce.td.*) }\n");
  //fprintf (ldscriptfp, ".tbss      : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }\n");
  fprintf(ldscriptfp, "/* Ensure the __preinit_array_start label is properly aligned.  We\n");
  fprintf(ldscriptfp, "could instead move the label definition inside the section, but\n");
  fprintf(ldscriptfp, "the linker would then create the section even if it turns out to\n");
  fprintf(ldscriptfp, "be empty, which isn't pretty.  */\n");
  fprintf(ldscriptfp, ". = ALIGN(32 / 8);\n");
  fprintf(ldscriptfp, "PROVIDE (__preinit_array_start = .);\n");
  fprintf(ldscriptfp, ".preinit_array     : { KEEP (*(.preinit_array)) }\n");
  fprintf(ldscriptfp, "PROVIDE (__preinit_array_end = .);\n");
  fprintf(ldscriptfp, "PROVIDE (__init_array_start = .);\n");
  fprintf(ldscriptfp, ".init_array     : { KEEP (*(.init_array)) }\n");
  fprintf(ldscriptfp, "PROVIDE (__init_array_end = .);\n");
  fprintf(ldscriptfp, "PROVIDE (__fini_array_start = .);\n");
  fprintf(ldscriptfp, ".fini_array     : { KEEP (*(.fini_array)) }\n");
  fprintf(ldscriptfp, "PROVIDE (__fini_array_end = .);\n");

  for (int i = 0; i < in->num_input_sections; i++) {
    DBG(INVOKE_LINKER, "adding input_section: %s\n",
        txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1));
    fprintf(ldscriptfp, "  . = 0x%lx;\n", (unsigned long)in->input_sections[i].addr);
    fprintf(ldscriptfp, "  %s : { %s(%s) }\n",
        txed_isection_name(in, in->input_sections[i].name, as1, sizeof as1), target,
        txed_isection_name(in, in->input_sections[i].name, as2, sizeof as2));
  }

  DBG(INVOKE_LINKER, "adding env section at %lx: %s\n", (unsigned long)out->env_section.addr,
      out->env_section.name);
  fprintf(ldscriptfp, "  . = 0x%lx;\n", (unsigned long)out->env_section.addr);
  fprintf(ldscriptfp, "  %s : { %s(%s) }\n",
      out->env_section.name, target, out->env_section.name);

  for (int i = 0; i < out->num_helper_sections; i++) {
    DBG(INVOKE_LINKER, "adding helper section at %lx: %s\n",
        (unsigned long)out->helper_sections[i].addr, out->helper_sections[i].name);
    fprintf(ldscriptfp, "  . = 0x%lx;\n", (unsigned long)out->helper_sections[i].addr);
    fprintf(ldscriptfp, "  %s : { %s(%s) }\n",
        out->helper_sections[i].name, target, out->helper_sections[i].name);
  }
  fprintf(ldscriptfp, "\n");
  fprintf(ldscriptfp, ".ctors          :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "/* gcc uses crtbegin.o to find the start of\n");
  fprintf(ldscriptfp, "the constructors, so we make sure it is\n");
  fprintf(ldscriptfp, "first.  Because this is a wildcard, it\n");
  fprintf(ldscriptfp, "doesn't matter if the user does not\n");
  fprintf(ldscriptfp, "actually link against crtbegin.o; the\n");
  fprintf(ldscriptfp, "linker won't look for a file to match a\n");
  fprintf(ldscriptfp, "wildcard.  The wildcard also means that it\n");
  fprintf(ldscriptfp, "doesn't matter which directory crtbegin.o\n");
  fprintf(ldscriptfp, "is in.  */\n");
  fprintf(ldscriptfp, "KEEP (*crtbegin*.o(.ctors))\n");
  fprintf(ldscriptfp, "/* We don't want to include the .ctor section from\n");
  fprintf(ldscriptfp, "from the crtend.o file until after the sorted ctors.\n");
  fprintf(ldscriptfp, "The .ctor section from the crtend file contains the\n");
  fprintf(ldscriptfp, "end of ctors marker and it must be last */\n");
  fprintf(ldscriptfp, "KEEP (*(EXCLUDE_FILE (*crtend*.o ) .ctors))\n");
  fprintf(ldscriptfp, "KEEP (*(SORT(.ctors.*)))\n");
  fprintf(ldscriptfp, "KEEP (*(.ctors))\n");
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ".dtors          :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "KEEP (*crtbegin*.o(.dtors))\n");
  fprintf(ldscriptfp, "KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))\n");
  fprintf(ldscriptfp, "KEEP (*(SORT(.dtors.*)))\n");
  fprintf(ldscriptfp, "KEEP (*(.dtors))\n");
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ".jcr            : { KEEP (*(.jcr)) }\n");
  fprintf(ldscriptfp, ".data.rel.ro : { *(.data.rel.ro.local) *(.data.rel.ro*) }\n");
  fprintf(ldscriptfp, ".dynamic        : { *(.dynamic) }\n");
  fprintf(ldscriptfp, ".got            : { *(.got) }\n");
  //fprintf(ldscriptfp, ". = DATA_SEGMENT_RELRO_END (12, .);\n");
  //fprintf(ldscriptfp, ".got.plt        : { *(.got.plt) }\n");
  fprintf(ldscriptfp, ".data           :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "*(.data .data.* .gnu.linkonce.d.*)\n");
  fprintf(ldscriptfp, "KEEP (*(.gnu.linkonce.d.*personality*))\n");
  fprintf(ldscriptfp, "SORT(CONSTRUCTORS)\n");
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ".data1          : { *(.data1) }\n");
  fprintf(ldscriptfp, "_edata = .;\n");
  fprintf(ldscriptfp, "PROVIDE (edata = .);\n");
  fprintf(ldscriptfp, "__bss_start = .;\n");
  fprintf(ldscriptfp, ".bss            :\n");
  fprintf(ldscriptfp, "{\n");
  fprintf(ldscriptfp, "*(.dynbss)\n");
  fprintf(ldscriptfp, "*(.bss .bss.* .gnu.linkonce.b.*)\n");
  fprintf(ldscriptfp, "*(COMMON)\n");
  fprintf(ldscriptfp, "/* Align here to ensure that the .bss section occupies space up to\n");
  fprintf(ldscriptfp, "_end.  Align after .bss to ensure correct alignment even if the\n");
  fprintf(ldscriptfp, ".bss section disappears because there are no input sections.  */\n");
  fprintf(ldscriptfp, ". = ALIGN(32 / 8);\n");
  fprintf(ldscriptfp, "}\n");
  fprintf(ldscriptfp, ". = ALIGN(32 / 8);\n");
  fprintf(ldscriptfp, "_end = .;\n");
  fprintf(ldscriptfp, "PROVIDE (end = .);\n");
  fprintf(ldscriptfp, ". = DATA_SEGMENT_END (.);\n");
  fprintf(ldscriptfp, "/* Stabs debugging sections.  */\n");
  fprintf(ldscriptfp, ".stab          0 : { *(.stab) }\n");
  fprintf(ldscriptfp, ".stabstr       0 : { *(.stabstr) }\n");
  fprintf(ldscriptfp, ".stab.excl     0 : { *(.stab.excl) }\n");
  fprintf(ldscriptfp, ".stab.exclstr  0 : { *(.stab.exclstr) }\n");
  fprintf(ldscriptfp, ".stab.index    0 : { *(.stab.index) }\n");
  fprintf(ldscriptfp, ".stab.indexstr 0 : { *(.stab.indexstr) }\n");
  fprintf(ldscriptfp, ".comment       0 : { *(.comment) }\n");
  fprintf(ldscriptfp, "/* DWARF debug sections.\n");
  fprintf(ldscriptfp, "Symbols in the DWARF debugging sections are relative to the beginning\n");
  fprintf(ldscriptfp, "of the section so we begin them at 0.  */\n");
  fprintf(ldscriptfp, "/* DWARF 1 */\n");
  fprintf(ldscriptfp, ".debug          0 : { *(EXCLUDE_FILE(%s) .debug) }\n", target);
  fprintf(ldscriptfp, ".line           0 : { *(EXCLUDE_FILE(%s) .line) }\n", target);
  fprintf(ldscriptfp, "/* GNU DWARF 1 extensions */\n");
  fprintf(ldscriptfp, ".debug_srcinfo  0 : { *(EXCLUDE_FILE(%s) .debug_srcinfo) }\n", target);
  fprintf(ldscriptfp, ".debug_sfnames  0 : { *(EXCLUDE_FILE(%s) .debug_sfnames) }\n", target);
  fprintf(ldscriptfp, "/* DWARF 1.1 and DWARF 2 */\n");
  fprintf(ldscriptfp, ".debug_aranges  0 : { *(EXCLUDE_FILE(%s) .debug_aranges) }\n", target);
  fprintf(ldscriptfp, ".debug_pubnames 0 : { *(EXCLUDE_FILE(%s) .debug_pubnames) }\n", target);
  fprintf(ldscriptfp, "/* DWARF 2 */\n");
  fprintf(ldscriptfp, ".debug_info     0 : { *(EXCLUDE_FILE(%s) .debug_info .gnu.linkonce.wi.*) }\n", target);
  fprintf(ldscriptfp, ".debug_abbrev   0 : { *(EXCLUDE_FILE(%s) .debug_abbrev) }\n", target);
  fprintf(ldscriptfp, ".debug_line     0 : { *(EXCLUDE_FILE(%s) .debug_line) }\n", target);
  fprintf(ldscriptfp, ".debug_frame    0 : { *(EXCLUDE_FILE(%s) .debug_frame) }\n", target);
  fprintf(ldscriptfp, ".debug_str      0 : { *(EXCLUDE_FILE(%s) .debug_str) }\n", target);
  fprintf(ldscriptfp, ".debug_loc      0 : { *(EXCLUDE_FILE(%s) .debug_loc) }\n", target);
  fprintf(ldscriptfp, ".debug_macinfo  0 : { *(EXCLUDE_FILE(%s) .debug_macinfo) }\n", target);
  fprintf(ldscriptfp, "/* SGI/MIPS DWARF 2 extensions */\n");
  fprintf(ldscriptfp, ".debug_weaknames 0 : { *(EXCLUDE_FILE(%s) .debug_weaknames) }\n", target);
  fprintf(ldscriptfp, ".debug_funcnames 0 : { *(EXCLUDE_FILE(%s) .debug_funcnames) }\n", target);
  fprintf(ldscriptfp, ".debug_typenames 0 : { *(EXCLUDE_FILE(%s) .debug_typenames) }\n", target);
  fprintf(ldscriptfp, ".debug_varnames  0 : { *(EXCLUDE_FILE(%s) .debug_varnames) }\n", target);
  fprintf(ldscriptfp, "/DISCARD/ : { *(.note.GNU-stack) }\n");
  fprintf(ldscriptfp, "}\n");
  fclose(ldscriptfp);

  char command[2048];
  if (in->type == ET_EXEC) {
    if (static_link_flag) {
      snprintf (command, sizeof command, "gcc -m32 -static -v -o %s "
          ABUILD_DIR "/linkin_main.o "
          ABUILD_DIR "/linux_syscall.o " ABUILD_DIR "/linux_thunk.o "
          ABUILD_DIR "/linux_signal.o "
          "-Wl,-T,%s/ld.script,-L" ABUILD_DIR
          ",-L" BUILD_DIR "/installs/lib32,-liberty -lm", out->name, out->name);
    } else {
      //char *flags = (char *)""; //"--verbose";
      char const *path = I386_LD_EXEC;
      bool ret;
      char ldscript[strlen(out->name) + 128];
      snprintf(ldscript, sizeof ldscript, "-T%s.ld.script", out->name);
      //char const *path = "/usr/bin/gcc";
      char const *args[] = {path, "-o", out->name, "--eh-frame-hdr", "-m",
        "elf_i386", "--dynamic-linker", "/lib32/ld-linux.so.2",
#if SRC_FALLBACK != FALLBACK_ID
        "/usr/lib32/crt1.o", "/usr/lib32/crti.o",
        GCC_LIB "/32/crtbegin.o",
        ABUILD_DIR "/linkin_main.o",
        ABUILD_DIR "/linkin_syscall.o",
        ABUILD_DIR "/dbg_functions.o",
        ABUILD_DIR "/linux_syscall.o",
        ABUILD_DIR "/linux_thunk.o",
        ABUILD_DIR "/linux_signal.o",
        "-L" GCC_LIB "/32", "-L/usr/lib32", "-L" ABUILD_DIR,
        //"-lgcc_s",
       GCC_LIB "/32/crtend.o", "/usr/lib32/crtn.o",
        QEMUFB_PATH "/" SRC_NAME "-linux-user/target-" SRC_NAME "/excp_helper.o",
        QEMUFB_PATH "/" SRC_NAME "-linux-user/target-" SRC_NAME "/int_helper.o",
        QEMUFB_PATH "/" SRC_NAME "-linux-user/target-" SRC_NAME "/mem_helper.o",
        QEMUFB_PATH "/" SRC_NAME "-linux-user/fpu/softfloat.o",
        "-lc", "-lgcc", 
#endif
        //"-T" ABUILD_DIR "/ld.script", //this must be at the end
        ldscript, //this must be at the end
        NULL};
      /*
          ABUILD_DIR "/installs/binutils-2.21-install/bin/ld -o %s"
          " %s --eh-frame-hdr -m elf_i386 --dynamic-linker /lib32/ld-linux.so.2 "
          "/usr/lib32/crt1.o /usr/lib32/crti.o "
          GCC_LIB "/32/crtbegin.o " ABUILD_DIR "/linkin_main.o "
          ABUILD_DIR "/linux/syscall.o " ABUILD_DIR "/linux/thunk.o "
          ABUILD_DIR "/linux/signal.o -T /tmp/ld.script "
          "-L" GCC_LIB "/32 -L/usr/lib32 -L" ABUILD_DIR " -lgcc_s -lc "
          "-lgcc " GCC_LIB "/32/crtend.o /usr/lib32/crtn.o",
          out->name, flags);
          */
//#elif ARCH_SRC == ARCH_ARM
      //char const *args[] = { path,"-o", out->name, "-melf_i386","-T", ABUILD_DIR "/ld.script" , 
          //ABUILD_DIR "/linkin_main.o", ABUILD_DIR "/dbg_functions.o", NULL };
      /*
          ABUILD_DIR "/installs/binutils-2.21-install/bin/ld -o %s"
          " %s --eh-frame-hdr -m elf_i386 --dynamic-linker /lib32/ld-linux.so.2 "
          "/usr/lib32/crt1.o /usr/lib32/crti.o "
          GCC_LIB "/32/crtbegin.o " ABUILD_DIR "/linkin_main.o "
          ABUILD_DIR "/linux/syscall.o " ABUILD_DIR "/linux/thunk.o "
          ABUILD_DIR "/linux/signal.o -T /tmp/ld.script "
          "-L" GCC_LIB "/32 -L/usr/lib32 -L" ABUILD_DIR " -lgcc_s -lc "
          "-lgcc " GCC_LIB "/32/crtend.o /usr/lib32/crtn.o",
          out->name, flags);
          */
//#endif
      ret = xsystem_exec(path, args, NULL);
      ASSERT(ret);
    }
  } else if (in->type == ET_DYN) {
    snprintf (command, sizeof command, "gcc -shared %s -o fiboPPC.so", target);
  } else if (in->type == ET_REL) {
    snprintf (command, sizeof command, "gcc -m32 %s", target);
    system(command);
  }
}

static int
find_or_add_jumptable_helper_section(output_exec_t *out,
    input_exec_t const *in, src_ulong src_addr, unsigned long *jtab_base)
{
  unsigned long jbase;
  int jtable_index, j;

  DYN_DBG2(stat_trans, "%s() called for src_addr %lx\n", __func__, (long)src_addr);
  jtable_index = src_addr % 4;
  jbase = SRC_ENV_JUMPTABLE_ADDR + jtable_index * SRC_ENV_JUMPTABLE_INTERVAL;
  if (jtab_base) {
    *jtab_base = jbase;
  }
  for (j = 0; j < out->num_helper_sections; j++) {
    src_ulong jt_addr = ROUND_DOWN(src_addr, 4) + jbase;
    if (   jt_addr >= out->helper_sections[j].addr
        && jt_addr <
              out->helper_sections[j].addr + out->helper_sections[j].size) {
      return j;
    }
  }

  input_section_t const *isec = NULL;

  for (j = 0; j < in->num_input_sections; j++) {
    if (   src_addr >= in->input_sections[j].addr
        && src_addr < in->input_sections[j].addr+in->input_sections[j].size
        && is_code_section(&in->input_sections[j])) {
      isec = &in->input_sections[j];
      break;
    }
  }

  if (!isec) {
    return -1;
  }
  j = out->num_helper_sections++;
  int name_len = strlen(isec->name)+64;
  out->helper_sections[j].name = (char *)malloc(name_len * sizeof(char));
  ASSERT(out->helper_sections[j].name);
  snprintf(out->helper_sections[j].name, name_len, ".jumptable.%s.%d", isec->name,
      jtable_index);
  ASSERT(strlen(out->helper_sections[j].name) + 1 < name_len);

  out->helper_sections[j].addr = ROUND_DOWN(isec->addr, 4) + jbase;
  out->helper_sections[j].size = ROUND_UP(isec->size, 4);
  out->helper_sections[j].data = (char *)malloc(out->helper_sections[j].size);
  ASSERT(out->helper_sections[j].data);
  memset(out->helper_sections[j].data, 0x0, out->helper_sections[j].size);
  out->helper_sections[j].type = SHT_PROGBITS;
  out->helper_sections[j].flags = SHF_ALLOC;

  return j;
}

/*
static int
get_symbol_name_from_plt_addr(translation_instance_t *ti, uint32_t val,
    char *name, int name_size)
{
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;
  int reloc_index = -1;

  // get reloc index from plt addr
  for (int i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, ".plt")) {
      if (val >= in->input_sections[i].addr
          && (val < in->input_sections[i].addr + in->input_sections[i].size)) {
        reloc_index = (val - in->input_sections[i].addr - 72);
        ASSERT((reloc_index%8) == 0);
        reloc_index = reloc_index/8;
        // get symbol name from reloc index
        ASSERT(name_size > strlen(in->relocs[reloc_index].symbolName)+4);
        snprintf(name, name_size, "%sSRC", in->relocs[reloc_index].symbolName);
        DBG(I386_READ, "Returning %s (reloc %d) for plt addr %x\n", name, reloc_index,
            val);
        return 1;
      }
    }
  }

  if (reloc_index == -1)
    return 0; // if val not inside .plt
  ASSERT(0);
}*/

static long
add_jumptable_stub_code_section(translation_instance_t *ti)
{
  output_exec_t *out = ti->out;

  unsigned long helper_sections_end_addr;
  long j;

  helper_sections_end_addr = out->env_section.addr + out->env_section.size;
  for (j = 0; j < out->num_helper_sections; j++) {
    helper_sections_end_addr =
      max(helper_sections_end_addr,
          (unsigned long)out->helper_sections[j].addr + out->helper_sections[j].size);
  }

  j = out->num_helper_sections++;
  std::string name_str = ".jumptable_stub_code";
  out->helper_sections[j].name = (char*)malloc((name_str.length()+1)*sizeof(char));
  ASSERT(out->helper_sections[j].name);
  strncpy(out->helper_sections[j].name, name_str.c_str(),
      (name_str.length() + 1) * sizeof(char));

  out->helper_sections[j].data = (char *)ti->jumptable_stub_code_buf;
  out->helper_sections[j].size = ti->jumptable_stub_code_ptr
    - ti->jumptable_stub_code_buf;
  out->helper_sections[j].addr = helper_sections_end_addr;
  out->helper_sections[j].type = SHT_PROGBITS;
  out->helper_sections[j].flags = SHF_ALLOC | SHF_EXECINSTR;

  /* fix pcrels */
  char name[512];
  long i;
  for (i = 0; i < ti->num_jumptable_stub_code_offsets; i++) {
    int offset = ti->jumptable_stub_code_offsets[i];
    uint8_t *ptr = ti->jumptable_stub_code_buf + offset;
    dst_ulong val = *(dst_ulong *)ptr;

    if (!ti_symbol_exists(ti, name, sizeof name, val, MYTEXT_SECTION_NAME)) {
      snprintf(name, sizeof name, "jt%ld", i);
      DYN_DBG(stat_trans, "addng symbol %s to value %lx.\n", name, (long)val);
      ti_gen_symbol(ti, name, val, 0, STB_GLOBAL, STT_NOTYPE, MYTEXT_SECTION_NAME, 0);
    }
 
    DYN_DBG(stat_trans, "adding reloc entry for %s at address %lx\n", name, (long)offset);
    ti_add_reloc_entry(ti, name, offset, ".jumptable_stub_code", R_386_PC32);
    *(int32_t *)ptr = -4;
  }

  DYN_DBG(stat_trans, "jumptable_stub_code_section: addr %lx, size %lu\n",
      (unsigned long)out->helper_sections[j].addr,
      (unsigned long)out->helper_sections[j].size);

  return (out->helper_sections[j].addr);
}

static int
emit_jump_insn_in_plt_section(translation_instance_t *ti, uint32_t where,
    unsigned long target)
{
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;

  for (int n = 0; n < in->num_input_sections; n++) {
    if (!strcmp(in->input_sections[n].name, ".plt")) {
      if (!in->input_sections[n].data) {
        ASSERT(in->input_sections[n].type == SHT_NOBITS);
        in->input_sections[n].type = SHT_PROGBITS;
        in->input_sections[n].data=(char *)malloc(in->input_sections[n].size);
        ASSERT(in->input_sections[n].data);
        memset(in->input_sections[n].data, 0x0, in->input_sections[n].size);
      }
      char *ptr = in->input_sections[n].data
        + (where - in->input_sections[n].addr);
      *ptr++ = 0xff; *ptr++ = 0x25;
      *(unsigned int *)ptr = target; ptr+=4;
      return 6;
    }
  }
  ASSERT(0);
}

static int
emit_plt_stubcode(char *buf, int size, int r, unsigned long plt_stubcode_addr,
    unsigned long cur_addr)
{
  char *ptr = buf, *end = buf + size;
  /* push r*8 */
  *ptr++ = 0x68; *(uint32_t *)ptr = r*8; ptr+=4;
  *ptr++ = 0xe9; *(uint32_t *)ptr = plt_stubcode_addr - (cur_addr+10); ptr+=4;
  return (ptr-buf);
}

static  void
write_target_exec(translation_instance_t *ti, uint8_t const *code_gen_buffer, size_t codesize, struct chash const *tcodes)
{
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;

  out->num_helper_sections = 0;
  read_env_section(out);

  static int exec_id = 0;
  char target[1024];

  //snprintf(target, sizeof target, "%s.%d", ABUILD_DIR "/target_exec.elf", exec_id);
  snprintf(target, sizeof target, "%s.target_exec.elf.%d", out->name, exec_id);
  exec_id++;

  append_src_id_to_symbols(ti->in);
  generate_symbols_using_input_exec(ti);

  /* initialize pELFO */
  elfio pELFO;

  //ELFIO::GetInstance()->CreateELFO( &pELFO );
  //ASSERT(pELFO);

  //pELFO->SetAttr (ELFCLASS32, ELFDATA2LSB, EV_CURRENT, ET_REL, EM_386, EV_CURRENT, 0);
  pELFO.create(ELFCLASS32, ELFDATA2LSB);
  pELFO.set_type(ET_REL);
  pELFO.set_machine(EM_386);

#if ARCH_SRC != ARCH_ETFG
  if (in->fresh) {
    void *iter;
    src_ulong src_addr;
    for (iter = jumptable_iter_begin(in->jumptable, &src_addr); iter;
        iter = jumptable_iter_next(in->jumptable, iter, &src_addr)) {
      find_or_add_jumptable_helper_section(out, in, src_addr, NULL);
    }
    long stub_code_section = add_jumptable_stub_code_section(ti);

    for (iter = jumptable_iter_begin(in->jumptable, &src_addr); iter;
        iter = jumptable_iter_next(in->jumptable, iter, &src_addr)) {
      unsigned long jtab_base;
      int j = find_or_add_jumptable_helper_section(out, in, src_addr, &jtab_base);

      if (j < 0) {
        continue;
      }
      long host_addr;
      struct myhash_elem *found;
      struct imap_elem_t needle;
      needle.pc = src_addr;
      found = myhash_find(&ti->indir_eip_map, &needle.h_elem);
      if (found) {
        unsigned int off;

        host_addr = chash_entry(found, imap_elem_t, h_elem)->val;
        host_addr += stub_code_section - (long)ti->jumptable_stub_code_buf;
        src_ulong jt_addr = (src_addr & ~0x3) + jtab_base;
        ASSERT(jt_addr >= out->helper_sections[j].addr);
        off = jt_addr - out->helper_sections[j].addr;
        ASSERT(off < out->helper_sections[j].size);
        DBG(JUMPTABLE, "src_addr=%x, jt_addr=%x, host_addr=%lx\n", src_addr, jt_addr,
            host_addr);
        *(uint32_t *)(&out->helper_sections[j].data[off]) = host_addr;
      }
    }
  } else {
    int i;
    for (i = 0; i < in->num_input_sections; i++) {
      if (strstart(in->input_sections[i].name, ".jumptable.", NULL)) {
        Elf32_Addr addr;
        DBG(JUMPTABLE, "looking at input section %s.\n", in->input_sections[i].name);
        char *ldata = (char *)in->input_sections[i].data;
        for (addr = 0; addr < in->input_sections[i].size; addr += 4) {
          src_ulong val1 = tswap32(*(uint32_t *)(ldata + addr));
          DBG(JUMPTABLE, "addr = %lx, val1 = %lx\n", (unsigned long)addr, (unsigned long)val1);
          src_ulong val = tswap32(val1);
          DBG(JUMPTABLE, "addr = %lx, val = %lx\n", (unsigned long)addr, (unsigned long)val);
          if (!val) {
            continue;
          }
          ASSERT(jumptable_belongs(in->jumptable, val));
          src_ulong host_addr;

          val = pc2index(in, val);
          ASSERT(val >= 0 && val < in->num_si);
          host_addr = (long)in->si[val].tc_ptr;
          host_addr += (src_ulong)((TX_TEXT_START_ADDR + 0x400 + 0x0 /*+ SETUP_BUF_SIZE*/)
              - (unsigned long)START_VMA);
          DBG(JUMPTABLE, "Converting %x -> %lx at 0x%x\n", (unsigned)val,
              (unsigned long)host_addr, (unsigned)(in->input_sections[i].addr + addr));
          *(src_ulong *)(ldata + addr) = host_addr;
        }
      }
    }
  }

  int ndyn = 0;
  for (int i = 0; i < in->num_relocs; i++) {
    if (strcmp(in->relocs[i].sectionName, ".rel.text")) {
      continue;
    }
    switch (in->relocs[i].type) {
#if ARCH_SRC == ARCH_I386
      case R_386_32:
      case R_386_PC32:
        if (!strcmp(in->relocs[i].sectionName, ".rel.text")) {
          long txed_offset;
          int delta;
          txed_offset = get_txed_code_address(in, in->relocs[i].offset, &delta);
          if (txed_offset != INSN_PC_INVALID) {
            ASSERT(delta >= 0 && delta < 16);
            txed_offset += delta;
            DBG(RELOC_STRINGS, "adding reloc entry for %s at address %lx\n",
                in->relocs[i].symbolName, (long)txed_offset);
            ti_add_reloc_entry(ti, in->relocs[i].symbolName, txed_offset,
                MYTEXT_SECTION_NAME, in->relocs[i].type);
            if (in->relocs[i].type == R_386_PC32) {
              DBG(RELOC_STRINGS, "changing code_gen_buffer at %ld offset from %x to %x\n",
                  txed_offset, *(uint32_t *)&code_gen_buffer[txed_offset],
                  in->relocs[i].orig_contents);
              *(uint32_t *)&code_gen_buffer[txed_offset] = in->relocs[i].orig_contents;
            }
          } else {
            DBG(RELOC_STRINGS, "Warning: reloc '%s' value %lx not found in code region\n",
                in->relocs[i].symbolName, (long)in->relocs[i].offset);
          }
        }
        break;
#endif
#if ARCH_SRC == ARCH_PPC
      case R_PPC_JMP_SLOT:
        //ASSERT(in->relocs[i].offset == in->relocs[i].symbolValue);
        ASSERT(in->relocs[i].calcValue == 0);
        ASSERT(in->relocs[i].addend == 0);
        DBG(I386_READ, "Emitting jump instruction at address 0x%x\n",
            (unsigned)in->relocs[i].offset);
        /* emit_jump_insn_in_plt_section(ti, in->relocs[i].offset,
           pltgot_section+0xc+i*4); */
        break;
#endif
      default:
        err("Unsupported relocation type %d at address 0x%lx, name %s, value %lx.\n",
            in->relocs[i].type, (unsigned long)in->relocs[i].offset,
            in->relocs[i].symbolName, (unsigned long)in->relocs[i].symbolValue);
        //exit(1);
        break;
    }
  }
#endif

  DYN_DBG(stat_trans, "%s", "calling dump_elf()\n");
  dump_elf(pELFO, ti, target, code_gen_buffer, codesize, tcodes);
  DYN_DBG(stat_trans, "%s", "returned from dump_elf()\n");

  if (in->type == ET_EXEC) {
#if ARCH_SRC == ARCH_ETFG
    etfg_invoke_linker(in, out, ti, target);
#else
    invoke_linker(in, out, ti, target);
#endif
  } else {
    raw_copy(out->name, target);
  }
}

#if 0
void
fix_tb_pcrels(translation_instance_t *ti, tb_t *tb)
{
  output_exec_t *out = ti->out;
  int j;
  for (j = 0; j < tb->tb_num_pcrels; j++) {
    uint8_t *ptr;
    uint32_t val;
    ptr = tb->tc_start + tb->tb_pcrel_offsets[j];
    val = *((uint32_t *)ptr);

    DBG(STAT_TRANS, "val = %x, ptr = %p\n", val, ptr);
    add_reloc_entry(ti, &out->reloc_strings[val], 0, ptr,
        REL_SECTION_MYTEXT);
    *(int32_t *)ptr = -4;
  }
}
#endif

int dump_xmm_regs_code(uint8_t *buf, int buf_size)
{
  uint8_t *ptr = buf, *end = buf+buf_size;
  int i;
  for (i = 0; i < 8; i++) {
    /* movupd xmm$i, 0x4*$i(%ebp) */
    *ptr++=0x66; *ptr++=0x0f; *ptr++=0x11; *ptr++=0x40|(i<<3)|5; *ptr++=4*i;
  }
  ASSERT(ptr <= end);
  return (ptr - buf);
}

int restore_xmm_regs_code(uint8_t *buf, int buf_size)
{
  uint8_t *ptr = buf, *end = buf+buf_size;
  int i;
  for (i = 0; i < 8; i++) {
    /* movupd xmm$i, 0x4*$i(%ebp) */
    *ptr++=0x66; *ptr++=0x0f; *ptr++=0x10; *ptr++=0x40|(i<<3)|5; *ptr++=4*i;
  }
  ASSERT(ptr <= end);
  return (ptr - buf);
}

static void
process_speed_functions (void)
{
  NOT_IMPLEMENTED();
  /*
  if (!num_pslices) {
    int i;
    for (i = 0; i < num_speed_functions; i++) {
      pslices_start[i] = symbol_start(speed_functions[i]);
      pslices_stop[i] = symbol_stop(speed_functions[i]);
    }
    num_pslices = num_speed_functions;
  } else if (num_speed_functions) {
    int i;
    int num_new_pslices = 0;
    for (i = 0; i < num_pslices; i++) {
      int j;
      for (j = 0; j < num_speed_functions; j++) {
        if (!strcmp(lookup_symbol(pslices_start[i]), speed_functions[j])) {
          pslices_start[num_new_pslices] = pslices_start[i];
          pslices_stop[num_new_pslices] =
            MIN(pslices_stop[i],(unsigned long)symbol_stop(speed_functions[j]));
          num_new_pslices++;
        } else if (!strcmp(lookup_symbol(pslices_stop[i]), speed_functions[j])){
          pslices_start[num_new_pslices] = symbol_start(speed_functions[j]);
          pslices_stop[num_new_pslices] = pslices_stop[i];
          num_new_pslices++;
        }
      }
    }
    num_pslices = num_new_pslices;
  }
  */
}

static char *
pslices_to_str(char *buf, int buf_size)
{
  char *ptr = buf, *end = buf+buf_size;
  *ptr = '\0';

  int i;
  for (i = 0; i < num_pslices; i++) {
    ptr += snprintf(ptr, end-ptr, "(%lx,%lx)", pslices_start[i], pslices_stop[i]);
  }
  return buf;
}



typedef struct memory_used_struct_t
{
  int tb_index;
  unsigned memory_used;
} memory_used_struct;

static int memory_used_struct_compare(const void *_a, const void *_b)
{
  memory_used_struct_t const *a = (memory_used_struct_t *)_a;
  memory_used_struct_t const *b = (memory_used_struct_t *)_b;
  return (b->memory_used-a->memory_used);
}

#if 0
static void
profile_memory_usage (input_exec_t* in)
{
  int memory_used[in->num_bbls];

  int i;
  for (i = 0; i < in->nb_tbs; i++) {
    tb_t *tb = &in->bbls[i];
    memory_used[i] = 0;
    int j;
    transmap_set_elem_t const *iter;
    for (j = 0; j < tb_num_insns(tb); j++) {
      int k;
      for (k = 0; k < tb->transmap_sets[j]->head_size; k++) {
        if (!tb->transmap_sets[j]->head[k]) {
          continue;
        }
        for (iter = tb->transmap_sets[j]->head[k]->list; iter; iter = iter->next)
          memory_used[i] += sizeof(transmap_set_elem_t);
      }
    }
  }

  unsigned long total_memory_used = 0;
  struct memory_used_struct_t memory_used_structs[in->nb_tbs];
  for (i = 0; i < in->nb_tbs; i++)
  {
    memory_used_structs[i].tb_index = i;
    memory_used_structs[i].memory_used = memory_used[i];
    total_memory_used += memory_used[i];
  }

  qsort(memory_used_structs, in->nb_tbs, sizeof(memory_used_struct_t),
      memory_used_struct_compare);

  printf("Total Memory Used: %lu\n", total_memory_used);
  for (i = 0; i < in->nb_tbs; i++)
  {
    tb_t *tb = &in->bbls[memory_used_structs[i].tb_index];
    printf("%lx: %u bytes (%.2f%%)\n", tb->pc,
  memory_used_structs[i].memory_used,
  (double)memory_used_structs[i].memory_used/total_memory_used);
  }
}
#endif

static void
read_line_table_prefix(translation_instance_t const *ti, char const *data)
{
  uint32_t area_len = 0;
  uint16_t version = 0;
  uint32_t prologue_len = 0;
  uint8_t min_instr_len = 0;
  uint8_t default_is_stmt = 0;
  int8_t line_base = 0;
  uint8_t line_range = 0;
  uint8_t opcode_base = 0;
  uint8_t *standard_opcode_lengths = 0;
  uint32_t const MAX_N_FILES = 32;
  char const *files[MAX_N_FILES];
  uint32_t num_files = 0;
  int i;

  char const *ptr = data;
  area_len = TARGET_READ_UINT32(ptr);
  ptr += sizeof(uint32_t);
  version = TARGET_READ_UINT16(ptr);
  ptr += sizeof(uint16_t);
  prologue_len = TARGET_READ_UINT32(ptr);
  ptr += sizeof(uint32_t);
  min_instr_len = TARGET_READ_UINT8(ptr);
  ptr += sizeof(uint8_t);
  default_is_stmt = TARGET_READ_UINT8(ptr);
  ptr += sizeof(uint8_t);
  line_base = TARGET_READ_INT8(ptr);
  ptr += sizeof(int8_t);
  line_range = TARGET_READ_UINT8(ptr);
  ptr += sizeof(uint8_t);
  opcode_base = TARGET_READ_UINT8(ptr);
  ptr += sizeof(uint8_t);
  
  standard_opcode_lengths = (uint8_t *)malloc(opcode_base*sizeof(uint8_t));
  ASSERT(standard_opcode_lengths);

  for (i = 0; i < opcode_base-1; i++)
  {
    standard_opcode_lengths[i] = TARGET_READ_UINT8(ptr);
    ptr+=sizeof(uint8_t);
  }

  _DBG(DWARF2OUT, "area_length = %u\n", area_len);
  _DBG(DWARF2OUT, "version = %hu\n", version);
  _DBG(DWARF2OUT, "prologue_len= %u\n", prologue_len);
  _DBG(DWARF2OUT, "min_instr_len= %hhu\n", min_instr_len);
  _DBG(DWARF2OUT, "default_is_stmt=%hhu\n", default_is_stmt);
  _DBG(DWARF2OUT, "line_base=%hhd\n", line_base);
  _DBG(DWARF2OUT, "line_range=%hhu\n", line_range);
  _DBG(DWARF2OUT, "opcode_base=%hhu\n", opcode_base);
  _DBG(DWARF2OUT, "%s", "standard_opcode_lengths:\n");
  for (i = 0; i < opcode_base-1; i++)
    _DBG(DWARF2OUT, "\t%hhu\n", standard_opcode_lengths[i]);

  _DBG(DWARF2OUT, "%s", "Include directories:\n");
  while (*ptr)
  {
    _DBG(DWARF2OUT, "\t%s\n", ptr);
    while (*ptr) ptr++;
    ptr++;
  }
  ptr++;

  _DBG(DWARF2OUT, "%s", "filenames:\n");
  while (*ptr)
  {
    if (num_files < MAX_N_FILES) files[num_files++] = ptr;

    char const *filename = ptr;
    while (*ptr) ptr++;
    ptr++;
    int incdir_index = READ_LEB128(ptr);
    int lastmod = READ_LEB128(ptr);
    int file_len = READ_LEB128(ptr);

    _DBG(DWARF2OUT, "\t%s [incdir %d, lastmod %d, file_len %d]\n", filename,
  incdir_index, lastmod, file_len);
  }
  ptr++;

  //process stmt program
  uint32_t cur_addr = 0;
  uint32_t host_cur_addr = 0;
  uint32_t cur_file = 1;
  int32_t cur_line = 1;
  //uint8_t min_host_instr_len = 1;
  _DBG (DWARF2OUT, "%s", "start: cur_addr = 0\n");

  while (ptr - data < (int)area_len + 4)
  {
    _DBG(DWARF2OUT, "0x%x [file %s, line %d]\n", cur_addr,
  (cur_file<MAX_N_FILES)?files[cur_file-1]:"(more)",
  cur_line);
    if ((uint8_t)(*ptr) >= opcode_base)
    {
      //special opcode
      uint8_t adjusted_opcode = (uint8_t)(*ptr) - opcode_base;

      uint8_t addr_increment = (adjusted_opcode/line_range)*min_instr_len;
      int8_t line_increment = line_base + (adjusted_opcode % line_range);

      cur_addr += addr_increment;

      /*
      uint32_t host_addr_increment = lookup_eip_map(cur_addr)
      - cur_host_addr;
      uint8_t max_host_addr_increment = (((255 - opcode_base)
      - (line_increment - line_base))/line_range)*min_host_instr_len;

      if (host_addr_increment > max_host_addr_increment) {
        host_addr_increment = max_host_addr_increment;
      }
      cur_host_addr += host_addr_increment;
      *ptr = (host_addr_increment/min_host_instr_len)*line_range
      + (line_increment - line_base) + opcode_base;
      ASSERT((*ptr) <= 255);
      */

      ptr++;

      DBG (DWARF2, "special opcode: cur_addr = 0x%x, cur_line=%d\n", cur_addr,
          cur_line);
      cur_line += line_increment;
    } else if ((*ptr) && (uint8_t)(*ptr) < opcode_base) {
      //standard opcode
      uint8_t adjusted_opcode, addr_increment;
      uint32_t host_addr_increment;
      char const *tmp_ptr;
      int column;

      switch ((uint8_t)(*ptr)) {
        case DW_LNS_copy:
          _DBG(DWARF2, "standard opcode copy\n");
          ptr++;
          break;
        case DW_LNS_advance_pc:
          _DBG(DWARF2, "standard opcode fixed_advance_pc\n");
          ptr++;
          tmp_ptr = ptr;
          addr_increment = READ_LEB128(ptr)*min_instr_len;
          cur_addr += addr_increment;
          /*
             host_addr_increment = lookup_eip_map(cur_addr)
             - cur_host_addr;
             tmp_ptr += WRITE_LEB128(tmp_ptr, ptr,
             (host_addr_increment/min_host_instr_len));
             FILL_NOP_BYTES(tmp_ptr, ptr);
             */
          DBG (DWARF2, "advance_pc: cur_addr = 0x%x, cur_line=%d\n", cur_addr,
              cur_line);
          break;
        case DW_LNS_advance_line:
          _DBG(DWARF2, "standard opcode advance_line\n");
          ptr++;
          cur_line += READ_LEB128(ptr);
          DBG (DWARF2, "advance_line: cur_addr = 0x%x, cur_line=%d\n", cur_addr,
              cur_line);
          break;
        case DW_LNS_set_file: 
          DBG (DWARF2, "setting file to %hhd\n", *ptr);
          ptr++;
          READ_LEB128(ptr);
          break;
        case DW_LNS_set_column:
          ptr++;
          column = READ_LEB128(ptr);
          _DBG(DWARF2, "standard opcode set_column %d\n", column);
          break;
        case DW_LNS_negate_stmt:
          _DBG(DWARF2, "standard opcode negate_stmt\n");
          ptr++;
          break;
        case DW_LNS_set_basic_block:
          _DBG(DWARF2, "standard opcode set_basic_block\n");
          ptr++;
          break;
        case DW_LNS_const_add_pc:
          _DBG(DWARF2, "standard opcode const_add_pc\n");
          adjusted_opcode = 255 - opcode_base;
          addr_increment = (adjusted_opcode/line_range)*min_instr_len;
          cur_addr += addr_increment;
          /*
             host_addr_increment = lookup_eip_map(cur_addr) - cur_host_addr;
             max_host_addr_increment =
             ((255 - opcode_base)/line_range)*min_host_instr_len;
             if (host_addr_increment > max_host_addr_increment)
             {
             host_addr_increment = max_host_addr_increment;
             }

             cur_host_addr += host_addr_increment;
           *ptr = (host_addr_increment/min_host_instr_len)*line_range
           + (0 - line_base) + opcode_base;
           ASSERT((*ptr) <= 255);
           */

          ptr++;
          DBG (DWARF2, "add_pc: cur_addr = 0x%x, cur_line = %d\n", cur_addr,
              cur_line);
          break;
        case DW_LNS_fixed_advance_pc:
          _DBG(DWARF2, "standard opcode fixed_advance_pc\n");
          ptr++;
          addr_increment = TARGET_READ_UINT16(ptr)*min_instr_len;
          cur_addr += addr_increment;

          /*
             host_addr_increment = lookup_eip_map(cur_addr) - cur_host_addr;
             max_host_addr_increment = ((1 << 16) - 1) * min_host_instr_len;

             if (host_addr_increment > max_host_addr_increment)
             {
             host_addr_increment = max_host_addr_increment;
             }
             cur_host_addr += host_addr_increment;
           *(uint16_t *)ptr = host_addr_increment/min_host_instr_len;
           */

          DBG (DWARF2, "fixed_advance_pc: cur_addr = 0x%x, cur_line = %d\n",
              cur_addr, cur_line);
          ptr+=2;
          break;
        default:
          break;
      }
    } else {
      ASSERT((*ptr) == 0);
      ptr++;
      int insn_len = READ_LEB128(ptr);
      switch (*ptr) {
        case DW_LNE_end_sequence:
          _DBG(DWARF2, "extended opcode DW_LNE_end_sequence. len %d\n",
              insn_len);
          break;
        case DW_LNE_set_address:
          cur_addr = TARGET_READ_UINT32(ptr+1);
          _DBG(DWARF2, "set_address: cur_address = 0x%x, cur_line = %d\n",
              cur_addr, cur_line);
          _DBG(DWARF2, "extended opcode DW_LNE_set_address. len %d\n",
              insn_len);
          ASSERT(insn_len == 5);
          break;
        case DW_LNE_define_file:
          _DBG(DWARF2, "extended opcode DW_LNE_define_file. len %d\n",
              insn_len);
          break;
        default:
          break;
      }
      ptr += insn_len;
    }
  }

  free(standard_opcode_lengths);
}


static int
is_compilation_unit (char const *infodata, char const *abbrevdata,
    int *entry_size)
{
  char const *abbrevptr = abbrevdata;
  char const *infoptr = infodata;

  int curabbrev = READ_LEB128(infoptr);
  DBG(DWARF2_DBG, "curabbrev=%d\n", curabbrev);

  for (;;) {
    int abbrev = READ_LEB128(abbrevptr);
    int tag = READ_LEB128(abbrevptr);

    DBG(DWARF2_DBG, "abbrev=%d, tag=%x\n", abbrev, tag);
    if (abbrev == 0)
      break;  /* reached the end of .debug_abbrev */

    if (abbrev != curabbrev)
      continue;  /* we are not interested in this entry */

    uint8_t has_children = TARGET_READ_UINT8(abbrevptr);
    abbrevptr += sizeof(uint8_t);

    DBG(DWARF2_DBG, "has_children=%d\n", has_children);

    int offset = -1;
    int attr_name = 1;
    int attr_form = 1;
    int tmp = 0;

    while (1) {
      attr_name = READ_LEB128(abbrevptr);
      attr_form = READ_LEB128(abbrevptr);

      if (!attr_name) {
        ASSERT(!attr_form);
        break;
      }

      DBG (DWARF2_DBG, "attrname=0x%x, attrform=0x%x\n", attr_name, attr_form);

      int new_attr_form;
      char const *old_infoptr = NULL;
      do {
        old_infoptr = infoptr;
        new_attr_form = 0;
        switch (attr_form) {
          case DW_FORM_addr:
            infoptr += 4;
            break;
          case  DW_FORM_block1:
            infoptr += *((uint8_t *)infoptr) + 1;
            break;
          case  DW_FORM_block2:
            infoptr += *((uint16_t *)infoptr) + 2;
            break;
          case  DW_FORM_block4:
            infoptr += *((uint32_t *)infoptr) + 4;
            break;
          case  DW_FORM_block:
            tmp = READ_LEB128(infoptr);
            infoptr += tmp;
            break;
          case  DW_FORM_data1: case DW_FORM_ref1:
            infoptr += 1;
            break;
          case  DW_FORM_data2: case DW_FORM_ref2:
            infoptr += 2;
            break;
          case  DW_FORM_data4: case DW_FORM_ref4:
            infoptr += 4;
            break;
          case  DW_FORM_data8: case DW_FORM_ref8:
            infoptr += 8;
            break;
          case DW_FORM_sdata: case DW_FORM_udata: case DW_FORM_ref_udata:
            READ_LEB128(infoptr);
            break;
          case DW_FORM_flag:
            infoptr += 1;
            break;
          case DW_FORM_ref_addr: /*size of addr on target */
            infoptr += 4;
            break;
          case DW_FORM_string:
            while (*(infoptr++));
            break;
          case DW_FORM_strp:
            infoptr += 4;
            break;
          case DW_FORM_indirect:
            new_attr_form = READ_LEB128(infoptr);
            break;
          default:
            fprintf(stderr, "Unrecognized DW_FORM %d\n", attr_form);
            ASSERT(0);
            break;
        }
        attr_form = new_attr_form;
      } while (new_attr_form);

      if (   tag == DW_TAG_compile_unit && attr_name == DW_AT_stmt_list
          && attr_form != DW_FORM_indirect) {
        ASSERT(offset == -1);
        offset = TARGET_READ_UINT32(old_infoptr);
      }
    }
    *entry_size = infoptr - infodata;
    return offset;
  }
  *entry_size = infoptr - infodata;
  return -1;
}

static void
fix_debug_lines(translation_instance_t *ti)
{
  output_exec_t *out = ti->out;
  input_exec_t *in = ti->in;
  input_section_t *debug_line_section = NULL;
  input_section_t *debug_info_section = NULL;
  input_section_t *debug_abbrev_section = NULL;

  for (int i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, ".debug_line")) {
      debug_line_section = &in->input_sections[i];
    }
    if (!strcmp(in->input_sections[i].name, ".debug_info")) {
      debug_info_section = &in->input_sections[i];
    }
    if (!strcmp(in->input_sections[i].name, ".debug_abbrev")) {
      debug_abbrev_section = &in->input_sections[i];
    }
  }

  if (!debug_line_section || !debug_info_section || !debug_abbrev_section) {
    /* fprintf(stderr, ".debug_%s section not found\n",
       debug_line_section?(debug_info_section?"abbrev":"info"):"line"); */
    return;
  }

  char const *info_ptr = debug_info_section->data;

  while ((info_ptr - debug_info_section->data) < (int)debug_info_section->size) {
    DBG(DWARF2_DBG, "info_ptr %p\ndebug_info_section->data %p\n"
        "info_ptr-debug_info_section->data %zd\ndebug_info_section->size %d\n\n",
        info_ptr, debug_info_section->data, info_ptr-debug_info_section->data,
        (int)debug_info_section->size);
    char const *old_info_ptr = info_ptr;
    uint32_t len = TARGET_READ_UINT32(info_ptr);
    info_ptr+=4;
    uint16_t version = TARGET_READ_UINT16(info_ptr);
    info_ptr+=2;
    uint32_t abbrev_off = TARGET_READ_UINT32(info_ptr);
    info_ptr+=4;
    uint8_t addrsize = TARGET_READ_UINT8(info_ptr);
    info_ptr+=1;

    DBG (DWARF2_DBG, "len %d, version %hx, abbrev_offset %d, addrsize %hhd\n",
        len, version, abbrev_off, addrsize);

    while (info_ptr - old_info_ptr < ((int)len + 4)) {
      char const *tmp_info_ptr = info_ptr;
      DBG (DWARF2_DBG, "info_ptr(%p): %hhx,%hhx,%hhx,%hhx,%hhx,%hhx,%hhx,%hhx\n",
          info_ptr, info_ptr[0],info_ptr[1],info_ptr[2],info_ptr[3],
          info_ptr[4],info_ptr[5],info_ptr[6],info_ptr[7]);
      if (READ_LEB128(tmp_info_ptr) == 0) {
        DBG(DWARF2_DBG, "Reached end of .debug_info section. info_ptr %p\n",
            tmp_info_ptr);
        break;
      }
      DBG (DWARF2_DBG, "%s", "Seen entry in .debug_info\n");

      int offset = 0;
      int entry_size = 0;
      if ((offset = is_compilation_unit(info_ptr,
              debug_abbrev_section->data+abbrev_off, &entry_size)) != -1) {
        read_line_table_prefix(ti, debug_line_section->data+offset);
      }
      info_ptr += entry_size;
    }
    DBG (DWARF2_DBG, "info_ptr %p, old_info_ptr %p, old_info_ptr+len+4 %p\n",
        info_ptr, old_info_ptr, old_info_ptr + len + 4);
    info_ptr = old_info_ptr + len + 4;
  }
}

static void
cfg_add_reachable_pcs(input_exec_t *in, src_ulong start_pc, graph<cfg_pc, node<cfg_pc>, edge<cfg_pc>> &cfg)
{
  DYN_DEBUG(cfg_make_reducible, cout << __func__ << " " << __LINE__ << ": start_pc = " << hex << start_pc << dec << endl);
  if (start_pc == PC_JUMPTABLE || start_pc == PC_UNDEFINED) {
    return;
  }
  long index = pc2bbl(&in->bbl_map, start_pc);
  ASSERT(index >= 0 && index < in->num_bbls);
  bbl_t *bbl = &in->bbls[index];
  for (int i = 0; i < bbl->num_nextpcs; i++) {
    if (bbl->nextpcs[i].get_tag() != tag_const) {
      continue;
    }
    src_ulong nextpc = bbl->nextpcs[i].get_val();
    long nextpc_index = pc2bbl(&in->bbl_map, nextpc);
    DYN_DEBUG(cfg_make_reducible, cout << __func__ << " " << __LINE__ << ": start_pc " << std::hex << start_pc << std::dec << ", nextpc = " << std::hex << nextpc << ": " << std::dec << nextpc_index << " --> " << std::hex << nextpc << std::dec << endl);
    if (nextpc_index < 0 || nextpc_index >= in->num_bbls) {
      ASSERT(nextpc_index == -1);
      if (nextpc != PC_JUMPTABLE && nextpc != PC_UNDEFINED) {
        printf("%s() %d: start_pc = 0x%x, nextpc = 0x%x\n", __func__, __LINE__, (int)start_pc, (int)nextpc);
      }
      ASSERT(nextpc == PC_JUMPTABLE || nextpc == PC_UNDEFINED);
      nextpc_index = nextpc;
    }
    if (!cfg.find_node(nextpc_index)) {
      dshared_ptr<node<cfg_pc>> n = make_dshared<node<cfg_pc>>(cfg_pc(nextpc_index));
      cfg.add_node(n);
      cfg_add_reachable_pcs(in, nextpc, cfg);
    }

    dshared_ptr<edge<cfg_pc> const> e = make_dshared<edge<cfg_pc>>(cfg_pc(index), cfg_pc(nextpc_index));
    cfg.add_edge(e);
  }
}

static void
bbl_copy(bbl_t *dst, bbl_t const *src)
{
  memset(dst, 0, sizeof(bbl_t));
  dst->pc = src->pc;
  dst->size = src->size;
  dst->num_insns = src->num_insns;
  dst->pcs = (src_ulong *)malloc(dst->num_insns * sizeof(src_ulong));
  ASSERT(dst->pcs);
  for (int i = 0; i < dst->num_insns; i++) {
    dst->pcs[i] = src->pcs[i];
  }

  dst->num_nextpcs = src->num_nextpcs;
  //dst->nextpcs = (src_ulong *)malloc(dst->num_nextpcs * sizeof(src_ulong));
  dst->nextpcs = new nextpc_t[dst->num_nextpcs];
  ASSERT(dst->nextpcs);
  for (int i = 0; i < dst->num_nextpcs; i++) {
    dst->nextpcs[i] = src->nextpcs[i];
  }

  dst->num_fcalls = src->num_fcalls;
  //dst->fcalls = (src_ulong *)malloc(dst->num_fcalls * sizeof(src_ulong));
  dst->fcalls = new nextpc_t[dst->num_fcalls];
  ASSERT(dst->fcalls);
  dst->fcall_returns = (src_ulong *)malloc(dst->num_fcalls * sizeof(src_ulong));
  ASSERT(dst->fcall_returns);
  for (int i = 0; i < dst->num_fcalls; i++) {
    dst->fcalls[i] = src->fcalls[i];
    dst->fcall_returns[i] = src->fcall_returns[i];
  }

  //this function is called when all other metadata has not been initialized
  ASSERT(!src->preds);
  dst->preds = NULL;
  ASSERT(!src->num_preds);
  dst->num_preds = 0;
  ASSERT(!src->loop);
  dst->loop = NULL;
  ASSERT(!src->pred_weights);
  dst->pred_weights = NULL;
  ASSERT(!src->nextpc_probs);
  dst->nextpc_probs = NULL;
  ASSERT(!src->live_regs);
  dst->live_regs = NULL;
  ASSERT(!src->live_ranges);
  dst->live_ranges = NULL;
  ASSERT(!src->rdefs);
  dst->rdefs = NULL;
}

static void
cfg_make_reducible_at_start_pc(input_exec_t *in, src_ulong start_pc)
{
  graph<cfg_pc, node<cfg_pc>, edge<cfg_pc>> cfg;
  long index = pc2bbl(&in->bbl_map, start_pc);
  DYN_DEBUG(cfg_make_reducible, cout << __func__ << " " << __LINE__ << ": start_pc = " << std::hex << start_pc << ": " << std::dec << index << " --> " << std::hex << start_pc << std::dec << endl);
  ASSERT(index >= 0 && index < in->num_bbls);
  dshared_ptr<node<cfg_pc>> n = make_dshared<node<cfg_pc>>(cfg_pc(index));
  cfg.add_node(n);
  cfg_add_reachable_pcs(in, start_pc, cfg);
  set<cfg_pc> do_not_copy_pcs;
  do_not_copy_pcs.insert(cfg_pc(PC_JUMPTABLE));
  do_not_copy_pcs.insert(cfg_pc(PC_UNDEFINED));
  dshared_ptr<graph<cfg_pc, node<cfg_pc>, edge<cfg_pc>>> rcfg = cfg.make_reducible(in->num_bbls, do_not_copy_pcs);
  list<dshared_ptr<node<cfg_pc>>> ns;
  rcfg->get_nodes(ns);
  long max_bbl_idx = in->num_bbls - 1;
  for (auto n : ns) {
    long bbl_idx = n->get_pc().get_bbl_index();
    if (bbl_idx != PC_JUMPTABLE && bbl_idx != PC_UNDEFINED) {
      max_bbl_idx = max(max_bbl_idx, bbl_idx);
    }
  }
  in->bbls = (bbl_t *)realloc(in->bbls, (max_bbl_idx + 1) * sizeof(bbl_t));
  ASSERT(in->bbls);
  map<bbl_index_t, copy_id_t> max_copy_id;
  for (auto n : ns) {
    long bbl_idx = n->get_pc().get_bbl_index();
    long orig_bbl_idx = n->get_pc().get_orig_bbl_index();
    /*if (orig_bbl_idx == PC_JUMPTABLE || orig_bbl_idx == PC_UNDEFINED) {
      memset(&in->bbls[bbl_idx], 0, sizeof(bbl_t));
      in->bbls[bbl_idx].pc = orig_bbl_idx;
      in->bbls[bbl_idx].size = 0;
      in->bbls[bbl_idx].num_insns = 0;
      in->bbls[bbl_idx].pcs = NULL;
      in->bbls[bbl_idx].num_nextpcs = 0;
      in->bbls[bbl_idx].nextpcs = NULL;
      in->bbls[bbl_idx].num_fcalls = 0;
      in->bbls[bbl_idx].fcalls = NULL;
      in->bbls[bbl_idx].fcall_returns = NULL;
      continue;
    }*/
    ASSERT((orig_bbl_idx >= 0 && orig_bbl_idx < in->num_bbls) || orig_bbl_idx == PC_JUMPTABLE || orig_bbl_idx == PC_UNDEFINED);
    if (bbl_idx != orig_bbl_idx) {
      ASSERT(orig_bbl_idx >= 0 && orig_bbl_idx < in->num_bbls);
      long copy_id;
      if (!max_copy_id.count(orig_bbl_idx)) {
        copy_id = 1;
      } else {
        copy_id = max_copy_id.at(orig_bbl_idx) + 1;
      }
      max_copy_id[orig_bbl_idx] = copy_id;
      ASSERT(bbl_idx >= in->num_bbls);
      ASSERT(bbl_idx <= max_bbl_idx);
      bbl_copy(&in->bbls[bbl_idx], &in->bbls[orig_bbl_idx]);
      in->bbls[bbl_idx].pc = pc_copy(in->bbls[orig_bbl_idx].pc, copy_id);
      //cout << __func__ << " " << __LINE__ << ": bbl_idx = " << bbl_idx << ", copy_id = " << copy_id << ", pc = " << hex << in->bbls[bbl_idx].pc << dec << endl;
      for (int i = 0; i < in->bbls[bbl_idx].num_insns; i++) {
        in->bbls[bbl_idx].pcs[i] = pc_copy(in->bbls[bbl_idx].pcs[i], copy_id);
      }
      in->bbls[bbl_idx].reachable = in->bbls[orig_bbl_idx].reachable;
      bbl_leaders_add(&in->bbl_leaders, in->bbls[bbl_idx].pc);
    }
  }
  in->num_bbls = max_bbl_idx + 1;
  for (auto n : ns) {
    list<dshared_ptr<edge<cfg_pc> const>> outgoing;
    rcfg->get_edges_outgoing(n->get_pc(), outgoing);
    long bbl_index = n->get_pc().get_bbl_index();
    long orig_bbl_index = n->get_pc().get_orig_bbl_index();
    if (orig_bbl_index == PC_JUMPTABLE || orig_bbl_index == PC_UNDEFINED) {
      continue;
    }
    if (bbl_index < 0 || bbl_index > in->num_bbls) {
      cout << __func__ << " " << __LINE__ << ": bbl_index = " << bbl_index << ", in->num_bbls = " << in->num_bbls << endl;
    }
    ASSERT(bbl_index >= 0 && bbl_index < in->num_bbls);
    bbl_t *bbl = &in->bbls[bbl_index];
    long new_num_nextpcs = outgoing.size();

    //bbl->nextpcs = (src_ulong *)realloc(bbl->nextpcs, new_num_nextpcs * sizeof(src_ulong));
    nextpc_t *old_nextpcs = bbl->nextpcs;
    bbl->nextpcs = new nextpc_t[new_num_nextpcs];
    for (size_t i = 0; i < min(new_num_nextpcs, (long)bbl->num_nextpcs); i++) {
      bbl->nextpcs[i] = old_nextpcs[i];
    }
    if (old_nextpcs) {
      delete[] old_nextpcs;
    }

    ASSERT(!new_num_nextpcs || bbl->nextpcs);
    long fallthrough_bbl_index = -1;
    int i = 0;
    for (auto e : outgoing) {
      long to_index = e->get_to_pc().get_bbl_index();
      long orig_to_index = e->get_to_pc().get_orig_bbl_index();
      if (orig_to_index == orig_bbl_index + 1) {
        //this is a fallthrough edge; this must be added at the very end.
        fallthrough_bbl_index = to_index;
        continue;
      }
      src_ulong nextpc;
      if ((src_ulong)to_index == PC_JUMPTABLE || (src_ulong)to_index == PC_UNDEFINED) {
        nextpc = to_index;
      } else {
        ASSERT(to_index >= 0 && to_index < in->num_bbls);
        nextpc = in->bbls[to_index].pc;
      }
      ASSERT(i < new_num_nextpcs);
      bbl->nextpcs[i] = nextpc;
      i++;
    }
    if (fallthrough_bbl_index != -1) {
      ASSERT(fallthrough_bbl_index >= 0 && fallthrough_bbl_index < in->num_bbls);
      bbl->nextpcs[i] = in->bbls[fallthrough_bbl_index].pc;
      i++;
    }
    ASSERT(i == new_num_nextpcs);
    bbl->num_nextpcs = new_num_nextpcs;
  }
  free_bbl_map(in);
  compute_bbl_map(in);
}

static void
cfg_make_reducible(input_exec_t *in)
{
  src_ulong addr;
  for (void *iter = jumptable_iter_begin(in->jumptable, &addr);
       iter;
       iter = jumptable_iter_next(in->jumptable, iter, &addr))
  {
    cfg_make_reducible_at_start_pc(in, addr);
  }
}

static void
process_cfg(input_exec_t *in)
{
  PROGRESS("%s", "Building CFG");
  build_cfg(in);

  if (in->num_bbls == 0) {
    return;
  }

  PROGRESS("%s", "Making CFG reducible");
  cfg_make_reducible(in);

  PROGRESS("%s", "Analyzing CFG");
  analyze_cfg(in);

  PROGRESS("%s", "Printing CFG");
  cfg_print(logfile, in);
}

static bool
input_section_contains_addr(input_section_t const *in, uint64_t addr)
{
  if (!(in->flags & SHF_ALLOC)) {
    return false;
  }
  if (addr >= in->addr && addr < in->addr + in->size) {
    return true;
  }
  return false;
}

bool
input_exec_addr_belongs_to_input_section(input_exec_t const *in, uint64_t addr)
{
  int i;

  for (i = 0; i < in->num_input_sections; i++) {
    if (input_section_contains_addr(&in->input_sections[i], addr)) {
      return true;
    }
  }
  return false;
}


//static void
//locals_info_init(input_exec_t *in)
//{
//  ASSERT(in->locals_info == NULL);
//  in->locals_info = (locals_info_t **)malloc(in->num_si * sizeof(locals_info_t *));
//  ASSERT(in->locals_info);
//  for (long i = 0; i < in->num_si; i++) {
//    in->locals_info[i] = NULL;
//  }
//}

static void
freshness_init(input_exec_t *in)
{
  int i;
  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, ".src_env")) {
      in->fresh = false;
      return;
    }
  }
  in->fresh = true;
}



void
init_input_file(input_exec_t *in, char const *function_name)
{
  autostop_timer func_timer(__func__);
  int n;

  for (n = 0; n < in->num_input_sections; n++) {
    if (!strcmp(in->input_sections[n].name, ".text")) {
      size_t name_len = 16;
      in->input_sections[n].name =
        (char*)realloc(in->input_sections[n].name, name_len*sizeof(char));
      snprintf(in->input_sections[n].name, name_len, ".srctext");
      ASSERT(strlen(in->input_sections[n].name) < name_len);
    }
    DYN_DBG(stat_trans, "in->input_sections[%d].name=%s\n", n,
        in->input_sections[n].name);
  }
  freshness_init(in);
  //PRINT_PROGRESS("%s() %d: after freshness_init()\n", __func__, __LINE__);
  process_cfg(in);
  //PRINT_PROGRESS("%s() %d: after process_cfg()\n", __func__, __LINE__);
  PROGRESS("%s", "Initializing si");
  si_init(in, function_name);
  //PRINT_PROGRESS("%s() %d: after si_init()\n", __func__, __LINE__);
  //locals_info_init(in);
  //PRINT_PROGRESS("%s() %d: after locals_info_init()\n", __func__, __LINE__);
}

/*
static void
tbs_fix_transmaps_from_logfile(translation_instance_t *ti, char const *logfile)
{
  FILE *logfp = fopen(logfile, "r");
  ASSERT(logfp);
  input_exec_t *in = ti->in;

  unsigned int nip, len, num_lastpcs;
  src_ulong lastpcs[SRC_MAX_ILEN];
  static char line_store[16384];
  transmap_t in_tmap, out_tmap;
  unsigned long pc;
  int cur_tb = 0;

  while (fscanf (logfp, "%[^\n]\n", line_store) == 1) {
    char *line = line_store;
    DBG2(FIX_TMAPS, "line: %s\n", line);
    if (line[0] == '0' && line[1] == 'x' && line[10] == ':') {
      line[10] = '\0';
      nip = strtol(line, NULL, 16);
      line += 11;

      int qemu_trans = 0;

      if (!strcmp (line, " Defaulted to qemu-translation")) {
        qemu_trans = 1;
        len = 1;
        DBG(FIX_TMAPS, "%x: qemu\n", nip);
      } else {
        char *colon = strchr(line, ':');
        char *lparen = strchr(line, '(');
        char *rparen = strchr(line, ')');
        char *end = NULL;
        int i;

        if (!lparen) continue;
        if (!rparen) continue;
        if (!colon) continue;
        if (lparen >= colon || rparen >= colon) continue;
        if (lparen >= rparen) continue;

        *lparen = '\0';
        *rparen = '\0';
        *colon = '\0';

        line[3] = '\0';
        if (strcmp(line, " pc")) continue;
        line += 4;
        pc = strtol(line, &end, 0);
        ASSERT(end);
        if (!pc) continue;
        line = end;

        line[12] = '\0';
        if (strcmp(line, " num_lastpcs")) continue;
        line += 13;
        num_lastpcs = strtol(line, &end, 0);
        ASSERT(end);
        line = end;

        line[8] = '\0';
        if (strcmp(line, " lastpcs")) continue;
        line += 9;
        for (i = 0; i < num_lastpcs; i++) {
          lastpcs[i] = strtol(line, &end, 0);
          ASSERT(end);
          line = end;
        }

        //line[7] = '\0';
        //if (strcmp(line, " nextpc")) continue;
        //line += 8;
        //nextpc = strtol(line, &end, 0);
        //ASSERT(end);
        //if (!nextpc) continue;
        //line = end;

        line[4] = '\0';
        if (strcmp (line, " len")) continue;
        line += 5;
        len = strtol(line, NULL, 0);
        if (!len) continue;

        line = lparen + 4;
        int pos = atoi(line);
        if (pos) continue; //this means, we are in the middle of a translation

        line = colon + 1;
        line[7] = '\0';
        if (strcmp(line, " WINNER")) continue;

        int n, tmap_read;
        tmap_read = transmap_from_stream(&in_tmap, logfp);
        ASSERT(tmap_read);
        //printf("in_tmap:%s\n", transmap_to_string(&in_tmap, as1, sizeof as1));
        n = fscanf(logfp, "out_tmap:%[^\n]\n", line);
        ASSERT(n == 0 || n == 1);
        //printf("pc %lx lastpc %lx line:'%s'\n", pc, lastpc, line);
        if (!strcmp(line, " same as in_tmap")) {
          transmap_copy(&out_tmap, &in_tmap);
        } else {
          tmap_read = transmap_from_stream(&out_tmap, logfp);
          ASSERT(tmap_read);
        }
        DBG(FIX_TMAPS, "%x:\n%s\n", nip, transmap_to_string(&in_tmap, as1, sizeof as1));
      }

      input_exec_t *in = ti->in;
      tb_t *tb = &in->bbls[cur_tb];
      if (nip < tb->pc) {
        err("nip = %x, tb->pc = %lx\n", nip, tb->pc);
        ASSERT(0);
      }
      while (tb->pc + tb->size <= nip && cur_tb < in->num_bbls) {
        tb = &in->bbls[++cur_tb];
      }

      if (cur_tb == in->num_bbls) {
        tb_t const *last_tb = &in->bbls[in->num_bbls - 1];
        if (tb_insn_address(last_tb, tb_num_insns(last_tb)) == nip) {
          // This is the special case of emitting the terminating pc.
          continue;
        }
        err("Translation block for nip %x not found\n", nip);
        exit (1);
      }

      if (!tb->transmap_sets) {
        tb_init_transmap_sets(tb);
      }
      int n = tb_insn_index(tb, nip);
      ASSERT(n >= 0 && n < tb_num_insns(tb));

      bool tmap_decision_is_null = true;
      if (belongs_to_peep_program_slice(nip)) {
        tmap_decision_is_null = false;
      } else {
        src_insn_t insn;

        if (src_insn_fetch(&insn, nip, NULL)) {
          if (peep_tab_has_base_translation_entry(ti->peep_table, &insn, &in_tmap,
              &out_tmap)) {
            tmap_decision_is_null = false;
          }
        }
      }

      if (   !tmap_decision_is_null
          && qemu_trans == 0
          && !tb->transmap_decision[n]) {
        transmap_set_row_init(&tb->transmap_sets[n]->head[len], pc, len, lastpcs,
            num_lastpcs);
        tb->transmap_sets[n]->head[len]->list = (transmap_set_elem_t *)malloc(
            sizeof(transmap_set_elem_t));
        ASSERT(tb->transmap_sets[n]->head[len]->list);

        transmap_set_elem_t *tmap_elem = tb->transmap_sets[n]->head[len]->list;
        tmap_elem->tcons = NULL;
        tmap_elem->next = NULL;
        tmap_elem->num_temporaries = -1;
        tmap_elem->num_decisions = 0;
        tmap_elem->decisions = NULL;
        tmap_elem->row = tb->transmap_sets[n]->head[len];
        tmap_elem->cost = 0;

        transmap_init(&tmap_elem->in_tmap);
        transmap_init(&tmap_elem->out_tmap);
        transmap_copy(tmap_elem->in_tmap, &in_tmap);
        transmap_copy(tmap_elem->out_tmap, &out_tmap);

        unsigned i;
        for (i = 0; i < len; i++) {
          tb->transmap_decision[n+i] = tmap_elem;
        }
      }
    }
  }

  int i;
  for (i = 0; i < in->num_bbls; i++) {
    tb_t *tb = &in->bbls[i];
    int n;
    for (n = 0; n < tb_num_insns(tb); n++) {
      if (!tb->transmap_decision && !use_fallback_translation_only) {
        err("tb->transmap_decision is NULL for tb %lx\n", tb->pc);
        exit(1);
      }
    }
  }
  fclose(logfp);
}
*/

static transmap_set_elem_t *
transmap_set_elem_list_jumptable(void)
{
  static transmap_set_elem_t *ret = NULL;
  transmap_set_row_t *row;

  if (!ret) {
    ret = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
    ASSERT(ret);
    //ret->out_tmaps = (transmap_t *)malloc(sizeof(transmap_t));
    ret->out_tmaps = new transmap_t;
    ASSERT(ret->out_tmaps);
    transmap_copy(&ret->out_tmaps[0], transmap_default());
    ret->in_tmap = NULL;
    ret->next = NULL;
    ret->cost = 0;
    /*row = (transmap_set_row_t *)malloc(sizeof(transmap_set_row_t));
    ASSERT(row);
    row->num_nextpcs = 1;*/
    ret->row = NULL;
  }
  ASSERT(ret);
  return ret;
}

#if ARCH_SRC == ARCH_ETFG
static transmap_t
etfg_callee_save_retval_regs_transmap(src_insn_t const &src_insn)
{
  transmap_t ret;
  transmap_entry_t tmap_entry;
  //tmap_entry.num_locs = 1;
  //tmap_entry.loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
  //tmap_entry.regnum[0] = -1;
  tmap_entry.set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_SRC_SIZED);
  for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
    stringstream ss;
    ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
    reg_identifier_t reg_id = get_reg_identifier_for_regname(ss.str());
    ret.extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][reg_id] = tmap_entry;
  }
  reg_identifier_t reg_id = etfg_insn_get_returned_reg(src_insn);
  if (   reg_id.reg_identifier_is_valid()
      && !reg_id.reg_identifier_denotes_constant()) {
    ret.extable[ETFG_EXREG_GROUP_GPRS][reg_id] = tmap_entry;
  }
  //ret.extable[ETFG_EXREG_GROUP_GPRS][reg_id] = tmap_entry;
  return ret;
}

static transmap_t
etfg_callee_save_regs_transmap()
{
  transmap_t ret;
  transmap_entry_t tmap_entry;
  //tmap_entry.num_locs = 1;
  //tmap_entry.loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
  //tmap_entry.regnum[0] = -1;
  tmap_entry.set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_SRC_SIZED);
  for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
    stringstream ss;
    ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
    reg_identifier_t reg_id = get_reg_identifier_for_regname(ss.str());
    ret.extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][reg_id] = tmap_entry;
  }
  return ret;
}

static transmap_set_elem_t *
transmap_set_elem_list_all_possibilities_for_callee_saves(void)
{
  static transmap_set_elem_t *ret = NULL;
  transmap_set_row_t *row;

  if (!ret) {
    transmap_entry_t tmap_entry_reg, tmap_entry_mem;
    tmap_entry_reg.set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), TRANSMAP_REG_ID_UNASSIGNED, TMAP_LOC_MODIFIER_SRC_SIZED);
    tmap_entry_mem.set_to_symbol_loc(TMAP_LOC_SYMBOL, -1);
    map<reg_identifier_t, list<transmap_entry_t>> choices;
    //regset_t regset_callee_save;
    for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
      stringstream ss;
      ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
      reg_identifier_t reg_id = get_reg_identifier_for_regname(ss.str());
      choices[reg_id].push_back(tmap_entry_reg);
      choices[reg_id].push_back(tmap_entry_mem);
      //regset_mark_ex_mask(&regset_callee_save, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS, reg_id, MAKE_MASK(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_CALLEE_SAVE_REGS)));
    }
    cartesian_t<reg_identifier_t, transmap_entry_t> cart(choices);

    transmap_t callee_save_regs = etfg_callee_save_regs_transmap();
    for (auto combo : cart.get_all_possibilities()) {
      transmap_set_elem_t *cur = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
      ASSERT(cur);
      //ret->out_tmaps = (transmap_t *)malloc(sizeof(transmap_t));
      cur->out_tmaps = new transmap_t;
      ASSERT(cur->out_tmaps);
      //transmap_copy(&cur->out_tmaps[0], transmap_default());
      for (const auto &m : combo) {
        cur->out_tmaps[0].extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][m.first] = m.second;
      }
      //cout << __func__ << " " << __LINE__ << ": adding cur->out_tmaps[0] =\n" << transmap_to_string(&cur->out_tmaps[0], as1, sizeof as1) << endl;
      cur->in_tmap = NULL;
      cur->cost = transmap_translation_cost(&callee_save_regs, &cur->out_tmaps[0]/*, &regset_callee_save*/, false);
      cur->row = NULL;
      cur->next = ret;
      ret = cur;
    }
  }
  return ret;
}
#endif

static void
ti_compute_pred_transmap_sets(translation_instance_t const *ti, si_entry_t const *si,
    src_ulong const *preds, long num_preds, pred_transmap_set_t *pred_transmap_sets[])
{
  pred_transmap_set_t *pts;
  pred_t *pred;
  long i;

  memset(pred_transmap_sets, 0, num_preds * sizeof(pred_transmap_set_t *));
  for (pred = ti->pred_set[si->index]; pred; pred = pred->next) {
    si_entry_t const *psi;
    long i;

    CPP_DBG_EXEC2(PEEP_SEARCH, cout << __func__ << " " << __LINE__ << ": si pc = " << hex << si->pc << dec << ": pred pc = " << hex << pred->si->pc << dec << ", num_lastpcs = " << pred->num_lastpcs << endl);
    for (i = 0; i < pred->num_lastpcs; i++) {
      si_entry_t const *psi;
      long fetchlen, li;

      psi = pred->si;
      fetchlen = pred->fetchlen;

      ASSERT(ti->transmap_sets[psi->index].head[fetchlen - 1]);

      li = array_search(preds, num_preds, pred->lastpcs[i]);
      ASSERT(li >= 0 && li < num_preds);

      pts = (pred_transmap_set_t *)malloc(sizeof(pred_transmap_set_t));
      ASSERT(pts);
      pts->list = ti->transmap_sets[psi->index].head[fetchlen - 1]->list;
      pts->next = pred_transmap_sets[li];
      pred_transmap_sets[li] = pts;
    }
  }
  for (i = 0; i < num_preds; i++) {
    if (preds[i] == PC_JUMPTABLE) {
      pts = (pred_transmap_set_t *)malloc(sizeof(pred_transmap_set_t));
      ASSERT(pts);
      pts->list = transmap_set_elem_list_jumptable();
      pts->next = pred_transmap_sets[i];
      pred_transmap_sets[i] = pts;
    /*} else if (ti->fixed_transmaps.count(preds[i])) {
#if ARCH_SRC == ARCH_ETFG
      pts = (pred_transmap_set_t *)malloc(sizeof(pred_transmap_set_t));
      ASSERT(pts);
      pts->list = transmap_set_elem_list_all_possibilities_for_callee_saves();
      pts->next = pred_transmap_sets[i];
      pred_transmap_sets[i] = pts;
#endif*/
    }
  }
}

static void
ti_add_pred_to_nextpcs(translation_instance_t *ti, nextpc_t const *nextpcs,
    long num_nextpcs, si_entry_t const *si, long fetchlen, src_ulong const *lastpcs,
    long num_lastpcs)
{
  src_ulong mylastpcs[num_lastpcs];
  input_exec_t const *in;
  si_entry_t const *nsi;
  long j, num_preds;
  pred_t *pred;
  long i, l;

  in = ti->in;
  for (i = 0; i < num_nextpcs; i++) {
    l = 0;
    if (nextpcs[i].get_val() == PC_UNDEFINED || nextpcs[i].get_val() == PC_JUMPTABLE) {
      continue;
    }
    j = pc2index(in, nextpcs[i].get_val());
    if (j == -1) {
      continue;
    }
    ASSERT(j >= 0 && j < in->num_si);
    nsi = &in->si[j];

    for (pred = ti->pred_set[nsi->index]; pred; pred = pred->next) {
      if (pred->si == si && pred->fetchlen == fetchlen) {
        return;
      }
    }

    src_ulong preds[nsi->bbl->num_preds];
    if (nsi->bblindex == 0) {
      num_preds = nsi->bbl->num_preds;
      memcpy(preds, nsi->bbl->preds, num_preds * sizeof(src_ulong));
    } else {
      num_preds = 1;
      preds[0] = bbl_insn_address(nsi->bbl, nsi->bblindex - 1);
    }
    for (j = 0; j < num_preds; j++) {
      long k;

      for (k = 0; k < num_lastpcs; k++) {
        if (preds[j] == lastpcs[k]) {
          mylastpcs[l++] = lastpcs[k];
        }
      }
    }
    CPP_DBG_EXEC(TI_PREDS,
        printf("Adding pred (%lx, %ld) with num_lastpcs %ld to %lx. lastpcs:",
            (long)si->pc, fetchlen, l, (long)nextpcs[i].get_val());
        for (j = 0; j < l; j++) { printf(" %lx", (long)mylastpcs[j]); }
        printf("\n");
    );
    pred = (pred_t *)malloc(sizeof(pred_t));
    ASSERT(pred);
    pred->si = si;
    pred->fetchlen = fetchlen;
    pred->lastpcs = (src_ulong *)malloc(l * sizeof(src_ulong));
    ASSERT(pred->lastpcs);
    memcpy(pred->lastpcs, mylastpcs, l * sizeof(src_ulong));
    pred->num_lastpcs = l;
    pred->next = ti->pred_set[nsi->index];
    ti->pred_set[nsi->index] = pred;
  }
}

static bool
edge_represents_phi_node_with_permuted_assignment(src_ulong from_pc, src_ulong to_pc)
{
  return true; //XXX: NOT_IMPLEMENTED();
}

static bool
transmap_decision_of_same_cost_is_better_than_other_for_nextpc(transmap_set_elem_t const *a, transmap_set_elem_t const *b, src_ulong nextpc)
{
  ASSERT(a);
  ASSERT(b);
  transmap_t const *atmap = find_out_tmap_for_nextpc(a, nextpc);
  transmap_t const *btmap = find_out_tmap_for_nextpc(b, nextpc);
  ASSERT(atmap);
  ASSERT(btmap);
  return atmap->transmap_is_superset_of(*btmap);
}

static void
ti_compute_costs_on_transmaps(translation_instance_t *ti, si_entry_t const *si,
    long fetchlen, long num_pred_transmap_sets,
    pred_transmap_set_t **pred_transmap_sets, double const *pred_transmap_weights)
{
  transmap_set_elem_t *iter, *ls;

  ASSERT(ti->transmap_sets[si->index].head[fetchlen - 1]);
  long num_iters = 0;
  for (iter = ti->transmap_sets[si->index].head[fetchlen - 1]->list;
       iter;
       iter = iter->next) {
#ifdef DEBUG_TMAP_CHOICE_LOGIC
    iter->tmap_elem_id = num_iters;
    num_iters++;
#endif
    //if (si->pc == 0x170000c/* && iter->tmap_elem_id == 13*/) {
    //  DBG_SET(PEEP_SEARCH, 2);
    //}


    DBG2(PEEP_SEARCH, "iter->in_tmap [id %ld]:\n%s\n", iter->tmap_elem_id, transmap_to_string(iter->in_tmap, as1, sizeof as1));

    long peepcost = iter->transmap_set_elem_get_peepcost();
    ASSERT(peepcost < COST_INFINITY);
    DBG2(PEEP_SEARCH, "%lx: num_pred_transmap_sets = %ld\n", (long)si->pc,
        num_pred_transmap_sets);
    /* Now search for optimal transition from predecessor rows */
    iter->num_decisions = num_pred_transmap_sets;
    if (iter->num_decisions) {
      iter->decisions = (transmap_set_elem_t **)malloc
        (iter->num_decisions * sizeof(transmap_set_elem_t *));
      ASSERT(iter->decisions);
    } else {
      iter->decisions = NULL;
    }

    //bool is_live[SRC_NUM_REGS];
    //bool ex_is_live[SRC_NUM_EXREG_GROUPS][SRC_NUM_EXREGS(0)];
    long i, k;
    /*for (k = 0; k < SRC_NUM_REGS; k++) {
      if (regset_belongs(&si->bbl->live_regs[si->bblindex], k)) {
        is_live[k] = true;
      } else {
        is_live[k] = false;
      }
    }*/
    /*for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
      for (k = 0; k < SRC_NUM_EXREGS(i); k++) {
        if (regset_belongs_ex(&si->bbl->live_regs[si->bblindex], i, k)) {
          ex_is_live[i][k] = true;
        } else {
          ex_is_live[i][k] = false;
        }
      }
    }*/

    long cost[num_pred_transmap_sets];
    for (k = 0; k < num_pred_transmap_sets; k++) {
      pred_transmap_set_t *piter;

      cost[k] = COST_INFINITY;
      DBG2(PEEP_SEARCH, "k = %ld, cost[k] %ld\n", k, cost[k]);
      iter->decisions[k] = NULL;

#ifdef DEBUG_TMAP_CHOICE_LOGIC
      if (k < MAX_NUM_PREDS) {
        iter->transcost[k] = COST_INFINITY;
        iter->pred_cost[k] = COST_INFINITY;
        iter->total_cost_on_pred_path[k] = COST_INFINITY;
        iter->pred_decision_pc[k] = (src_ulong)-1;
        iter->pred_decision_id[k] = -1;
      }
#endif
      bool found_pred = false;

      for (piter = pred_transmap_sets[k]; piter; piter = piter->next) {
        transmap_set_elem_t *iter2;
        long transcost, new_cost;

        if (ti->transmap_sets[si->index].head[fetchlen - 1]->list == piter->list) {
          /* I am my own predecessor (loop), ignore. */
          continue;
        }

        for (iter2 = piter->list; iter2; iter2 = iter2->next) {
          //long exitnum;
          found_pred = true;

          //ASSERT(iter2->cost < COST_INFINITY);
          ASSERT(iter2->out_tmaps);

          /*exitnum = array_search(iter2->row->nextpcs, iter2->row->num_nextpcs, si->pc);
          ASSERT(exitnum >= 0 && exitnum < iter2->row->num_nextpcs);*/
          //if a reg is not live-in, do not count its swap in cost
          // neither count its swap out cost
          transmap_t const *prev_out_tmap;
          prev_out_tmap = find_out_tmap_for_nextpc(iter2, si->pc);
          src_ulong prev_pc = (iter2->row && iter2->row->si) ? iter->row->si->pc : PC_UNDEFINED;
          ASSERT(prev_out_tmap);
          ASSERT(iter->in_tmap);
          DBG2(PEEP_SEARCH, "prev_out_tmap:\n%s\n", transmap_to_string(prev_out_tmap, as1, sizeof as1));
          DBG2(PEEP_SEARCH, "iter->in_tmap:\n%s\n", transmap_to_string(iter->in_tmap, as1, sizeof as1));
          transcost = transmap_translation_cost(prev_out_tmap,
              iter->in_tmap/*, &si->bbl->live_regs[si->bblindex]*/, (num_pred_transmap_sets > 1) && (pred_transmap_weights[k] > 0.5) && edge_represents_phi_node_with_permuted_assignment(prev_pc, si->pc));

          if (iter2->cost == COST_INFINITY || transcost == COST_INFINITY) {
            new_cost = COST_INFINITY;
          } else {
            new_cost = /*peepcost + */transcost + iter2->cost;
          }
          DBG2(PEEP_SEARCH, "peepcost %ld, transcost %ld iter2->cost %ld "
              "new_cost %ld, cost[k] %ld\n",
              peepcost, transcost, iter2->cost, new_cost, cost[k]);

          /* DBG(PEEP_SEARCH, "iter->bitmap=%" PRIx64 ", iter2->bitmap=%"
             PRIx64 ".\n", iter->bitmap, iter2->bitmap); */

          if (   new_cost < cost[k]
              || (new_cost != COST_INFINITY && new_cost == cost[k] && transmap_decision_of_same_cost_is_better_than_other_for_nextpc(iter2, iter->decisions[k], si->pc))) {
            cost[k] = new_cost;
            iter->decisions[k] = iter2;

            DBG2(PEEP_SEARCH, "setting decision[%ld] %p, cost[k] %ld, "
                "decision->cost %ld\n", k, iter2, cost[k], iter2->cost);
#ifdef DEBUG_TMAP_CHOICE_LOGIC
            if (k < MAX_NUM_PREDS && iter->decisions[k]) {
              iter->transcost[k] = transcost;
              iter->pred_cost[k] = iter2->cost;
              iter->total_cost_on_pred_path[k] = cost[k];
              if (iter->decisions[k]->row && iter->decisions[k]->row->si) {
                iter->pred_decision_pc[k] = iter->decisions[k]->row->si->pc;
              }
              iter->pred_decision_id[k] = iter->decisions[k]->tmap_elem_id;
            }
#endif
          }
        }
      }

      DBG2(PEEP_SEARCH, "k = %ld, cost[k] %ld\n", k, cost[k]);
      if (!found_pred) {
        ASSERT(cost[k] == COST_INFINITY);
        cost[k] = 0; //this is the case where this pred has not been enumerated; optimistically assign cost 0
      }

      ASSERT(cost[k] <= COST_INFINITY);
      if (cost[k] == COST_INFINITY) {
        iter->decisions[k] = NULL;
      }
      DBG2(PEEP_SEARCH, "k = %ld, cost[k] %ld\n", k, cost[k]);
    }

#ifdef DEBUG_TMAP_CHOICE_LOGIC
    iter->peepcost = peepcost;
#endif
    //iter->cost = num_pred_transmap_sets ? 0 : COST_INFINITY;
    iter->cost = peepcost;
    for (k = 0; k < num_pred_transmap_sets; k++) {
      DBG2(PEEP_SEARCH, "k = %ld, cost[k] %ld\n", k, cost[k]);
      if (cost[k] < COST_INFINITY) {
        iter->cost += pred_transmap_weights[k] * cost[k];
      } else {
        iter->cost = COST_INFINITY;
      }
      DBG2(PEEP_SEARCH, "k = %ld, cost[k] %ld, iter->cost = %ld\n", k, cost[k], iter->cost);
    }
    //ASSERT(iter->cost < COST_INFINITY);
    DBG2(PEEP_SEARCH, "%lx: iter->cost=%ld\n", (long)si->pc, iter->cost);

    DBG(PEEP_SEARCH, "%lx: Finally: iter %p [id %ld], iter->in_tmap:\n%s\niter->cost=%ld\n", (long)si->pc, iter, (long)iter->tmap_elem_id, transmap_to_string(iter->in_tmap, as1, sizeof as1), (long)iter->cost);

    //if (si->pc == 0x170000c/* && iter->tmap_elem_id == 13*/) {
    //  DBG_SET(PEEP_SEARCH, 0);
    //}
  }
  ls = ti->transmap_sets[si->index].head[fetchlen - 1]->list;
  ASSERT(ls);

  /*if (si->pc == 0x100010) {
    DBG_SET(PRUNE, 2);
  }*/
  ls = prune_transmaps(ls, prune_row_size, ti->transmap_set_elem_pruned_out_list);
  CPP_DBG_EXEC2(PRUNE,
      cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": after prune, ls =\n";
      for (auto iter = ls; iter; iter = iter->next) {
        printf("%s() %d: iter %p, iter->in_tmap:\n%s\niter->cost=%ld\n", __func__, __LINE__, iter, transmap_to_string(iter->in_tmap, as1, sizeof as1), (long)iter->cost);
      }
  );
  /*if (si->pc == 0x100010) {
    DBG_SET(PRUNE, 0);
  }*/

  if (!ls) {
    cout << __func__ << " " << __LINE__ << ": si->pc = " << hex << si->pc << dec << endl;
  }
  ASSERT(ls);
  ti->transmap_sets[si->index].head[fetchlen - 1]->list = ls;
  DBG(PEEP_SEARCH, "%lx fetchlen %ld: list=%p\n", (long)si->pc, fetchlen, ls);
}

static void
ti_init_transmap_set(translation_instance_t *ti, long index)
{
  ti->transmap_sets[index].head = NULL;
  ti->transmap_sets[index].head_size = 0;
  /*ti->transmap_sets[index].head = (transmap_set_row_t **)malloc(head_size *
      sizeof(transmap_set_row_t *));
  ti->transmap_sets[index].head_size = head_size;
  memset(ti->transmap_sets[index].head, 0, head_size * sizeof(transmap_set_row_t *));*/
}

static void
transmap_set_alloc(transmap_set_t *set, long head_size)
{
  long i;

  if (set->head_size >= head_size) {
    return;
  }
  set->head = (transmap_set_row_t **)realloc(set->head, head_size *
      sizeof(transmap_set_row_t *));
  ASSERT(set->head);
  for (i = set->head_size; i < head_size; i++) {
    set->head[i] = NULL;
  }
  set->head_size = head_size;
}

static void
transmap_elem_list_set_row(transmap_set_elem_t *list, transmap_set_row_t const *row)
{
  transmap_set_elem_t *cur;

  for (cur = list; cur; cur = cur->next) {
    cur->row = row;
  }
}

static transmap_set_row_t *
transmap_set_row_init(si_entry_t const *si, long fetchlen, nextpc_t *nextpcs,
    long num_nextpcs, src_ulong *lastpcs, double *lastpc_weights, long num_lastpcs)
{
  transmap_set_row_t *ret;

  ret = (transmap_set_row_t *)malloc(sizeof(transmap_set_row_t));
  ASSERT(ret);

  ret->si = si;
  ret->fetchlen = fetchlen;

  //ret->nextpcs = (src_ulong *)malloc(num_nextpcs * sizeof(src_ulong));
  //ASSERT(ret->nextpcs);
  //memcpy(ret->nextpcs, nextpcs, num_nextpcs * sizeof(src_ulong));
  ret->nextpcs = new nextpc_t[num_nextpcs];
  ret->num_nextpcs = num_nextpcs;
  nextpc_t::nextpc_array_copy(ret->nextpcs, nextpcs, num_nextpcs);

  ret->lastpcs = (src_ulong *)malloc(num_lastpcs * sizeof(src_ulong));
  ASSERT(ret->lastpcs);
  memcpy(ret->lastpcs, lastpcs, num_lastpcs * sizeof(src_ulong));

  ret->lastpc_weights = (double *)malloc(num_lastpcs * sizeof(double));
  ASSERT(ret->lastpc_weights);
  memcpy(ret->lastpc_weights, lastpc_weights, num_lastpcs * sizeof(double));

  ret->num_lastpcs = num_lastpcs;

  ret->list = NULL;
  return ret;
}

static void
transmap_set_row_add_list(transmap_set_row_t *row, transmap_set_elem_t *list)
{
  row->list = list;
  transmap_elem_list_set_row(list, row);
}

static void
transmap_set_row_free(transmap_set_row_t *row)
{
  transmap_set_elem_t *cur;

  if (!row) {
    return;
  }
  ASSERT(row->nextpcs);
  free(row->nextpcs);
  row->nextpcs = NULL;

  ASSERT(row->lastpcs);
  free(row->lastpcs);
  row->lastpcs = NULL;

  ASSERT(row->lastpc_weights);
  free(row->lastpc_weights);
  row->lastpc_weights = NULL;

  cur = row->list;
  while (cur) {
    transmap_set_elem_t *next;

    next = cur->next;
    transmap_set_elem_free(cur);
    cur = next;
  }
  row->list = NULL;
  free(row);
}

static void
pred_transmap_sets_free(pred_transmap_set_t **sets, long n)
{
  autostop_timer func_timer(__func__);
  pred_transmap_set_t *cur, *next;
  long i;

  for (i = 0; i < n; i++) {
    for (cur = sets[i]; cur; cur = next) {
      next = cur->next;
      free(cur);
    }
  }
}

static void
si_enumerate_transmaps(translation_instance_t *ti, si_entry_t *si)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << si->pc << dec << endl;
  long num_preds = si_get_num_preds(si);

  pred_transmap_set_t *pred_transmap_sets_for_enum_tmaps[num_preds];
  pred_transmap_set_t *pred_transmap_sets[num_preds];
  transmap_set_elem_t *transmap_elem_list;
  double pred_transmap_weights[num_preds];
  long fetchlen, src_iseq_len, i;
  long num_nextpcs, num_lastpcs;
  src_ulong /**nextpcs, */*lastpcs;
  src_ulong preds[num_preds];
  regset_t *live_outs;
  double *lastpc_weights;
  src_insn_t *src_iseq;
  input_exec_t *in;
  long max_alloc;
  src_ulong pc;
  bool fetch;

  in = ti->in;

  //autostop_timer func2_timer(string(__func__) + ".2");
  /*if (si->is_nop) {
    return;
  }*/
  /*if (si->pc == 0x170019c) {
    DBG_SET(PEEP_SEARCH, 2);
    DBG_SET(PEEP_TAB, 2);
    DBG_SET(PEEP_TRANS, 2);
    DBG_SET(INSN_MATCH, 2);
    DBG_SET(CONNECT_ENDPOINTS, 2);
    DBG_SET(SRC_ISEQ_FETCH, 2);
  }*/

  max_alloc = (in->num_si + 100) * 2;
  //src_iseq = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
  src_iseq = new src_insn_t[max_alloc];
  ASSERT(src_iseq);
  //nextpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  //ASSERT(nextpcs);
  nextpc_t *nextpcs = new nextpc_t[max_alloc];
  ASSERT(nextpcs);
  lastpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  ASSERT(lastpcs);
  lastpc_weights = (double *)malloc(max_alloc * sizeof(double));
  ASSERT(lastpc_weights);
  //live_outs = (regset_t *)malloc(max_alloc * sizeof(regset_t));
  live_outs = new regset_t[max_alloc];
  ASSERT(live_outs);

  //autostop_timer func3_timer(string(__func__) + ".3");
  pc = si->pc;
  si_get_preds(in, si, preds, pred_transmap_weights);
  ti_compute_pred_transmap_sets(ti, si, preds, num_preds, pred_transmap_sets);

  //autostop_timer func4_timer(string(__func__) + ".4");
  memcpy(pred_transmap_sets_for_enum_tmaps, pred_transmap_sets, num_preds * sizeof pred_transmap_sets[0]);
#if ARCH_SRC == ARCH_ETFG
  for (long i = 0; i < num_preds; i++) {
    if (ti->fixed_transmaps.count(preds[i])) {
      pred_transmap_set_t *pts = (pred_transmap_set_t *)malloc(sizeof(pred_transmap_set_t));
      ASSERT(pts);
      pts->list = transmap_set_elem_list_all_possibilities_for_callee_saves();
      pts->next = pred_transmap_sets_for_enum_tmaps[i];
      pred_transmap_sets_for_enum_tmaps[i] = pts;
    }
  }
#endif

  //autostop_timer func5_timer(string(__func__) + ".5");
  for (fetchlen = 1; fetchlen < FETCHLEN_MAX; fetchlen++) {
    //DBG(TMP, "calling src_iseq_fetch(%lx, %ld)\n", (long)pc, fetchlen);
    //if (pc == 0x500004 && fetchlen == 1) {
    //  DBG_SET(SRC_ISEQ_FETCH, 3);
    //}
    //autostop_timer func51_timer(string(__func__) + ".51");
    stopwatch_run(&i386_iseq_fetch_timer);
    fetch = src_iseq_fetch(src_iseq, max_alloc, &src_iseq_len, in, &ti->peep_table, pc, fetchlen,
        &num_nextpcs, nextpcs, NULL, &num_lastpcs, lastpcs, lastpc_weights,
        false);
    stopwatch_stop(&i386_iseq_fetch_timer);
    //if (!num_nextpcs) {
    //  cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ", fetchlen = " << fetchlen << ", src_iseq =\n" << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl;
    //}
    //ASSERT(num_nextpcs);
    if (!fetch) {
      DBG(SRC_ISEQ_FETCH, "src_iseq_fetch(%lx, %ld) returned false\n", (long)pc, fetchlen);
      //if (pc == 0x500004 && fetchlen == 1) {
      //  DBG_SET(SRC_ISEQ_FETCH, 0);
      //}
      break;
    }
    //if (pc == 0x100096 && fetchlen == 12) {
    //  DBG_SET(SRC_ISEQ_FETCH, 3);
    //  DBG_SET(PEEP_TAB, 2);
    //  DBG_SET(INSN_MATCH, 2);
    //  DBG_SET(ENUM_TMAPS, 2);
    //  DBG_SET(PEEP_SEARCH, 2);
    //}


    //autostop_timer func52_timer(string(__func__) + ".52");
    obtain_live_outs(in, live_outs, nextpcs, num_nextpcs);

    DBG_EXEC(SRC_ISEQ_FETCH,
        dbg("src_iseq_fetch(%lx, %ld) returned %d: %s\n",
            (long)pc, fetchlen, fetch, src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
        dbg("num_nextpcs=%ld, nextpcs:",
            num_nextpcs);
        long i;
        for (i = 0; i < num_nextpcs; i++) {
          printf(" %lx", (long)nextpcs[i].get_val());
        }
        printf("\nnum_lastpcs=%ld, lastpcs:", num_lastpcs);
        for (i = 0; i < num_lastpcs; i++) {
          printf(" %lx", (long)lastpcs[i]);
        }
        printf("\n");
    );

    //autostop_timer func53_timer(string(__func__) + ".53");
    src_iseq_len = src_iseq_canonicalize(src_iseq, nullptr, src_iseq_len);
    DBG(SRC_ISEQ_FETCH, "%lx fetchlen %ld: src_iseq: %s\n",
        (long)pc, fetchlen, src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));

    if (   fetchlen <= ti->transmap_sets[si->index].head_size
        && ti->transmap_sets[si->index].head[fetchlen - 1]) {
      transmap_elem_list = ti->transmap_sets[si->index].head[fetchlen - 1]->list;
      ASSERT(transmap_elem_list);
    } else {
      transmap_elem_list = NULL;
    }

    //autostop_timer func54_timer(string(__func__) + ".54");
    DBG(SRC_ISEQ_FETCH, "%lx %ld: calling peep_enumerate_transmaps()\n", (long)pc,
        fetchlen);
    transmap_elem_list = peep_enumerate_transmaps(ti, si, src_iseq, src_iseq_len,
        fetchlen, live_outs, num_nextpcs, &ti->peep_table, pred_transmap_sets_for_enum_tmaps,
        num_preds, transmap_elem_list);
    DBG(SRC_ISEQ_FETCH, "%lx %ld: returned from peep_enumerate_transmaps(). transmap_elem_list = %p\n",
        (long)pc, fetchlen, transmap_elem_list);
    //autostop_timer func55_timer(string(__func__) + ".55");

    //autostop_timer func7_timer(string(__func__) + ".7");
    if (!transmap_elem_list) {
      /*if (!peep_entry_exists_for_prefix_insns(&ti->peep_table, src_iseq, src_iseq_len)) {
        break;
      } else */{
        //if (pc == 0x100096 && fetchlen == 12) {
        //  DBG_SET(SRC_ISEQ_FETCH, 0);
        //  DBG_SET(PEEP_TAB, 0);
        //  DBG_SET(INSN_MATCH, 0);
        //  DBG_SET(ENUM_TMAPS, 0);
        //  DBG_SET(PEEP_SEARCH, 0);
        //}
        continue;
      }
    }
    //stopwatch_run(&peep_search_timer);
    //transmap_elem_list = peep_search_transmaps(ti, si, src_iseq, src_iseq_len, fetchlen,
    //    &ti->peep_table, live_outs, num_nextpcs, transmap_elem_list);
    //stopwatch_stop(&peep_search_timer);
    //DBG(SRC_ISEQ_FETCH, "%lx %ld: returned from peep_search_transmaps(). transmap_elem_list = %p\n",
    //    (long)pc, fetchlen, transmap_elem_list);
    ASSERT(transmap_elem_list);
    /*if (!transmap_elem_list) {
      //ASSERT(!ti->transmap_sets[si->index].head[fetchlen - 1]);
      break;
    }*/
    //autostop_timer func8_timer(string(__func__) + ".8");
    transmap_set_alloc(&ti->transmap_sets[si->index], fetchlen);
    if (!ti->transmap_sets[si->index].head[fetchlen - 1]) {
      ti->transmap_sets[si->index].head[fetchlen - 1] = transmap_set_row_init(si,
        fetchlen, nextpcs, num_nextpcs, lastpcs, lastpc_weights, num_lastpcs);
    }
    transmap_set_row_add_list(ti->transmap_sets[si->index].head[fetchlen - 1],
        transmap_elem_list);

    ti_add_pred_to_nextpcs(ti, nextpcs, num_nextpcs, si, fetchlen, lastpcs,
        num_lastpcs);

    ti_compute_costs_on_transmaps(ti, si, fetchlen, num_preds, pred_transmap_sets,
        pred_transmap_weights);
    //if (pc == 0x100096 && fetchlen == 12) {
    //  DBG_SET(SRC_ISEQ_FETCH, 0);
    //  DBG_SET(PEEP_TAB, 0);
    //  DBG_SET(INSN_MATCH, 0);
    //  DBG_SET(ENUM_TMAPS, 0);
    //  DBG_SET(PEEP_SEARCH, 0);
    //}
  }
  //autostop_timer func6_timer(string(__func__) + ".6");
  pred_transmap_sets_free(pred_transmap_sets_for_enum_tmaps, num_preds); //freeing pred_transmap_sets_for_enum_tmaps also frees pred_transmap_sets because the former is a superset of the latter
  //free(src_iseq);
  delete [] src_iseq;
  //free(nextpcs);
  delete[] nextpcs;
  free(lastpcs);
  free(lastpc_weights);
  //free(live_outs);
  delete [] live_outs;
  /*if (si->pc == 0x170019c) {
    DBG_SET(PEEP_SEARCH, 0);
    DBG_SET(PEEP_TAB, 0);
    DBG_SET(PEEP_TRANS, 0);
    DBG_SET(INSN_MATCH, 0);
    DBG_SET(CONNECT_ENDPOINTS, 0);
    DBG_SET(SRC_ISEQ_FETCH, 0);
  }*/
}

static void
ti_init_pred_set_and_transmap_sets(translation_instance_t *ti)
{
  input_exec_t *in = ti->in;
  long i;

  ti->pred_set = (pred_t **)malloc(in->num_si * sizeof(pred_t *));
  ASSERT(ti->pred_set);
  memset(ti->pred_set, 0, in->num_si * sizeof(pred_t *));

  ti->transmap_sets = (transmap_set_t *)malloc(in->num_si * sizeof(transmap_set_t));
  ASSERT(ti->transmap_sets);

  for (i = 0; i < in->num_si; i++) {
    ti_init_transmap_set(ti, i);
  }
  ti->transmap_set_elem_pruned_out_list = NULL;
}

static void
enumerate_transmaps(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << endl;
  input_exec_t *in = ti->in;
  long i;

  ti_init_pred_set_and_transmap_sets(ti);
  //autostop_timer func2_timer(string(__func__) + ".2");

  int iter;
  //int max_iter = (use_fast_mode == 2)?1:2;
  int max_iter = 4; //XXX: making this >=2 works, but it results in memory leaks, e.g., fixed_transmaps are allocated over and over again
  for (iter = 0; iter < max_iter; iter++) {
    for (i = 0; i < in->num_si; i++) {
      if (i % 1024 == 0) {
        PROGRESS("enum_tmaps iter %d/%d, insn %ld/%ld [%.0f%%]", iter, max_iter - 1, i,
            in->num_si - 1, (double)(iter*in->num_si + i)*100/(max_iter * in->num_si));
      }
      si_enumerate_transmaps(ti, &in->si[i]);
      //autostop_timer func2_timer(string(__func__) + ".3");
    }
  }
}

static bool
transmap_set_is_empty(transmap_set_t const *set)
{
  long i;

  for (i = 0; i < set->head_size; i++) {
    if (set->head[i] && set->head[i]->list) {
      return false;
    }
  }
  return true;
}

static src_ulong
highest_weight_nextpc(input_exec_t const *in, transmap_set_row_t const *row)
{
  double nextpc_weights[16];
  long i, j, num_nextpcs;
  nextpc_t nextpcs[16];
  src_ulong lastpc, ret;
  si_entry_t *lsi;

  if (row->num_lastpcs == 0) {
    return PC_UNDEFINED;
  }
  i = array_max(row->lastpc_weights, row->num_lastpcs);
  ASSERT(i >= 0 && i < row->num_lastpcs);

  lastpc = row->lastpcs[i];

  num_nextpcs = pc_get_nextpcs(in, lastpc, nextpcs, nextpc_weights);

  //cout << __func__ << " " << __LINE__ << ": lastpc = " << hex << lastpc << dec << ": nextpcs =";
  //for (long n = 0; n < num_nextpcs; n++) {
  //  cout << " " << hex << nextpcs[n] << dec << " (" << nextpc_weights[n] << ")\n";
  //}

  i = array_max(nextpc_weights, num_nextpcs);
  ASSERT(i >= 0 && i < num_nextpcs);
  ret = nextpcs[i].get_val();
  return ret;
}

static src_ulong
highest_weight_nextpc_that_has_made_decision(translation_instance_t const *ti,
    transmap_set_row_t const *row)
{
  double nextpc_weights[16];
  input_exec_t const *in;
  long i, j, num_nextpcs;
  nextpc_t nextpcs[16];
  src_ulong lastpc, ret;
  si_entry_t *lsi;

  i = array_max(row->lastpc_weights, row->num_lastpcs);
  ASSERT(i >= 0 && i < row->num_lastpcs);

  lastpc = row->lastpcs[i];

  in = ti->in;

  num_nextpcs = pc_get_nextpcs(in, lastpc, nextpcs, nextpc_weights);

  //cout << __func__ << " " << __LINE__ << ": lastpc = " << hex << lastpc << dec << ": nextpcs =";
  //for (long n = 0; n < num_nextpcs; n++) {
  //  cout << " " << hex << nextpcs[n] << dec << " (" << nextpc_weights[n] << ")\n";
  //}

  for (i = 0; i < num_nextpcs; i++) {
    long index;

    if (nextpcs[i].get_val() == PC_UNDEFINED || nextpcs[i].get_val() == PC_JUMPTABLE) {
      continue;
    }
    index = pc2index(in, nextpcs[i].get_val());
    ASSERT(index >= 0 && index < ti->in->num_si);
    if (!ti->transmap_decision[index]) {
      nextpc_weights[i] = 0;
    }
  }
  //for (long n = 0; n < num_nextpcs; n++) {
  //  cout << " " << hex << nextpcs[n] << dec << " (" << nextpc_weights[n] << ")\n";
  //}
  i = array_max(nextpc_weights, num_nextpcs);
  ASSERT(i >= 0 && i < num_nextpcs);
  //cout << "i = " << i << ": " << hex << nextpcs[i] << dec << " (" << nextpc_weights[i] << ")\n";


  ret = nextpcs[i].get_val();
  return ret;
}

static bool
all_successors_have_made_decision(translation_instance_t const *ti, si_entry_t const *si)
{
  transmap_set_row_t *row;
  long n, nindex;
  src_ulong next;

  for (n = 0; n < ti->transmap_sets[si->index].head_size; n++) {
    if (   !ti->transmap_sets[si->index].head[n]
        || !ti->transmap_sets[si->index].head[n]->list) {
      continue;
    }
    row = ti->transmap_sets[si->index].head[n];
    next = highest_weight_nextpc(ti->in, row);

    if (next == PC_UNDEFINED || next == PC_JUMPTABLE) {
      continue;
    }
    nindex = pc2index(ti->in, next);
    ASSERT(nindex >= 0 && nindex < ti->in->num_si);
    if (   !ti->transmap_decision[nindex]
        && !transmap_set_is_empty(&ti->transmap_sets[nindex])) {
      DBG2(CHOOSE_SOLUTION, "returning false for %lx due to %lx\n", (long)si->pc,
          (long)next);
      return false;
    }
  }
  DBG2(CHOOSE_SOLUTION, "returning true for %lx\n", (long)si->pc);
  return true;
}

static void
propagate(translation_instance_t *ti, transmap_set_elem_t *winner/*, bool *visited*/)
{
  transmap_set_elem_t *prev, *old;
  long index, i, j;
  src_ulong pc;
  //ASSERT(visited);
  if (!winner || !winner->row) {
    return;
  }

  pc = winner->row->si->pc;
  if (pc == PC_UNDEFINED || pc == PC_JUMPTABLE) {
    return;
  }
  index = pc2index(ti->in, pc);
  ASSERT(index >= 0 && index < ti->in->num_si);
  //if (visited[index]) {
  //  return;
  //}
  old = ti->transmap_decision[index];
  if (winner == old) {
    return;
  }

  DBG(CHOOSE_SOLUTION, "setting decision for %lx to %p [id %ld], num_decisions=%ld\n", (long)pc,
      winner, (long)winner->tmap_elem_id, winner->num_decisions);
  ti->transmap_decision[index] = winner;
  //visited[index] = true;
  //if (depth > 0 && old == winner) {
  //  DBG(CHOOSE_SOLUTION, "%lx: decision remained unchanged; returning without propagating ruther\n", (long)pc);
  //  return;
  //}
  for (i = 0; i < winner->num_decisions; i++) {
    prev = winner->decisions[i];
    src_ulong pred_highest_weight_nextpc = (prev && prev->row) ? highest_weight_nextpc_that_has_made_decision(ti, prev->row) : PC_UNDEFINED;
    DBG2(CHOOSE_SOLUTION, "%lx: pred=%lx, "
        "highest_weight_nextpc_that_has_made_decision(%lx)=%lx\n", (long)pc,
        (prev && prev->row)?
        (long)prev->row->si->pc:(long)PC_UNDEFINED,
        (prev && prev->row)?
        (long)prev->row->si->pc:(long)PC_UNDEFINED,
        (long)pred_highest_weight_nextpc);
    if (   prev
        && prev->row
        && pred_highest_weight_nextpc == pc) {
      propagate(ti, prev/*, visited*/);
    }
  }
}

static void
pick_best_transmap_and_propagate(translation_instance_t *ti, si_entry_t const *si/*,
    bool *visited*/)
{
  transmap_set_elem_t *best, *iter;
  long i, n, bestcost = 0, cur;
  transmap_set_row_t *row;
  bool row_has_successor = false;

  cur = si->index;
  best = NULL;

  //ASSERT(visited);
  //if (/*visited && */visited[cur]) {
  //  return;
  //}
  /* find the best cost in the maximum non-NULL fetchlen row. */
  for (i = ti->transmap_sets[cur].head_size - 1; i >= 0; i--) {
    row = ti->transmap_sets[cur].head[i];
    if (row) {
      /*row_has_successor = false;
      for (n = 0; n < row->num_nextpcs; n++) {
        if (row->nextpcs[n] != PC_UNDEFINED && row->nextpcs[n] != PC_JUMPTABLE) {
          row_has_successor = true;
        }
      }
      DBG(CHOOSE_SOLUTION, "si->pc = %lx, visited = %p, row_has_successor = %d\n", (long)si->pc, visited, row_has_successor);
      if (visited && row_has_successor) {
        return;
      }*/
      for (iter = row->list; iter; iter = iter->next) {
        if (!best || bestcost > iter->cost) {
          bestcost = iter->cost;
          best = iter;
        }
      }
      ASSERT(best);
      DBG(CHOOSE_SOLUTION, "si->pc = %lx, row_has_successor = %d, found best cost decision at fetchlen %ld, tmap_elem_id %ld, cost %lld\n", (long)si->pc, row_has_successor, i, best->tmap_elem_id, (long long)bestcost);
      break;
    }
  }

  //if (visited) {
    DBG(CHOOSE_SOLUTION, "picked best transmap for %lx, cost %lx, best = %p, best->row = %p, propagating\n", (long)si->pc,
        bestcost, best, best ? best->row:NULL);
    //if (!best) {
    //  best = ti->transmap_decision[si->index];
    //}
    //if (best) {
    //  ASSERT(best->row);
    //  ASSERT(best->row->si == si);
    //}
    propagate(ti, best/*, visited*/);
  //} else {
  //  DBG(CHOOSE_SOLUTION, "setting transmap decision for %lx (base case)\n", (long)si->pc);
  //  ti->transmap_decision[si->index] = best;
  //}
}

static void
eliminate_insns_with_overlapping_decisions(translation_instance_t *ti)
{
  src_ulong pcs[ti->in->num_si + 100];
  long i, j, s, fetchlen, index, len;
  input_exec_t *in;
  long *sorted_si;
  bool ret;

  in = ti->in;
  ASSERT(!ti->insn_eliminated);
  ti->insn_eliminated = (bool *)malloc(in->num_si * sizeof(bool));
  ASSERT(ti->insn_eliminated);

  for (i = 0; i < in->num_si; i++) {
    ti->insn_eliminated[i] = false;
  }

  sorted_si = (long *)malloc(in->num_si * sizeof(long));
  ASSERT(sorted_si);

  /* we are giving preference to si's in static order. this can be
     changed to look at execution frequencies, in-flow, etc. in future. */
  for (i = 0; i < in->num_si; i++) {
    sorted_si[i] = i;
  }

  for (i = 0; i < in->num_si; i++) {
    s = sorted_si[i];
    ASSERT(s >= 0 && s < in->num_si);
    if (!ti->transmap_decision[s]) {
      continue;
    }
    ASSERT(ti->transmap_decision[s]->row);
    fetchlen = ti->transmap_decision[s]->row->fetchlen;
    ret = src_iseq_fetch(NULL, 0, &len, in, NULL, in->si[s].pc, fetchlen,
        NULL, NULL, pcs, NULL, NULL, NULL, false);
    ASSERT(ret);
    for (j = 0; j < len; j++) {
      if (pcs[j] == in->si[s].pc) {
        continue;
      }
      index = pc2index(in, pcs[j]);
      if (index == -1) {
        ASSERT(pcs[j] == 0 || pcs[j] == INSN_PC_INVALID);
        continue;
      }
      ti->transmap_decision[index] = NULL;
    }
  }
  free(sorted_si);
}

void
translation_instance_init(translation_instance_t *ti, translation_mode_t mode)
{
  ti->jumptable_stub_code_buf = NULL;
  ti->jumptable_stub_code_offsets = 0;
  ti->mode = mode;
  if (mode == FBGEN) {
    ti->set_base_reg(-1);
  } else {
#if ARCH_SRC == ARCH_ETFG
    ti->set_base_reg(R_ESP);
#else
    ti->set_base_reg(-1);
#endif
  }

  peep_table_init(&ti->peep_table);

  ti->superoptdb_server = NULL;
  ti->in = NULL;
  ti->out = NULL;

  ti->pred_set = NULL;
  ti->transmap_sets = NULL;
  ti->transmap_decision = NULL;

  ti->peep_in_tmap = NULL;
  ti->peep_out_tmaps = NULL;
  ti->peep_regcons = NULL;
  ti->peep_temp_regs_used = NULL;
  ti->peep_nomatch_pairs_set = NULL;
  ti->touch_not_live_in = NULL;

  ti->insn_eliminated = NULL;
  ti->bbl_headers = NULL;
  ti->bbl_header_sizes = NULL;

  ti->reloc_strings = NULL;
  ti->reloc_strings_size = 0;

  ti->code_gen_buffer = NULL;
  myhash_init(&ti->tcodes, imap_hash_func, imap_equal_func, NULL);
  ti->codesize = 0;
  //myhash_init(&tcodes, imap_hash_func, imap_equal_func, NULL);
}

//static void
//input_exec_locals_info_free(input_exec_t *in)
//{
//  if (!in->locals_info) {
//    return;
//  }
//  for (long i = 0; i < in->num_si; i++) {
//    //de-duplicate locals_info[i] pointers
//    //locals_info_delete(in->locals_info[i]);
//  }
//  free(in->locals_info);
//  //locals_info_delete(locals_info);
//}

bool
input_exec_addr_belongs_to_rodata_section(input_exec_t const *in, src_ulong val)
{
  input_section_t const *rodata_section;

  rodata_section = input_exec_get_rodata_section(in);
  if (!rodata_section) {
    return false;
  }
  if (   val >= rodata_section->addr
      && val < rodata_section->addr + rodata_section->size) {
    return true;
  }
  return false;
}

void
input_exec_add_rodata_ptr(input_exec_t *in, src_ulong val)
{
  long i;

  for (i = 0; i < in->num_rodata_ptrs; i++) {
    if (in->rodata_ptrs[i] == val) {
      return;
    }
  }
  ASSERT(in->num_rodata_ptrs <= in->rodata_ptrs_size);
  if (in->num_rodata_ptrs == in->rodata_ptrs_size) {
    in->rodata_ptrs_size = (in->rodata_ptrs_size + 1) * 2;
    in->rodata_ptrs = (src_ulong *)realloc(in->rodata_ptrs,
        in->rodata_ptrs_size * sizeof(src_ulong));
    ASSERT(in->rodata_ptrs);
  }
  ASSERT(in->num_rodata_ptrs < in->rodata_ptrs_size);
  in->rodata_ptrs[in->num_rodata_ptrs] = val;
  in->num_rodata_ptrs++;
}

static void
output_exec_free(output_exec_t *out)
{
  long i;
  if (out->generated_relocs) {
    free(out->generated_relocs);
    out->generated_relocs = NULL;
  }
  for (i = 0; i < out->num_helper_sections; i++) {
    free(out->helper_sections[i].name);
    free(out->helper_sections[i].data);
  }
  free(out->generated_symbols);
  free(out);
}

static uint8_t *
alloc_code_gen_buffer(size_t sz)
{
  uint8_t *ret;
  ret = (uint8_t *)mmap(TX_TEXT_START_PTR, CODE_GEN_BUFFER_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  ASSERT(ret == TX_TEXT_START_PTR);
  return ret;
}

static void
free_code_gen_buffer(uint8_t *code_gen_buffer)
{
  int rc = munmap(code_gen_buffer, CODE_GEN_BUFFER_SIZE);
  if (rc != 0) {
    printf("munmap failed: %s\n", strerror(errno));
  }
  ASSERT(rc == 0);
}

void
translation_instance_free_tx_structures(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  pred_t *pred, *nextpred;
  input_exec_t *in;
  long i, j;

  in = ti->in;
  if (ti->pred_set) {
    for (i = 0; i < in->num_si; i++) {
      for (pred = ti->pred_set[i]; pred; pred = nextpred) {
        free(pred->lastpcs);
        nextpred = pred->next;
        free(pred);
      }
    }
    free(ti->pred_set);
    ti->pred_set = NULL;
  }
  if (ti->transmap_sets) {
    for (i = 0; i < in->num_si; i++) {
      for (j = 0; j < ti->transmap_sets[i].head_size; j++) {
        transmap_set_row_free(ti->transmap_sets[i].head[j]);
      }
      if (ti->transmap_sets[i].head) {
        free(ti->transmap_sets[i].head);
      }
    }
    free(ti->transmap_sets);
    ti->transmap_sets = NULL;
  }
  if (ti->peep_in_tmap) {
    //free(ti->peep_in_tmap);
    delete [] ti->peep_in_tmap;
    ti->peep_in_tmap = NULL;
  }
  if (ti->peep_out_tmaps) {
    for (i = 0; i < in->num_si; i++) {
      if (ti->peep_out_tmaps[i]) {
        //free(ti->peep_out_tmaps[i]);
        delete [] ti->peep_out_tmaps[i];
        ti->peep_out_tmaps[i] = NULL;
      }
    }
    free(ti->peep_out_tmaps);
    ti->peep_out_tmaps = NULL;
  }
  if (ti->peep_regcons) {
    //free(ti->peep_regcons);
    delete [] ti->peep_regcons;
    ti->peep_regcons = NULL;
  }
  if (ti->peep_temp_regs_used) {
    delete [] ti->peep_temp_regs_used;
    ti->peep_temp_regs_used = NULL;
  }
  if (ti->peep_nomatch_pairs_set) {
    delete [] ti->peep_nomatch_pairs_set;
    ti->peep_nomatch_pairs_set = NULL;
  }
  if (ti->touch_not_live_in) {
    free(ti->touch_not_live_in);
    ti->touch_not_live_in = NULL;
  }
  if (ti->transmap_decision) {
    free(ti->transmap_decision);
    ti->transmap_decision = NULL;
  }

  if (ti->in) {
    input_exec_free(ti->in);
    //free(in);
    delete ti->in;
    ti->in = NULL;
  }
  if (ti->out) {
    output_exec_free(ti->out);
    ti->out = NULL;
  }
  if (ti->reloc_strings) {
    free(ti->reloc_strings);
    ti->reloc_strings = NULL;
  }
  if (ti->jumptable_stub_code_offsets) {
    free(ti->jumptable_stub_code_offsets);
    ti->jumptable_stub_code_offsets = NULL;
    myhash_destroy(&ti->indir_eip_map, imap_elem_free);
  }
  if (ti->insn_eliminated) {
    free(ti->insn_eliminated);
    ti->insn_eliminated = NULL;
  }
  if (ti->code_gen_buffer) {
    free_code_gen_buffer(ti->code_gen_buffer);
    ti->code_gen_buffer = NULL;
  }
  if (ti->superoptdb_server) {
    free(ti->superoptdb_server);
    ti->superoptdb_server = NULL;
  }
  myhash_clear(&ti->tcodes, imap_elem_free);
}

void
translation_instance_free(translation_instance_t *ti)
{
  translation_instance_free_tx_structures(ti);
  peephole_table_free(&ti->peep_table);
  myhash_destroy(&ti->tcodes, NULL);
}

int
is_frequent_nip(unsigned long nip)
{
  return is_executed_more_than(nip, 1000);
}

char const *
translation_mode_to_string(translation_mode_t mode, char *buf, int size)
{
  char *ptr = buf, *end = ptr + size;
  switch (mode) {
    case FBGEN: ptr += snprintf(ptr, end - ptr, "fbgen"); break;
    //case SUBSETTING: ptr += snprintf(ptr, end - ptr, "subsetting"); break;
    case ANALYZE_STACK: ptr += snprintf(ptr, end - ptr, "analyze_stack"); break;
    case UNROLL_LOOPS: ptr += snprintf(ptr, end - ptr, "unroll_loops"); break;
    case SUPEROPTIMIZE: ptr += snprintf(ptr, end - ptr, "superoptimize"); break;
    case TRANSLATION_MODE0: ptr += snprintf(ptr, end - ptr, "translation_mode0"); break;
    case TRANSLATION_MODE_END: ptr += snprintf(ptr, end - ptr, "translation_mode_end");
                               break;
    default: NOT_REACHED();
  }
  return buf;
}

static void
preds_print(FILE *file, translation_instance_t const *ti)
{
  input_exec_t const *in;
  long i;

  in = ti->in;
  fprintf(file, "\n\nPrinting predecessors:\n");
  for (i = 0; i < in->num_si; i++) {
    pred_t *pred;
    long j;

    fprintf(file, "\n%lx:\n", (long)in->si[i].pc);
    for (pred = ti->pred_set[i]; pred; pred = pred->next) {
      fprintf(file, "\t[ %lx, %ld ] through ", (long)pred->si->pc, pred->fetchlen);
      for (j = 0; j < pred->num_lastpcs; j++) {
        fprintf(file, "%c%lx", j?' ':'(', (long)pred->lastpcs[j]);
      }
      fprintf(file, ")\n");
    }
  }
}

static void
peep_tmaps_and_regcons_print(FILE *file, translation_instance_t const *ti)
{
  input_exec_t const *in;

  in = ti->in;
  fprintf(file, "\n\nPrinting peep transmaps and regcons:\n");
  for (long i = 0; i < in->num_si; i++) {
    transmap_set_elem_t const *d = ti->transmap_decision[i];
    fprintf(file, "%lx:\n", (long)in->si[i].pc);
    if (!d) {
      fprintf(file, "null decision\n");
      continue;
    }
    transmap_t const *peep_in_tmap = &ti->peep_in_tmap[i];
    transmap_t const *peep_out_tmaps = ti->peep_out_tmaps[i];
    regcons_t const *peep_regcons = &ti->peep_regcons[i];
    regset_t const *peep_temp_regs_used = &ti->peep_temp_regs_used[i];
    nomatch_pairs_set_t const *peep_nomatch_pairs_set = &ti->peep_nomatch_pairs_set[i];
    transmap_set_row_t const *row = d->row;
    fprintf(file, "peep_in_tmap:\n%s\n",
        transmap_to_string(peep_in_tmap, as1, sizeof as1));
    fprintf(file, "out_tmaps (num %ld):\n", row->num_nextpcs);
    for (int i = 0; i < row->num_nextpcs; i++) {
      fprintf(file, "%s--\n",
         transmap_to_string(&peep_out_tmaps[i], as1, sizeof as1));
    }
    fprintf(file, "peep_regcons:\n%s\n",
        regcons_to_string(peep_regcons, as1, sizeof as1));
    fprintf(file, "peep_temp_regs_used:\n%s\n",
        regset_to_string(peep_temp_regs_used, as1, sizeof as1));
    fprintf(file, "nomatch_pairs_set:\n%s\n",
        nomatch_pairs_set_to_string(*peep_nomatch_pairs_set, as1, sizeof as1));
    fprintf(file, "\n\n");
  }
}

static void
enumerated_transmaps_print(FILE *file, translation_instance_t const *ti)
{
  transmap_set_elem_t const *cur;
  transmap_set_row_t const *row;
  long i, len, n, tmap_num;
  input_exec_t const *in;

  in = ti->in;
  fprintf(file, "\n\nPrinting enumerated transmaps:\n");
  for (i = 0; i < in->num_si; i++) {
    for (len = 0; len < ti->transmap_sets[i].head_size; len++) {
      row = ti->transmap_sets[i].head[len];
      if (!row) {
        continue;
      }
      fprintf(file, "%lx fetchlen %ld:\n", (long)in->si[i].pc, len + 1);
      fprintf(file, "  nextpcs:");
      for (n = 0; n < row->num_nextpcs; n++) {
        fprintf(file, " %lx", (long)row->nextpcs[n].get_val());
      }
      fprintf(file, "\n");
      fprintf(file, "  lastpcs:");
      for (n = 0; n < row->num_lastpcs; n++) {
        fprintf(file, " %lx[%f]", (long)row->lastpcs[n], row->lastpc_weights[n]);
      }
      fprintf(file, "\n");
      for (cur = row->list, tmap_num = 0; cur; cur = cur->next, tmap_num++) {
        fprintf(file, "  in_tmap %ld (cost %ld)", tmap_num, cur->cost);
        fprintf(file, " peep line number %ld\n", cur->get_peeptab_entry_id()->peep_entry_get_line_number());
        if (ti->transmap_decision) {
          fprintf(file, "  %s", (cur == ti->transmap_decision[i]) ? " WINNER" : " ");
        }
#ifdef DEBUG_TMAP_CHOICE_LOGIC
        fprintf(file, " [id %ld, peepcost %ld; ", cur->tmap_elem_id, cur->peepcost);
        for (size_t p = 0; p < min((long)MAX_NUM_PREDS, cur->num_decisions); p++) {
          fprintf(file, "trans%zd %ld, pred%zd %ld, total_pred%zd %ld decision_pc%zd %lx decision_id%zd %ld; ", p, cur->transcost[p], p, cur->pred_cost[p], p, cur->total_cost_on_pred_path[p], p, (long)cur->pred_decision_pc[p], p, cur->pred_decision_id[p]);
        }
        fprintf(file, "]");
#endif
        fprintf(file, "\n  %s\n",
            transmap_to_string(cur->in_tmap, as1, sizeof as1));
        fprintf(file, "  out_tmaps (num %ld):\n", row->num_nextpcs);
        for (int i = 0; i < row->num_nextpcs; i++) {
          fprintf(file, "%s--\n",
             transmap_to_string(&cur->out_tmaps[i], as1, sizeof as1));
        }
      }
    }
    fprintf(file, "\n\n");
  }
}

static void
transmap_decisions_print(FILE *file, translation_instance_t const *ti)
{
  transmap_set_elem_t const *elem;
  long i, j;

  fprintf(logfile, "\n\nPrinting transmap decisions:\n");
  for (i = 0; i < ti->in->num_si; i++) {
    elem = ti->transmap_decision[i];
    peep_entry_t const *peep_entry = elem ? elem->get_peeptab_entry_id() : NULL;
    fprintf(logfile, "\n%lx: fetchlen %lx: peep_entry line number %ld", (long)ti->in->si[i].pc, elem ? elem->row->fetchlen : 1, peep_entry ? peep_entry->peep_entry_get_line_number() : -1);
    if (!elem) {
      fprintf(logfile, " (null)\n");
      continue;
    }
    fprintf(logfile, "\nin_tmap:\n%s\n",
        transmap_to_string(elem->in_tmap, as1, sizeof as1));
    for (j = 0; j < elem->row->num_nextpcs; j++) {
      fprintf(logfile, "\nout_tmap[%ld] for nextpc %lx:\n%s\n", j,
          (long)elem->row->nextpcs[j].get_val(),
          transmap_to_string(&elem->out_tmaps[j], as1, sizeof as1));
    }
  }
  fprintf(logfile, "\n\nPrinting stack alloc maps:\n");
  for (const auto &fs : ti->stack_slot_map_for_function) {
    for (const auto &ps : fs.second) {
      fprintf(logfile, "%s : %s :\n%s\n", fs.first.c_str(), ps.first.to_string().c_str(), ps.second.to_string().c_str());
    }
  }
  fprintf(logfile, "\n");
}


void
get_live_outs(input_exec_t const *in, regset_t const **live_outs,
    src_ulong const *pcs, long num_pcs)
{
  //input_exec_t const *in;
  long i, index;

  //in = ti->in;
  for (i = 0; i < num_pcs; i++) {
    if (pcs[i] == PC_JUMPTABLE || pcs[i] == PC_UNDEFINED) {
      live_outs[i] = regset_all();
      continue;
    }
    index = pc2index(/*ti->*/in, pcs[i]);
    if (index == -1) {
      live_outs[i] = regset_all();
    } else {
      live_outs[i] = &in->si[index].bbl->live_regs[in->si[index].bblindex];
    }
  }
}

static void
get_tmaps(translation_instance_t const *ti, transmap_t const **tmaps,
    nextpc_t const *pcs, long num_pcs)
{
  long i, index;

  for (i = 0; i < num_pcs; i++) {
    if (pcs[i].get_val() == PC_JUMPTABLE || pcs[i].get_val() == PC_UNDEFINED) {
      tmaps[i] = transmap_default();
      continue;
    }
    index = pc2index(ti->in, pcs[i].get_val());
    if (index == -1) {
      tmaps[i] = transmap_default();
    } else if (!ti->transmap_decision[index]) {
      /*printf("Warning: transmap_decision[%ld] (%lx) is null!\n", index,
          (long)pcs[i]);*/
      tmaps[i] = transmap_fallback();
    } else {
      tmaps[i] = ti->transmap_decision[index]->in_tmap;
    }
  }
}

static long
si_gen_dst_iseq(translation_instance_t *ti, si_entry_t const *si,
    ///*struct chash const *tcodes, */map<src_ulong, dst_insn_t const *> const *tinsns_map,
    dst_insn_t *dst_iseq, size_t dst_iseq_size,
    transmap_t *out_tmaps, transmap_t *peep_in_tmap, transmap_t *peep_out_tmaps,
    regcons_t *peep_regcons,
    regset_t *peep_temp_regs_used,
    nomatch_pairs_set_t *peep_nomatch_pairs_set,
    regcount_t *touch_not_live_in)
{
  long src_iseq_len, dst_iseq_len, num_nextpcs;
  nomatch_pairs_set_t l_peep_nomatch_pairs_set;
  eqspace::state const *start_state = NULL;
  regcount_t l_touch_not_live_in;
  regset_t l_peep_temp_regs_used;
  transmap_set_elem_t const *d;
  transmap_t *l_peep_out_tmaps;
  long i, fetchlen, max_alloc;
  transmap_t l_peep_in_tmap;
  regcons_t l_peep_regcons;
  transmap_t *l_out_tmaps;
  input_exec_t const *in;
  src_insn_t *src_iseq;
  regset_t *live_outs;
  nextpc_t *nextpcs;
  bool fetch;

  ASSERT(ti->transmap_decision);
  d = ti->transmap_decision[si->index];
  in = ti->in;

#if ARCH_SRC == ARCH_ETFG
  start_state = ti->ti_get_input_state_at_etfg_pc(si->pc); //&etfg_pc_get_input_state(in, si->pc);
#endif

  max_alloc = (in->num_si + 100) * 2;

  if (d) {
    fetchlen = d->row->fetchlen;
  } else {
    fetchlen = 1;
  }
  //src_iseq = (src_insn_t *)malloc(max_alloc * sizeof(src_insn_t));
  src_iseq = new src_insn_t[max_alloc];
  ASSERT(src_iseq);
  //nextpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
  nextpcs = new nextpc_t[max_alloc];
  ASSERT(nextpcs);
  //live_outs = (regset_t *)malloc(max_alloc * sizeof(regset_t));
  live_outs = new regset_t[max_alloc];
  ASSERT(live_outs);
  //l_out_tmaps = (transmap_t *)malloc(max_alloc * sizeof(transmap_t));
  l_out_tmaps = new transmap_t[max_alloc];
  ASSERT(l_out_tmaps);
  //l_peep_out_tmaps = (transmap_t *)malloc(max_alloc * sizeof(transmap_t));
  l_peep_out_tmaps = new transmap_t[max_alloc];
  ASSERT(l_peep_out_tmaps);
  //if (si->pc == 0x1700006) {
  //  DBG_SET(PEEP_TAB, 0);
  //  DBG_SET(PEEP_TRANS, 0);
  //  DBG_SET(INSN_MATCH, 0);
  //  DBG_SET(CONNECT_ENDPOINTS, 0);
  //  DBG_SET(SRC_ISEQ_FETCH, 2);
  //}


  fetch = src_iseq_fetch(src_iseq, max_alloc, &src_iseq_len, in, NULL, si->pc, fetchlen,
      &num_nextpcs, nextpcs, NULL, NULL, NULL, NULL, false);
  ASSERT(!d || fetch);
  if (!fetch) {
    //free(src_iseq);
    delete [] src_iseq;
    //free(nextpcs);
    delete [] nextpcs;
    //free(live_outs);
    delete [] live_outs;
    //free(l_out_tmaps);
    delete [] l_out_tmaps;
    //free(l_peep_out_tmaps);
    delete [] l_peep_out_tmaps;
    //free(l_peep_regcons);
    //free(l_temps_count);
    return 0;
  }
  obtain_live_outs(in, live_outs, nextpcs, num_nextpcs);
  DBG(PEEP_TRANS, "%lx %ld: src_iseq=%s\n", (long)si->pc, fetchlen,
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
  DBG_EXEC(PEEP_TRANS,
    for (i = 0; i < num_nextpcs; i++) {
      printf("nextpcs[%ld] = %lx\n", i, (long)nextpcs[i].get_val());
      printf("live_outs[%ld] = %s\n", i, regset_to_string(&live_outs[i], as1, sizeof as1));
    }
  );

  if (!out_tmaps) {
    out_tmaps = l_out_tmaps;
  }
  if (!peep_in_tmap) {
    peep_in_tmap = &l_peep_in_tmap;
  }
  if (!peep_out_tmaps) {
    peep_out_tmaps = l_peep_out_tmaps;
  }
  if (!peep_regcons) {
    peep_regcons = &l_peep_regcons;
  }
  if (!peep_nomatch_pairs_set) {
    peep_nomatch_pairs_set = &l_peep_nomatch_pairs_set;
  }
  if (!peep_temp_regs_used) {
    peep_temp_regs_used = &l_peep_temp_regs_used;
  }
  if (!touch_not_live_in) {
    touch_not_live_in = &l_touch_not_live_in;
  }

  if (d) {
    DBG(PEEP_TAB, "%lx: translating using transmap:\n%s\n", (long)si->pc,
        transmap_to_string(d->in_tmap, as1, sizeof as1));
    for (size_t i = 0; i < num_nextpcs; i++) {
      DBG(PEEP_TAB, "%lx: out_transmap[%zd]:\n%s\n", (long)si->pc, i,
          transmap_to_string(&out_tmaps[i], as1, sizeof as1));
    }
    DBG(PEEP_TAB, "src_iseq:\n%s\n",
        src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1));
    CPP_DBG_EXEC(PEEP_TAB,
        cout << __func__ << " " << __LINE__ << ": nextpcs =";
        for (size_t i = 0; i < num_nextpcs; i++) {
          cout << " " << hex << nextpcs[i].get_val() << dec;
        }
        cout << endl;
    );
    ASSERT(d->get_peeptab_entry_id());
    //if (si->pc == 0x180004a) {
    //  DBG_SET(PEEP_TAB, 2);
    //  DBG_SET(PEEP_TRANS, 2);
    //  DBG_SET(INSN_MATCH, 2);
    //}
    peep_translate(ti, d->get_peeptab_entry_id(), src_iseq, src_iseq_len, nextpcs, live_outs,
        num_nextpcs, d->in_tmap, out_tmaps, peep_in_tmap, peep_out_tmaps, peep_regcons,
        peep_temp_regs_used,
        peep_nomatch_pairs_set, touch_not_live_in, dst_iseq, &dst_iseq_len, dst_iseq_size/*, false*/);
    //if (si->pc == 0x180004a) {
    //  DBG_SET(PEEP_TAB, 0);
    //  DBG_SET(PEEP_TRANS, 0);
    //  DBG_SET(INSN_MATCH, 0);
    //}
    //ASSERT(dst_iseq_is_well_formed(dst_iseq, dst_iseq_len));
  } else {
    dst_iseq_len = 0;
    num_nextpcs = 0;
  }
  // if peep_regcons is given, then we are not interested in transmap translation code
  if (peep_regcons == &l_peep_regcons) {
    ASSERT(peep_in_tmap == &l_peep_in_tmap);
    ASSERT(peep_out_tmaps == l_peep_out_tmaps);
    ASSERT(peep_temp_regs_used == &l_peep_temp_regs_used);
    ASSERT(peep_nomatch_pairs_set == &l_peep_nomatch_pairs_set);
    //ASSERT(out_tmaps == l_out_tmaps);
    ASSERT(touch_not_live_in == &l_touch_not_live_in);
    transmap_t const *next_tmaps[num_nextpcs];
    //txinsn_t next_tinsns[num_nextpcs];

    DBG(CONNECT_ENDPOINTS, "si->pc=%lx\n", (long)si->pc);
    DYN_DBG(tmap_trans, "si->pc=%lx\n", (long)si->pc);

    //get_tcodes(tcodes, next_tcodes, nextpcs, num_nextpcs);
    //get_tinsns(tinsns_map, next_tinsns, nextpcs, num_nextpcs);
    get_tmaps(ti, next_tmaps, nextpcs, num_nextpcs);

    //extern char TMAP_TRANS_flag;
    //if (si->pc == 0x1001d888) {
    //  TMAP_TRANS_flag = 2;
    //}
    DBG(CONNECT_ENDPOINTS, "%lx: before connect_end_points\n%s\n", (long)si->pc,
        dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
    //ASSERT(dst_iseq_is_well_formed(dst_iseq, dst_iseq_len));
    dst_iseq_len = dst_iseq_connect_end_points(dst_iseq, dst_iseq_len, dst_iseq_size,
        nextpcs/*, next_tinsns*/, d?d->out_tmaps:NULL, next_tmaps, live_outs, num_nextpcs,
        in->fresh && !src_pc_belongs_to_plt_section(in, si->pc), start_state, ti->get_base_reg(),
        ti->ti_get_dst_fixed_reg_mapping(), in->symbol_map, I386_AS_MODE_32);
    //ASSERT(dst_iseq_is_well_formed(dst_iseq, dst_iseq_len));
    DBG(CONNECT_ENDPOINTS, "%lx: after connect_end_points\n%s\n", (long)si->pc,
        dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
#if ARCH_SRC == ARCH_ETFG
    dst_iseq_rename_nextpc_vars_to_pcrel_relocs(ti, dst_iseq, dst_iseq_len, etfg_get_nextpc_map_at_pc(in, si->pc));
    DBG(CONNECT_ENDPOINTS, "%lx: after rename nextpc vars to pcrel relocs\n%s\n", (long)si->pc,
        dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
#endif
  }
  //if (si->pc == 0x1700006) {
  //  DBG_SET(PEEP_TAB, 0);
  //  DBG_SET(INSN_MATCH, 0);
  //  DBG_SET(CONNECT_ENDPOINTS, 0);
  //  DBG_SET(SRC_ISEQ_FETCH, 0);
  //}

  //free(src_iseq);
  delete [] src_iseq;
  free(nextpcs);
  //free(live_outs);
  delete [] live_outs;
  //free(l_out_tmaps);
  delete [] l_out_tmaps;
  //free(l_peep_out_tmaps);
  delete [] l_peep_out_tmaps;
  //free(l_temps_count);

  //ASSERT(dst_iseq_is_well_formed(dst_iseq, dst_iseq_len));
  return dst_iseq_len;
}

static void
si_gen_peep_tmaps_and_regcons(translation_instance_t *ti, si_entry_t const *si)
{
  long dst_iseq_len, max_alloc;
  transmap_set_elem_t *d;
  dst_insn_t *dst_iseq;
  input_exec_t const *in;
  size_t bufsize;
  char *buf;

  ti->peep_out_tmaps[si->index] = NULL;
  d = ti->transmap_decision[si->index];
  if (!d) {
    return;
  }

  in = ti->in;

  max_alloc = (in->num_si + 100) * 2;
  //dst_iseq = (dst_insn_t *)malloc(max_alloc * sizeof(dst_insn_t));
  dst_iseq = new dst_insn_t[max_alloc];
  ASSERT(dst_iseq);
  bufsize = max_alloc * 80;
  buf = (char *)malloc(bufsize * sizeof(char));
  ASSERT(buf);

  /*ti->peep_out_tmaps[si->index] = (transmap_t *)malloc(
      d->row->num_nextpcs * sizeof(transmap_t));*/
  ti->peep_out_tmaps[si->index] = new transmap_t[d->row->num_nextpcs];
  ASSERT(ti->peep_out_tmaps[si->index]);

  DBG(PEEP_TAB, "%lx: generating peep_tmaps and regcons\n", (long)si->pc);
  dst_iseq_len = si_gen_dst_iseq(ti, si/*, NULL*/, dst_iseq, max_alloc, d->out_tmaps,
      &ti->peep_in_tmap[si->index], ti->peep_out_tmaps[si->index],
      &ti->peep_regcons[si->index],
      &ti->peep_temp_regs_used[si->index],
      &ti->peep_nomatch_pairs_set[si->index],
      &ti->touch_not_live_in[si->index]);

  CPP_DBG_EXEC2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_in_tmap = " << transmap_to_string(&ti->peep_in_tmap[si->index], as1, sizeof as1) << endl);
  for (size_t i = 0; i < d->row->num_nextpcs; i++) {
    CPP_DBG_EXEC2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_out_tmaps[" << i << "] = " << transmap_to_string(&ti->peep_out_tmaps[si->index][i], as1, sizeof as1) << endl);
  }
  CPP_DBG_EXEC2(PEEP_TAB, cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_regcons = " << regcons_to_string(ti->peep_regcons, as1, sizeof as1) << endl);
  DBG_EXEC2(PEEP_TAB,
    printf("%s() %d:\n", __func__, __LINE__);
    printf("%lx: dst_iseq_len = %ld, d->in_tmap:\n", (long)si->pc, dst_iseq_len);
    printf("%s\n", transmap_to_string(d->in_tmap, as1, sizeof as1));
    printf("%lx: dst_iseq_len = %ld, d->out_tmaps:\n", (long)si->pc, dst_iseq_len);
    for (int i = 0; i < d->row->num_nextpcs; i++) {
      printf("%s\n", transmap_to_string(&d->out_tmaps[i], as1, sizeof as1));
    }
  );

  //free(dst_iseq);
  delete [] dst_iseq;
  free(buf);
}

static void
transmap_assign_stack_slots_for_in_tmap(transmap_t *tmap, alloc_map_t<stack_offset_t> const &ssm, size_t stack_frame_size)
{
  for (auto &g : tmap->extable) {
    for (auto &r : g.second) {
      r.second.transmap_entry_assign_stack_slots(ssm, stack_frame_size, g.first, r.first);
    }
  }
}

static void
transmap_assign_stack_slots_for_out_tmap(transmap_t *out_tmap, transmap_t const *in_tmap, transmap_t const *peep_in_tmap, transmap_t const *peep_out_tmap, regset_t const *live_in, alloc_map_t<stack_offset_t> const &ssm, size_t stack_frame_size)
{
  //for slots that are common between peep_in_tmap and peep_out_tmap, just use peep_in_tmap, peep_out_tmap, and in_tmap to get out_tmap
  //for other slots, call transmap_entry_assign_stack_slots with ssm, stack_frame_size
  
  //first assign using ssm
  transmap_assign_stack_slots_for_in_tmap(out_tmap, ssm, stack_frame_size);

  //now change the names for the ones that are common between peep_in_tmap and peep_out_tmap
  for (const auto &g : peep_out_tmap->extable) {
    for (const auto &r : g.second) {
      if (!r.second.contains_mapping_to_loc(TMAP_LOC_SYMBOL)) {
        continue;
      }
      dst_ulong regnum = r.second.get_regnum_for_loc(TMAP_LOC_SYMBOL);
      if (regnum == (dst_ulong)-1) {
        continue;
      }
      reg_identifier_t src_reg_id = reg_identifier_t::reg_identifier_invalid();
      exreg_group_id_t src_group;
      if (peep_in_tmap->transmap_maps_to_symbol_loc(regnum, live_in, src_group, src_reg_id)) {
        dst_ulong iregnum = in_tmap->extable.at(src_group).at(src_reg_id).get_regnum_for_loc(TMAP_LOC_SYMBOL);
        if (iregnum == (dst_ulong)-1) {
          cout << __func__ << " " << __LINE__ << ": peep_in_tmap =\n" << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": peep_out_tmap =\n" << transmap_to_string(peep_out_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": in_tmap =\n" << transmap_to_string(in_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": out_tmap =\n" << transmap_to_string(out_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": src_group = " << src_group << endl;
          cout << __func__ << " " << __LINE__ << ": src_reg_id = " << src_reg_id.reg_identifier_to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << endl;
          cout << __func__ << " " << __LINE__ << ": r.first = " << r.first.reg_identifier_to_string() << endl;
        }
        ASSERT(iregnum != (dst_ulong)-1);
        if (   out_tmap->extable.count(g.first)
            && out_tmap->extable.at(g.first).count(r.first)) {
          out_tmap->extable.at(g.first).at(r.first).transmap_entry_update_regnum_for_loc(TMAP_LOC_SYMBOL, iregnum);
        }
      }
    }
  }
}


static size_t
get_stack_size(alloc_map_t<stack_offset_t> const &ssm)
{
  size_t ret = 0;
  for (const auto &r : ssm.get_map()) {
    //for (const auto &r : g.second) {
      if (   !r.second.get_is_arg()
          && ret < r.second.get_offset() + STACK_SLOT_SIZE) {
        ret = r.second.get_offset() + STACK_SLOT_SIZE;
      }
    //}
  }
  return ret;
}


static size_t
get_stack_size_for_function(graph_locals_map_t const &locals_map, map<prog_point_t, alloc_map_t<stack_offset_t>> const &ssm)
{
  size_t ret = 0;
  for (const auto &fs : ssm) {
    ret = max(ret, get_stack_size(fs.second));
  }
  ret += locals_map.graph_locals_map_get_stack_size();
  return ret;
}

static size_t
ti_get_stack_alloc_space_for_function(translation_instance_t const *ti, string const &function_name)
{
  if (ti->stack_slot_map_for_function.count(function_name) == 0) {
    return 0;
  }
  input_exec_t const *in = ti->in;
  graph_locals_map_t const &locals_map = etfg_input_exec_get_locals_map_for_function(in, function_name);
  return get_stack_size_for_function(locals_map, ti->stack_slot_map_for_function.at(function_name));
}

static void
si_restrict_regcons_at_function_entry(translation_instance_t *ti, si_entry_t const *si)
{
  //Ensure that callee-save regs are mapped to appropriate dst regs. This function can also be used for calling conventions where function arguments are passed in specific registers.
  transmap_t *peep_in_tmap, *peep_out_tmaps;
  regcons_t *peep_regcons;
  transmap_set_elem_t *d;

  input_exec_t const *in = ti->in;
  d = ti->transmap_decision[si->index];
  if (!d) {
    cout << __func__ << " " << __LINE__ << ": transmap_decision is NULL at pc " << hex << si->pc << dec << endl;
  }
  ASSERT(d);
  ASSERT(d->row->num_nextpcs == 1);
  ASSERT(si->index >= 0 && si->index < in->num_si);
  peep_regcons = &ti->peep_regcons[si->index];
  peep_in_tmap = &ti->peep_in_tmap[si->index];
  peep_out_tmaps = ti->peep_out_tmaps[si->index];
  for (size_t i = 0; i < ETFG_NUM_CALLEE_SAVE_REGS; i++) {
    regcons_entry_t regcons_entry;
    transmap_entry_t tmap_entry;
    stringstream ss;
    int dst_reg = dst_get_callee_save_regnum(i);
    ASSERT(dst_reg >= 0 && dst_reg < DST_NUM_EXREGS(DST_EXREG_GROUP_GPRS));
    tmap_entry.set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), i, TMAP_LOC_MODIFIER_SRC_SIZED);
    /*tmap_entry.num_locs = 1;
    tmap_entry.loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
    tmap_entry.regnum[0] = i;*/
    regcons_entry.bitmap = (1ULL << dst_reg);

    ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
    reg_identifier_t regnum = reg_identifier_t(ss.str());
    peep_in_tmap->extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][regnum] = tmap_entry;
    peep_out_tmaps[0].extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][regnum] = tmap_entry;
    peep_regcons->regcons_extable[DST_EXREG_GROUP_GPRS][i] = regcons_entry;
  }
}

//static bool
//si_acquiesce(translation_instance_t *ti, si_entry_t const *si/*, set<src_ulong> seen*/)
//{
//  transmap_t *peep_in_tmap, *peep_out_tmaps;
//  regcons_t const *peep_regcons;
//  input_exec_t const *in;
//  transmap_set_elem_t *d;
//  string function_name;
//  transmap_t orig_tmap;
//  bool changed;
//  long i, j;
//
//  regset_t const *in_live = &si->bbl->live_regs[si->bblindex];
//  CPP_DBG_EXEC(TMAPS_ACQUIESCE, cout << __func__ << " " << __LINE__ << ": pc = " << hex << si->pc << dec << endl);
//  in = ti->in;
//  changed = false;
//
//  d = ti->transmap_decision[si->index];
//  if (!d) {
//    return changed;
//  }
//  /*if (set_belongs(seen, si->pc)) {
//    return false;
//  }*/
//  ASSERT(si->index >= 0 && si->index < in->num_si);
//  peep_regcons = &ti->peep_regcons[si->index];
//  peep_in_tmap = &ti->peep_in_tmap[si->index];
//  peep_out_tmaps = ti->peep_out_tmaps[si->index];
//
//  //cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_in_tmap = " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//  //transmaps_assign_regs_where_unassigned(peep_in_tmap, peep_out_tmaps,
//  //    d->row->num_nextpcs, &ti->touch_not_live_in[si->index]);
//  //cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_in_tmap = " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//
//  transmap_copy(&orig_tmap, d->in_tmap);
//
//  DBG(TMAPS_ACQUIESCE, "%lx: num_decisions = %ld\n", (long)si->pc, d->num_decisions);
//  //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << si->pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//
//  //cout << __func__ << " " << __LINE__ << ": peep_in_tmap = " << peep_in_tmap << endl;
//  //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << si->pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//
//  transmaps_acquiesce(d->in_tmap, peep_in_tmap);
//  for (i = 0; i < d->row->num_nextpcs; i++) {
//    if (!PC_IS_NEXTPC_CONST(d->row->nextpcs[i])) {
//      transmaps_acquiesce(&d->out_tmaps[i], &peep_out_tmaps[i]);
//    }
//  }
//
//  CPP_DBG_EXEC(TMAPS_ACQUIESCE, cout << __func__ << " " << __LINE__ << ": after acquiescing with peep_in_tmap: " << hex << si->pc << dec << ": d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl);
//
//#if ARCH_SRC == ARCH_DST
//  transmaps_acquiesce(d->in_tmap, transmap_default());
//  for (i = 0; i < d->row->num_nextpcs; i++) {
//    if (!PC_IS_NEXTPC_CONST(d->row->nextpcs[i])) {
//      transmaps_acquiesce(&d->out_tmaps[i], transmap_default());
//    }
//  }
//  CPP_DBG_EXEC(TMAPS_ACQUIESCE, cout << __func__ << " " << __LINE__ << ": after acquiescing with transmap_default: " << hex << si->pc << dec << ": d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl);
//#endif
//
//  long num_preds, max_alloc;
//  src_ulong *preds;
//
//  max_alloc = (in->num_si + 100);
//  
//  preds = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
//  ASSERT(preds);
//  num_preds = si_get_preds(in, si, preds, NULL);
//  ASSERT(num_preds < max_alloc);
//  /* acquiesce the preds in reverse order. so that the mfp gets the last
//     go and thus the highest preference */
//  for (i = num_preds - 1; i >= 0; i--) {
//    transmap_t const *prev_out_tmap;
//    long pred_index;
//
//    if (preds[i] == PC_JUMPTABLE) {
//      continue;
//    }
//    pred_index = pc2index(in, preds[i]);
//    ASSERT(pred_index >= 0 && pred_index < in->num_si);
//    if (!ti->transmap_decision[pred_index]) {
//      continue;
//    }
//    prev_out_tmap = find_out_tmap_for_nextpc(ti->transmap_decision[pred_index],
//        si->pc);
//    transmaps_acquiesce(d->in_tmap, prev_out_tmap);
//
//    //cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": after acquiescing with " << hex << preds[i] << dec << ", d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl;
//
//    DBG(TMAPS_ACQUIESCE, "prev_out_tmap:\n%s\n",
//        transmap_to_string(prev_out_tmap, as1, sizeof as1));
//    DBG(TMAPS_ACQUIESCE, "%lx: after acquiescing with pred %ld (%lx):\n%s\n",
//        (long)si->pc, i, (long)preds[i],
//        transmap_to_string(d->in_tmap, as1, sizeof as1));
//  }
//  free(preds);
//
//  /*alloc_map_t ssm = transmap_assign_stack_slots_where_unassigned(d->in_tmap);
//  for (auto &g : d->in_tmap->extable) {
//    for (auto &r : g.second) {
//      for (unsigned i = 0; i < r.second.num_locs; i++) {
//        if (r.second.loc[i] == TMAP_LOC_SYMBOL) {
//          dst_ulong rnum = r.second.regnum[i];
//          if (rnum == (dst_ulong)-1) {
//            r.second.regnum[i] = ssm.get_offset(g.first, r.first);
//          }
//        }
//      }
//    }
//  }*/
//
//  DBG(TMAPS_ACQUIESCE, "%lx: calling transmap_acquiesce_with_regcons\n", (long)si->pc);
//  DBG(TMAPS_ACQUIESCE, "%lx: peep_in_tmap:\n%s\n", (long)si->pc,
//      transmap_to_string(peep_in_tmap, as1, sizeof as1));
//  DBG(TMAPS_ACQUIESCE, "%lx: peep_regcons:\n%s\n", (long)si->pc,
//      regcons_to_string(peep_regcons, as1, sizeof as1));
//  transmap_acquiesce_with_regcons(d->in_tmap, peep_in_tmap, peep_regcons, in_live);
//  DBG(TMAPS_ACQUIESCE, "%lx: after acquiesce with regcons, in_tmap:\n%s\n", (long)si->pc,
//      transmap_to_string(d->in_tmap, as1, sizeof as1));
//  for (i = 0; i < d->row->num_nextpcs; i++) {
//    DBG(TMAPS_ACQUIESCE, "%lx: before rename, out_tmap[%ld]:\n%s\n", (long)si->pc, i,
//        transmap_to_string(&d->out_tmaps[i], as1, sizeof as1));
//    out_transmap_rename(&d->out_tmaps[i], d->in_tmap, &peep_out_tmaps[i], peep_in_tmap,
//        /*d->row->num_nextpcs, */peep_regcons);
//    DBG(TMAPS_ACQUIESCE, "%lx: after rename, out_tmap[%ld]:\n%s\n", (long)si->pc, i,
//        transmap_to_string(&d->out_tmaps[i], as1, sizeof as1));
//  }
//
//  if (!transmaps_equal_mod_regnames(&orig_tmap, d->in_tmap)) {
//    CPP_DBG_EXEC(TMAPS_ACQUIESCE, cout << __func__ << " " << __LINE__ << ": changed = true for pc " << hex << si->pc << dec << ": orig_tmap =\n" << transmap_to_string(&orig_tmap, as1, sizeof as1) << endl);
//    CPP_DBG_EXEC(TMAPS_ACQUIESCE, cout << __func__ << " " << __LINE__ << ": changed = true for pc " << hex << si->pc << dec << ": d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl);
//    src_ulong *nextpcs = (src_ulong *)malloc(max_alloc * sizeof(src_ulong));
//    ASSERT(nextpcs);
//    long num_nextpcs = pc_get_nextpcs(in, si->pc, nextpcs, NULL);
//    for (size_t i = 0; i < num_nextpcs; i++) {
//      if (nextpcs[i] == PC_JUMPTABLE) {
//        continue;
//      }
//      long nextpc_index = pc2index(in, nextpcs[i]);
//      ASSERT(nextpc_index >= 0 && nextpc_index < in->num_si);
//      si_acquiesce(ti, &in->si[nextpc_index]); //call it recursively till a fixed-point is reached
//    }
//    free(nextpcs);
//    changed = true;
//  }
//
//  return changed;
//}

//static void
//acquiesce_decisions(translation_instance_t *ti)
//{
//  bool si_some_change, some_change;
//  transmap_set_elem_t *decision;
//  struct input_exec_t *in;
//  long i, n, iter;
//
//  in = ti->in;
//  some_change = true;
//
//  /* Step 0: Unassign all regnums in all si's */
//  for (i = 0; i < in->num_si; i++) {
//    decision = ti->transmap_decision[i];
//    if (!decision) {
//      continue;
//    }
//    transmap_unassign_regs(decision->in_tmap);
//    for (n = 0; n < decision->row->num_nextpcs; n++) {
//      transmap_unassign_regs(&decision->out_tmaps[n]);
//      DBG(TMAPS_ACQUIESCE, "%lx: After unassignment, out_tmaps[%ld]:\n%s\n",
//          (long)in->si[i].pc, n,
//          transmap_to_string(&decision->out_tmaps[n], as1, sizeof as1));
//    }
//  }
//
//  /* Step 2: Do not ignore dominated predecessors */
//  for (iter = 0; /*iter < 2 && */some_change; iter++) {
//    some_change = false;
//
//    for (i = 0; i < in->num_si; i++) {
//      DBG(TMAPS_ACQUIESCE, "%lx: calling si_acquiesce\n", (long)in->si[i].pc);
//      si_some_change = si_acquiesce(ti, &in->si[i]);
//      CPP_DBG_EXEC3(TMAPS_ACQUIESCE,
//         cout << __func__ << " " << __LINE__ << ": after acquiescing " << hex << in->si[i].pc << dec << ":\n";
//         for (size_t j = 0; j < in->num_si; j++) {
//           auto d = ti->transmap_decision[j];
//           if (!d) {
//             continue;
//           }
//           cout << hex << in->si[j].pc << dec << ":\n";
//           cout << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl;
//         }
//      );
//      some_change = some_change || si_some_change;
//    }
//  }
//}

static void
ti_gen_peep_tmaps_and_regcons(translation_instance_t *ti)
{
  input_exec_t const *in;
  si_entry_t const *si;
  long i;

  in = ti->in;

  //ti->peep_in_tmap = (transmap_t *)malloc(in->num_si * sizeof(transmap_t));
  ti->peep_in_tmap = new transmap_t[in->num_si];
  ASSERT(ti->peep_in_tmap);

  ti->peep_out_tmaps = (transmap_t **)malloc(in->num_si * sizeof(transmap_t *));
  ASSERT(ti->peep_out_tmaps);

  //ti->peep_regcons = (regcons_t *)malloc(in->num_si * sizeof(regcons_t));
  ti->peep_regcons =  new regcons_t[in->num_si];
  ASSERT(ti->peep_regcons);

  ti->peep_temp_regs_used = new regset_t[in->num_si];
  ASSERT(ti->peep_temp_regs_used);

  ti->peep_nomatch_pairs_set =  new nomatch_pairs_set_t[in->num_si];
  ASSERT(ti->peep_nomatch_pairs_set);

  ti->touch_not_live_in = (regcount_t *)malloc(in->num_si * sizeof(regcount_t));
  ASSERT(ti->touch_not_live_in);

  for (i = 0; i < (long)in->num_si; i++) {
    si = &in->si[i];
    si_gen_peep_tmaps_and_regcons(ti, si);
    string function_name;
    if (in->pc_is_etfg_function_entry(si->pc, function_name) && function_name != "main") {
      si_restrict_regcons_at_function_entry(ti, si);
      //cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": peep_in_tmap = " << transmap_to_string(&ti->peep_in_tmap[si->index], as1, sizeof as1) << endl;
    }
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_in_tmap(alloc_constraints_t<VALUETYPE> &ssm_constraints, transmap_t const *tmap/*, regset_t const *live_regs*/, prog_point_t const &pp, transmap_loc_id_t loc)
{
  set<alloc_regname_t> live_regs_mem_mapped;
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      if (/*   regset_belongs_ex(live_regs, g.first, r.first)
          && */r.second.contains_mapping_to_loc(loc)) {
        live_regs_mem_mapped.insert(alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first));
      }
    }
  }
  for (auto l : live_regs_mem_mapped) {
    for (auto m : live_regs_mem_mapped) {
      if (!(l == m)) {
        ssm_constraints.add_negative_mapping(alloc_elem_t(pp, l), alloc_elem_t(pp, m));
      }
    }
  }
}

static dst_ulong
transmap_entry_get_regnum_or_reg_id_for_loc(transmap_entry_t const &e, transmap_loc_id_t loc)
{
  dst_ulong ret;
  if (loc == TMAP_LOC_SYMBOL) {
    ret = e.get_regnum_for_loc(loc);
  } else {
    ASSERT(   loc >= TMAP_LOC_EXREG(0)
           && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
    reg_identifier_t const &reg_id = e.get_reg_id_for_loc(loc);
    if (reg_id.reg_identifier_is_unassigned()) {
      ret = (dst_ulong)-1;
    } else {
      ret = reg_id.reg_identifier_get_id();
    }
  }
  return ret;
}

template<typename VALUETYPE>
void
alloc_constraints_add_equality_mappings_across_peep_translation_for_loc_for_pcs(alloc_constraints_t<VALUETYPE> &ssm_constraints, prog_point_t const &pp1, prog_point_t const &pp2/*src_ulong pc, long nextpc_num, src_ulong nextpc*/, transmap_t const *peep_tmap1, transmap_t const *peep_tmap2/*, regset_t const *pc_live_regs, regset_t const *nextpc_live_regs*/, transmap_loc_id_t loc)
{
  for (const auto &g : peep_tmap1->extable) {
    for (const auto &r : g.second) {
      if (!r.second.contains_mapping_to_loc(loc)) {
        continue;
      }
      dst_ulong r_regnum = transmap_entry_get_regnum_or_reg_id_for_loc(r.second, loc);
      if (r_regnum == (dst_ulong)-1) {
        continue;
      }
      /*if (!regset_belongs_ex(pc_live_regs, g.first, r.first)) {
        continue;
      }*/
      for (const auto &og : peep_tmap2->extable) {
        for (const auto &oreg : og.second) {
          if (!oreg.second.contains_mapping_to_loc(loc)) {
            continue;
          }
          dst_ulong oreg_regnum = transmap_entry_get_regnum_or_reg_id_for_loc(oreg.second, loc);
          if (oreg_regnum == (dst_ulong)-1) {
            continue;
          }
          /*if (!regset_belongs_ex(nextpc_live_regs, og.first, oreg.first)) {
            continue;
          }*/
          if (r_regnum == oreg_regnum) {
            //ssm_constraints.ssc_add_mapping(make_pair(pc, make_pair(g.first, r.first)), make_pair(nextpc, make_pair(og.first, oreg.first)));
            //ssm_constraints.ssc_add_mapping(alloc_elem_t(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), alloc_elem_t(prog_point_t(pc, prog_point_t::OUT, nextpc_num), alloc_regname_t(alloc_regname_t::SRC_REG, og.first, oreg.first)), COST_INFINITY);
            ssm_constraints.ssc_add_mapping(alloc_elem_t(pp1, alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), alloc_elem_t(pp2, alloc_regname_t(alloc_regname_t::SRC_REG, og.first, oreg.first)), COST_INFINITY);

            //if (   nextpc != PC_JUMPTABLE
            //    && nextpc != PC_UNDEFINED
            //    && !PC_IS_NEXTPC_CONST(nextpc)) {
            //  //ASSERT(out_live[i] != NULL);
            //  ssm_constraints.ssc_add_mapping(make_pair(prog_point_t(pc, prog_point_t::OUT, nextpc_num), make_pair(g.first, r.first)), make_pair(prog_point_t(nextpc, prog_point_t::IN, 0), make_pair(og.first, oreg.first)));
            //}
          }
        }
      }
    }
  }

  for (const auto &g : peep_tmap1->extable) {
    for (const auto &r : g.second) {
      if (!r.second.contains_mapping_to_loc(loc)) {
        continue;
      }
      //if (!regset_belongs_ex(pc_live_regs, g.first, r.first)) {
      //  continue;
      //}
      //if (!regset_belongs_ex(nextpc_live_regs, g.first, r.first)) {
      //  continue;
      //}
      //dst_ulong r_regnum = r.second.get_regnum_for_loc(loc);
      dst_ulong r_regnum = transmap_entry_get_regnum_or_reg_id_for_loc(r.second, loc);
      dst_ulong oreg_regnum;
      if (r_regnum == (dst_ulong)-1) {
        if (   peep_tmap2->extable.count(g.first)
            && peep_tmap2->extable.at(g.first).count(r.first)
            && peep_tmap2->extable.at(g.first).at(r.first).contains_mapping_to_loc(loc)
            //&& (oreg_regnum = peep_out_tmap->extable.at(g.first).at(r.first).get_regnum_for_loc(loc)) == -1)
            && (oreg_regnum = transmap_entry_get_regnum_or_reg_id_for_loc(peep_tmap2->extable.at(g.first).at(r.first), loc)) == (dst_ulong)-1) {
          //this reg (g.first, r.first) is not touched by this peep translation. its mappings should be maintained as-is
          //ssm_constraints.ssc_add_mapping(make_pair(pc, make_pair(g.first, r.first)), make_pair(nextpc, make_pair(g.first, r.first)));
          //ssm_constraints.ssc_add_mapping(alloc_elem_t(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), alloc_elem_t(prog_point_t(pc, prog_point_t::OUT, nextpc_num), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), COST_INFINITY);
          ssm_constraints.ssc_add_mapping(alloc_elem_t(pp1, alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), alloc_elem_t(pp2, alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), COST_INFINITY);

          //if (   nextpc != PC_JUMPTABLE
          //    && nextpc != PC_UNDEFINED
          //    && !PC_IS_NEXTPC_CONST(nextpc)) {
          //  ssm_constraints.ssc_add_mapping(make_pair(prog_point_t(pc, prog_point_t::OUT, nextpc_num), make_pair(g.first, r.first)), make_pair(prog_point_t(nextpc, prog_point_t::IN, 0), make_pair(g.first, r.first)));
          //}
        }
      }
    }
  }
}

static cost_t
compute_eq_mapping_break_cost(input_exec_t const *in, src_ulong from_pc, src_ulong to_pc, transmap_t const *from_out_tmap, transmap_t const *to_in_tmap, transmap_loc_id_t loc)
{
  if (!to_in_tmap) {
    return COST_INFINITY;
  }
  ASSERT(from_out_tmap);
  ASSERT(to_in_tmap);
  long from_bbl_id = pc_find_bbl(in, from_pc);
  ASSERT(from_bbl_id >= 0 && from_bbl_id < in->num_bbls);
  long to_bbl_id = pc_find_bbl(in, to_pc);
  ASSERT(to_bbl_id >= 0 && to_bbl_id < in->num_bbls);
  bbl_t const *from_bbl = &in->bbls[from_bbl_id];
  bbl_t const *to_bbl = &in->bbls[to_bbl_id];
  double path_taken_prob = 1.0; //XXX: compute path_taken_prob from from_bbl->to_bbl
  //if (to_bbl != from_bbl) {
  //  long nextpc_idx = -1;
  //  for (size_t i = 0; i < from_bbl->num_nextpcs; i++) {
  //    if (from_bbl->nextpcs[i] == to_pc) {
  //      nextpc_idx = i;
  //      break;
  //    }
  //  }
  //  if (!(nextpc_idx >= 0 && nextpc_idx < from_bbl->num_nextpcs)) {
  //    cout << __func__ << " " << __LINE__ << ": " << hex << from_pc << dec << ": nextpcs =";
  //    for (size_t n = 0; n < from_bbl->num_nextpcs; n++) {
  //      cout << " " << hex << from_bbl->nextpcs[n] << dec;
  //    }
  //    cout << endl;
  //    cout << "to_pc = " << hex << to_pc << dec << endl;
  //  }
  //  ASSERT(nextpc_idx >= 0 && nextpc_idx < from_bbl->num_nextpcs);
  //  path_taken_prob = from_bbl->nextpc_probs[nextpc_idx];
  //} else {
  //  path_taken_prob = 1.0;
  //}
  cost_t cost = from_bbl->in_flow * EQ_MAPPING_BREAK_COST_FLOW_WEIGHT * path_taken_prob;
  //XXX: TODO: assign higher cost if both from_out_tmap and to_in_tmap exhaust all registers of loc; because then we will need some kind of swap/xchg mechanism that is likely to be expensive
  return cost;
}

template<typename VALUETYPE>
void
alloc_constraints_add_equality_mappings_across_pcs(alloc_constraints_t<VALUETYPE> &ssm_constraints, src_ulong pc, long nextpc_num, src_ulong nextpc, transmap_t const *peep_out_tmap, transmap_t const *nextpc_peep_in_tmap, input_exec_t const *in, transmap_loc_id_t loc)
{
  //cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << ", nextpc = " << nextpc << dec << endl;
  if (nextpc == PC_JUMPTABLE || nextpc == PC_UNDEFINED || PC_IS_NEXTPC_CONST(nextpc)) {
    return;
  }
#if ARCH_SRC == ARCH_ETFG
  string pc_function, nextpc_function;
  long pc_insn_num, nextpc_insn_num;
  etfg_pc_get_function_name_and_insn_num(in, pc, pc_function, pc_insn_num);
  etfg_pc_get_function_name_and_insn_num(in, nextpc, nextpc_function, nextpc_insn_num);
  if (nextpc_insn_num == 0) { //proxy for identifying if this nextpc represents a function call; should use better logic in future
    return;
  }
#endif
  ASSERT(peep_out_tmap);
  ASSERT(nextpc_peep_in_tmap);
  for (const auto &g : peep_out_tmap->extable) {
    for (const auto &r : g.second) {
      if (!r.second.contains_mapping_to_loc(loc)) {
        continue;
      }
      ssm_constraints.ssc_add_mapping(alloc_elem_t(prog_point_t(pc, prog_point_t::OUT, nextpc_num), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), alloc_elem_t(prog_point_t(nextpc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)), compute_eq_mapping_break_cost(in, pc, nextpc, peep_out_tmap, nextpc_peep_in_tmap, loc));
    }
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_equality_mappings_across_peep_translation_for_loc(alloc_constraints_t<VALUETYPE> &ssm_constraints, src_ulong pc, nextpc_t const *nextpcs, long num_nextpcs, transmap_t const *peep_in_tmap, transmap_t const *peep_out_tmaps, transmap_t const **nextpc_peep_in_tmaps, input_exec_t const *in, transmap_loc_id_t loc)
{
  for (const auto &g : peep_in_tmap->extable) {
    for (const auto &r : g.second) {
      //cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << endl;
      //cout << __func__ << " " << __LINE__ << ": r = " << r.first.reg_identifier_to_string() << endl;
      if (!r.second.contains_mapping_to_loc(loc)) {
        //cout << __func__ << " " << __LINE__ << ": ignoring\n";
        continue;
      }
      //if (!regset_belongs_ex(in_live, g.first, r.first)) {
      //  //cout << __func__ << " " << __LINE__ << ": ignoring\n";
      //  continue;
      //}
      //dst_ulong r_regnum = r.second.get_regnum_for_loc(loc);
      //cout << __func__ << " " << __LINE__ << ": r_regnum = " << r_regnum << "\n";
      ssm_constraints.add_elem(alloc_elem_t(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)));
    }
  }
  for (size_t i = 0; i < num_nextpcs; i++) {
    for (const auto &g : peep_out_tmaps[i].extable) {
      for (const auto &r : g.second) {
        if (!r.second.contains_mapping_to_loc(loc)) {
          //cout << __func__ << " " << __LINE__ << ": ignoring\n";
          continue;
        }
        ssm_constraints.add_elem(alloc_elem_t(prog_point_t(pc, prog_point_t::OUT, i), alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first)));
      }
    }
  }
  for (size_t i = 0; i < num_nextpcs; i++) {
    //cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << ", nextpcs[" << dec << i << hex << "] = " << nextpcs[i] << dec << endl;
    alloc_constraints_add_equality_mappings_across_peep_translation_for_loc_for_pcs<VALUETYPE>(ssm_constraints, prog_point_t(pc, prog_point_t::IN, 0), prog_point_t(pc, prog_point_t::OUT, i)/*, nextpcs[i]*/, peep_in_tmap, &peep_out_tmaps[i]/*, in_live, out_live[i]*/, loc);
    for (size_t j = i + 1; j < num_nextpcs; j++) {
      alloc_constraints_add_equality_mappings_across_peep_translation_for_loc_for_pcs<VALUETYPE>(ssm_constraints, prog_point_t(pc, prog_point_t::OUT, i), prog_point_t(pc, prog_point_t::OUT, j)/*, nextpcs[i]*/, &peep_out_tmaps[i], &peep_out_tmaps[j]/*, in_live, out_live[i]*/, loc);
    }
    alloc_constraints_add_equality_mappings_across_pcs<VALUETYPE>(ssm_constraints, pc, i, nextpcs[i].get_val(), &peep_out_tmaps[i], nextpc_peep_in_tmaps[i], in, loc);
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmap_pair(alloc_constraints_t<VALUETYPE> &alloc_constraints, nomatch_pairs_set_t const *nomatch_pairs_set, transmap_t const *in_tmap, transmap_t const *out_tmap, prog_point_t const &in_pp, prog_point_t const &out_pp, transmap_loc_id_t loc)
{
  for (const auto &in_g : in_tmap->extable) {
    for (const auto &in_r : in_g.second) {
      transmap_entry_t const &in_tmap_entry = in_r.second;
      if (!in_tmap_entry.contains_mapping_to_loc(loc)) {
        continue;
      }
      for (const auto &out_g : out_tmap->extable) {
        for (const auto &out_r : out_g.second) {
          transmap_entry_t const &out_tmap_entry = out_r.second;
          if (!out_tmap_entry.contains_mapping_to_loc(loc)) {
            continue;
          }
          if (nomatch_pairs_set->belongs(in_g.first, in_r.first, out_g.first, out_r.first)) {
            alloc_elem_t ssm_elem1(in_pp, alloc_regname_t(alloc_regname_t::SRC_REG, in_g.first, in_r.first));
            alloc_elem_t ssm_elem2(out_pp, alloc_regname_t(alloc_regname_t::SRC_REG, out_g.first, out_r.first));
            //cout << __func__ << " " << __LINE__ << ": LOC_ID = " << (int)loc << ": adding negative mapping between " << ssm_elem_to_string(ssm_elem1) << " and " << ssm_elem_to_string(ssm_elem2) << endl;
            alloc_constraints.add_negative_mapping(ssm_elem1, ssm_elem2);
          }
        }
      }
    }
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmaps(alloc_constraints_t<VALUETYPE> &alloc_constraints, nomatch_pairs_set_t const *nomatch_pairs_set, transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_out_tmaps, src_ulong pc, transmap_loc_id_t loc)
{
  prog_point_t in_pp(pc, prog_point_t::IN, 0);
  for (long i = 0; i < num_out_tmaps; i++) {
    prog_point_t out_pp(pc, prog_point_t::OUT, i);
    alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmap_pair(alloc_constraints, nomatch_pairs_set, in_tmap, &out_tmaps[i], in_pp, out_pp, loc);
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmap_and_temp_regs_used(alloc_constraints_t<VALUETYPE> &alloc_constraints, transmap_t const *tmap, regset_t const *temp_regs_used, prog_point_t const &tmap_pp, prog_point_t const &temps_pp, transmap_loc_id_t loc)
{
  ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
  exreg_group_id_t dst_group = loc - TMAP_LOC_EXREG(0);
  if (!temp_regs_used->excregs.count(dst_group)) {
    return;
  }
  for (const auto &tempr : temp_regs_used->excregs.at(dst_group)) {
    for (const auto &g : tmap->extable) {
      for (const auto &r : g.second) {
        transmap_entry_t const &tmap_entry = r.second;
        if (!tmap_entry.contains_mapping_to_loc(loc)) {
          continue;
        }
        alloc_elem_t ssm_elem1(temps_pp, alloc_regname_t(alloc_regname_t::TEMP_REG, dst_group, tempr.first));
        alloc_elem_t ssm_elem2(tmap_pp, alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first));
        //cout << __func__ << " " << __LINE__ << ": LOC_ID = " << (int)loc << ": adding negative mapping between " << ssm_elem_to_string(ssm_elem1) << " and " << ssm_elem_to_string(ssm_elem2) << endl;
        alloc_constraints.add_negative_mapping(ssm_elem1, ssm_elem2);

      }
    }
  }
}

template<typename VALUETYPE>
void
alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmaps_and_temp_regs_used(alloc_constraints_t<VALUETYPE> &alloc_constraints, transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_out_tmaps, regset_t const *temp_regs_used, src_ulong pc, transmap_loc_id_t loc)
{
  ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
  exreg_group_id_t dst_group = loc - TMAP_LOC_EXREG(0);
  if (!temp_regs_used->excregs.count(dst_group)) {
    return;
  }
  for (const auto &tempr : temp_regs_used->excregs.at(dst_group)) {
    alloc_constraints.add_elem(alloc_elem_t(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::TEMP_REG, dst_group, tempr.first)));
  }
  prog_point_t in_pp(pc, prog_point_t::IN, 0);
  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmap_and_temp_regs_used(alloc_constraints, in_tmap, temp_regs_used, in_pp, in_pp, loc);
  for (long i = 0; i < num_out_tmaps; i++) {
    alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmap_and_temp_regs_used(alloc_constraints, &out_tmaps[i], temp_regs_used, prog_point_t(pc, prog_point_t::OUT, i), in_pp, loc);
  }
}


static void
alloc_constraints_add_constant_mapping_using_bitmap(alloc_constraints_t<exreg_id_obj_t> &rm_constraints, alloc_elem_t const &ssm, uint32_t bitmap, transmap_loc_id_t loc)
{
  ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
  exreg_group_id_t dst_group = loc - TMAP_LOC_EXREG(0);
  /*if (bitmap == MAKE_MASK(DST_NUM_EXREGS(dst_group))) {
    //cout << __func__ << " " << __LINE__ << ": found loc, LOC_ID = " << (int)loc << ": pp = " << pp.to_string() << ", group = " << g.first << ", reg = " << r.first.reg_identifier_to_string() << ": dst_reg_id = " << dst_reg_id.reg_identifier_to_string() << ", bitmap = " << hex << bitmap << dec << ", ignoring" << endl;
    continue;
  } else */{
    set<exreg_id_obj_t> allowed_dst_regnums;
    for (exreg_id_t dst_regnum = 0; dst_regnum < DST_NUM_EXREGS(dst_group); dst_regnum++) {
      if (bitmap & (1ULL << dst_regnum)) {
        //cout << __func__ << " " << __LINE__ << ": found loc, LOC_ID = " << (int)loc << ": pp = " << pp.to_string() << ", group = " << g.first << ", reg = " << r.first.reg_identifier_to_string() << ": dst_reg_id = " << dst_reg_id.reg_identifier_to_string() << ", bitmap = " << hex << bitmap << dec << ", adding dst_regnum " << dst_regnum << " to set of allowed dst regnums" << endl;
        allowed_dst_regnums.insert(exreg_id_obj_t(dst_regnum));
      }
    }
    rm_constraints.add_constraint(ssm, allowed_dst_regnums);
  }
}

static uint32_t
dst_exreg_group_get_default_bitmap(exreg_group_id_t g)
{
  uint32_t ret = 0;
  for (exreg_id_t r = 0; r < DST_NUM_EXREGS(g); r++) {
    if (!regset_belongs_ex(&dst_reserved_regs, g, r)) {
      ret = ret | (1UL << r);
    }
  }
  return ret;
}

static void
alloc_constraints_add_constant_mappings_using_regcons_for_transmap(alloc_constraints_t<exreg_id_obj_t> &rm_constraints, prog_point_t const &pp, transmap_t const *peep_tmap, regcons_t const *peep_regcons, transmap_loc_id_t loc)
{
#if ARCH_DST == ARCH_I386
  if (loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_SEGS)) {
    return; //XXX: is this really needed? perhaps not
  }
#endif
  exreg_group_id_t dst_group = loc - TMAP_LOC_EXREG(0);
  uint32_t default_bitmap = dst_exreg_group_get_default_bitmap(dst_group);
  for (const auto &g : peep_tmap->extable) {
    for (const auto &r : g.second) {
      transmap_entry_t const &e = r.second;
      bool found_loc = false;
      //cout << __func__ << " " << __LINE__ << ": LOC_ID = " << (int)loc << ": pp = " << pp.to_string() << ", group = " << g.first << ", reg = " << r.first.reg_identifier_to_string() << endl;
      for (const auto &l : e.get_locs()) {
        if (l.get_loc() != loc) {
          continue;
        }
        ASSERT(!found_loc); //there should be only one matching loc
        found_loc = true;
        reg_identifier_t const &dst_reg_id = l.get_reg_id();
        alloc_elem_t ssm(pp, alloc_regname_t(alloc_regname_t::SRC_REG, g.first, r.first));
        if (dst_reg_id.reg_identifier_is_unassigned()) {
          alloc_constraints_add_constant_mapping_using_bitmap(rm_constraints, ssm, default_bitmap, loc);
        } else {
          //cout << __func__ << " " << __LINE__ << ": found loc, LOC_ID = " << (int)loc << ": pp = " << pp.to_string() << ", group = " << g.first << ", reg = " << r.first.reg_identifier_to_string() << ": dst_reg_id = " << dst_reg_id.reg_identifier_to_string() << endl;
          dst_ulong dst_regnum = dst_reg_id.reg_identifier_get_id();
          ASSERT(dst_regnum >= 0 && dst_regnum < DST_NUM_EXREGS(dst_group));
          uint32_t bitmap = regcons_get_bitmap(peep_regcons, dst_group, dst_regnum);
          //cout << __func__ << " " << __LINE__ << ": found loc, LOC_ID = " << (int)loc << ": pp = " << pp.to_string() << ", group = " << g.first << ", reg = " << r.first.reg_identifier_to_string() << ": dst_reg_id = " << dst_reg_id.reg_identifier_to_string() << ", bitmap = " << hex << bitmap << dec << endl;
          alloc_constraints_add_constant_mapping_using_bitmap(rm_constraints, ssm, bitmap, loc);
        }
      }
    }
  }
}

static void
alloc_constraints_add_constant_mappings_using_regcons_for_temp_regs_used(alloc_constraints_t<exreg_id_obj_t> &alloc_constraints, prog_point_t const &pp, regcons_t const *peep_regcons, regset_t const *peep_temp_regs_used, transmap_loc_id_t loc)
{
#if ARCH_DST == ARCH_I386
  if (loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_SEGS)) {
    return; //XXX: is this really needed? perhaps not
  }
#endif
  ASSERT(loc >= TMAP_LOC_EXREG(0) && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS));
  exreg_group_id_t dst_group = loc - TMAP_LOC_EXREG(0);
  if (!peep_temp_regs_used->excregs.count(dst_group)) {
    return;
  }
  for (const auto &tempr : peep_temp_regs_used->excregs.at(dst_group)) {
    uint32_t bitmap = regcons_get_bitmap(peep_regcons, dst_group, tempr.first);
    alloc_elem_t ssm(pp, alloc_regname_t(alloc_regname_t::TEMP_REG, dst_group, tempr.first));
    alloc_constraints_add_constant_mapping_using_bitmap(alloc_constraints, ssm, bitmap, loc);
  }
}

static void
alloc_constraints_add_constant_mappings_using_regcons(alloc_constraints_t<exreg_id_obj_t> &alloc_constraints, src_ulong pc, nextpc_t const *nextpcs, long num_nextpcs, transmap_t const *peep_in_tmap, transmap_t const *peep_out_tmaps, regcons_t const *peep_regcons, regset_t const *peep_temp_regs_used, transmap_loc_id_t loc)
{
  alloc_constraints_add_constant_mappings_using_regcons_for_transmap(alloc_constraints, prog_point_t(pc, prog_point_t::IN, 0), peep_in_tmap, peep_regcons, loc);
  for (size_t i = 0; i < num_nextpcs; i++) {
    alloc_constraints_add_constant_mappings_using_regcons_for_transmap(alloc_constraints, prog_point_t(pc, prog_point_t::OUT, i), &peep_out_tmaps[i], peep_regcons, loc);
  }
  alloc_constraints_add_constant_mappings_using_regcons_for_temp_regs_used(alloc_constraints, prog_point_t(pc, prog_point_t::IN, 0), peep_regcons, peep_temp_regs_used, loc);
}

#if ARCH_SRC == ARCH_ETFG
static size_t
alloc_stack_insns(dst_insn_t *buf, size_t buf_size, size_t alloc_size, i386_as_mode_t mode)
{
  if (alloc_size == 0) {
    return 0;
  }
  dst_insn_t *ptr = buf;
  dst_insn_t *end = ptr + buf_size;
  ptr += dst_insn_put_add_imm_with_reg(-alloc_size, DST_STACK_REGNUM, tag_const, ptr, end - ptr);
  ASSERT(ptr <= end);
  return ptr - buf;
}

static size_t
dealloc_stack_insns(dst_insn_t *buf, size_t buf_size, size_t alloc_size, i386_as_mode_t mode)
{
  if (alloc_size == 0) {
    return 0;
  }
  dst_insn_t *ptr = buf;
  dst_insn_t *end = ptr + buf_size;
  ptr += dst_insn_put_add_imm_with_reg(alloc_size, DST_STACK_REGNUM, tag_const, ptr, end - ptr);
  ASSERT(ptr <= end);
  return ptr - buf;
}

static void
stack_slot_constraints_add_farg_constraints_at_function_entry(alloc_constraints_t<stack_offset_t> &ssm_constraints, vector<string> const &farg_names, src_ulong pc)
{
  int i = 0;
  for (const auto &farg_name : farg_names) {
    exreg_group_id_t g = ETFG_EXREG_GROUP_GPRS;
    ASSERT(farg_name.substr(0, strlen(G_INPUT_KEYWORD ".")) == G_INPUT_KEYWORD ".");
    reg_identifier_t r = get_reg_identifier_for_regname(farg_name.substr(strlen(G_INPUT_KEYWORD ".")));
    alloc_elem_t ssm(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g, r));
    set<stack_offset_t> constraints;
    constraints.insert(stack_offset_t::stack_offset_arg(i));
    ssm_constraints.add_constraint(ssm, constraints);
    i++;
  }
}

static void
stack_slot_constraints_add_farg_constraints_at_function_callsite(alloc_constraints_t<stack_offset_t> &ssm_constraints, vector<string> const &farg_names, src_ulong pc, bool is_tail_call)
{
  int i = 0;
  for (const auto &farg_name : farg_names) {
    exreg_group_id_t g = ETFG_EXREG_GROUP_GPRS;
    ASSERT(farg_name.substr(0, strlen(G_INPUT_KEYWORD ".")) == G_INPUT_KEYWORD ".");
    reg_identifier_t r = get_reg_identifier_for_regname(farg_name.substr(strlen(G_INPUT_KEYWORD ".")));
    alloc_elem_t ssm(prog_point_t(pc, prog_point_t::IN, 0), alloc_regname_t(alloc_regname_t::SRC_REG, g, r));
    set<stack_offset_t> constraints;
    if (is_tail_call) {
      constraints.insert(stack_offset_t::stack_offset_arg(i));
    } else {
      constraints.insert(stack_offset_t::stack_offset_nonarg(i));
    }
    ssm_constraints.add_constraint(ssm, constraints);
    i++;
  }
}

static void
transmap_set_elem_update_stack_slot_map_constraints(alloc_constraints_t<stack_offset_t> &ssm_constraints, src_ulong pc, transmap_set_elem_t const *tmap_set_elem, input_exec_t const *in, translation_instance_t const *ti)
{
  long idx = pc2index(in, pc);
  si_entry_t const *si = &in->si[idx];
  regset_t const *in_live = &si->bbl->live_regs[si->bblindex];
  transmap_t const *in_tmap = tmap_set_elem->in_tmap;
  transmap_t const *out_tmaps = tmap_set_elem->out_tmaps;
  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_in_tmap<stack_offset_t>(ssm_constraints, in_tmap, prog_point_t(pc, prog_point_t::IN, 0), TMAP_LOC_SYMBOL);
  nextpc_t const *nextpcs = tmap_set_elem->row->nextpcs;
  long num_nextpcs = tmap_set_elem->row->num_nextpcs;
  transmap_t const *nextpc_peep_in_tmaps[num_nextpcs];
  for (size_t i = 0; i < num_nextpcs; i++) {
    alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_in_tmap<stack_offset_t>(ssm_constraints, &out_tmaps[i], prog_point_t(pc, prog_point_t::OUT, i), TMAP_LOC_SYMBOL);
    if (   nextpcs[i].get_val() == PC_JUMPTABLE
        || nextpcs[i].get_val() == PC_UNDEFINED
        || PC_IS_NEXTPC_CONST(nextpcs[i].get_val())) {
      nextpc_peep_in_tmaps[i] = NULL;
    } else {
      long nextpc_idx = pc2index(in, nextpcs[i].get_val());
      if (nextpc_idx < 0 || nextpc_idx >= in->num_si) {
        cout << __func__ << " " << __LINE__ << ": nextpcs[" << i << "] = " << hex << nextpcs[i].get_val() << dec << endl;
        cout << __func__ << " " << __LINE__ << ": nextpc_idx = " << nextpc_idx << endl;
        cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << endl;
      }
      ASSERT(nextpc_idx >= 0 && nextpc_idx < in->num_si);
      nextpc_peep_in_tmaps[i] = &ti->peep_in_tmap[nextpc_idx];
    }
  }
  nomatch_pairs_set_t const *peep_nomatch_pairs_set = &ti->peep_nomatch_pairs_set[si->index];
  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmaps<stack_offset_t>(ssm_constraints, peep_nomatch_pairs_set, in_tmap, out_tmaps, num_nextpcs, pc, TMAP_LOC_SYMBOL);
  transmap_t const *peep_in_tmap = &ti->peep_in_tmap[si->index];
  transmap_t const *peep_out_tmaps = ti->peep_out_tmaps[si->index];
  alloc_constraints_add_equality_mappings_across_peep_translation_for_loc<stack_offset_t>(ssm_constraints, pc, nextpcs, num_nextpcs, peep_in_tmap, peep_out_tmaps, nextpc_peep_in_tmaps, in, TMAP_LOC_SYMBOL);
  string function_name;
  if (in->pc_is_etfg_function_entry(pc, function_name)) {
    stack_slot_constraints_add_farg_constraints_at_function_entry(ssm_constraints, ti->function_arg_names.at(function_name), pc);
  }
  vector<string> argnames;
  if (in->pc_is_etfg_function_callsite(pc, argnames)) {
    if (ti->pc_is_tail_call(pc, function_name)) {
      stack_slot_constraints_add_farg_constraints_at_function_callsite(ssm_constraints, argnames, pc, true);
    } else {
      stack_slot_constraints_add_farg_constraints_at_function_callsite(ssm_constraints, argnames, pc, false);
    }
  }
}

static map<prog_point_t, alloc_map_t<stack_offset_t>>
etfg_identify_stack_slot_map_for_function(translation_instance_t const *ti, input_exec_t const *in, string const &function_name, src_ulong function_pc)
{
  alloc_constraints_t<stack_offset_t> ssm_constraints(ti, TMAP_LOC_SYMBOL);
  long idx = pc2index(in, function_pc);
  //size_t max_offset_used = 0;
  ASSERT(idx >= 0 && idx < in->num_si);
  map<prog_point_t, alloc_map_t<stack_offset_t>> ssm;
  while (idx < in->num_si) {
    long insn_num_in_function;
    string pc_function_name;
    etfg_pc_get_function_name_and_insn_num(in, in->si[idx].pc, pc_function_name, insn_num_in_function);
    if (function_name != pc_function_name) {
      break;
    }
    auto d = ti->transmap_decision[idx];
    if (!d) {
      idx++;
      continue;
    }
    si_entry_t const *si = &in->si[idx];
    //src_ulong nextpcs[SI_MAX_NUM_NEXTPCS];
    //long num_nextpcs = pc_get_nextpcs(in, si->pc, nextpcs, NULL);
    ssm[prog_point_t(si->pc, prog_point_t::IN, 0)] = alloc_map_t<stack_offset_t>();
    for (long i = 0; i < d->row->num_nextpcs; i++) {
      prog_point_t pp(si->pc, prog_point_t::OUT, i);
      //cout << __func__ << " " << __LINE__ << ": function_name: " << function_name << ": " << pp.to_string() << ": initializing alloc_map\n";
      ssm[pp] = alloc_map_t<stack_offset_t>();
    }
    
    //cout << __func__ << " " << __LINE__ << ": si->pc = " << hex << si->pc << dec << endl;
    transmap_set_elem_update_stack_slot_map_constraints(ssm_constraints, si->pc, d, in, ti);
    idx++;
  }
  CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": calling ssm_constraints.solve()\n");
  ssm_constraints.solve(ssm);
  //size_t max_offset_used = 0;
  //return max_offset_used;
  return ssm;
}

static void
etfg_identify_stack_slot_map_for_each_function(translation_instance_t *ti)
{
  input_exec_t const *in = ti->in;
  string function_name;
  for (size_t i = 0; i < in->num_si; i++) {
    src_ulong pc = in->si[i].pc;
    if (in->pc_is_etfg_function_entry(pc, function_name)) {
      //ti->stack_space_for_function[function_name] = etfg_identify_stack_space_for_function(ti, in, function_name, pc);
      CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": function_name = " << function_name << endl);
      ti->stack_slot_map_for_function[function_name] = etfg_identify_stack_slot_map_for_function(ti, in, function_name, pc);
    }
  }
}

static void
ti_update_stack_slot_addresses(translation_instance_t *ti)
{
  transmap_t *peep_in_tmap, *peep_out_tmaps;
  input_exec_t const *in = ti->in;
  regcons_t const *peep_regcons;
  long insn_num_in_function;
  string function_name;

  for (size_t s = 0; s < in->num_si; s++) {
    si_entry_t const *si = &in->si[s];
    transmap_set_elem_t *d = ti->transmap_decision[s];
    ASSERT(si->index == s);
    if (!d) {
      continue;
    }
    //peep_regcons = &ti->peep_regcons[si->index];
    peep_in_tmap = &ti->peep_in_tmap[si->index];
    peep_out_tmaps = ti->peep_out_tmaps[si->index];
    regset_t const *in_live = &si->bbl->live_regs[si->bblindex];

    etfg_pc_get_function_name_and_insn_num(in, si->pc, function_name, insn_num_in_function);
    prog_point_t in_pp = prog_point_t(si->pc, prog_point_t::IN, 0);
    size_t stack_frame_size = ti_get_stack_alloc_space_for_function(ti, function_name);
    if (ti->stack_slot_map_for_function.at(function_name).count(in_pp)) {
      alloc_map_t<stack_offset_t> const &in_ssm = ti->stack_slot_map_for_function.at(function_name).at(in_pp);
      transmap_assign_stack_slots_for_in_tmap(d->in_tmap, in_ssm, stack_frame_size);
    }
    //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << si->pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": " << hex << si->pc << dec << ": after assigning stack slots, d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << "\nin_ssm =\n" << in_ssm.to_string() << endl;
    //src_ulong nextpcs[SI_MAX_NUM_NEXTPCS];
    //long num_nextpcs = pc_get_nextpcs(in, si->pc, nextpcs, NULL);
    for (long i = 0; i < d->row->num_nextpcs; i++) {
      //cout << __func__ << " " << __LINE__ << ": function_name = " << function_name << ": si->pc = " << hex << si->pc << dec << ": i = " << i << endl;
      ASSERT(ti->stack_slot_map_for_function.count(function_name));
      //ASSERT(ti->stack_slot_map_for_function.at(function_name).count(prog_point_t(si->pc, prog_point_t::OUT, i)));
      if (ti->stack_slot_map_for_function.at(function_name).count(prog_point_t(si->pc, prog_point_t::OUT, i))) {
        //cout << __func__ << " " << __LINE__ << ": no mapping exists for " << function_name << ": pc " << hex << si->pc << dec << ", OUT, " << i << endl;
        alloc_map_t<stack_offset_t> const &out_ssm = ti->stack_slot_map_for_function.at(function_name).at(prog_point_t(si->pc, prog_point_t::OUT, i));
        transmap_assign_stack_slots_for_out_tmap(&d->out_tmaps[i], d->in_tmap, peep_in_tmap, &peep_out_tmaps[i], in_live, out_ssm, stack_frame_size);
      }
      //if (   d->row->nextpcs[i] != PC_JUMPTABLE
      //    && d->row->nextpcs[i] != PC_UNDEFINED
      //    && !PC_IS_NEXTPC_CONST(d->row->nextpcs[i])
      //    && ti->stack_slot_map_for_function.at(function_name).count(d->row->nextpcs[i])) { //if the nextpc belongs to the same function
      //  alloc_map_t<stack_offset_t> const &out_ssm = ti->stack_slot_map_for_function.at(function_name).at(d->row->nextpcs[i]);
      //  //transmap_assign_stack_slots(&d->out_tmaps[i], in_ssm /*XXX: shouldn't this be out_ssm?*/, stack_frame_size);
      //  transmap_assign_stack_slots_for_out_tmap(&d->out_tmaps[i], d->in_tmap, peep_in_tmap, &peep_out_tmaps[i], in_live, out_ssm, stack_frame_size);
      //  //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << si->pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
      //} else {
      //  transmap_copy(&d->out_tmaps[i], d->in_tmap);
      //}
    }
  }
}

static void
etfg_identify_args_for_each_function(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  input_exec_t const *in = ti->in;
  string function_name;
  ti->function_arg_names.clear();
  for (size_t i = 0; i < in->num_si; i++) {
    src_ulong pc = in->si[i].pc;
    if (in->pc_is_etfg_function_entry(pc, function_name)) {
      ti->function_arg_names[function_name] = etfg_input_exec_get_arg_names_for_function(in, function_name);
      DYN_DEBUG(stat_trans,
          cout << __func__ << " " << __LINE__ << ": args for function " << function_name << ":";
        for (const auto &arg: ti->function_arg_names.at(function_name)) {
          cout << " " << arg;
        }
        cout << endl;
      );
    }
  }
}
#endif

static void
transmap_set_elem_update_regmap_constraints_for_group(alloc_constraints_t<exreg_id_obj_t> &rm_constraints, src_ulong pc, transmap_set_elem_t const *tmap_set_elem, input_exec_t const *in, translation_instance_t const *ti , exreg_group_id_t g)
{
  long idx = pc2index(in, pc);
  si_entry_t const *si = &in->si[idx];
  transmap_t const *in_tmap = tmap_set_elem->in_tmap;
  transmap_t const *out_tmaps = tmap_set_elem->out_tmaps;
  transmap_t const *peep_in_tmap = &ti->peep_in_tmap[si->index];
  transmap_t const *peep_out_tmaps = ti->peep_out_tmaps[si->index];
  regcons_t const *peep_regcons = &ti->peep_regcons[si->index];
  regset_t const *peep_temp_regs_used = &ti->peep_temp_regs_used[si->index];
  nextpc_t const *nextpcs = tmap_set_elem->row->nextpcs;
  long num_nextpcs = tmap_set_elem->row->num_nextpcs;

  //cout << __func__ << " " << __LINE__ << ": pc " << hex << pc << dec << ": peep_regcons:\n" << regcons_to_string(peep_regcons, as1, sizeof as1) << endl;

  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_in_tmap<exreg_id_obj_t>(rm_constraints, in_tmap, prog_point_t(pc, prog_point_t::IN, 0), TMAP_LOC_EXREG(g));
  //regset_t const *out_live[num_nextpcs];
  /*for (size_t i = 0; i < num_nextpcs; i++) {
    if (   nextpcs[i] == PC_JUMPTABLE
        || nextpcs[i] == PC_UNDEFINED
        || PC_IS_NEXTPC_CONST(nextpcs[i])) {
      out_live[i] = NULL;
    } else {
      long nextpc_idx = pc2index(in, nextpcs[i]);
      if (nextpc_idx < 0 || nextpc_idx >= in->num_si) {
        cout << __func__ << " " << __LINE__ << ": i = " << i << endl;
        cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << endl;
        cout << __func__ << " " << __LINE__ << ": nextpcs[i] = " << hex << nextpcs[i] << dec << endl;
        cout << __func__ << " " << __LINE__ << ": nextpc_idx = " << nextpc_idx << endl;
      }
      ASSERT(nextpc_idx >= 0 && nextpc_idx < in->num_si);
      si_entry_t const *nextsi = &in->si[nextpc_idx];
      //out_live[i] = &nextsi->bbl->live_regs[nextsi->bblindex];
    }
  }*/
  transmap_t const *nextpc_peep_in_tmaps[num_nextpcs];
  for (size_t i = 0; i < num_nextpcs; i++) {
    alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_in_tmap<exreg_id_obj_t>(rm_constraints, &out_tmaps[i], prog_point_t(pc, prog_point_t::OUT, i), TMAP_LOC_EXREG(g));
    if (   nextpcs[i].get_val() == PC_JUMPTABLE
        || nextpcs[i].get_val() == PC_UNDEFINED
        || PC_IS_NEXTPC_CONST(nextpcs[i].get_val())) {
      nextpc_peep_in_tmaps[i] = NULL;
    } else {
      long nextpc_idx = pc2index(in, nextpcs[i].get_val());
      ASSERT(nextpc_idx >= 0 && nextpc_idx < in->num_si);
      nextpc_peep_in_tmaps[i] = &ti->peep_in_tmap[nextpc_idx];
    }
  }
  nomatch_pairs_set_t const *peep_nomatch_pairs_set = &ti->peep_nomatch_pairs_set[si->index];
  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmaps<exreg_id_obj_t>(rm_constraints, peep_nomatch_pairs_set, in_tmap, out_tmaps, num_nextpcs, pc, TMAP_LOC_EXREG(g));

  alloc_constraints_add_negative_mappings_for_regs_mapped_to_loc_across_tmaps_and_temp_regs_used<exreg_id_obj_t>(rm_constraints, in_tmap, out_tmaps, num_nextpcs, peep_temp_regs_used, pc, TMAP_LOC_EXREG(g));

  alloc_constraints_add_equality_mappings_across_peep_translation_for_loc<exreg_id_obj_t>(rm_constraints, pc, nextpcs, num_nextpcs, peep_in_tmap, peep_out_tmaps, nextpc_peep_in_tmaps, in, TMAP_LOC_EXREG(g));
  alloc_constraints_add_constant_mappings_using_regcons(rm_constraints, pc, nextpcs, num_nextpcs, peep_in_tmap, peep_out_tmaps, peep_regcons, peep_temp_regs_used, TMAP_LOC_EXREG(g));
  //cout << __func__ << " " << __LINE__ << ": after adding constraints for " << hex << pc << dec << ": rm_constraints =\n" << rm_constraints.to_string() << endl;
}

static void
transmap_assign_regnames_for_in_tmap_for_group(transmap_t *tmap, alloc_map_t<exreg_id_obj_t> const &rm, exreg_group_id_t dst_group)
{
  for (auto &g : tmap->extable) {
    for (auto &r : g.second) {
      r.second.transmap_entry_assign_regnames_for_group(rm, g.first, r.first, dst_group);
    }
  }
}

static void
transmap_assign_regnames_for_out_tmap_for_group(transmap_t *out_tmap, transmap_t const *in_tmap, transmap_t const *peep_in_tmap, transmap_t const *peep_out_tmap, alloc_map_t<exreg_id_obj_t> const &out_rm, exreg_group_id_t dst_group)
{
  //for regs that are common between peep_in_tmap and peep_out_tmap, just use peep_in_tmap, peep_out_tmap, and in_tmap to get out_tmap
  //for other slots, call transmap_entry_assign_stack_slots with out_rm
  
  //first assign using out_rm
  transmap_assign_regnames_for_in_tmap_for_group(out_tmap, out_rm, dst_group);

  //cout << __func__ << " " << __LINE__ << ": " << ": out_tmap =\n" << transmap_to_string(out_tmap, as1, sizeof as1) << "\nout_rm =\n" << out_rm.to_string() << endl;

  //now change the names for the ones that are common between peep_in_tmap and peep_out_tmap
  for (const auto &g : peep_out_tmap->extable) {
    for (const auto &r : g.second) {
      if (!r.second.contains_mapping_to_loc(TMAP_LOC_EXREG(dst_group))) {
        continue;
      }
      reg_identifier_t const &reg_id = r.second.get_reg_id_for_loc(TMAP_LOC_EXREG(dst_group));
      if (reg_id.reg_identifier_is_unassigned()) {
        continue;
      }
      dst_ulong regnum = reg_id.reg_identifier_get_id(); //r.second.get_regnum_for_loc(TMAP_LOC_EXREG(dst_group));
      ASSERT(regnum >= 0 && regnum < DST_NUM_EXREGS(dst_group));
      reg_identifier_t src_reg_id = reg_identifier_t::reg_identifier_invalid();
      exreg_group_id_t src_group;
      if (peep_in_tmap->transmap_maps_to_exreg_loc(dst_group, regnum, src_group, src_reg_id)) {
        reg_identifier_t const &ireg_id = in_tmap->extable.at(src_group).at(src_reg_id).get_reg_id_for_loc(TMAP_LOC_EXREG(dst_group));
        if (ireg_id.reg_identifier_is_unassigned()) {
          cout << __func__ << " " << __LINE__ << ": peep_in_tmap =\n" << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": peep_out_tmap =\n" << transmap_to_string(peep_out_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": in_tmap =\n" << transmap_to_string(in_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": out_tmap =\n" << transmap_to_string(out_tmap, as1, sizeof as1) << endl;
          cout << __func__ << " " << __LINE__ << ": src_group = " << src_group << endl;
          cout << __func__ << " " << __LINE__ << ": src_reg_id = " << src_reg_id.reg_identifier_to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << endl;
          cout << __func__ << " " << __LINE__ << ": r.first = " << r.first.reg_identifier_to_string() << endl;
        }
        ASSERT(!ireg_id.reg_identifier_is_unassigned());
        if (   out_tmap->extable.count(g.first)
            && out_tmap->extable.at(g.first).count(r.first)) {
          out_tmap->extable.at(g.first).at(r.first).transmap_entry_update_regnum_for_loc(TMAP_LOC_EXREG(dst_group), ireg_id.reg_identifier_get_id());
        }
      }
    }
  }
}

static void
transmap_set_elem_assign_regnames_for_group(transmap_set_elem_t *d, src_ulong pc, transmap_t const *peep_in_tmap, transmap_t const *peep_out_tmaps, map<prog_point_t, alloc_map_t<exreg_id_obj_t>> const &rm, exreg_group_id_t g)
{
  prog_point_t in_pp(pc, prog_point_t::IN, 0);
  if (rm.count(in_pp)) {
    transmap_assign_regnames_for_in_tmap_for_group(d->in_tmap, rm.at(in_pp), g);
  }
  //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": " << hex << pc << dec << ": after assigning regnames, d->in_tmap =\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << "\nin_rm =\n" << rm.at(pc).to_string() << endl;
  for (size_t i = 0; i < d->row->num_nextpcs; i++) {
    prog_point_t out_pp(pc, prog_point_t::OUT, i);
    if (rm.count(out_pp)) {
      alloc_map_t<exreg_id_obj_t> const &out_rm = rm.at(out_pp);
      transmap_assign_regnames_for_out_tmap_for_group(&d->out_tmaps[i], d->in_tmap, peep_in_tmap, &peep_out_tmaps[i], out_rm, g);
    }
  }
//    if (d->row->nextpcs[i] == PC_JUMPTABLE) {
//#if ARCH_DST == ARCH_I386
//      transmap_copy(&d->out_tmaps[i], transmap_default());
//#endif
//    } else if (   d->row->nextpcs[i] != PC_UNDEFINED
//               && !PC_IS_NEXTPC_CONST(d->row->nextpcs[i])
//               && rm.count(d->row->nextpcs[i])) {
//      //alloc_map_t const &out_ssm = ti->stack_slot_map_for_function.at(function_name).at(d->row->nextpcs[i]);
//      //cout << __func__ << " " << __LINE__ << ": " << (long)pc << ": nextpcs[" << i << "] = " << (long)d->row->nextpcs[i] << "\n";
//      //if (!rm.count(d->row->nextpcs[i])) {
//      //  continue;
//      //  //cout << __func__ << " " << __LINE__ << ": " << hex << (long)pc << dec << ": nextpcs[" << i << "] = " << hex << (long)d->row->nextpcs[i] << dec << "\n" << flush;
//      //}
//      //ASSERT(rm.count(d->row->nextpcs[i]));
//      alloc_map_t<exreg_id_obj_t> const &out_rm = rm.at(d->row->nextpcs[i]);
//      //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//      //cout << __func__ << " " << __LINE__ << ": peep_out_tmaps[" << i << "]: " << hex << pc << dec << ": " << transmap_to_string(&peep_out_tmaps[i], as1, sizeof as1) << endl;
//      transmap_assign_regnames_for_out_tmap_for_group(&d->out_tmaps[i], d->in_tmap, peep_in_tmap, &peep_out_tmaps[i], out_rm, g);
//      //cout << __func__ << " " << __LINE__ << ": " << hex << pc << dec << ": after assigning regnames, d->out_tmaps[" << i << "] =\n" << transmap_to_string(&d->out_tmaps[i], as1, sizeof as1) << "\nout_rm =\n" << out_rm.to_string() << endl;
//      //cout << __func__ << " " << __LINE__ << ": peep_in_tmap: " << hex << si->pc << dec << ": " << transmap_to_string(peep_in_tmap, as1, sizeof as1) << endl;
//    } else {
//      transmap_copy(&d->out_tmaps[i], d->in_tmap);
//    }
}

static void
ti_identify_regname_map(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  input_exec_t const *in = ti->in;
  for (exreg_group_id_t g = 0; g < DST_NUM_EXREG_GROUPS; g++) {
    alloc_constraints_t<exreg_id_obj_t> rm_constraints(ti, TMAP_LOC_EXREG(g));
    
    map<prog_point_t, alloc_map_t<exreg_id_obj_t>> rm;
    for (size_t i = 0; i < in->num_si; i++) {
      if (!ti->transmap_decision[i]) {
        continue;
      }
      src_ulong pc = in->si[i].pc;
      rm[prog_point_t(pc, prog_point_t::IN, 0)] = alloc_map_t<exreg_id_obj_t>();
      long num_nextpcs = ti->transmap_decision[i]->row->num_nextpcs;
      for (long n = 0; n < num_nextpcs; n++) {
        rm[prog_point_t(pc, prog_point_t::OUT, n)] = alloc_map_t<exreg_id_obj_t>();
      }
      transmap_set_elem_update_regmap_constraints_for_group(rm_constraints, pc, ti->transmap_decision[i], in, ti, g);
    }
    CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS, cout << __func__ << " " << __LINE__ << ": calling rm_constraints.solve(). g = " << g << "\n");
    rm_constraints.solve(rm);
    for (size_t i = 0; i < in->num_si; i++) {
      if (!ti->transmap_decision[i]) {
        continue;
      }
      transmap_t const *peep_in_tmap = &ti->peep_in_tmap[i];
      transmap_t const *peep_out_tmaps = ti->peep_out_tmaps[i];

      //alloc_map_t<exreg_id_obj_t> const &aregmap = rm.at(in->si[i].pc);
      transmap_set_elem_assign_regnames_for_group(ti->transmap_decision[i], in->si[i].pc, peep_in_tmap, peep_out_tmaps, rm, g);
    }
  }
}

static void
choose_solution(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  input_exec_t *in;
  bool any_change;
  si_entry_t *si;
  long i;

  in = ti->in;
  ti->transmap_decision = (transmap_set_elem_t **)malloc(in->num_si *
      sizeof(transmap_set_elem_t *));
  ASSERT(ti->transmap_decision);
  memset(ti->transmap_decision, 0, in->num_si * sizeof(transmap_set_elem_t *));

  any_change = true;
  in = ti->in;

  //for (i = in->num_si - 1; i >= 0; i--) {
  //  si = &in->si[i];
  //  pick_best_transmap_and_propagate(ti, si, NULL);
  //}
  for (i = in->num_si - 1; i >= 0; i--) {
    si = &in->si[i];
    string function_name;
    nextpc_t nextpcs[MAX_NUM_NEXTPCS];

    long num_nextpcs = pc_get_nextpcs(in, si->pc, nextpcs, NULL);
    //cout << __func__ << " " << __LINE__ << ": pc " << hex << si->pc << dec << ": num_nextpcs = " << num_nextpcs << endl;
    if (   num_nextpcs == 0
        || ((num_nextpcs == 1) && (nextpcs[0].get_val() == PC_JUMPTABLE))) {
      DBG(CHOOSE_SOLUTION, "calling pick_best_transmap_and_propagate on %lx with basecase flag = false (visited!=NULL)\n", (long)si->pc);
      pick_best_transmap_and_propagate(ti, si/*, visited*/);
    }
  }
  for (i = in->num_si - 1; i >= 0; i--) {
    vector<string> argnames;
    if (!ti->transmap_decision[i] && in->pc_is_etfg_function_callsite(in->si[i].pc, argnames)) { //to deal with calls to exit functions
      si = &in->si[i];
      pick_best_transmap_and_propagate(ti, si);
    }
  }
  eliminate_insns_with_overlapping_decisions(ti);
  ti_gen_peep_tmaps_and_regcons(ti);

  peep_tmaps_and_regcons_print(logfile, ti);
#if ARCH_SRC == ARCH_ETFG
  PROGRESS("%s", "Identifying stack space for each function");
  etfg_identify_stack_slot_map_for_each_function(ti);
  ti_update_stack_slot_addresses(ti);
#endif
  //acquiesce_decisions(ti);
}

static void
identify_branches_to_fallthrough_pc(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  long i, dst_iseq_len, max_dst_iseq_len;
  dst_insn_t *dst_iseq;
  si_entry_t *prev_si;
  input_exec_t *in;

  in = ti->in;
  dst_iseq_len = 0;
  max_dst_iseq_len = max(in->num_si * 2, 256UL);
  //dst_iseq = (dst_insn_t *)malloc(max_dst_iseq_len * sizeof(dst_insn_t));
  dst_iseq = new dst_insn_t[max_dst_iseq_len];
  ASSERT(dst_iseq);
  //cout << __func__ << " " << __LINE__ << endl;
  for (i = 0; i < (long)in->num_si; i++) {
    if (   dst_iseq_len > 0
        && dst_insn_is_unconditional_branch_not_fcall(&dst_iseq[dst_iseq_len - 1])
        && dst_insn_branch_target_src_insn_pc(&dst_iseq[dst_iseq_len - 1]) == in->si[i].pc) {
      ASSERT(prev_si);
      prev_si->eliminate_branch_to_fallthrough_pc = true;
    }
    if (ti->insn_eliminated[i]) {
      continue;
    }
    //cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << endl;
    /*extern char REGMAP_ASSIGN_flag;
    if (in->si[i].pc == 0x10004cec) {
      REGMAP_ASSIGN_flag = 1;
    }*/
    dst_iseq_len = si_gen_dst_iseq(ti, &in->si[i]/*, NULL*/, dst_iseq, max_dst_iseq_len,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    /*if (in->si[i].pc == 0x10004cec) {
      REGMAP_ASSIGN_flag = 0;
    }*/
    prev_si = &in->si[i];
  }
  //cout << __func__ << " " << __LINE__ << endl;
  //free(dst_iseq);
  delete [] dst_iseq;
  //cout << __func__ << " " << __LINE__ << endl;
}

static size_t
i386_put_unconditional_branch_code_to_entry(uint8_t *buf, size_t size, src_ulong entry,
    struct chash const *tcodes)
{
  uint8_t const *entry_tcode;
  uint8_t *ptr, *end;

  ptr = buf;
  end = buf + size;

  entry_tcode = tcodes_query(tcodes, entry);

  *ptr++ = 0xe9;
  *(uint32_t *)ptr = entry_tcode - (ptr + 4);
  return 5;
}


static void
fprintf_translation(FILE *fp, char const *comment, si_entry_t const *si, uint8_t *p,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    char const *reloc_strings, size_t reloc_strings_size)
{
  long i;
  fprintf(logfile, "%lx[%ld]: %s (%lx)\n%s\n\n", (long)si->pc, (long)si->pc, comment, (p - TX_TEXT_START_PTR) + TX_TEXT_START_ADDR,
      dst_iseq_with_relocs_to_string(dst_iseq, dst_iseq_len, reloc_strings, reloc_strings_size, I386_AS_MODE_32, as1, sizeof as1));
  fflush(logfile);
}

static bool
pc_belongs_to_start_function(input_exec_t const *in, src_ulong pc)
{
#if ARCH_SRC == ARCH_ETFG
  return false;
#endif
  char const *function_name;
  src_ulong function_pc;
  function_pc = get_function_containing_pc(in, pc);
  if (function_pc == INSN_PC_INVALID) {
    return false;
  }
  function_name = pc2function_name(in, function_pc);
  if (!strcmp(function_name, "_start")) {
    return true;
  }
  return false;
}

void
dst_iseq_rename_immediate_operands_containing_code_addresses(dst_insn_t *dst_iseq,
    long dst_iseq_len, struct chash const *tcodes)
{
  long i;
  for (i = 0; i < dst_iseq_len; i++) {
    dst_insn_rename_immediate_operands_containing_code_addresses(&dst_iseq[i], tcodes);
  }
}

static long
eliminate_branch_to_fallthrough_pc(dst_insn_t *dst_iseq, long dst_iseq_len)
{
  dst_ulong fallthrough_pc;
  valtag_t *op, *last_pcrel;
  long i;

  ASSERT(dst_iseq_len > 0);
  last_pcrel = dst_insn_get_pcrel_operand(&dst_iseq[dst_iseq_len - 1]);
  ASSERT(last_pcrel);
  ASSERT(last_pcrel->tag == tag_var || last_pcrel->tag == tag_src_insn_pc);
  fallthrough_pc = last_pcrel->val;

  //do {
  for (i = dst_iseq_len - 2; i >= 0; i--) {
    if (   (op = dst_insn_get_pcrel_operand(&dst_iseq[i]))
        && op->tag == tag_const
        && op->val == dst_iseq_len - (i + 1) - 1) {
      op->tag = last_pcrel->tag;
      op->val = fallthrough_pc;
    }
  }
  dst_iseq_len--;
  /*} while (   dst_iseq_len > 0
           && dst_insn_is_unconditional_branch(&dst_iseq[dst_iseq_len - 1])
           && dst_insn_branch_target_var(&dst_iseq[dst_iseq_len - 1])
                  == fallthrough_pc);*/
  return dst_iseq_len;
}

static size_t
debug_call_print_machine_state(translation_instance_t *ti,
    dst_insn_t *insn_buf, size_t insn_buf_size, src_ulong pc,
    regset_t const *live_in)
{
  dst_insn_t *ptr = insn_buf, *end = insn_buf + insn_buf_size;
  uint64_t bitmap;

  bitmap = regset_compute_bitmap(live_in);

#if ARCH_DST == ARCH_I386
  ptr += i386_insn_put_movl_imm_to_reg(SRC_TMP_STACK, R_ESP, ptr, end - ptr);
 
  /* push high-order bits of live_regs_bitmap */
  ptr += i386_insn_put_push_imm((uint32_t)(bitmap >> 32), tag_const, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  /* push low-order bits of live_regs_bitmap */
  ptr += i386_insn_put_push_imm((uint32_t)bitmap, tag_const, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);

  /* push address of the environment structure */
  ptr += i386_insn_put_push_imm((uint32_t)SRC_ENV_ADDR, tag_const, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);

  /* push pc */
  ptr += i386_insn_put_push_imm((uint32_t)pc, tag_const, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);

  /* call __dbg_print_machine_state */
  ptr += i386_insn_put_call_helper_function(ti, "__dbg_print_machine_state", ti->ti_get_dst_fixed_reg_mapping(), ptr,
      end - ptr);
#else
  NOT_IMPLEMENTED();
#endif

  return ptr - insn_buf;
}

#if ARCH_SRC == ARCH_ETFG
static size_t
etfg_dyn_debug_print_globals_argc_argv_at_start(translation_instance_t *ti, dst_insn_t *buf, size_t buf_size)
{
  DBG(SETUP_CODE, "%s", "entry\n");
  dst_insn_t *ptr = buf, *end = buf + buf_size;
  fixed_reg_mapping_t const &fixed_reg_mapping = ti->ti_get_dst_fixed_reg_mapping();
  ptr += i386_insn_put_movl_reg_to_reg(R_EBP, R_ESP, ptr, end - ptr);
  stringstream format_string;
  input_exec_t const *in = ti->in;
  for (const auto &sym : in->symbol_map) {
    if (   sym.second.type == STT_FUNC
        /*|| sym.second.name == string(SYMBOL_ID_DST_RODATA_NAME)*/) {
      continue;
    }
    format_string << sym.second.name << ": " << hex << sym.second.value << dec << "\n";
  }
  format_string << "argc: %x\n";
  format_string << "argv: %x\n";
  string format_string_str = format_string.str();
  ptr += i386_insn_put_pushb_imm('\0', fixed_reg_mapping, ptr, end - ptr);
  for (ssize_t i = format_string_str.size() - 1; i >= 0; i--) {
    ptr += i386_insn_put_pushb_imm(format_string_str.at(i), fixed_reg_mapping, ptr, end - ptr);
  }
  ptr += i386_insn_put_movl_reg_to_reg(R_EAX, R_ESP, ptr, end - ptr);
  ptr += i386_insn_put_push_mem(8, R_EBP, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_push_mem(4, R_EBP, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_push_reg(R_EAX, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_call_helper_function(ti, "printf", fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(format_string_str.size() + 13, R_ESP, tag_const, ptr, end - ptr);
  format_string.str("");
  format_string << "argv[%d] = %x : \"%s\"\n";
  format_string_str = format_string.str();
  ptr += i386_insn_put_pushb_imm('\0', fixed_reg_mapping, ptr, end - ptr);
  for (ssize_t i = format_string_str.size() - 1; i >= 0; i--) {
    ptr += i386_insn_put_pushb_imm(format_string_str.at(i), fixed_reg_mapping, ptr, end - ptr);
  }
  ptr += i386_insn_put_movl_reg_to_reg(R_EBX, R_ESP, ptr, end - ptr);
  ptr += i386_insn_put_movl_imm_to_reg(0, R_ESI, ptr, end - ptr);
  ptr += i386_insn_put_movl_mem_to_reg(R_EDI, 8, R_EBP, ptr, end - ptr);
  dst_insn_t *loop_start = ptr;
  ptr += i386_insn_put_push_mem(0, R_EDI, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_push_mem(0, R_EDI, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_push_reg(R_ESI, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_push_reg(R_EBX, fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_call_helper_function(ti, "printf", fixed_reg_mapping, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(16, R_ESP, tag_const, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(1, R_ESI, tag_const, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(4, R_EDI, tag_const, ptr, end - ptr);
  ptr += i386_insn_put_cmpl_mem_to_reg(R_ESI, 4, R_EBP, ptr, end - ptr);
  size_t jcc_size = i386_insn_put_jcc_to_pcrel("jl", loop_start - ptr, ptr, end - ptr);
  src_ulong pcrel = loop_start - (ptr + jcc_size);
  DBG(SETUP_CODE, "pcrel = %lx\n", (long)pcrel);
  ptr += i386_insn_put_jcc_to_pcrel("jl", pcrel, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(format_string_str.size() + 1, R_ESP, tag_const, ptr, end - ptr);
  ASSERT(ptr < end);
  CPP_DBG_EXEC(SETUP_CODE, cout << __func__ << " " << __LINE__ << ": ptr - buf = " << (ptr - buf) << endl);
  return ptr - buf;
}

static size_t
etfg_dyn_debug_gen_insns_printing_cur_state(translation_instance_t *ti, src_ulong pc, long fetchlen,
    transmap_t const *in_tmap, regset_t const &live_out, dst_insn_t *buf, size_t buf_size)
{
  autostop_timer func_timer(__func__);
  dst_insn_t *ptr = buf, *end = buf + buf_size;
  fixed_reg_mapping_t const &dst_fixed_reg_mapping = ti->ti_get_dst_fixed_reg_mapping();
#if ARCH_DST == ARCH_I386
#define PUSHA_SIZE (8*(DWORD_LEN/BYTE_LEN))
  input_exec_t const *in = ti->in;
  size_t orig_stack_offset = 0;
  ptr += i386_insn_put_pusha(ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr, I386_AS_MODE_32);
  orig_stack_offset += PUSHA_SIZE;
  //ptr += i386_insn_put_pushf(ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  //orig_stack_offset += DWORD_LEN/BYTE_LEN;
  stringstream format_string;
  size_t alloc_size = fetchlen * 4 + 256;
  etfg_insn_t *src_iseq = new etfg_insn_t[alloc_size];
  nextpc_t *nextpcs = new nextpc_t[alloc_size];
  long src_iseq_len, num_nextpcs, num_lastpcs;
  regset_t use, def;
  memset_t memuse, memdef;
  bool fetched;
  fetched = src_iseq_fetch(src_iseq, alloc_size, &src_iseq_len, in, &ti->peep_table, pc, fetchlen,
        &num_nextpcs, nextpcs, NULL, NULL, NULL, NULL, false);
  ASSERT(fetched);
  delete[] nextpcs;
  src_iseq_get_usedef(src_iseq, src_iseq_len, &use, &def, &memuse, &memdef);
  format_string << "pc: " << hex << pc << dec << ": fetchlen " << fetchlen << ": ";
  for (long i = 0; i < src_iseq_len; i++) {
    string s = etfg_insn_edges_to_string(src_iseq[i]);
    string_replace(s, "%", "%%");
    format_string << s << "; ";
  }
  format_string << "\n";
  delete[] src_iseq;
  //format_string << etfg_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << "\n";
  format_string << "esp: %x\n";
  vector<state_elem_desc_t> values_to_push;
  for (const auto &g : in_tmap->extable) {
    for (const auto &r : g.second) {
      transmap_entry_t const *in_tmap_entry = transmap_get_elem(in_tmap, g.first, r.first);
      if (in_tmap_entry->get_locs().size() == 0 || !regset_belongs_ex(&live_out, g.first, r.first)) {
        continue;
      }
      //cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << endl;
      //cout << __func__ << " " << __LINE__ << ": r.first = " << r.first.reg_identifier_to_string() << endl;
      string reg_id = r.first.reg_identifier_to_string();
      string_replace(reg_id, "%", "%%");
      format_string << "exr" << g.first << "." << reg_id << " :" << in_tmap_entry->transmap_entry_to_string() << " :";
      for (size_t l = 0; l < in_tmap_entry->get_locs().size(); l++) { //print all locs
        format_string << " %x";
        state_elem_desc_t e;
        e.loc = TS_TAB_ENTRY_STATE_LOC_EXREG;
        e.base = g.first;
        e.index = r.first;
        e.regmask = MAKE_MASK(ETFG_EXREG_LEN(g.first));
        e.len = ETFG_EXREG_LEN(g.first);
        e.disp.val = l;
        e.disp.tag = tag_const;
        values_to_push.push_back(e);
      }
      format_string << "\n";
    }
  }
  if (false && src_iseq_len == 1) { //XXX: when src_iseq_len, the memsets seem to be out of sync; need to debug; for now just restrict memset printing to the case when src_iseq_len == 1
    memset_union(&memdef, &memuse);
    for (const auto &ma : memdef.memaccess) {
      string memname = state_elem_desc_to_string(&ma, as1, sizeof as1);
      string_replace(memname, "%", "%%");
      format_string << memname << ": %x\n";
      values_to_push.push_back(ma);
    }
  }
  format_string << DYN_DEBUG_STATE_END_OF_STATE_MARKER << "\n";
  ptr += i386_insn_put_pushb_imm('\0', ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  orig_stack_offset++;
  string format_string_str = format_string.str();
  //cout << "format_string = " << format_string_str << endl;
  //cout << "format_string.str().size() = " << format_string.str().size() << endl;
  //cout << "end - ptr = " << end - ptr << endl;
  for (ssize_t i = format_string_str.size() - 1; i >= 0; i--) {
    ptr += i386_insn_put_pushb_imm(format_string_str.at(i), ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
    //cout << "i = " << i << ", end - ptr = " << end - ptr << endl;
    orig_stack_offset++;
  }
  ASSERT(orig_stack_offset == format_string_str.size() + PUSHA_SIZE + 1);
  //cout << __func__ << " " << __LINE__ << hex << " pc: " << pc << ": orig_stack_offset = " << orig_stack_offset << " : format_string_str.size() = " << format_string_str.size() << dec << endl;
  size_t format_string_stack_offset = 0;

  for (ssize_t i = values_to_push.size() - 1; i >= 0; i--) {
    //cout << __func__ << " " << __LINE__ << ": g = " << values_to_push.at(i).first << endl;
    //cout << __func__ << " " << __LINE__ << ": r = " << values_to_push.at(i).second.reg_identifier_to_string() << endl;
    state_elem_desc_t const &se = values_to_push.at(i);
    if (se.loc == TS_TAB_ENTRY_STATE_LOC_EXREG) {
      exreg_group_id_t g = se.base.reg_identifier_get_id();
      reg_identifier_t const &r = se.index;
      ASSERT(se.disp.tag == tag_const);
      int locnum = se.disp.val;
      ASSERT(locnum >= 0 && locnum < in_tmap->extable.at(g).at(r).get_locs().size());
      ptr += i386_insn_compute_transmap_entry_loc_value(R_EAX, in_tmap->extable.at(g).at(r), R_ESP, orig_stack_offset, locnum, dst_fixed_reg_mapping, ptr, end - ptr);
      ptr += i386_insn_put_push_reg(R_EAX, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
      orig_stack_offset += 4;
      format_string_stack_offset += 4;
    } else if (se.loc == TS_TAB_ENTRY_STATE_LOC_MEM) {
      ASSERT(se.disp.tag == tag_const);
      long d = se.disp.val;
      ptr += i386_insn_put_movl_imm_to_reg(0, R_EAX, ptr, end - ptr);
      if (se.base.reg_identifier_is_valid()) {
        ptr += i386_insn_compute_transmap_entry_loc_value(R_EDX, in_tmap->extable.at(ETFG_EXREG_GROUP_GPRS).at(se.base), R_ESP, orig_stack_offset, -1, dst_fixed_reg_mapping, ptr, end - ptr);
        ptr += i386_insn_put_add_reg_to_reg(R_EAX, R_EDX, ptr, end - ptr);
      }
      if (se.index.reg_identifier_is_valid()) {
        ptr += i386_insn_compute_transmap_entry_loc_value(R_EDX, in_tmap->extable.at(ETFG_EXREG_GROUP_GPRS).at(se.index), R_ESP, orig_stack_offset, -1, dst_fixed_reg_mapping, ptr, end - ptr);
        ptr += i386_insn_put_add_reg_to_reg(R_EAX, R_EDX, ptr, end - ptr);
      }
      if (se.len == 1) {
        ptr += i386_insn_put_movb_mem_to_reg(R_EAX, 0, R_EAX, ptr, end - ptr);
      } else if (se.len == 2) {
        ptr += i386_insn_put_movw_mem_to_reg(R_EAX, 0, R_EAX, ptr, end - ptr);
      } else if (se.len == 4) {
        ptr += i386_insn_put_movl_mem_to_reg(R_EAX, 0, R_EAX, ptr, end - ptr);
      } else {
        cout << __func__ << " " << __LINE__ << ": unsupported len in mem-access: " << se.len << endl;
        NOT_REACHED();
      }
      ptr += i386_insn_put_push_reg(R_EAX, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
      orig_stack_offset += 4;
      format_string_stack_offset += 4;
    } else {
      NOT_REACHED();
    }
  }
  //ptr += i386_insn_put_push_imm(pc, tag_const, ptr, end - ptr);
  //orig_stack_offset += 4;
  //format_string_stack_offset += 4;

  ptr += i386_insn_put_movl_reg_to_reg(R_EAX, R_ESP, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(orig_stack_offset, R_EAX, tag_const, ptr, end - ptr);
  ptr += i386_insn_put_push_reg(R_EAX, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  orig_stack_offset += 4;
  format_string_stack_offset += 4;

  ptr += i386_insn_put_movl_reg_to_reg(R_EAX, R_ESP, ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(format_string_stack_offset, R_EAX, tag_const, ptr, end - ptr);
  ASSERT(ptr < end);
  ptr += i386_insn_put_push_reg(R_EAX, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  orig_stack_offset += 4;
  format_string_stack_offset += 4;

  ptr += i386_insn_put_call_helper_function(ti, "printf", ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  ptr += i386_insn_put_add_imm_with_reg(orig_stack_offset - PUSHA_SIZE, R_ESP, tag_const, ptr, end - ptr);

  ptr += i386_insn_put_push_imm(0, tag_const, ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  ptr += i386_insn_put_call_helper_function(ti, "print_icount_and_fflush", ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr); //fflush(NULL) flushes all file descriptors including stdout
  ptr += i386_insn_put_add_imm_with_reg(4, R_ESP, tag_const, ptr, end - ptr);

  //ptr += i386_insn_put_popf(ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr);
  ptr += i386_insn_put_popa(ti->ti_get_dst_fixed_reg_mapping(), ptr, end - ptr, I386_AS_MODE_32);
  //ptr += i386_insn_put_add_imm_with_reg(pc, R_EAX, tag_const, ptr, end - ptr);
  //ptr += i386_insn_put_add_imm_with_reg(-pc, R_EAX, tag_const, ptr, end - ptr);
#else
  NOT_IMPLEMENTED();
#endif
  return ptr - buf;
}
#endif

static size_t
si_gen_translated_insns(translation_instance_t *ti, si_entry_t const *si,
    struct chash const *tcodes,
    //map<src_ulong, dst_insn_t const  *> &tinsns_map,
    dst_insn_t *dst_iseq, size_t max_dst_iseq_len,
    long iter)
{
  dst_insn_t *dst_iseq_ptr, *dst_iseq_end;
  long dst_iseq_len, len;
  input_exec_t const *in;
  eqspace::state const *start_state = NULL;

  in = ti->in;

#if ARCH_SRC == ARCH_ETFG
  //start_state = &etfg_pc_get_input_state(in, si->pc);
  start_state = ti->ti_get_input_state_at_etfg_pc(si->pc); //&etfg_pc_get_input_state(in, si->pc);
#endif

  dst_iseq_ptr = dst_iseq;
  dst_iseq_end = dst_iseq + max_dst_iseq_len;

#if ARCH_SRC == ARCH_ETFG
  string function_name;
  if (in->pc_is_etfg_function_entry(si->pc, function_name)) {
    if (dyn_debug && function_name == "main") {
      dst_iseq_ptr += etfg_dyn_debug_print_globals_argc_argv_at_start(ti, dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr);
    }
    dst_iseq_ptr += alloc_stack_insns(dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr,
        ti_get_stack_alloc_space_for_function(ti, function_name),
        I386_AS_MODE_32);
  }
  if (   in->pc_is_etfg_function_return(si->pc, function_name)
      || ti->pc_is_tail_call(si->pc, function_name)) {
    dst_iseq_ptr += dealloc_stack_insns(dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr,
        ti_get_stack_alloc_space_for_function(ti, function_name),
        I386_AS_MODE_32);
  }
#endif

  if (dyn_debug) {
    transmap_set_elem_t *d;
    d = ti->transmap_decision[si->index];
    if (d) {
#if ARCH_SRC == ARCH_ETFG
      //cout << __func__ << " " << __LINE__ << ": d->in_tmap at " << hex << si->pc << dec << " is:\n" << transmap_to_string(d->in_tmap, as1, sizeof as1) << endl;
      dst_iseq_ptr += etfg_dyn_debug_gen_insns_printing_cur_state(ti, si->pc,
          d->row->fetchlen,
          d->in_tmap, si->bbl->live_regs[si->bblindex],
          dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr);
#else
      dst_iseq_ptr += transmap_translation_insns(d->in_tmap, transmap_no_regs(), start_state,
          dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr,
          ti->get_base_reg(), ti->ti_get_dst_fixed_reg_mapping(), I386_AS_MODE_32);
      dst_iseq_ptr += debug_call_print_machine_state(ti, dst_iseq_ptr,
          dst_iseq_end - dst_iseq_ptr, si->pc, &si->bbl->live_regs[si->bblindex]);
      dst_iseq_ptr += transmap_translation_insns(transmap_no_regs(), d->in_tmap, start_state,
          dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr,
          ti->get_base_reg(), ti->ti_get_dst_fixed_reg_mapping(), I386_AS_MODE_32);
#endif
    }
  }
  ASSERT(dst_iseq_ptr < dst_iseq_end);

  auto d = ti->transmap_decision[si->index];
  transmap_t *out_tmaps = NULL;
  long fetchlen = 1;
  if (d) {
    out_tmaps = new transmap_t[d->row->num_nextpcs];
    transmaps_copy(out_tmaps, d->out_tmaps, d->row->num_nextpcs);
    fetchlen = d->row->fetchlen;
  }

  if (ti->mode == FBGEN) {
    for (long i = 0; i < fetchlen; i++) {
      dst_iseq_ptr += dst_emit_translation_marker_insns(dst_iseq_ptr, dst_iseq_end - dst_iseq_ptr);
    }
  }

  dst_iseq_ptr += si_gen_dst_iseq(ti, si, dst_iseq_ptr,
      dst_iseq_end - dst_iseq_ptr, out_tmaps, NULL, NULL, NULL, NULL, NULL, NULL);
  ASSERT(dst_iseq_ptr < dst_iseq_end);
  DBG(CONNECT_ENDPOINTS, "si->pc = %lx: dst_iseq:\n%s\n", (long)si->pc,
      dst_iseq_to_string(dst_iseq, dst_iseq_ptr - dst_iseq, as1, sizeof as1));
  if (d) {
    delete [] out_tmaps;
  }

  dst_iseq_len = dst_iseq_ptr - dst_iseq;
  if (si->eliminate_branch_to_fallthrough_pc) {
    dst_iseq_len = eliminate_branch_to_fallthrough_pc(dst_iseq, dst_iseq_len);
  }
  if (iter && pc_belongs_to_start_function(in, si->pc)) {
    ASSERT(tcodes);
    dst_iseq_rename_immediate_operands_containing_code_addresses(dst_iseq, dst_iseq_len, tcodes); //for correct dynamic linking
  }
  DBG(CONNECT_ENDPOINTS, "si->pc = %lx: dst_iseq:\n%s\n", (long)si->pc,
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  dst_iseq_rename_symbols_to_addresses(dst_iseq, dst_iseq_len, in, tcodes);
  DBG(CONNECT_ENDPOINTS, "dst_iseq:\n%s\n",
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));

#if ARCH_SRC == ARCH_ETFG
  //vector<pair<string, size_t>> const &locals_map = etfg_input_exec_get_locals_map_for_function(in, function_name);
  graph_locals_map_t const &locals_map = etfg_input_exec_get_locals_map_for_function(in, function_name);
  dst_iseq_rename_locals_using_locals_map(dst_iseq, dst_iseq_len, locals_map, ti_get_stack_alloc_space_for_function(ti, function_name));
  DBG(CONNECT_ENDPOINTS, "dst_iseq:\n%s\n",
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  string function_name_unused;
  if (ti->pc_is_tail_call(si->pc, function_name_unused)) {
    dst_insn_convert_function_call_to_branch(&dst_iseq[dst_iseq_len - 1]);
  }
  if (dyn_debug) {
    ti->dst_iseq_convert_malloc_to_mymalloc(dst_iseq, dst_iseq_len);
  }
#endif
  return dst_iseq_len;
}

static void
dst_iseq_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(dst_insn_t *dst_iseq, long dst_iseq_len, struct chash const *tcodes)
{
  for (long i = 0; i < dst_iseq_len; i++) {
    dst_insn_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(&dst_iseq[i], tcodes);
  }
}

static size_t
si_gen_translated_code(translation_instance_t *ti, si_entry_t const *si,
    struct chash const *tcodes,
    //map<src_ulong, dst_insn_t const *> &tinsns_map,
    uint8_t *buf, size_t size, long iter, long call_iter)
{
  size_t max_dst_iseq_len = dyn_debug ? 40960 : 4096;
  dst_insn_t *dst_iseq;
  uint8_t *ptr, *end;
  size_t len;

  dst_iseq = new dst_insn_t[max_dst_iseq_len];
  ASSERT(dst_iseq);

  ptr = buf;
  end = buf + size;

  long dst_iseq_len = si_gen_translated_insns(ti, si, tcodes/*, tinsns_map*/, dst_iseq, max_dst_iseq_len, iter);
  dst_iseq_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(dst_iseq, dst_iseq_len, tcodes);

  if (iter && call_iter) {
    fprintf_translation(logfile, "no folding opts", si, ptr, dst_iseq, dst_iseq_len, ti->reloc_strings, ti->reloc_strings_size);
  }

  len = dst_iseq_assemble(ti, dst_iseq, dst_iseq_len, ptr, end - ptr,
      I386_AS_MODE_32);
  DBG(CONNECT_ENDPOINTS, "dst_iseq at %p:\n%s\n%s\n", ptr,
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1),
      hex_string(ptr, len, as2, sizeof as2));
  ptr += len;
  //free(dst_iseq);
  delete [] dst_iseq;
  ASSERT(ptr < end);
  return ptr - buf;
}

static size_t
si_gen_header_code(translation_instance_t const *ti, si_entry_t const *si, uint8_t *buf,
    size_t size, long iter)
{
  input_exec_t const *in;
  uint8_t *ptr, *end;
  long bbl_id;

  in = ti->in;
  ptr = buf;
  end = buf + size;

  if (!ti->bbl_headers) {
    return 0;
  }
  if (si->bblindex != 0) {
    return 0;
  }
  bbl_id = pc2bbl(&in->bbl_map, si->pc);
  ASSERT(bbl_id >= 0 && bbl_id < in->num_bbls);
  if (!ti->bbl_headers[bbl_id]) {
    return 0;
  }
  DBG2(UNROLL_LOOPS, "generating header code for %lx\n", (long)in->bbls[bbl_id].pc);
  ASSERT(ti->bbl_header_sizes[bbl_id] <= size);
  memcpy(ptr, ti->bbl_headers[bbl_id], ti->bbl_header_sizes[bbl_id]);
  ptr += ti->bbl_header_sizes[bbl_id];
  ASSERT(ptr < end);
  return ti->bbl_header_sizes[bbl_id];
}

static size_t
setup_code(translation_instance_t *ti, uint8_t *buf, size_t size)
{
  uint8_t *ptr = buf, *end = buf + size;

  DBG(SETUP_CODE, "%s", "entry\n");
#if ARCH_DST == ARCH_I386
  int max_alloc = 256;
  i386_insn_t *i386_insns = new i386_insn_t[max_alloc];
  i386_insn_t *i386_insn_ptr, *i386_insn_end;
  input_exec_t const *in;
  int n;

  in = ti->in;
  i386_insn_ptr = i386_insns;
  i386_insn_end = i386_insns + max_alloc;

#if ARCH_SRC != ARCH_ETFG
  i386_insn_ptr += transmap_translation_insns(transmap_default(),
      transmap_no_regs(), NULL, i386_insn_ptr,
      i386_insn_end - i386_insn_ptr, ti->get_base_reg(),
      ti->ti_get_dst_fixed_reg_mapping(), I386_AS_MODE_32);

  i386_insn_ptr += i386_insn_put_movl_reg_to_mem(
      SRC_ENV_ADDR + SRC_STACK_REGNUM * 4, -1, R_ESP, i386_insn_ptr,
      i386_insn_end - i386_insn_ptr);
  i386_insn_ptr += i386_insn_put_movl_imm_to_reg(SRC_TMP_STACK, R_ESP,
      i386_insn_ptr, i386_insn_end - i386_insn_ptr);
#endif

#if SRC_ENDIANNESS != DST_ENDIANNESS
  i386_insn_ptr += i386_insn_put_push_imm(SRC_ENV_ADDR, tag_const, ti->ti_get_dst_fixed_reg_mapping(), i386_insn_ptr,
      i386_insn_end - i386_insn_ptr);
  i386_insn_ptr += i386_insn_put_call_helper_function(ti,
      "__dbg_bswap_stack_and_phdrs_on_startup", i386_insn_ptr,
      i386_insn_end - i386_insn_ptr);
#endif

#if ARCH_SRC != ARCH_ETFG
  if (dyn_debug) {
    char const *fptr = in->filename + strlen(in->filename);

    i386_insn_ptr += i386_insn_put_movl_imm_to_reg(SRC_TMP_STACK, R_ESP, i386_insn_ptr,
        i386_insn_end - i386_insn_ptr);
    while (fptr >= in->filename) {
      i386_insn_ptr += i386_insn_put_pushb_imm(*fptr, ti->ti_get_dst_fixed_reg_mapping(), i386_insn_ptr,
          i386_insn_end - i386_insn_ptr);
      fptr--;
    }
    i386_insn_ptr += i386_insn_put_push_reg(R_ESP, ti->ti_get_dst_fixed_reg_mapping(), i386_insn_ptr,
        i386_insn_end - i386_insn_ptr);
    i386_insn_ptr += i386_insn_put_push_imm(SRC_ENV_ADDR, tag_const, ti->ti_get_dst_fixed_reg_mapping(), i386_insn_ptr,
        i386_insn_end - i386_insn_ptr);

    i386_insn_ptr += i386_insn_put_call_helper_function(ti,
        "__dbg_init_dyn_debug", ti->ti_get_dst_fixed_reg_mapping(), i386_insn_ptr, i386_insn_end - i386_insn_ptr);
  }
#endif

#if ARCH_SRC != ARCH_ETFG
  /* restore the stack pointer */
  i386_insn_ptr += i386_insn_put_movl_imm_to_reg(SRC_TMP_STACK, R_ESP, i386_insn_ptr,
      i386_insn_end - i386_insn_ptr);

  long entry_index;
  entry_index = pc2index(in, in->entry);
  ASSERT(entry_index >= 0 && entry_index < in->num_si);
  if (!ti->transmap_decision[entry_index]) {
    printf("%s() %d: in->entry = %lx\n", __func__, __LINE__, (long)in->entry);
  }
  ASSERT(ti->transmap_decision[entry_index]);

  i386_insn_ptr += transmap_translation_insns(transmap_no_regs(),
      ti->transmap_decision[entry_index]->in_tmap, NULL,
      i386_insn_ptr,
      i386_insn_end - i386_insn_ptr, ti->get_base_reg(),
      ti->ti_get_dst_fixed_reg_mapping(),
      I386_AS_MODE_32);
#endif
  ASSERT(i386_insn_ptr < i386_insn_end);
  size_t sz = i386_iseq_assemble(ti, i386_insns, i386_insn_ptr - i386_insns, ptr,
      end - ptr, I386_AS_MODE_32);
  delete [] i386_insns;
  //ASSERT(sz > 0);
  ptr += sz;
  ASSERT(ptr <= end);
#else
  NOT_IMPLEMENTED();
#endif
  DBG(SETUP_CODE, "exit: returning %zd\n", ptr - buf);
  return ptr - buf;
}

/*
static void
tcodes_add_plt_mappings(struct hash *tcodes, input_exec_t const *in)
{
  long plt_addr, plt_entry_size, plt_size, i;

  input_exec_get_plt_section_attrs(in, &plt_addr, &plt_entry_size, &plt_size);

  for (i = plt_entry_size; i < plt_size; i += plt_entry_size) {
    tcodes_add_mapping(tcodes, plt_addr + i, (uint8_t *)plt_addr + i);
  }
}
*/

static void
dst_iseq_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map(dst_insn_t *dst_iseq, long dst_iseq_len, map<src_ulong, dst_insn_t const *> const &tinsns_map)
{
  for (long i = 0; i < dst_iseq_len; i++) {
    dst_insn_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map(&dst_iseq[i], tinsns_map);
  }
}

//static size_t
//ti_get_stack_size_for_translation_unit(translation_instance_t *ti, translation_unit_t const &tunit)
//{
//  return ti_get_stack_alloc_space_for_function(ti, tunit.get_name());
//}

bool
translation_instance_t::ti_need_to_perform_dst_insn_folding_optimization_on_function(string const& fname) const
{
  if (vector_find(m_disable_dst_insn_folding_functions, string("ALL")) != -1) {
    return false;
  }
  if (vector_find(m_disable_dst_insn_folding_functions, fname) != -1) {
    return false;
  }
  return true;
}

static size_t
gen_translated_code(translation_instance_t *ti, translation_unit_t const &tunit, int call_iter, uint8_t *buf, size_t size)
{
  autostop_timer func_timer(__func__);
  long iter, i, tx_entry;
  input_exec_t const *in;
  uint8_t *ptr, *end;

  end = buf + size;
  in = ti->in;

  size_t start_si = in->get_translation_unit_start_si(tunit);
  size_t stop_si = in->get_translation_unit_stop_si(tunit);
  dst_insn_t *insn_buf = new dst_insn_t[MAX_NUM_DST_INSNS_PER_TRANSLATION_UNIT];
  ASSERT(insn_buf);
  dst_insn_t *insn_ptr, *insn_end = insn_buf + MAX_NUM_DST_INSNS_PER_TRANSLATION_UNIT;
  map<src_ulong, dst_insn_t const *> tinsns_map;
  map<long, flow_t> dst_insn_computed_flow;

  string function_name = lookup_symbol(ti->in, in->si[start_si].pc);
  bool perform_dst_insn_folding = ti->ti_need_to_perform_dst_insn_folding_optimization_on_function(function_name);

  for (iter = 0; iter < (perform_dst_insn_folding ? 1 : 2); iter++) {
    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": tunit = " << tunit.get_name() << ", iter = " << iter << ", start_si = " << start_si << ", stop_si = " << stop_si << endl);
    ptr = buf;
    insn_ptr = insn_buf;

    for (i = start_si; i < stop_si; i++) {
      //size_t header_size = si_gen_header_code(ti, &in->si[i], ptr, end - ptr, iter);
      if (!perform_dst_insn_folding) {
        CPP_DBG_EXEC(CONNECT_ENDPOINTS, cout << __func__ << " " << __LINE__ << ": adding mapping from " << hex << in->si[i].pc << " to " << (long)ptr << dec << endl);
        tcodes_add_mapping(&ti->tcodes, in->si[i].pc, ptr);
        in->si[i].tc_ptr = ptr;
        ptr += si_gen_translated_code(ti, &in->si[i], &ti->tcodes/*, tinsns_map*/, ptr, end - ptr, iter, call_iter);
      } else {
        in->si[i].tc_ptr = NULL;
        tinsns_map[in->si[i].pc] = insn_ptr;
        size_t num_insns;
        dst_insn_computed_flow[insn_ptr - insn_buf] = in->si[i].bbl->in_flow;
        num_insns = si_gen_translated_insns(ti, &in->si[i], &ti->tcodes/*, tinsns_map*/, insn_ptr, insn_end - insn_ptr, iter);
        if (call_iter) {
          fprintf_translation(logfile, "peep translation", &in->si[i], buf, insn_ptr, num_insns, ti->reloc_strings, ti->reloc_strings_size);
        }
        DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": " << hex << in->si[i].pc << dec << ":\n" << i386_iseq_to_string(insn_ptr, num_insns, as1, sizeof as1) << endl);
        //cout << __func__ << " " << __LINE__ << ": pc = " << hex << in->si[i].pc << dec << endl;
        //ASSERT(dst_iseq_is_well_formed(insn_ptr, num_insns));
        insn_ptr += num_insns;
      }
      //printf("%lx: ptr-buf=%zd\n", (long)in->si[i].pc, ptr-buf);
    }
    //printf("ptr-buf=%zd\n", ptr-buf);
  }
  if (perform_dst_insn_folding) {
    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": " << hex << in->si[start_si].pc << dec << ":\n" << i386_iseq_to_string(insn_buf, insn_ptr - insn_buf, as1, sizeof as1) << endl);
    dst_iseq_rename_tag_src_insn_pcs_to_dst_pcrels_using_tinsns_map(insn_buf, insn_ptr - insn_buf, tinsns_map);
    dst_iseq_rename_tag_src_insn_pcs_to_tag_var_using_tcodes(insn_buf, insn_ptr - insn_buf, &ti->tcodes);
    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": " << hex << in->si[start_si].pc << dec << ":\n" << i386_iseq_to_string(insn_buf, insn_ptr - insn_buf, as1, sizeof as1) << endl);

    if (call_iter) {
      fprintf_translation(logfile, "before dst_insn_folding_optimizations", &in->si[start_si], buf, insn_buf, insn_ptr - insn_buf, ti->reloc_strings, ti->reloc_strings_size);
    }
    //ASSERT(dst_iseq_is_well_formed(insn_buf, insn_ptr - insn_buf));
    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": " << hex << in->si[start_si].pc << ": calling dst_insns_folding_optimizations\n");
    size_t dst_iseq_len = dst_insns_folding_optimizations(insn_buf, insn_ptr - insn_buf, insn_end - insn_buf, dst_insn_computed_flow/*, stack_size*/);
    //size_t dst_iseq_len = insn_ptr - insn_buf;
    insn_ptr = insn_buf + dst_iseq_len;

    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": adding mapping from " << hex << in->si[start_si].pc << " to " << (long)buf << dec << endl);
    tcodes_add_mapping(&ti->tcodes, in->si[start_si].pc, buf); ///tcodes needs to add mappings only for symbols; in this case, the only interesting symbols are at the start of translation units
    in->si[start_si].tc_ptr = buf;  //similarly, set tc_ptr to non-NULL values for only the start of translation units
    if (call_iter) {
      fprintf_translation(logfile, "after dst_insn_folding_optimizations", &in->si[start_si], buf, insn_buf, insn_ptr - insn_buf, ti->reloc_strings, ti->reloc_strings_size);
    }
    ptr = buf;
    size_t codesize = dst_iseq_assemble(ti, insn_buf, insn_ptr - insn_buf, buf, size, I386_AS_MODE_32);
    if (!(insn_ptr - insn_buf == 0 || codesize > 0)) {
      cout << __func__ << " " << __LINE__ << ": insn_buf =\n" << dst_iseq_to_string(insn_buf, insn_ptr - insn_buf, as1, sizeof as1) << endl;
    }
    if (call_iter) {
      ASSERT(insn_ptr - insn_buf == 0 || codesize > 0);
      fprintf(logfile, "%lx[%ld]: binary code: %s\n", (long)in->si[start_si].pc, (long)in->si[start_si].pc, hex_string(ptr, codesize, as1, sizeof as1));
    }
    ptr += codesize;
    DYN_DEBUG(stat_trans, cout << __func__ << " " << __LINE__ << ": after assembling: " << hex_string(buf, ptr - buf, as1, sizeof as1) << endl);
  }
  return ptr - buf;
}

static void
write_jumptable_stub_code(translation_instance_t *ti, uint8_t const *tc_start)
{
  autostop_timer func_timer(__func__);
  transmap_t const *in_tmap;
  struct imap_elem_t *e;
  input_exec_t *in;
  si_entry_t *si;
  long i;

  in = ti->in;
  ti->jumptable_stub_code_buf = (uint8_t *)malloc(
      MAX_JUMPTABLE_STUB_CODESIZE * sizeof(uint8_t));
  ASSERT(ti->jumptable_stub_code_buf);
  ti->jumptable_stub_code_offsets = (int *)malloc(
      (MAX_JUMPTABLE_STUB_CODESIZE >> 2) * sizeof(int));
  ASSERT(ti->jumptable_stub_code_offsets);
  ti->jumptable_stub_code_ptr = ti->jumptable_stub_code_buf;
  ti->num_jumptable_stub_code_offsets = 0;
  uint8_t *jumptable_stub_code_end = ti->jumptable_stub_code_buf
    + MAX_JUMPTABLE_STUB_CODESIZE;

  myhash_init(&ti->indir_eip_map, imap_hash_func, imap_equal_func, NULL);
  for (i = 0; i < (long)in->num_si; i++) {
    si = &in->si[i];
    if (!jumptable_belongs(in->jumptable, si->pc)) {
      continue;
    }
#if ARCH_SRC == ARCH_ETFG
    if (!si->tc_ptr) { //hack: need a more principled solution
      continue;
    }
#endif
    //cout << __func__ << " " << __LINE__ << ": jumptable belongs returned true for " << hex << si->pc << dec << endl;
    //ASSERT(si->bblindex == 0);
    ASSERT(si->tc_ptr);
    e = (struct imap_elem_t *)malloc(sizeof *e);
    e->pc = si->pc;
    e->val = (unsigned long)ti->jumptable_stub_code_ptr;
    myhash_insert(&ti->indir_eip_map, &e->h_elem);
    if (ti->transmap_decision[si->index]) {
      in_tmap = ti->transmap_decision[si->index]->in_tmap;
    } else {
      in_tmap = transmap_default();
    }
    ti->jumptable_stub_code_ptr += transmap_translation_code(transmap_default(),
        in_tmap,// &si->bbl->live_regs[si->bblindex], false,
        ti->get_base_reg(),
        ti->ti_get_dst_fixed_reg_mapping(),
        I386_AS_MODE_32,
        ti->jumptable_stub_code_ptr,
        jumptable_stub_code_end - ti->jumptable_stub_code_ptr);
    *ti->jumptable_stub_code_ptr++ = 0xe9;
    *(dst_ulong *)ti->jumptable_stub_code_ptr =
        (dst_ulong)(si->tc_ptr - tc_start);
    ti->jumptable_stub_code_offsets[ti->num_jumptable_stub_code_offsets++]
      = ti->jumptable_stub_code_ptr - ti->jumptable_stub_code_buf;
    ti->jumptable_stub_code_ptr += 4;
    ASSERT(ti->jumptable_stub_code_ptr < jumptable_stub_code_end);
  }
}

static size_t
transmap_get_max_stack_offset(transmap_t const *tmap)
{
  size_t ret = 0;
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      transmap_entry_t const &te = r.second;
      /*for (size_t i = 0; i < te.num_locs; i++) {
        if (te.loc[i] == TMAP_LOC_SYMBOL) {
          ret = max(ret, (size_t)te.regnum[i] + (SRC_EXREG_LEN(SRC_EXREG_GROUP_GPRS)/BYTE_LEN));
        }
      }*/
      for (const auto &i : te.get_locs()) {
        if (i.get_loc() == TMAP_LOC_SYMBOL) {
          ret = max(ret, (size_t)i.get_regnum() + (SRC_EXREG_LEN(SRC_EXREG_GROUP_GPRS)/BYTE_LEN));
        }
      }
    }
  }
  return ret;
}

/*static size_t
transmap_set_elem_get_max_stack_offset(transmap_set_elem_t const *tmap_set_elem)
{
  size_t ret = transmap_get_max_stack_offset(tmap_set_elem->in_tmap);
  for (size_t i = 0; i < tmap_set_elem->row->num_nextpcs; i++) {
    size_t off = transmap_get_max_stack_offset(&tmap_set_elem->out_tmaps[i]);
    ret = max(ret, off);
  }
  return ret;
}*/

/*static size_t
transmap_set_elem_update_stack_slot_map(transmap_set_elem_t const *tmap_set_elem, alloc_map_t &ssm)
{
  size_t ret = transmap_update_stack_slot_map(tmap_set_elem->in_tmap, ssm);
  for (size_t i = 0; i < tmap_set_elem->row->num_nextpcs; i++) {
    NOT_IMPLEMENTED(); //need to 
  }
  return ret;
}*/

static void
ti_identify_pcs_with_fixed_transmaps(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  //this function ensures that the required src-elems are reg-allocated (remaining live src-elems are mem-allocated later)
#if ARCH_SRC == ARCH_ETFG
  input_exec_t const *in = ti->in;
  for (size_t i = 0; i < in->num_si; i++) {
    src_ulong pc = in->si[i].pc;
    string function_name;
    if (   in->pc_is_etfg_function_entry(pc, function_name)
        && function_name != "main") {
      vector<transmap_t> tmaps;
      transmap_t tmap_callee_save_regs = etfg_callee_save_regs_transmap();
      tmaps.push_back(tmap_callee_save_regs);
      ti->fixed_transmaps[pc] = tmaps;
    } else if (   (   in->pc_is_etfg_function_return(pc, function_name)
                   && function_name != "main")
               || ti->pc_is_tail_call(pc, function_name)) {
      vector<transmap_t> tmaps;
      src_insn_t src_insn;
      //bool fetched = src_insn_fetch(&src_insn, in, pc, NULL);
      long len;
      bool fetched = src_iseq_fetch(&src_insn, 1, &len, in, NULL, pc, 1,
          NULL, NULL, NULL, NULL, NULL, NULL, false);
      ASSERT(fetched);
      ASSERT(len == 1);
      transmap_t tmap_callee_save_retval_regs = etfg_callee_save_retval_regs_transmap(src_insn);
      //transmap_t tmap_callee_save_regs_retval_mem = etfg_callee_save_regs_retval_mem_tmap();
      tmaps.push_back(tmap_callee_save_retval_regs);
      //tmaps.push_back(tmap_callee_save_regs_retval_mem);
      ti->fixed_transmaps[pc] = tmaps;
    }
  }
#endif
}

void
translation_instance_t::ti_identify_tail_calls()
{
  autostop_timer func_timer(__func__);
  input_exec_t const *in = this->in;
  long max_alloc = (in->num_si + 100) * 2;
  src_insn_t *src_iseq = new src_insn_t[max_alloc];
  ASSERT(src_iseq);
  long src_iseq_len;
  for (size_t i = 0; i < in->num_si - 1; i++) {
    bool fetched;
    fetched = src_iseq_fetch(src_iseq, max_alloc, &src_iseq_len, in, NULL, in->si[i].pc, 3, NULL, NULL, NULL, NULL, NULL, NULL, false);
    CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": fetched = " << fetched << endl);
    if (!fetched) {
      continue;
    }
    ASSERT(src_iseq_len <= max_alloc);
    CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": src_iseq_len = " << src_iseq_len << endl);
    if (src_iseq_len != 3) {
      continue;
    }
    CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": src_iseq = " << src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1) << endl);
    if (!src_iseq_is_function_tailcall_opt_candidate(src_iseq, 3)) {
      CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": check failed\n");
      continue;
    }
    vector<string> argnames;
    if (!in->pc_is_etfg_function_callsite(in->si[i].pc, argnames)) {
      CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": check failed\n");
      continue;
    }
    string caller_name = in->pc_get_function_name(in->si[i].pc);
    CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": caller_name = " << caller_name << endl);
    size_t num_caller_args = this->function_arg_names.at(caller_name).size();
    CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": pc " << hex << in->si[i].pc << dec << ": caller_name = " << caller_name << ", num_caller_args = " << num_caller_args << ", argnames.size() = " << argnames.size() << endl);
    if (argnames.size() <= num_caller_args) {
      this->m_tail_calls_set.tail_calls_set_add(in->si[i].pc, caller_name);
      CPP_DBG_EXEC(TAIL_CALL, cout << __func__ << " " << __LINE__ << ": found tail call at pc " << hex << in->si[i].pc << dec << endl);
    }
  }
  delete [] src_iseq;
}

bool
translation_instance_t::pc_is_tail_call(src_ulong pc, string &caller_name) const
{
  return m_tail_calls_set.tail_calls_set_belongs(pc, caller_name);
  //if (pc == 0x10001a) {
  //  caller_name = "DeleteTree";
  //  return true;
  //}
  //return false;
}

void
static_translate_for_ti(translation_instance_t *ti)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << endl;
  size_t codesize;

  init_input_file(ti->in, NULL);
  PROGRESS("%s", "Identifying args for each function");
#if ARCH_SRC == ARCH_ETFG
  //autostop_timer func2_timer(string(__func__) + ".2");
  etfg_identify_args_for_each_function(ti);
  //autostop_timer func3_timer(string(__func__) + ".3");
  ti->ti_identify_tail_calls();
  //autostop_timer func4_timer(string(__func__) + ".4");
#endif
  ti_identify_pcs_with_fixed_transmaps(ti);
  //autostop_timer func5_timer(string(__func__) + ".5");

  PROGRESS("%s", "Enumerating transmaps");
  enumerate_transmaps(ti);
  //autostop_timer func6_timer(string(__func__) + ".6");

  PROGRESS("%s", "Printing enumerated transmaps to log");
  si_print(logfile, ti->in);
  preds_print(logfile, ti);
  //autostop_timer func7_timer(string(__func__) + ".7");
  PROGRESS("%s", "Choosing solution");
//#ifdef DEBUG_TMAP_CHOICE_LOGIC
//  enumerated_transmaps_print(logfile, ti);
//#endif
  choose_solution(ti);
  //autostop_timer func8_timer(string(__func__) + ".8");
#ifdef DEBUG_TMAP_CHOICE_LOGIC
  enumerated_transmaps_print(logfile, ti);
#endif
  ti_identify_regname_map(ti);
  //autostop_timer func9_timer(string(__func__) + ".9");

  PROGRESS("%s", "Printing transmap decisions");
  transmap_decisions_print(logfile, ti);

  PROGRESS("%s", "Identifying branches to fallthrough pc");
  identify_branches_to_fallthrough_pc(ti);
  //autostop_timer func10_timer(string(__func__) + ".10");

  /*if (ti->mode == UNROLL_LOOPS) {
    unroll_loops(ti);
  }*/
  PROGRESS("%s", "Generating code");

  ti->code_gen_buffer = alloc_code_gen_buffer(CODE_GEN_BUFFER_SIZE);
  //autostop_timer func11_timer(string(__func__) + ".11");
  uint8_t *code_gen_ptr;
  uint8_t *code_gen_end = ti->code_gen_buffer + CODE_GEN_BUFFER_SIZE;
  input_exec_t const *in = ti->in;
  map<translation_unit_t, uint8_t *> tunit_start_ptrs;

  fprintf(logfile, "\n\nTranslation\n\n");
  for (int iter = 0; iter < 2; iter++) {
    code_gen_ptr = ti->code_gen_buffer;
#if ARCH_SRC != ARCH_DST
    code_gen_ptr += setup_code(ti, code_gen_ptr, code_gen_end - code_gen_ptr);
    ASSERT(code_gen_ptr < code_gen_end);
#endif
#if ARCH_SRC != ARCH_ETFG
    if (in->fresh) {
      code_gen_ptr += i386_put_unconditional_branch_code_to_entry(code_gen_ptr, code_gen_end - code_gen_ptr,
          in->entry, &ti->tcodes);
    }
#endif
    for (const auto &tunit : in->get_translation_units()) {
      if (iter == 0) {
        tunit_start_ptrs.insert(make_pair(tunit, code_gen_ptr));
      } else {
        ASSERT(iter == 1);
        ASSERT(tunit_start_ptrs.count(tunit));
        ASSERT(code_gen_ptr == tunit_start_ptrs.at(tunit));
      }
      //autostop_timer func12_timer(string(__func__) + ".12");
      code_gen_ptr += gen_translated_code(ti, tunit, iter, code_gen_ptr, code_gen_end - code_gen_ptr);
      //autostop_timer func13_timer(string(__func__) + ".13");
    }
    //ASSERT(iter == 0 || ti->codesize == code_gen_ptr - ti->code_gen_buffer);
    ti->codesize = code_gen_ptr - ti->code_gen_buffer;
  }
  //XXX: free ti->transmap_set_elem_pruned_out_list

  if (ti->in->fresh) {
    PROGRESS("%s", "Writing jumptable stub code");
    write_jumptable_stub_code(ti, ti->code_gen_buffer);
  }
}

void
static_translate(translation_instance_t *ti, char const *filename,
    char const *output_file)
{
  //ti->in = (input_exec_t *)malloc(sizeof(input_exec_t));
  ti->in = new input_exec_t;
  ti->out = (output_exec_t *)malloc(sizeof(output_exec_t));
  ASSERT(ti->in);      ASSERT(ti->out);

  output_exec_init(ti->out, output_file);

  read_input_file(ti->in, filename);

  /*if (!preserve_instruction_order) {
    PROGRESS("%s", "Performing usedef-sort");
    usedef_sort(ti->in);
  }*/
  static_translate_for_ti(ti);
  DYN_DBG(stat_trans, "%s", "calling write_target_exec()\n");
  PROGRESS("%s", "Writing the executable");
  printf("\n");
  write_target_exec(ti, ti->code_gen_buffer, ti->codesize, &ti->tcodes);
  PROGRESS("%s", "Done.                                                         \n");
}

char const *
input_section_get_ptr_from_addr(input_section_t const *sec, src_ulong addr)
{
  long off;
  ASSERT(addr >= sec->addr && addr < sec->addr + sec->size);
  off = addr - sec->addr;
  return &sec->data[off];
}

struct input_section_t const *
input_exec_get_rodata_section(input_exec_t const *in)
{
  long i;

  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, ".rodata")) {
      return &in->input_sections[i];
    }
  }
  NOT_REACHED();
  return NULL;
}

void
input_file_annotate_using_ddsum(input_exec_t *in, char const *ddsum)
{
  fun_type_info_t fti;
  FILE *fp;

  in->num_fun_type_info = 0;
  fp = fopen(ddsum, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file %s: %s.\n", ddsum, strerror(errno));
    printf("Could not open file %s.\n", ddsum);
  }
  ASSERT(fp);
  while (fscanf_fun_type_info(&fti, fp)) {
    in->fun_type_info = (fun_type_info_t *)realloc(in->fun_type_info,
        (in->num_fun_type_info + 1) * sizeof(fun_type_info_t));
    ASSERT(in->fun_type_info);
    fun_type_info_copy_shallow(&in->fun_type_info[in->num_fun_type_info], &fti);
    in->num_fun_type_info++;
  }
  fclose(fp);
}

int
input_exec_get_retsize_for_function(input_exec_t const *in, src_ulong pc)
{
  char const *function_name;
  int i;

  function_name = pc2function_name(in, pc);
  //DBG(TMP, "pc = %lx, function_name = %s.\n", (long)pc, function_name);
  if (!function_name) {
    return -1;
  }
  for (i = 0; i < in->num_fun_type_info; i++) {
    if (!strcmp(in->fun_type_info[i].ret.name, function_name)) {
      //DBG(TMP, "returning %d\n", in->fun_type_info[i].ret.typesize);
      return in->fun_type_info[i].ret.typesize;
    }
  }
  return -1;
}

static int
params_compute_num_arg_words(sym_type_info_t const *params, size_t num_params)
{
  int i, ret;
  ret = 0;
  for (i = 0; i < num_params; i++) {
    ret += DIV_ROUND_UP(params[i].typesize, DWORD_LEN);
  }
  DBG(HARVEST, "num_params = %zu, ret = %d\n", num_params, ret);
  return ret;
}

int
input_exec_function_get_num_arg_words(input_exec_t const *in, src_ulong pc)
{
  char const *function_name;
  int i;

  function_name = pc2function_name(in, pc);
  DBG(HARVEST, "pc = 0x%lx, function_name = %s.\n", (unsigned long)pc, function_name);
  if (!function_name) {
    return 0;
  }

  for (i = 0; i < in->num_fun_type_info; i++) {
    if (!strcmp(in->fun_type_info[i].ret.name, function_name)) {
      return params_compute_num_arg_words(in->fun_type_info[i].params,
          in->fun_type_info[i].num_params);
    }
  }
  return 0;
}

int
input_exec_get_section_index(input_exec_t const *in, char const *name)
{
  int i;
  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(name, in->input_sections[i].name)) {
      return i;
    }
  }
  return -1;
}

/*static void
symbol_map_add_function_symbol(graph_symbol_map_t &symbol_map, string const &fsym)
{
  symbol_id_t fsym_id = symbol_map_find_unused_key(symbol_map);
  symbol_map[fsym_id] = make_tuple(fsym, 0, false);
}*/

void
codegen(map<string, dshared_ptr<tfg>> const &function_tfg_map, string const &ofile, vector<string> const& disable_dst_insn_folding_for_functions, context *ctx)
{
  //translation_instance_t *ti = (translation_instance_t *)malloc(sizeof(translation_instance_t));
  translation_instance_t *ti = new translation_instance_t(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  ASSERT(ti);
  translation_instance_init(ti, SUPEROPTIMIZE);
  //ti->in = (input_exec_t *)malloc(sizeof(input_exec_t));
  ti->in = new input_exec_t;
  ti->out = (output_exec_t *)malloc(sizeof(output_exec_t));
  ASSERT(ti->in);      ASSERT(ti->out);
  ti->ti_set_disable_dst_insn_folding_functions(disable_dst_insn_folding_for_functions);

  read_default_peeptabs(ti);
  output_exec_init(ti->out, ofile.c_str());

  etfg_read_input_file(ti->in, function_tfg_map);

  static_translate_for_ti(ti);
  DYN_DBG(stat_trans, "%s", "calling write_target_exec()\n");
  PROGRESS("%s", "Writing the executable");
  printf("\n");
  write_target_exec(ti, ti->code_gen_buffer, ti->codesize, &ti->tcodes);
  PROGRESS("%s", "Done.                                                         \n");
  translation_instance_free(ti);
  delete ti;
}

translation_instance_t &
get_ti_fallback()
{
  autostop_timer func_timer(__func__);
  static translation_instance_t ret(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), {"ALL"});
  return ret;
}

vector<translation_unit_t>
input_exec_t::get_translation_units() const
{
#if ARCH_SRC == ARCH_ETFG
  return etfg_input_exec_get_translation_units(this);
#else
  vector<translation_unit_t> vec;
  vec.push_back(translation_unit_t("all_functions"));
  return vec;
#endif
}

size_t
input_exec_t::get_translation_unit_start_si(translation_unit_t const &tunit) const
{
  if (tunit.get_name() == "all_functions") {
    return 0;
  } else {
#if ARCH_SRC == ARCH_ETFG
    src_ulong pc = etfg_input_exec_get_start_pc_for_function_name(this, tunit.get_name());
    size_t ret = pc2index(this, pc);
    ASSERT(ret >= 0 && ret < this->num_si);
    return ret;
#else
    NOT_REACHED();
#endif
  }
}

size_t
input_exec_t::get_translation_unit_stop_si(translation_unit_t const &tunit) const
{
  if (tunit.get_name() == "all_functions") {
    return this->num_si;
  } else {
#if ARCH_SRC == ARCH_ETFG
    src_ulong pc = etfg_input_exec_get_stop_pc_for_function_name(this, tunit.get_name());
    //cout << __func__ << " " << __LINE__ << ": tunit.get_name() = " << tunit.get_name() << endl;
    ASSERT(pc != (src_ulong)-1);
    //cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << endl;
    size_t ret = pc2index(this, pc);
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    if (ret == -1) {
      ret = this->num_si;
    }
    //ASSERT(ret >= 0 && ret < this->num_si);
    return ret;
#else
    NOT_REACHED();
#endif
  }
}

std::string
alloc_elem_t::alloc_elem_to_string() const
{
  std::stringstream ss;
  ss << "(" << m_pp.to_string() << ", " << m_regname.alloc_regname_to_string() << ")";
  return ss.str();
}

graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>>
input_exec_t::get_cfg_as_graph() const
{
  graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>> ret;
  for (long i = 0; i < num_bbls; i++) {
    ret.add_node(make_dshared<node<bbl_ptr_t>>(&bbls[i]));
    for (unsigned long n = 0; n < bbls[i].num_nextpcs; n++) {
      src_ulong nextpc = bbls[i].nextpcs[n].get_val();
      if (nextpc != PC_JUMPTABLE && nextpc != PC_UNDEFINED) {
        long to = pc_find_bbl(this, nextpc);
        ASSERT(to >= 0 && to < this->num_bbls);
        ret.add_edge(make_dshared<edge<bbl_ptr_t>>(bbl_ptr_t(&bbls[i]), bbl_ptr_t(&bbls[to])));
      }
    }
  }
  return ret;
}

graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>>
input_exec_t::get_fcalls_as_graph() const
{
  map<src_ulong, pair<long, string>> function_entries; //pc->(idx,name)
  graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>> ret;
  for (long i = 0; i < num_bbls; i++) {
    string function_name;
    if (this->pc_is_etfg_function_entry(bbls[i].pc, function_name)) {
      function_entries.insert(make_pair(bbls[i].pc, make_pair(i, function_name)));
      ret.add_node(make_dshared<node<bbl_ptr_t>>(&bbls[i]));
    }
  }

  for (long i = 0; i < num_bbls; i++) {
    for (long f = 0; f < bbls[i].num_fcalls; f++) {
      nextpc_t const& nextpc = bbls[i].fcalls[f];
      if (nextpc.get_val() != PC_JUMPTABLE && nextpc.get_val() != PC_UNDEFINED) {
        ASSERT(function_entries.count(nextpc.get_val()));
        long to = pc_find_bbl(this, nextpc.get_val());
        ASSERT(to >= 0 && to < this->num_bbls);
        ret.add_edge(make_dshared<edge<bbl_ptr_t>>(bbl_ptr_t(&bbls[i]), bbl_ptr_t(&bbls[to])));
      }
    }
  }
  return ret;
}

string
input_exec_t::pc_get_function_name(src_ulong pc) const
{
#if ARCH_SRC == ARCH_ETFG
  for (const auto &tunit : etfg_input_exec_get_translation_units(this)) {
    if (   pc >= etfg_input_exec_get_start_pc_for_function_name(this, tunit.get_name())
        && pc <  etfg_input_exec_get_stop_pc_for_function_name(this, tunit.get_name())) {
      return tunit.get_name();
    }
  }
  NOT_REACHED();
#else
  NOT_IMPLEMENTED();
#endif
}

size_t
translation_instance_t::dst_iseq_disassemble_considering_nextpc_symbols(dst_insn_t *dst_iseq, size_t dst_iseq_size) const
{
  input_exec_t const* in = this->in;
  map<nextpc_id_t, src_ulong> nextpc_symbol_pcs;
  for (auto const& sym : in->symbol_map) {
    nextpc_id_t nextpc_id = -1;
    if (!symbol_represents_nextpc_symbol(sym.second, nextpc_id)) {
      continue;
    }
    ASSERT(nextpc_id != -1);
    nextpc_symbol_pcs.insert(make_pair(nextpc_id, sym.second.value));
  }
  dst_ulong *bin_offsets = new dst_ulong[this->codesize];
  nextpc_t dst_nextpcs[2];
  long dst_num_nextpcs;

  size_t dst_iseq_len = dst_iseq_disassemble_with_bin_offsets(dst_iseq, this->code_gen_buffer, this->codesize, NULL, dst_nextpcs, &dst_num_nextpcs, bin_offsets, I386_AS_MODE_32);

  for (auto const& npc : nextpc_symbol_pcs) {
    nextpc_id_t nextpc_id = npc.first;
    dst_ulong pc = npc.second;

    DYN_DEBUG(gen_sandboxed_iseq, cout << __func__ << " " << __LINE__ << ": nextpc_id " << nextpc_id << ", pc 0x" << hex << pc << endl);
    uint8_t const *binptr = tcodes_query(&this->tcodes, pc); //get the dst code pointer
    if (!binptr) {
      cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    }
    ASSERT(binptr);
    ASSERT(binptr >= this->code_gen_buffer && binptr < this->code_gen_buffer + this->codesize);
    dst_ulong offset = binptr - this->code_gen_buffer; //convert dst code pointer to dst code offset
    long index = array_search(bin_offsets, dst_iseq_len, offset); //convert dst code offset to dst insn index
    ASSERT(index != -1);
    long ret = dst_insn_put_jump_to_nextpc(&dst_iseq[index], 1, nextpc_id);  //put jump to nextpc
    ASSERT(ret == 1);
  }
  delete[] bin_offsets;
  return dst_iseq_len;
}

void
translation_instance_t::dst_iseq_convert_malloc_to_mymalloc(dst_insn_t *iseq, size_t iseq_len)
{
  for (size_t i = 0; i < iseq_len; i++) {
    dst_insn_convert_malloc_to_mymalloc(&iseq[i], &this->reloc_strings, &this->reloc_strings_size);
  }
}

#if ARCH_SRC == ARCH_ETFG
state const*
translation_instance_t::ti_get_input_state_at_etfg_pc(src_ulong pc)
{
  unsigned fid = pc / ETFG_MAX_INSNS_PER_FUNCTION;
  if (m_function_id_to_input_state_map.count(fid)) {
    return m_function_id_to_input_state_map.at(fid).get_shared_ptr().get();
  } else {
    dshared_ptr<state const> ret = etfg_pc_get_input_state(this->in, pc);
    m_function_id_to_input_state_map.insert(make_pair(fid, ret));
    return ret.get_shared_ptr().get();
  }
}
#endif
