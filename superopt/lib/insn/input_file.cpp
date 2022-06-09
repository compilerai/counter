#include <map>
#include <iostream>
#include <elfio/elfio.hpp>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/mman.h>

#include "config-host.h"

#include "support/timers.h"
#include "support/src-defs.h"
#include "support/globals.h"
#include "support/c_utils.h"
#include "support/debug.h"

#include "cutils/imap_elem.h"
#include "cutils/enumerator.h"
#include "cutils/rbtree.h"
#include "cutils/large_int.h"
#include "cutils/pc_elem.h"

#include "valtag/memslot_map.h"
#include "valtag/memslot_set.h"
#include "valtag/nextpc.h"
#include "valtag/regmap.h"
#include "valtag/transmap.h"
#include "valtag/elf/elf.h"
#include "valtag/elf/elftypes.h"
#include "valtag/imm_vt_map.h"
#include "valtag/pc_with_copy_id.h"

#include "expr/memlabel.h"

#include "gsupport/graph_symbol.h"

#include "insn/live_ranges.h"
#include "insn/src-insn.h"
#include "i386/cpu_consts.h"
#include "insn/rdefs.h"
#include "insn/cfg.h"
#include "insn/jumptable.h"
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "insn/fun_type_info.h"
#include "i386/regs.h"
#include "x64/regs.h"

#include "rewrite/transmap_genset.h"
#include "rewrite/static_translate.h"
#include "rewrite/bbl.h"
#include "rewrite/peephole.h"
#include "rewrite/transmap_set.h"
#include "rewrite/translation_instance.h"
#include "rewrite/symbol_map.h"

#include "superopt/rand_states.h"
#include "superopt/superopt.h"
#include "superopt/superoptdb.h"

using namespace std;
using namespace ELFIO;

bool use_calling_conventions = true;


static char const *
input_exec_type_to_string(unsigned char input_exec_type)
{
  switch (input_exec_type) {
    case ET_NONE: return "No file type";
    case ET_EXEC: return "Executable file";
    case ET_DYN: return "Shared object file";
    case ET_REL: return "Relocatable file";
    case ET_CORE: return "Core file";
    default: NOT_REACHED();
  }
}

static void
input_section_read_attrs_from_elfio_section(input_section_t *isec, section const *sec)
{
  isec->addr = sec->get_address();
  isec->type = sec->get_type();
  isec->flags = sec->get_flags();
  isec->size = sec->get_size();
  isec->info = sec->get_info();
  isec->align = sec->get_addr_align();
  isec->entry_size = sec->get_entry_size();
}


static void
read_dynamic_section(input_exec_t *in, elfio &pReader, section *pSec)
{
  dynamic_section_accessor const pDynSec(pReader, pSec);
  int orig_num_dyn_entries;

  int name_len = pSec->get_name().length() + 64;
  in->dynamic_section.name = (char *)malloc(name_len * sizeof(char));
  ASSERT(in->dynamic_section.name);
  snprintf(in->dynamic_section.name, name_len, "%s", pSec->get_name().c_str());

  in->has_dynamic_section = true;
  input_section_read_attrs_from_elfio_section(&in->dynamic_section, pSec);
  in->dynamic_section.data= (char *)malloc(in->dynamic_section.size);
  ASSERT(in->dynamic_section.data);
  memcpy(in->dynamic_section.data, pSec->get_data(), pSec->get_size());

  orig_num_dyn_entries = in->num_dyn_entries;
  in->num_dyn_entries += pDynSec.get_entries_num();
  in->dyn_entries = (dyn_entry_t *)realloc(in->dyn_entries, in->num_dyn_entries
      * sizeof(dyn_entry_t));
  ASSERT(in->dyn_entries);

  for (int i = 0; i < in->num_dyn_entries; i++) {
    Elf_Xword tag = 0;
    Elf_Xword value = 0;
    std::string name;

    pDynSec.get_entry(i, tag, value, name);
    in->dyn_entries[i].tag = tag;
    in->dyn_entries[i].value = value;
    /*dyn_entry_read_string(pReader, in->dyn_entries[i].name,
        sizeof in->dyn_entries[i].name, in->dyn_entries[i].tag,
        in->dyn_entries[i].value);*/
    strncpy(in->dyn_entries[i].name, name.c_str(), sizeof in->dyn_entries[i].name);
    ASSERT(strlen(in->dyn_entries[i].name) < sizeof in->dyn_entries[i].name);
  }
}




