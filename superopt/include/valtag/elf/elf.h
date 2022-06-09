#ifndef _QEMU_ELF_H
#define _QEMU_ELF_H

//#include <stdint.h>
//#include <inttypes.h>
#include <elf.h>

/* 32-bit ELF base types. */
/*typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;*/

/* 64-bit ELF base types. */
/*typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef int16_t	 Elf64_SHalf;
typedef uint64_t Elf64_Off;
typedef int32_t	 Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;*/

// Attention! Platform depended definitions.
typedef uint16_t Elf_Half;
typedef uint32_t Elf_Word;
typedef int32_t  Elf_Sword;
typedef uint64_t Elf_Xword;
typedef int64_t  Elf_Sxword;

/*typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;

#define Elf32_Half Elf_Half
#define Elf64_Half Elf_Half
#define Elf32_Word Elf_Word
#define Elf64_Word Elf_Word
#define Elf32_Sword Elf_Sword
#define Elf64_Sword Elf_Sword*/

#if ELF_CLASS == ELFCLASS32

#define Elfhdr		Elf32_Ehdr
#define Elf_phdr	Elf32_Phdr
#define Elf_note	Elf32_note
#define Elf_shdr	Elf32_Shdr
#define Elf_sym		Elf32_sym

#ifdef ELF_USES_RELOCA
# define ELF_RELOC      Elf32_Rela
#else
# define ELF_RELOC      Elf32_Rel
#endif

#else

#define elfhdr		elf64_hdr
#define elf_phdr	elf64_phdr
#define elf_note	elf64_note
#define elf_shdr	elf64_shdr
#define elf_sym		elf64_sym

#ifdef ELF_USES_RELOCA
# define ELF_RELOC      Elf64_Rela
#else
# define ELF_RELOC      Elf64_Rel
#endif

#endif /* ELF_CLASS */



#ifndef ElfW
# if ELF_CLASS == ELFCLASS32
#  define ElfW(x)  Elf32_ ## x
#  define ELFW(x)  ELF32_ ## x
# else
#  define ElfW(x)  Elf64_ ## x
#  define ELFW(x)  ELF64_ ## x
# endif
#endif

#endif /* _QEMU_ELF_H */
