#ifndef __DBG_FUNCTIONS_H
#define __DBG_FUNCTIONS_H
#include <stdint.h>
#include "support/types.h"

struct state_t;

void __dbg_functions(void);
int __dbg_copy_str(char *buf, int buf_size, char const *str);
void __dbg_print_str(char const *str, int size);
int __dbg_int2str_concise(char *buf, int buf_size, unsigned long n);
/*void __dbg_print_str(char *str, int size);
void __dbg_print_chr(int chr);
void __dbg_print_int(unsigned long n);*/
void __dbg_print_machine_state(unsigned long pc,
    struct state_t *cpu, uint64_t live_regs_bitmap);
/*void __dbg_print_machine_state_concise(unsigned long nip, int len,
    void *regs, uint64_t live_regs_bitmap);
void __dbg_print_esp(void *regs, long nip);
void __dbg_read_debug_info(void *regs, char const *filename);
void __dbg_print_stack(void *regs, int depth);*/
//void __dbg_print_memlocs(int num_memlocs, ...);
//void __dbg_print_initial_stack(src_ulong sp);
void __dbg_init_dyn_debug(struct state_t const *state, char const *filename);
void __dbg_bswap_stack_and_phdrs_on_startup(struct state_t const *state);
//void __dbg_do_syscall(void *regs);
void __dbg_nop (long a);

extern int dyn_debug_fd;

#endif