char const *
pc2function_name(struct input_exec_t const *in, src_ulong pc)
{
  src_ulong orig_pc = pc_get_orig_pc(pc);
  CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << pc << dec << ", orig_pc = 0x" << hex << pc << dec << "\n");
  for (const auto &s : in->symbol_map) {
    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": symbol name " << s.second.name << " : value 0x" << hex << s.second.value << dec << endl);
    if (   s.second.value == orig_pc
        && s.second.type == STT_FUNC) {
      CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": returning symbol name " << s.second.name << endl);
      return s.second.name;
    }
  }
  return nullptr;
}

char const *
lookup_symbol(input_exec_t const *in, src_ulong pc)
{
  int i, ret = -1;
  if (pc == PC_JUMPTABLE) {
    return "indir";
  }
  DYN_DEBUG(lookup_symbol, printf("lookup_symbol on %lx\n", (unsigned long)pc));
  for (const auto &s : in->symbol_map) {
    DYN_DEBUG(lookup_symbol, printf("lookup_symbol on %lx: checking symbol %s, shndx %d (SHN_UNDEF %d), value %lx\n", (unsigned long)pc, s.second.name, (int)s.second.shndx, (int)SHN_UNDEF, (long)s.second.value));
#if ARCH_SRC != ARCH_ETFG
    if (s.second.shndx == SHN_UNDEF) {
      continue;
    }
#endif
    if (   pc >= s.second.value
        && (ret == -1 || s.second.value >= in->symbol_map.at(ret).value)) {
      if (ret == -1 || !strstart(s.second.name, "jt", NULL)) {
        //printf("symbol %s, value %lx\n", in->symbols[i].name, (unsigned long)in->symbols[i].value);
        ret = s.first;
      }
    }
  }
  if (ret == -1) {
    DYN_DEBUG(lookup_symbol, printf("lookup_symbol on %lx. returning undef\n", (unsigned long)pc));
    return "undef";
  }
  DYN_DEBUG(lookup_symbol, printf("lookup_symbol on %lx. returning %s\n", (unsigned long)pc, in->symbol_map.at(ret).name));
  return in->symbol_map.at(ret).name;
}



void
input_exec_init(input_exec_t *in)
{
  in->filename = NULL;
  in->num_input_sections = 0;
  in->num_plt_sections = 0;
  //in->num_symbols = 0;
  //in->symbols = NULL;
  //in->num_dynsyms = 0;
  //in->dynsyms = NULL;
  in->num_dyn_entries = 0;
  in->dyn_entries = NULL;
  in->bbls = NULL;
  in->num_bbls = 0;
  in->dominators = NULL;
  in->post_dominators = NULL;
  in->innermost_natural_loop = NULL;
  in->si = NULL;
  //in->locals_info = NULL;
  in->num_si = 0;
  in->jumptable = NULL;
  in->rodata_ptrs = NULL;
  in->rodata_ptrs_size = 0;
  in->num_rodata_ptrs = 0;
  in->fun_type_info = NULL;
  in->num_fun_type_info = 0;
  insn_line_map_init(&in->insn_line_map);
}

void
get_text_section(input_exec_t const *in, long *start_pc, long *end_pc)
{
  int i;

  for (i = 0; i < in->num_input_sections; i++) {
    //printf("in->input_sections[i].name = %s.\n", in->input_sections[i].name);
    if (!strcmp(in->input_sections[i].name, ".text")) {
      *start_pc = in->input_sections[i].addr;
      *end_pc = *start_pc + in->input_sections[i].size;
      return;
    }
  }
  NOT_REACHED();
}

