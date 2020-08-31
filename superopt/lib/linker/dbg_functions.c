#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "support/types.h"
#include "i386/cpu_consts.h"
#define ELF_CLASS ELFCLASS32
#include "valtag/elf/elf.h"
#include "linker/dbg_functions.h"
#include "linker/linux/syscall_nr.h"
#include "superopt/src_env.h"
#include "exec/state.h"
#include "support/bswap.h"
//#include "rewrite/regset.h"

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(x) do { if (!(x)) __asm__ ("movl $0x1, 0x0"); } while (0)
#define NOT_IMPLEMENTED() ASSERT(0)
#define NOT_REACHED() ASSERT(0)

long do_syscall(void *cpu_env, int num, long arg1, long arg2, long arg3, 
                long arg4, long arg5, long arg6);

int dyn_debug_fd = -1;
//static char const *dyn_debug_fifo = BUILD_DIR "/fifo";

/* THIS SHOULD BE THE FIRST FUNCTION IN THIS FILE */
void __dbg_functions (void)
{
}

static void
print_hex_chr(char *ptr, unsigned long hexnum)
{
  switch (hexnum) {
    case 0: *ptr = '0'; break;
    case 1: *ptr = '1'; break;
    case 2: *ptr = '2'; break;
    case 3: *ptr = '3'; break;
    case 4: *ptr = '4'; break;
    case 5: *ptr = '5'; break;
    case 6: *ptr = '6'; break;
    case 7: *ptr = '7'; break;
    case 8: *ptr = '8'; break;
    case 9: *ptr = '9'; break;
    case 0xa: *ptr = 'a'; break;
    case 0xb: *ptr = 'b'; break;
    case 0xc: *ptr = 'c'; break;
    case 0xd: *ptr = 'd'; break;
    case 0xe: *ptr = 'e'; break;
    case 0xf: *ptr = 'f'; break;
    default: *ptr = 'X'; break;
  }
}

void
__dbg_print_str(char const *str, int size)
{
  __asm__ volatile (
      "mov %1, %%ebx\n\t"
      "int $0x80\n\t"
      : 
      : "a" (4),
        "g" (dyn_debug_fd),
        "c" (str),
        "d" (size)
	: "%ebx");
  /*
  __asm__ (
      "addr64 movq   $4, %%rax\n\t"
      "movzlq %0, %%rdi\n\t"
      "movzlq %1, %%rsi\n\t"
      "movzlq %2, %%rdx\n\t"
      "syscall\n\t"
      : 
      : "g" (dyn_debug_fd),
        "g" (str),
        "g" (size)
	: "%eax", "%ecx", "%edx", "%esi", "%edi");
  */
}



static void
__dbg_print_chr(int chr)
{
  char str[1];
  str[0] = (char)chr;
  __dbg_print_str(str, 1);
}

int
__dbg_int2str_concise(char *buf, int buf_size, unsigned long n)
{
  int i;
  char *ptr = buf, *end = buf+buf_size;
  for (i = 0; i < 8; i++) {
    if (i == 7 || n >= (1 << ((7-i)*4))) {
      print_hex_chr (ptr, (n >> ((7 - i)*4)) & 0xf);
      ptr++;
    }
  }

  ASSERT(ptr <= end);
  return ptr - buf;
}

// decimal representation of number
int __dbg_int2str10_concise (char *buf, int buf_size, unsigned long n)
{
  char *ptr = buf, *end = buf+buf_size;

  unsigned long rem = n % 10;
  unsigned long quot = n / 10;

  if (quot > 0) {
    ptr += __dbg_int2str10_concise(ptr, end - ptr, quot);
  }
  *ptr++ = '0' + rem;

  ASSERT(ptr <= end);
  return ptr - buf;
}


static void
__dbg_print_int(unsigned long n)
{
  int i;
  char out[10];
  char *ptr = out;
  *ptr++='0'; *ptr++='x';
  ptr += 7;	/* to accound for change in endian-ness */
  for (i = 0; i < 8; i++) {
    print_hex_chr (ptr, n & 0xf);
    n = n >> 4;
    ptr--;
  }

  __dbg_print_str (out, sizeof out);
  return;
}

