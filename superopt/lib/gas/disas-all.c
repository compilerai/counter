/* General "disassemble this chunk" code.  Used for debugging. */
#include "config-host.h"
#include <bfd.h>
#include "gas/dis-asm.h"
#include "valtag/elf/elf.h"
#include <errno.h>

//#include "cpu.h"
//#include "exec-all.h"
#include "gas/disas.h"

#include "ppc/regs.h"
//#include "rewrite/memset.h"

/* Filled in by elfload.c.  Simplistic, but will do for now. */
struct syminfo *syminfos = NULL;


/* Print an error message.  We can assume that this is in response to
   an error return from buffer_read_memory.  */
size_t
perror_memory (buf, size, status, memaddr, info)
     char *buf;
     size_t size;
     int status;
     bfd_vma memaddr;
     struct disassemble_info *info;
{
  char *ptr = buf, *end = ptr + size;
  if (status != EIO) {
    /* Can't happen.  */
    ptr += (*info->snprintf_func) (ptr, end - ptr, "Unknown error %d\n", status);
  } else {
    /* Actually, address between memaddr and memaddr + len was
       out of bounds.  */
    ptr += (*info->snprintf_func) (ptr, end - ptr,
			   "Address 0x%" PRIx64 " is out of bounds.\n", memaddr);
  }
  return ptr - buf;
}

/* This could be in a separate file, to save miniscule amounts of space
   in statically linked executables.  */

/* Just print the address is hex.  This is included for completeness even
   though both GDB and objdump provide their own (to print symbolic
   addresses).  */

size_t
generic_print_address (buf, size, addr, info)
     char *buf;
     size_t size;
     bfd_vma addr;
     struct disassemble_info *info;
{
    return (*info->snprintf_func) (buf, size, "0x%" PRIx64, addr);
}

/* Just return the given address.  */

int
generic_symbol_at_address (addr, info)
     bfd_vma addr;
     struct disassemble_info * info;
{
  return 1;
}

/*
bfd_vma
bfd_getl32 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0];
  v |= (unsigned long) addr[1] << 8;
  v |= (unsigned long) addr[2] << 16;
  v |= (unsigned long) addr[3] << 24;
  return (bfd_vma) v;
}

bfd_vma
bfd_getb32 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0] << 24;
  v |= (unsigned long) addr[1] << 16;
  v |= (unsigned long) addr[2] << 8;
  v |= (unsigned long) addr[3];
  return (bfd_vma) v;
}

bfd_vma
bfd_getl16 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0];
  v |= (unsigned long) addr[1] << 8;
  return (bfd_vma) v;
}

bfd_vma
bfd_getb16 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0] << 24;
  v |= (unsigned long) addr[1] << 16;
  return (bfd_vma) v;
}
*/

/* Look up symbol for debugging purpose.  Returns "" if unknown. */
#if 0
static const char *lookup_symbol(target_ulong orig_addr)
{
    unsigned int i;
    /* Hack, because we know this is x86. */
    Elf32_Sym *sym;
    struct syminfo *s;
    target_ulong addr;
    
    for (s = syminfos; s; s = s->next) {
	sym = s->disas_symtab;
	for (i = 0; i < s->disas_num_syms; i++) {
	    if (sym[i].st_shndx == SHN_UNDEF
		|| sym[i].st_shndx >= SHN_LORESERVE)
		continue;

	    if (ELF_ST_TYPE(sym[i].st_info) != STT_FUNC)
		continue;

	    addr = sym[i].st_value;
#ifdef TARGET_ARM
            /* The bottom address bit marks a Thumb symbol.  */
            addr &= ~(target_ulong)1;
#endif
	    if (orig_addr >= addr
		&& orig_addr < addr + sym[i].st_size)
		return s->disas_strtab + sym[i].st_name;
	}
    }
    return "";
}

/* Return the start addr of symbol sym. Return -1 if unknown symbol */
long int symbol_start(char const *symbol)
{
    unsigned int i;
    /* Hack, because we know this is x86. */
    Elf32_Sym *sym;
    struct syminfo *s;
    target_ulong addr;
    
    for (s = syminfos; s; s = s->next) {
	sym = s->disas_symtab;
	for (i = 0; i < s->disas_num_syms; i++) {
	    if (sym[i].st_shndx == SHN_UNDEF
		|| sym[i].st_shndx >= SHN_LORESERVE)
		continue;

	    if (ELF_ST_TYPE(sym[i].st_info) != STT_FUNC)
		continue;

	    addr = sym[i].st_value;
#ifdef TARGET_ARM
            /* The bottom address bit marks a Thumb symbol.  */
            addr &= ~(target_ulong)1;
#endif
	    if (!strcmp(s->disas_strtab + sym[i].st_name, symbol))
	      return addr;
	}
    }
    return -1;
}

/* Return the stop addr of symbol sym. Return -1 if unknown symbol */
long int symbol_stop(char const *symbol)
{
    unsigned int i;
    /* Hack, because we know this is x86. */
    Elf32_Sym *sym;
    struct syminfo *s;
    target_ulong addr;
    
    for (s = syminfos; s; s = s->next) {
	sym = s->disas_symtab;
	for (i = 0; i < s->disas_num_syms; i++) {
	    if (sym[i].st_shndx == SHN_UNDEF
		|| sym[i].st_shndx >= SHN_LORESERVE)
		continue;

	    if (ELF_ST_TYPE(sym[i].st_info) != STT_FUNC)
		continue;

	    addr = sym[i].st_value;
#ifdef TARGET_ARM
            /* The bottom address bit marks a Thumb symbol.  */
            addr &= ~(target_ulong)1;
#endif
	    if (!strcmp(s->disas_strtab + sym[i].st_name, symbol))
	      return addr + sym[i].st_size;
	}
    }
    return -1;
}
#endif