static void
free_cfg(input_exec_t *in)
{
  if (in->dominators) {
    free(in->dominators);
    in->dominators = NULL;
  }
  if (in->post_dominators) {
    free(in->post_dominators);
    in->post_dominators = NULL;
  }
  if (in->innermost_natural_loop) {
    delete [] in->innermost_natural_loop;
    in->dominators = NULL;
  }
  if (in->jumptable) {
    jumptable_free(in->jumptable);
    in->jumptable = NULL;
  }
  if (in->bbls) {
    rbtree_destroy(&in->bbl_leaders, pc_elem_free);
    free_bbl_map(in);
    int i;
    for (i = 0; i < in->num_bbls; i++) {
      bbl_free(&in->bbls[i]);
    }
    free(in->bbls);
    in->bbls = NULL;
#if ARCH_SRC == ARCH_I386
    myhash_destroy(&in->insn_boundaries, imap_elem_free);
#endif
  }
}



static void
input_section_free(input_section_t *input_section)
{
  if (input_section->data) {
#if ARCH_SRC != ARCH_ETFG
    free(input_section->data);
#endif
    input_section->data = NULL;
  }
  if (input_section->name) {
    free(input_section->name);
    input_section->name = NULL;
  }
}

static void
input_sections_free(input_section_t *input_sections, long num_input_sections)
{
  int i;
  for (i = 0; i < num_input_sections; i++) {
    input_section_free(&input_sections[i]);
  }
}



void
input_exec_free(input_exec_t *in)
{
  long i;
  if (in->filename) {
    free((void *)in->filename);
  }
  in->symbol_map.clear();
  in->dynsym_map.clear();
  /*if (in->symbols) {
    free(in->symbols);
    in->symbols = NULL;
  }
  if (in->dynsyms) {
    free(in->dynsyms);
    in->dynsyms = NULL;
  }*/
  if (in->fun_type_info) {
    int i;
    for (i = 0; i < in->num_fun_type_info; i++) {
      fun_type_info_free(&in->fun_type_info[i]);
    }
    free(in->fun_type_info);
    in->fun_type_info = NULL;
  }
  if (in->dyn_entries) {
    free(in->dyn_entries);
    in->dyn_entries = NULL;
  }
  if (in->relocs) {
    free(in->relocs);
    in->relocs = NULL;
  }
  input_sections_free(in->input_sections, in->num_input_sections);
  input_sections_free(in->plt_sections, in->num_plt_sections);
  if (in->has_dynamic_section) {
    input_section_free(&in->dynamic_section);
  }
  free_cfg(in);
  //insn_cache_free(&in->insn_cache);
  //input_exec_locals_info_free(in);
  si_free(in);
  if (in->rodata_ptrs) {
    free(in->rodata_ptrs);
    in->rodata_ptrs = NULL;
    in->rodata_ptrs_size = 0;
    in->num_rodata_ptrs = 0;
  }
  insn_line_map_free(&in->insn_line_map);
}


static int
duplicate_symbol_exists(map<symbol_id_t, symbol_t> const &symbol_map, char const *needle)
{
  char const *nn;
  int ret, len;
  //size_t i;

  ret = 0;
  for (const auto &s : symbol_map) {
    nn = strstr(s.second.name, "NN");
    if (!nn) {
      if (!strcmp(s.second.name, needle)) {
        ret++;
      }
    } else {
      len = nn - s.second.name;
      ASSERT(len >= 0);
      if (   (len >= (int)(sizeof s.second.name - 5) || len == (int)strlen(needle))
          && !memcmp(s.second.name, needle, len)) {
        ret++;
      }
    }
  }
  return ret;
}

/*static int
duplicate_symbol_exists(symbol_t const *symbols, size_t num_symbols, char const *needle)
{
  char const *nn;
  int ret, len;
  size_t i;

  ret = 0;
  for (i = 0; i < num_symbols; i++) {
    nn = strstr(symbols[i].name, "NN");
    if (!nn) {
      if (!strcmp(symbols[i].name, needle)) {
        ret++;
      }
    } else {
      len = nn - symbols[i].name;
      ASSERT(len >= 0);
      if (   (len >= (int)(sizeof symbols[i].name - 5) || len == (int)strlen(needle))
          && !memcmp(symbols[i].name, needle, len)) {
        ret++;
      }
    }
  }
  return ret;
}*/