static void
__dbg_print_int8 (int n)
{
  int i;
  char out[2];
  char *ptr = out;
  //*ptr++='0'; *ptr++='x';
  ptr += 1;	/* to accound for change in endian-ness */
  for (i = 0; i < 2; i++)
  {
    print_hex_chr (ptr, n & 0xf);
    n = n >> 4;
    ptr--;
  }

  __dbg_print_str (out, sizeof out);
}

/*__attribute__((noinline)) void
__dbg_mknod(char const *name, mode_t mode)
{
  // NOT TESTED.
  ASSERT(name);
  __asm__ volatile (
      "mov %1, %%ebx\n\t"
      "int $0x80\n\t"
      :
      : "a" (14),
        "g" (name),
        "c" (mode),
        "d" (0)
      : "%ebx");
}

__attribute__((noinline)) int
__dbg_open(char const *name, int flags)
{
  // NOT TESTED.
  volatile int ret;
  ASSERT(name);
  __asm__ volatile (
      "mov %2, %%ebx\n\t"
      "int $0x80\n\t"
      "mov %%eax, %0\n\t"
      : "=g" (ret)
      : "a" (5),
        "g" (name),
        "c" (flags),
        "d" (0)
      : "%ebx");
  return ret;
}
*/

int
__dbg_copy_str(char *buf, int buf_size, char const *str)
{
  char const *src = str;
  char *dst = buf, *end = buf+buf_size;

  while ((*dst++ = *src++));
  ASSERT(dst <= end);
  return (dst - buf - 1);
}



static void
__dbg_print_initial_stack(src_ulong sp)
{
  char space[1], *a = space;
  char newline[1], *b = newline;
  char colon[1], *c = colon;
  char tab[1], *t = tab;
  int i;
  int num_zeros_seen;

#define MAX_STRINGS 512
  src_ulong strings[MAX_STRINGS];
  int num_strings;

  *a++=' '; *b++='\n'; *c++=':'; *t++='\t';

  src_ulong t0, t1 = -1, last_t1 = -1;
  i = 0;
  num_strings = 0;
  num_zeros_seen = 0;

  t0 = sp;
/*#if SRC == ARCH_PPC
  t0 = regs->gpr[1] ;
#elif SRC == ARCH_I386
  t0 = regs->gpr[4] ;
#endif*/
  do {
    last_t1 = t1;

#if SRC_ENDIANNESS != DST_ENDIANNESS
    t1 = bswap32(*(src_ulong *)t0);
#else
    t1 = *(src_ulong *)t0;
#endif

    __dbg_print_str(tab, 1);
    __dbg_print_int(t0);
    __dbg_print_str(colon, 1);
    __dbg_print_str(space, 1);
    __dbg_print_int(t1);
    __dbg_print_str(newline, 1);

    if (t1 == 0) {
      num_zeros_seen++;
    }
    if (i > 0 && num_zeros_seen < 2 && t1!=0) { /* eliminate argc and auxv*/
      strings[num_strings] = t1;
      num_strings++;
    }
    i++;
    t0 += sizeof (src_ulong);
  } while (!(t1 == 0 && last_t1 == 0));

  for (i = 0; i < num_strings; i++) {
    t0 = strings[i];

    __dbg_print_str(tab, 1);
    __dbg_print_int(t0);
    __dbg_print_str(colon, 1);

    do {
      t1 = *(uint8_t *)(t0);

      __dbg_print_str(space, 1);
      __dbg_print_int8(t1);

      t0 += sizeof (uint8_t);
    } while (t1 != 0);

    __dbg_print_str (newline, 1);
  }

  char buf[256];
  char *ptr = buf, *end = buf + sizeof buf;
  ptr += __dbg_copy_str(ptr, end - ptr, "Starting execution\n");
  __dbg_print_str(buf, ptr-buf);
}

