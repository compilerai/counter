//#include </usr/include/elf.h>
#include "valtag/elf/elf.h"
#include "support/src-defs.h"
/* for recent libc, we add these dummy symbols which are not declared
   when generating a linked object (bug in ld ?) */
//#if (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3))
long __preinit_array_start[0];
long __preinit_array_end[0];
long __init_array_start[0];
long __init_array_end[0];
long __fini_array_start[0];
long __fini_array_end[0];
//#endif

void start_vma(void);

int main(int argc, char **argv, char **env)
{
  int i;
  char **envp = env;
  long volatile *stack_ptr;
  Elf32_auxv_t *auxv;
  long dummy;
  long volatile stack[1024];

  stack_ptr = stack;
  *stack_ptr++ = argc;

#if ARCH_SRC != SRC_ARM
  for (i = 0; i < argc; i++) {
    *stack_ptr++ = (long)argv[i];
  }
#endif

  *stack_ptr++ = 0;

  while (*envp) {
    *stack_ptr++ = (long)*envp++;
  }

  *stack_ptr++ = 0;
  envp++;

  for (auxv = (Elf32_auxv_t *)envp; auxv->a_type != AT_NULL; auxv++) {
    /* auxv->a_type = AT_NULL marks the end of auxv */
    *stack_ptr++ = (long)auxv->a_type;
    *stack_ptr++ = (long)auxv->a_un.a_val;
  }

  *stack_ptr++ = 0;
  *stack_ptr++ = 0;

  //__asm__ volatile ("mov %1, %%esp" : "=g"(dummy) : "g"(stack));
  __asm__ volatile ("mov %0, %%esp" :: "g"(stack));
  //goto *(&start_vma);
  goto *(&start_vma);
  //start_vma(argc, argv, env);
}