static void
read_symtab(input_exec_t *in, elfio &pReader, section *pSec)
{
  symbol_section_accessor pSymTbl(pReader, pSec);

  int num_old_symbols = in->symbol_map.size(); //in->num_symbols;
  int num_new_symbols = pSymTbl.get_symbols_num();

  int max_num_symbols = num_old_symbols + num_new_symbols;

  //in->symbols = (symbol_t *)realloc(in->symbols,
  //    max_num_symbols*sizeof(symbol_t));

  //ASSERT(in->symbols);

  map<symbol_id_t, symbol_t> orig_symbol_map = in->symbol_map;
  int s = num_old_symbols;
  for (int i = 0; i < num_new_symbols; i++) {
    std::string name;
    int n_exists;

    pSymTbl.get_symbol((Elf_Xword)i,
        name,
        in->symbol_map[s].value,
        in->symbol_map[s].size,
        in->symbol_map[s].bind,
        in->symbol_map[s].type,
        in->symbol_map[s].shndx,
        in->symbol_map[s].other);

    snprintf(in->symbol_map[s].name, sizeof in->symbol_map[s].name, "%s",
        name.c_str());

    if (   in->symbol_map[s].name[0] == '\0'
        && in->symbol_map[s].shndx >= 0
        && in->symbol_map[s].shndx < pReader.sections.size()) {
      snprintf(in->symbol_map[s].name, sizeof in->symbol_map[s].name, "%s",
          pReader.sections[in->symbol_map[s].shndx]->get_name().c_str());
      in->symbol_map[s].size = pReader.sections[in->symbol_map[s].shndx]->get_size();
      DBG(SYMBOLS, "in->symbol_map[s].name %s\n", in->symbol_map[s].name);
    }
    in->symbol_map[s].name[(sizeof in->symbol_map[s].name) - 1] = '\0';

    DBG(SYMBOLS, "%d: %s, value 0x%x, size 0x%x, bind 0x%x, type 0x%x, "
        "shndx 0x%x\n", s,
        in->symbol_map[s].name, (unsigned)in->symbol_map[s].value,
        (unsigned)in->symbol_map[s].size, (unsigned)in->symbol_map[s].bind,
        (unsigned)in->symbol_map[s].type, (unsigned)in->symbol_map[s].shndx);

    if ((n_exists = duplicate_symbol_exists(orig_symbol_map, in->symbol_map[s].name))) {
      unsigned long len;
      char *ptr, *end;

      //DBG(TMP, "n_exists = %d for %s.\n", n_exists, in->symbols[s].name);
      len = strlen(in->symbol_map[s].name);
      len = min(len, (sizeof in->symbol_map[s].name) - 5);
      ptr = &in->symbol_map[s].name[len];
      end = (char *)in->symbol_map[s].name + sizeof(in->symbol_map[s].name);
      ptr += snprintf(ptr, end - ptr, "NN%d", n_exists + 1);
      //DBG(TMP, "renamed symbol to %s.\n", in->symbol_map[s].name);
      ASSERT(ptr + 1 < end);
    }

    /*if (in->symbols[s].shndx != SHN_UNDEF) */{ //record undef symbols in symtab; deal with them selectively if required
      s++;
    }
  }
  //in->num_symbols = s;
}

static section *
get_symbol_table_section(elfio const &pReader)
{
  int i;

  for (i = 0; i < pReader.sections.size(); i++) {
    section *sec;
    sec = pReader.sections[i];

    if (sec->get_name() == ".symtab") {
      return sec;
    }
  }
  return NULL;
}

static int
get_plt_section_by_name(input_exec_t const *in, char const *name)
{
  int i;
  for (i = 0; i < in->num_plt_sections; i++) {
    DYN_DBG(stat_trans, "section %d: %s\n", i, in->plt_sections[i].name);
    if (!strcmp(name, in->plt_sections[i].name)) {
      return i;
    }
  }
  return -1;
}

static void
input_exec_get_plt_section_attrs(input_exec_t const *in,
    long *plt_addr, long *plt_entry_size, long *plt_size)
{
  int section_idx;

  section_idx = get_input_section_by_name(in, ".plt");

  ASSERT(section_idx >= 0 && section_idx < in->num_input_sections);

  if (plt_addr) {
    *plt_addr = in->input_sections[section_idx].addr;
    DBG(SYMBOLS, "plt_addr = 0x%lx\n", (long)*plt_addr);
  }
  if (plt_entry_size) {
    *plt_entry_size = in->input_sections[section_idx].entry_size * 4 /* XXX : hack */;
    ASSERT(*plt_entry_size > 0);
    DBG(SYMBOLS, "entry_size = 0x%lx\n", (long)*plt_entry_size);
  }
  if (plt_size) {
    *plt_size = in->input_sections[section_idx].size;
    DBG(SYMBOLS, "plt_size = 0x%lx\n", (long)*plt_size);
  }
}



