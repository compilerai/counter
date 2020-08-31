#ifndef GAS_COMMON_H
#define GAS_COMMON_H
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include <string.h>
//#include "ansidecl.h"
//#include "libiberty.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <alloca.h>
#include "support/consts.h"
#ifdef __STDC__
#include <stddef.h>
#else
#define size_t unsigned long
#define ptrdiff_t long
#endif

#define TARGET_BYTES_BIG_ENDIAN 0
#define TARGET_FORMAT AOUT_TARGET_FORMAT
#define AOUT_TARGET_FORMAT      "a.out-i386-linux"
#define TARGET_ARCH             bfd_arch_i386

#define bfd_mach_i386_intel_syntax     (1 << 0)
#define bfd_mach_i386_i8086            (1 << 1)
#define bfd_mach_i386_i386             (1 << 2)
#define bfd_mach_x86_64                (1 << 3)
#define bfd_mach_x64_32                (1 << 4)
#define bfd_mach_i386_i386_intel_syntax (bfd_mach_i386_i386 | bfd_mach_i386_intel_syntax)
#define bfd_mach_x86_64_intel_syntax   (bfd_mach_x86_64 | bfd_mach_i386_intel_syntax)
#define bfd_mach_x64_32_intel_syntax   (bfd_mach_x64_32 | bfd_mach_i386_intel_syntax)
enum SHT
{
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_INIT_ARRAY = 14,
  SHT_FINI_ARRAY = 15,
  SHT_PREINIT_ARRAY = 16,
  SHT_GROUP = 17,
  SHT_SYMTAB_SHNDX = 18,
  SHT_LOOS = 0x60000000,
  SHT_HIOS = 0x6fffffff,
  SHT_LOPROC = 0x70000000,
  SHT_HIPROC = 0x7fffffff,
  SHT_LOUSER = 0x80000000,
  SHT_HIUSER = 0xffffffff,
  // The remaining values are not in the standard.
  // Incremental build data.
  SHT_GNU_INCREMENTAL_INPUTS = 0x6fff4700,
  SHT_GNU_INCREMENTAL_SYMTAB = 0x6fff4701,
  SHT_GNU_INCREMENTAL_RELOCS = 0x6fff4702,
  SHT_GNU_INCREMENTAL_GOT_PLT = 0x6fff4703,
  // Object attributes.
  SHT_GNU_ATTRIBUTES = 0x6ffffff5,
  // GNU style dynamic hash table.
  SHT_GNU_HASH = 0x6ffffff6,
  // List of prelink dependencies.
  SHT_GNU_LIBLIST = 0x6ffffff7,
  // Versions defined by file.
  SHT_SUNW_verdef = 0x6ffffffd,
  SHT_GNU_verdef = 0x6ffffffd,
  // Versions needed by file.
  SHT_SUNW_verneed = 0x6ffffffe,
  SHT_GNU_verneed = 0x6ffffffe,
  // Symbol versions,
  SHT_SUNW_versym = 0x6fffffff,
  SHT_GNU_versym = 0x6fffffff,

  SHT_SPARC_GOTDATA = 0x70000000,

  // ARM-specific section types.
  // Exception Index table.
  SHT_ARM_EXIDX = 0x70000001,
  // BPABI DLL dynamic linking pre-emption map.
  SHT_ARM_PREEMPTMAP = 0x70000002,
  // Object file compatibility attributes.
  SHT_ARM_ATTRIBUTES = 0x70000003,
  // Support for debugging overlaid programs.
  SHT_ARM_DEBUGOVERLAY = 0x70000004,
  SHT_ARM_OVERLAYSECTION = 0x70000005,

  // x86_64 unwind information.
  SHT_X86_64_UNWIND = 0x70000001,

  // MIPS-specific section types.
  // Section contains register usage information.
  SHT_MIPS_REGINFO = 0x70000006,
  // Section contains miscellaneous options.
  SHT_MIPS_OPTIONS = 0x7000000d,

  // AARCH64-specific section type.
  SHT_AARCH64_ATTRIBUTES = 0x70000003,

  // Link editor is to sort the entries in this section based on the
  // address specified in the associated symbol table entry.
  SHT_ORDERED = 0x7fffffff
};

/*
#ifdef BFD_ASSEMBLER
extern segT reg_section, expr_section;
// Shouldn't these be eliminated someday?  
extern segT text_section, data_section, bss_section;
#define absolute_section	bfd_abs_section_ptr
#define undefined_section	bfd_und_section_ptr
#else
#define reg_section		SEG_REGISTER
#define expr_section		SEG_EXPR
#define text_section		SEG_TEXT
#define data_section		SEG_DATA
#define bss_section		SEG_BSS
#define absolute_section	SEG_ABSOLUTE
#define undefined_section	SEG_UNKNOWN
#endif
*/

#ifndef COMMON
#ifdef TEST
#define COMMON			/* declare our COMMONs storage here.  */
#else
//#define COMMON extern		/* our commons live elswhere */
#define COMMON /* sorav */
#endif
#endif
/* COMMON now defined */

#define md_number_to_chars number_to_chars_littleendian

#define _(x) x
#define TARGET_ALIAS "x86_64-unknown-linux-gnu"
#define PACKAGE "gas"
#define LOCALEDIR ""
/* hex character manipulation routines */

#define _hex_array_size 256
#define _hex_bad        99
extern const unsigned char _hex_value[_hex_array_size];
extern void hex_init (void);
#define hex_p(c)        (hex_value (c) != _hex_bad)
/* If you change this, note well: Some code relies on side effects in
   the argument being performed exactly once.  */
#define hex_value(c)    ((unsigned int) _hex_value[(unsigned char) (c)])

//#define md_convert_frag i386_md_convert_frag
#define md_convert_frag(...)
//#define md_apply_fix i386_md_apply_fix
#define md_apply_fix(...)

#endif