static void
__dbg_bswap_phdrs_on_startup(src_ulong sp)
{
  int i;
  char tbuf[128];
  char *ptr = tbuf, *end = tbuf + (sizeof tbuf);

  src_ulong t0, t1 = -1, last_t1 = -1;
  Elf_phdr *phdr = NULL;
  int phnum = 0;
  int num_zeros_seen;

  num_zeros_seen = 0;

  t0 = sp;
  ptr = tbuf;
  do {
    last_t1 = t1;

    t1 = bswap32(*(src_ulong *)t0);

    if (t1 == 0) {
      num_zeros_seen++;
    }
    if (num_zeros_seen >= 2) {
      if (last_t1 == AT_PHDR) {
        phdr = (Elf_phdr *)t1;
      } else if (last_t1 == AT_PHNUM) {
        phnum = t1;
      }
    }
    t0 += sizeof(src_ulong);
  } while (!(t1 == 0 && last_t1 == 0));

  /*
  char out[10], *ptr = out;
  char newline[1];
  *ptr++ = 'p'; *ptr++ = 'h'; *ptr++ = 'd'; *ptr++ = 'r'; *ptr++ = ':';
  *ptr++ = ' ';
  newline[0] = '\n';
  __dbg_print_str (out, 6);
  __dbg_print_int ((src_ulong)phdr);
  __dbg_print_str (newline, 1);
  ptr = out;
  *ptr++ = 'p'; *ptr++ = 'h'; *ptr++ = 'n'; *ptr++ = 'u'; *ptr++ = 'm';
  *ptr++ = ':'; *ptr++ = ' ';
  __dbg_print_str (out, 7);
  __dbg_print_int (phnum);
  __dbg_print_str (newline, 1);
  */

  for (i = 0; i < phnum; i++) {
    bswap32s(&phdr->p_type);		/* Segment type */
    bswaptls((uint32_t *)&phdr->p_offset);	/* Segment file offset */
    bswaptls((uint32_t *)&phdr->p_vaddr);	/* Segment virtual address */
    bswaptls((uint32_t *)&phdr->p_paddr);	/* Segment physical address */
    bswaptls((uint32_t *)&phdr->p_filesz);	/* Segment size in file */
    bswaptls((uint32_t *)&phdr->p_memsz);	/* Segment size in memory */
    bswap32s((uint32_t *)&phdr->p_flags);	/* Segment flags */
    bswaptls((uint32_t *)&phdr->p_align);	/* Segment alignment */

    phdr++;
  }
}

static void
__dbg_bswap_stack_on_startup(src_ulong sp)
{
  uint32_t *ptr = (uint32_t *)sp;
  uint32_t val, oldval;

  val = 1;
  do {
    bswap32s(ptr);
    oldval = val;
    val = *ptr;
    ptr++;
  } while (oldval || val);

  *(ptr - 2) = 0x13000000;
  *(ptr - 1) = 0x20000000;
  *ptr = 0x14000000;
  *(ptr + 1) = 0x20000000;
  *(ptr + 2) = 0x15000000;
  *(ptr + 3) = 0;
  *(ptr + 4) = 0;
  *(ptr + 5) = 0;
}

void
__dbg_bswap_stack_and_phdrs_on_startup(state_t const *state)
{
#if SRC_ENDIANNESS != DST_ENDIANNESS
  __dbg_bswap_stack_on_startup(state->exregs[GPR_GROUP][SRC_STACK_REGNUM]);
  __dbg_bswap_phdrs_on_startup(state->exregs[GPR_GROUP][SRC_STACK_REGNUM]);
#endif
  asm volatile("xorl %eax, %eax");
  asm volatile("xorl %ecx, %ecx");
  asm volatile("xorl %edx, %edx");
  asm volatile("xorl %ebx, %ebx");

  asm volatile("movl %ebp, %esp");
  asm volatile("popl %ebp");
  asm volatile("retl");
}