static void
read_dynsym(input_exec_t *in, elfio const &pReader, section *pSec)
{
  symbol_section_accessor pSymTbl(pReader, pSec);

  int num_old_dynsyms = in->dynsym_map.size(); //in->num_dynsyms;
  int num_new_dynsyms = pSymTbl.get_symbols_num();
  long plt_addr, plt_entry_size;

  int max_num_dynsyms = num_old_dynsyms + num_new_dynsyms;

  input_exec_get_plt_section_attrs(in, &plt_addr, &plt_entry_size, NULL);

  /*in->dynsyms = (symbol_t *)realloc(in->dynsyms,
      max_num_dynsyms*sizeof(symbol_t));*/

  //ASSERT(in->dynsyms);

  int s = num_old_dynsyms;
  //bool functions_started = false;
  int num_plt_syms = 0;
  for (int i = 0; i < num_new_dynsyms; i++) {
    unsigned char other;
    std::string name;

    pSymTbl.get_symbol(i,
        name,
        in->dynsym_map[s].value,
        in->dynsym_map[s].size,
        in->dynsym_map[s].bind,
        in->dynsym_map[s].type,
        in->dynsym_map[s].shndx,
        other);

    snprintf(in->dynsym_map[s].name, sizeof in->dynsym_map[s].name, "%s",
        name.c_str());

/*
    if (   (   in->dynsyms[s].type == STT_FUNC
            //|| name == "__gmon_start__" : hack
           )
        && in->dynsyms[s].value == 0) {
      in->dynsyms[s].value = plt_addr + plt_entry_size * (num_plt_syms + 1);
      in->dynsyms[s].size = plt_entry_size;
      num_plt_syms++;
    }
*/
    DBG(SYMBOLS, "%s, value 0x%x, size 0x%x, bind 0x%x, type 0x%x, "
        "shndx %d\n",
        in->dynsym_map[s].name, (unsigned)in->dynsym_map[s].value,
        (unsigned)in->dynsym_map[s].size, (unsigned)in->dynsym_map[s].bind,
        (unsigned)in->dynsym_map[s].type, (unsigned)in->dynsym_map[s].shndx);

    if (in->dynsym_map[s].shndx == SHN_UNDEF) {
      s++;
    }
  }
  //in->num_dynsyms = s;
}


static void
update_dynsym_value_and_size(map<symbol_id_t, symbol_t> &dynsym_map, char const *symbol_name, Elf64_Addr value, Elf_Xword size)
{
  for (auto &s : dynsym_map) {
    if (!strcmp(s.second.name, symbol_name)) {
      s.second.value = value;
      s.second.size = size;
      DBG(SYMBOLS, "Updated %s, value 0x%x, size 0x%x, bind 0x%x, type 0x%x, "
          "shndx %d\n",
          s.second.name, (unsigned)s.second.value,
          (unsigned)s.second.size, (unsigned)s.second.bind,
          (unsigned)s.second.type, (unsigned)s.second.shndx);
      return;
    }
  }
  cout << _FNLN_ << ": syymbol_name = " << symbol_name << endl;
  for (auto &s : dynsym_map) {
    cout << "s = " << s.second.name << endl;
  }
  NOT_REACHED();
}



static void
read_reloc_table(input_exec_t *in, elfio const &pReader, section *pSec)
{
  relocation_section_accessor const pRel(pReader, pSec);
  char sectionName[MAX_SYMNAME_LEN];
  long orig_num_relocs;

  orig_num_relocs = in->num_relocs;
  in->num_relocs += pRel.get_entries_num();
  in->relocs = (reloc_t *)realloc(in->relocs, in->num_relocs * sizeof(reloc_t));
  ASSERT(in->relocs);
  
  strncpy(sectionName, pSec->get_name().c_str(), sizeof sectionName);
  sectionName[sizeof(sectionName) - 1] = '\0';
  DBG(SYMBOLS, "\nSection %s:\n", pSec->get_name().c_str());
  for (int i = orig_num_relocs; i < in->num_relocs; i++) {
    unsigned char symbolBind, symbolType;
    std::string symbolName;
    Elf64_Addr symbolValue;
    Elf_Xword symbolSize;
    Elf_Half symbolShndx;
    unsigned char other;
    Elf_Word symbol_id;

    pRel.get_entry(i, in->relocs[i].offset, symbol_id, in->relocs[i].type,
        in->relocs[i].addend);
    /*pSymTbl.get_symbol(symbol_id, symbolName, symbolValue, symbolSize,
        symbolBind, symbolType, symbolShndx, other);*/
    /* pRel.get_entry(i, in->relocs[i].offset, in->relocs[i].symbolValue,
        symbolName, in->relocs[i].type, in->relocs[i].addend,
        in->relocs[i].calcValue);
    snprintf(in->relocs[i].symbolName, sizeof in->relocs[i].symbolName, "%s",
        symbolName.c_str());
    in->relocs[i].symbolName[sizeof(in->relocs[i].symbolName) - 1] = '\0';*/
    /*snprintf(in->relocs[i].symbolName, sizeof in->relocs[i].symbolName, "%s",
        symbolName.c_str());*/

    //the following overloaded function get_entry seems to be buggy in ELFIO lib (it returns the wrong symbol name if the same address, e.g., 0,  has multiple symbols); thus we also check the symbolName manually  in the following IF condition
    pRel.get_entry(i - orig_num_relocs, in->relocs[i].offset,
        in->relocs[i].symbolValue,
        symbolName, in->relocs[i].type, in->relocs[i].addend,
        in->relocs[i].calcValue);
    //cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << ", symbolName = " << symbolName << endl;
    if (in->symbol_map.count(symbol_id)) {
      //cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << ", in->symbol_map.at(symbol_id).name = " << in->symbol_map.at(symbol_id).name << endl;
      symbolName = in->symbol_map.at(symbol_id).name;
    }
    snprintf(in->relocs[i].symbolName, sizeof in->relocs[i].symbolName, "%s",
        symbolName.c_str());
    in->relocs[i].symbolName[sizeof(in->relocs[i].symbolName) - 1] = '\0';
    strncpy(in->relocs[i].sectionName, sectionName, sizeof in->relocs[i].sectionName);
    in->relocs[i].sectionName[sizeof(in->relocs[i].sectionName) - 1] = '\0';

    in->relocs[i].orig_contents = 0;
    char const *remaining;
    if (   (   strstart(in->relocs[i].sectionName, ".rela", &remaining) //check rela before rel because it is more specific
            || strstart(in->relocs[i].sectionName, ".rel", &remaining))
        && in->relocs[i].type == R_386_PC32) {
      int isec_index;
      isec_index = get_input_section_by_name(in, remaining);
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": in->relocs[" << i << "].symbolName = " << in->relocs[i].symbolName << endl);
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": in->relocs[" << i << "].sectionName = " << in->relocs[i].sectionName << endl);
      if (isec_index == -1) {
        cout << __func__ << " " << __LINE__ << ": remaining = " << remaining << endl;
        NOT_REACHED();
      }
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": in->relocs[" << i << "].sectionName = " << in->relocs[i].sectionName << endl);
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": isec_index = " << isec_index << endl);
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": in->input_sections[" << isec_index << "].addr = " << hex << in->input_sections[isec_index].addr << dec << endl);
      CPP_DBG_EXEC(SYMBOLS, cout << __func__ << " " << __LINE__ << ": in->input_sections[" << isec_index << "].data = " << in->input_sections[isec_index].addr << endl);
      in->relocs[i].orig_contents = *(uint32_t *)&in->input_sections[isec_index].data[in->relocs[i].offset - in->input_sections[isec_index].addr];
    }

    if (!strcmp(in->relocs[i].sectionName, ".rel.plt")) {
      long plt_addr, plt_entry_size;
      input_exec_get_plt_section_attrs(in, &plt_addr, &plt_entry_size, NULL);
      update_dynsym_value_and_size(in->dynsym_map/*, in->num_dynsyms*/, in->relocs[i].symbolName, plt_addr + (i - orig_num_relocs + 1) * plt_entry_size, plt_entry_size);
   }

    DBG(SYMBOLS, "%s", "  Num Type Offset   Addend    Calc   SymValue   OrigContents"
        " SymName   SecName\n");
    DBG(SYMBOLS, "[%4x] %02d %08x %08x %08x %08x %08x %s %s\n", i,
        in->relocs[i].type, (unsigned)in->relocs[i].offset,
        (unsigned)in->relocs[i].addend, (unsigned)in->relocs[i].calcValue,
        (unsigned)in->relocs[i].symbolValue, (unsigned)in->relocs[i].orig_contents,
        in->relocs[i].symbolName,
        in->relocs[i].sectionName);
  }
}