void
__dbg_init_dyn_debug(state_t const *state, char const *filename)
{
  void *cur_brk;
  dyn_debug_fd = 2; // stderr
  __dbg_print_str(filename, strlen(filename));
  __dbg_print_str("\n", 1);
  cur_brk = sbrk(0);
  __dbg_print_int((uint32_t)cur_brk);
  __dbg_print_str("\n\n", 2);
  __dbg_print_initial_stack(state->exregs[GPR_GROUP][SRC_STACK_REGNUM]);
/*
  __dbg_mknod(dyn_debug_fifo, S_IFIFO|0666);
  int fd = __dbg_open(dyn_debug_fifo, O_WRONLY);
  if (fd < 0 && fd != -EEXIST) {
    char buf[256];
    char *ptr = buf, *end = buf + sizeof buf;
    ptr += __dbg_copy_str(ptr, end-ptr, "open failed for fifo ");
    ptr += __dbg_copy_str(ptr, end-ptr, dyn_debug_fifo);
    ptr += __dbg_copy_str(ptr, end-ptr, "\n");
    ptr += __dbg_copy_str(ptr, end-ptr, ". error = ");
    ptr += __dbg_int2str10_concise(ptr, end-ptr, fd);
    //ptr += __dbg_copy_str(ptr, end-ptr, strerror(errno));
    __dbg_print_str(buf, ptr-buf);
    ASSERT(0);
  } else {
    dyn_debug_fd = fd;
  }
*/
}

static size_t
__dbg_reg2str(char *buf, size_t buf_size, int regno, int value,
    uint64_t live_regs_bitmap)
{
  char *ptr = buf, *end = buf+buf_size;

  *ptr++ = '\t';
  //ptr += __dbg_copy_str (ptr, end-ptr, "\tregs[");
  *ptr++ = 'r';
  ptr += __dbg_int2str10_concise(ptr, end-ptr, regno);
  *ptr++ = ':'; *ptr++ = ' ';
  //ptr += __dbg_copy_str (ptr, end-ptr, "] = ");

  if (live_regs_bitmap & (1ULL<<regno)) {
    ptr += __dbg_int2str_concise (ptr, end-ptr, value);
  } else {
    *ptr++ = 'X';
  }
  ptr += __dbg_copy_str (ptr, end-ptr, "\n");

  return ptr - buf;
}

static size_t
__dbg_exreg2str(char *buf, size_t buf_size, int groupno, int regno, int value,
    uint64_t live_regs_bitmap)
{
  char *ptr = buf, *end = buf+buf_size;

  *ptr++ = '\t';
  //ptr += __dbg_copy_str (ptr, end-ptr, "\tregs[");
  *ptr++ = 'e';   *ptr++ = 'x';   *ptr++ = 'r';
  ptr += __dbg_int2str10_concise(ptr, end - ptr, groupno);
  *ptr++ = '.';
  ptr += __dbg_int2str10_concise(ptr, end - ptr, regno);
  *ptr++ = ':'; *ptr++ = ' ';
  //ptr += __dbg_copy_str (ptr, end-ptr, "] = ");

  if (live_regs_bitmap & (1ULL<<regno)) {
    ptr += __dbg_int2str_concise(ptr, end - ptr, value);
  } else {
    *ptr++ = 'X';
  }
  ptr += __dbg_copy_str(ptr, end - ptr, "\n");

  return ptr - buf;
}

void
__dbg_print_machine_state(unsigned long nip, state_t *env,
    uint64_t live_regs_bitmap)
{
  static char buf[8192];
  char *ptr = buf, *end = buf+sizeof buf;
  ptr += __dbg_copy_str(ptr, end-ptr, "PC: ");
  ptr += __dbg_int2str_concise(ptr, end-ptr, nip);
  *ptr++='\n';

  int i, j;
  /*for (i = 0; i < SRC_NUM_REGS; i++) {
    ptr += __dbg_reg2str(ptr, end - ptr, i, *(uint32_t *)((uint8_t *)env + SRC_ENV_OFF(i))/, live_regs_bitmap);
  }*/

  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      ptr += __dbg_exreg2str(ptr, end - ptr, i, j, *(uint32_t *)((uint8_t *)env + SRC_ENV_EXOFF(i, j))/*env->exregs[i][j]*/, live_regs_bitmap);
    }
  }

  __dbg_print_str(buf, ptr - buf);
}