void
read_input_file(input_exec_t *in, char const *filename)
{
  elfio pELFI;

  input_exec_init(in);
  if (filename) {
    bool ret;
    //ELFIO::GetInstance()->CreateELFI(&pELFI);
    ret = pELFI.load(filename);
    if (!ret) {
      printf("pELFI.load(%s) returned %d\n", filename, ret);
      fflush(stdout);
      NOT_REACHED();
      //exit(1);
    }
  } else {
    exit(1);
  }

  in->filename = strdup(filename);
  DYN_DBG(stat_trans, "Number of Sections = %d\n", (int)pELFI.sections.size());

  //insn_cache_init(&in->insn_cache, in);
  in->num_input_sections = 0;
  in->num_plt_sections = 0;
  in->has_dynamic_section = false;
  //in->num_symbols = 0;
  //in->num_dynsyms = 0;
  in->symbol_map.clear();
  in->dynsym_map.clear();
  in->num_dyn_entries = 0;
  //in->symbols = NULL;
  //in->dynsyms = NULL;
  in->dyn_entries = NULL;
  in->num_relocs = 0;
  in->relocs = NULL;

  in->type = pELFI.get_type();
  DYN_DBG(stat_trans, "input exec type: %s\n", input_exec_type_to_string(in->type));
  in->entry = pELFI.get_entry();

  for (int i = 0; i < pELFI.sections.size(); i++) {
    section *sec;
    sec = pELFI.sections[i];

    if (sec->get_type() == SHT_NULL) {
      continue;
    }
    if (sec->get_name() == ".shstrtab" || sec->get_name() == ".strtab") {
      continue;
    }
    if (sec->get_name() == ".symtab") {
      DYN_DBG(stat_trans, "\n\n\n%s", "calling read_symtab(.symtab)\n");
      read_symtab(in, pELFI, sec);
      continue;
    }

    if (sec->get_name() == ".dynamic") {
      continue;
    }

    if (sec->get_name() == ".rela.dyn" || sec->get_name() == ".rela.plt") {
      continue;
    }

    if (false
        /*|| sec->get_name() == ".hash"
        || sec->get_name() == ".dynstr"
        || sec->get_name() == ".dynsym"
        || sec->get_name() == ".interp"*/) {
      continue;
    }

    input_section_t *isec;

    if (/*   sec->get_name() == ".plt"
        || sec->get_name() == ".rel.plt"
        || sec->get_name() == ".got.plt"
        || */false) {
      isec = &in->plt_sections[in->num_plt_sections];
      in->num_plt_sections++;
    } else {
      isec = &in->input_sections[in->num_input_sections];
      in->num_input_sections++;
    }

    //int n = in->num_input_sections;
    //in->input_sections[n].orig_index = sec->get_index();
    isec->orig_index = sec->get_index();
    DYN_DBG(stat_trans, "section: %s. index %d\n", sec->get_name().c_str(),
        sec->get_index());
    int name_len = sec->get_name().length() + 64;
    isec->name = (char *)malloc(name_len * sizeof(char));
    ASSERT(isec->name);

    snprintf(isec->name, name_len, "%s", sec->get_name().c_str());
    //DBG(STAT_TRANS, "in->input_sections[%d].name=%s\n", n, isec->name);
    input_section_read_attrs_from_elfio_section(isec, sec);

    DYN_DBG(stat_trans, "%s: addr %lx, type %lx, flags %lx size %lx info %lx\n",
        isec->name,
        (unsigned long)isec->addr,
        (unsigned long)isec->type,
        (unsigned long)isec->flags,
        (unsigned long)isec->size,
        (unsigned long)isec->info);

    if (isec->type != SHT_NOBITS) {
      char const *tdata;
      tdata = sec->get_data();
      if (isec->size > 0) {
        isec->data= (char *)malloc(isec->size);
        ASSERT(isec->data);
        memcpy(isec->data, tdata, isec->size);
      } else {
        isec->data = NULL;
      }
    } else {
      isec->data = NULL;
    }
  }

  for (int i = 0; i < pELFI.sections.size(); i++) {
    section *sec;
    sec = pELFI.sections[i];

    if (sec->get_name() == ".dynamic") {
      read_dynamic_section(in, pELFI, sec);
    } else if (sec->get_name() == ".dynsym") {
      DYN_DBG(stat_trans, "\n\n\n%s", "calling read_dynsym(.dynsym)\n");
      read_dynsym(in, pELFI, sec);
    }
  }

  for (int i = 0; i < pELFI.sections.size(); i++) {
    section *sec;
    sec = pELFI.sections[i];

    if (sec->get_type() == SHT_REL || sec->get_type() == SHT_RELA) {
      read_reloc_table(in, pELFI, sec);
    }
  }

  //ldst_register_input_file(in);
}