/*
#define CONCISE_OUTPUT(regnum, regname) do { \
  if (last_nip == -1 || last_state.regname != regs->regname \
      || (last_live_regs_bitmap & (1ULL << regnum)) \
                                != (live_regs_bitmap & (1ULL << regnum))) \
  { \
    ptr += __dbg_reg2str(ptr, end-ptr, regnum, regs->regname,live_regs_bitmap);\
    last_state.regname = regs->regname; \
  } \
} while (0)

void
__dbg_print_machine_state_concise(unsigned long nip, int len, void *regs,
    uint64_t live_regs_bitmap)
{
  ASSERT(0);
}


void __dbg_read_debug_info (struct CPUState *regs, char const *filename)
{
}

void __dbg_print_stack(void *regs, int depth)
{
  ASSERT(0);
#if 0
  char space[1], *a = space;
  char newline[1], *b = newline;
  char colon[1], *c = colon;
  char tab[1], *t = tab;
  int i;

  *a++=' '; *b++='\n'; *c++=':'; *t++='\t';

  for (i = -depth; i < depth; i++) {
    src_ulong t0, t1;
    t0 = regs->gpr[1];
    t0 += i * sizeof (src_ulong);
    t1 = ldl(t0);

    __dbg_print_str(tab, 1);
    __dbg_print_int(t0);
    __dbg_print_str(colon, 1);
    __dbg_print_str(space, 1);
    __dbg_print_int(t1);
    __dbg_print_str(newline, 1);
  }
  //__dbg_print_str(newline, 1);
#endif
}
*/

static void
__dbg_print_memlocs(int num_memlocs, ...)
{
  va_list ap;
  va_start(ap, num_memlocs);

  int i;

  char buf[256];
  char *ptr = buf, *end = buf + sizeof buf;

  /*
  if (num_memlocs > 0) {
    ptr += __dbg_copy_str(ptr, end - ptr, "\tM:\n");
  }

  for (i = 0; i < num_memlocs; i++) {
    unsigned long t0, t1;
    t0 = va_arg(ap, uint32_t);
    t1 = ldl(t0);

    *ptr++ = '\t';
    ptr += __dbg_int2str_concise (ptr, end-ptr, t0);
    *ptr++ = ':'; *ptr++ = ' ';
    __dbg_print_str(buf, ptr-buf); ptr = buf;
    ptr += __dbg_int2str_concise (ptr, end-ptr, t1);
    *ptr++ = '\n';
    __dbg_print_str(buf, ptr-buf); ptr = buf;
  }
  */

  //__dbg_print_str(buf, ptr-buf);
  va_end (ap);
}


void __dbg_nop(long a)
{
}

static int
__dbg_int2str(char *buf, int buf_size, unsigned long n)
{
  int i;
  ASSERT(buf_size >= 10);
  char *ptr = buf;
  *ptr++='0'; *ptr++='x';
  ptr += 7;
  for (i = 0; i < 8; i++) {
    print_hex_chr (ptr, n & 0xf);
    n = n >> 4;
    ptr--;
  }
  return 10;
}


void __dbg_syscall_print_counters_on_exit(uint32_t flags, uint32_t edi,
    uint32_t esi, uint32_t ebp, uint32_t dummy, uint32_t ebx, uint32_t edx,
    uint32_t ecx, uint32_t eax, uint32_t n)
{
  NOT_IMPLEMENTED();
/*
  static char buf[40960];
  if (eax == 1) {
    int i;
    for (i = 0; i < n; i++) {
      char *ptr = buf, *end = ptr + sizeof buf;
      uint32_t val = *(uint32_t *)(SRC_ENV_COUNTER0 + i*4);
      ptr += __dbg_copy_str(ptr, end - ptr, "COUNTER[");
      ptr += __dbg_int2str(ptr, end - ptr, i);
      ptr += __dbg_copy_str(ptr, end - ptr, "] = ");
      ptr += __dbg_int2str(ptr, end - ptr, val);
      ptr += __dbg_copy_str(ptr, end - ptr, "\n");
      __dbg_print_str(buf, ptr - buf);
    }
  }
*/
}