#if ARCH_SRC == ARCH_ETFG
void
input_exec_t::populate_phi_edge_incident_pcs()
{
  input_exec_t const *in = this;
  for (long i = 0; i < in->num_si; i++) {
    src_insn_t const *insn = in->si[i].fetched_insn;
    if (insn->m_edges.size() > 1) {
      this->m_phi_edge_incident_pcs.insert(in->si[i].pc);
      continue;
    }
    ASSERT(insn->m_edges.size() == 1);
    etfg_insn_edge_t const &e = insn->m_edges.at(0);
    shared_ptr<tfg_edge const> const &te = e.get_tfg_edge();
    if (   pc::subsubindex_is_phi_node(te->get_from_pc().get_subsubindex())
        || pc::subsubindex_is_phi_node(te->get_to_pc().get_subsubindex())) {
      this->m_phi_edge_incident_pcs.insert(in->si[i].pc);
    }
  }
}

void
input_exec_t::populate_executable_pcs()
{
  input_exec_t const *in = this;
  for (long i = 0; i < in->num_si; i++) {
    if (etfg_pc_is_valid_pc(in, in->si[i].pc)) {
      this->m_executable_pcs.insert(in->si[i].pc);
    }
  }
}
#endif

bool
input_exec_t::pc_is_etfg_function_entry(src_ulong pc, string &function_name) const
{
#if ARCH_SRC == ARCH_ETFG
  long insn_num_in_function;
  etfg_pc_get_function_name_and_insn_num(this, pc, function_name, insn_num_in_function);
  return insn_num_in_function == 0;
#else
  return false;
#endif
}

bool
input_exec_t::pc_is_etfg_function_return(src_ulong pc, string &function_name) const
{
  input_exec_t const* in = this;
#if ARCH_SRC == ARCH_ETFG
  src_insn_t src_insn[2];
  long len;
  if (!src_iseq_fetch(src_insn, 2, &len, in, NULL, pc, 1, NULL, NULL, NULL, NULL, NULL, NULL, false)) {
    return false;
  }
  ASSERT(len <= 2);
  if (src_insn_is_function_return(&src_insn[0])) {
    long insn_num_in_function;
    etfg_pc_get_function_name_and_insn_num(in, pc, function_name, insn_num_in_function);
    return true;
  }
  return false;
#else
  return false;
#endif
}

bool
input_exec_t::pc_is_etfg_function_callsite(src_ulong pc, vector<string> &argnames) const
{
  input_exec_t const* in = this;
#if ARCH_SRC == ARCH_ETFG
  src_insn_t src_insn[2];
  long len;
  if (!src_iseq_fetch(src_insn, 2, &len, in, NULL, pc, 1, NULL, NULL, NULL, NULL, NULL, NULL, false)) {
    return false;
  }
  ASSERT(len <= 2);
  if (src_insn_is_function_call(&src_insn[0])) {
    argnames = etfg_insn_get_fcall_argnames(&src_insn[0]);
    return true;
  }
  return false;
#else
  return false;
#endif
}


