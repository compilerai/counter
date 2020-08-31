//#include </usr/include/elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "support/types.h"
#include "linker/linux/syscall_nr.h"
#include "i386/cpu_consts.h"
#include "superopt/src_env.h"
#include "linker/dbg_functions.h"

//helper functions for qemu
void qemu_log_mask(int mask, const char *fmt, ...)
{
}

static long
__do_syscall(void *cpu, long num, long arg1, long arg2, long arg3,
    long arg4, long arg5, long arg6, long arg7, long arg8)
{
  src_ulong ret = 0;

  /* printf("num=%ld, arg1=0x%lx, arg2=0x%lx, arg3=0x%lx, arg4=0x%lx\n", num, arg1,
      arg2, arg3, arg4); */
#if ARCH_SRC == ARCH_PPC
  //env->crf &= ~0x10000000;	/* cleary the 4th bit from the left (xer_so) */
  //cpu->crf[0] &= ~0x1;
  *(uint32_t *)(QEMU_ENV_CRF(cpu, 0)) &= ~0x1;
  *(src_ulong *)(QEMU_ENV_XER_SO(cpu)) = 0;
  //set_xer_so(0);
#endif

  switch (num) {
    case TARGET_NR_fork:
    case TARGET_NR_execve:
      if (dyn_debug_fd != -1) {
        char buf[80], *ptr = buf, *end = buf + sizeof buf;
        ptr += __dbg_copy_str(ptr, end-ptr, "Encountered fork/execve. crashing"
            " syscall #");
        ptr += __dbg_int2str10_concise(ptr, end - ptr, num);
        *ptr++ = '\n';
        __dbg_print_str(buf, ptr - buf);
      }
      __asm__ volatile ("movl %ebx, 0x0");
      break;
    case TARGET_NR_oldstat:
    case TARGET_NR_oldfstat:
    case TARGET_NR_utime:
    case TARGET_NR_ustat:
    case TARGET_NR_sigaction:
    case TARGET_NR_sigpending:
    case TARGET_NR_setrlimit:
    case TARGET_NR_getrlimit:
    case TARGET_NR_getrusage:
    case TARGET_NR_gettimeofday:
    case TARGET_NR_settimeofday:
    case TARGET_NR_getgroups32:
    case TARGET_NR_setgroups32:
    case TARGET_NR_select:
    case TARGET_NR_oldlstat:
    case TARGET_NR_mmap:
    case TARGET_NR_statfs:
    case TARGET_NR_fstatfs:
    case TARGET_NR_setitimer:
    case TARGET_NR_getitimer:
    case TARGET_NR_stat:
    case TARGET_NR_fstat:
    case TARGET_NR_olduname:
    case TARGET_NR_vm86:
    case TARGET_NR_wait4:
    case TARGET_NR_uname:
    case TARGET_NR_adjtimex:
    case TARGET_NR_sigprocmask:
    case TARGET_NR_init_module:
    case TARGET_NR_get_kernel_syms:
    case TARGET_NR__llseek:
    case TARGET_NR__newselect:
    case TARGET_NR_readv:
    case TARGET_NR_writev:
    case TARGET_NR__sysctl:
    case TARGET_NR_sched_setparam:
    case TARGET_NR_sched_getparam:
    case TARGET_NR_sched_setscheduler:
    case TARGET_NR_sched_getscheduler:
    case TARGET_NR_sched_rr_get_interval:
    case TARGET_NR_nanosleep:
    case TARGET_NR_getresuid32:
    case TARGET_NR_query_module:
    case TARGET_NR_poll:
    case TARGET_NR_getresgid32:
    case TARGET_NR_rt_sigaction:
    case TARGET_NR_rt_sigprocmask:
    case TARGET_NR_rt_sigpending:
    case TARGET_NR_rt_sigtimedwait:
    case TARGET_NR_rt_sigqueueinfo:
    case TARGET_NR_rt_sigsuspend:
    case TARGET_NR_sigaltstack:
    case TARGET_NR_sendfile:
    case TARGET_NR_stat64:
    case TARGET_NR_lstat64:
    case TARGET_NR_fstat64:
    case TARGET_NR_fcntl64:
    case TARGET_NR_statfs64:
    case TARGET_NR_fstatfs64:
    case TARGET_NR_times:
    case TARGET_NR_ioctl:
      if (dyn_debug_fd != -1) {
        char buf[80], *ptr = buf, *end = buf + sizeof buf;
        ptr += __dbg_copy_str(ptr, end - ptr, "Executing syscall. "
            "syscall #");
        ptr += __dbg_int2str10_concise(ptr, end - ptr, num);
        ptr += __dbg_copy_str(ptr, end - ptr, " on (0x");
        ptr += __dbg_int2str_concise(ptr, end - ptr, arg1);
        ptr += __dbg_copy_str(ptr, end - ptr, ",...)\n");
        __dbg_print_str(buf, ptr - buf);
      }
      ret = do_syscall(cpu, num, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
          arg8);
      break;
    default:
      if (dyn_debug_fd != -1) {
        char buf[80], *ptr = buf, *end = buf + sizeof buf;
        ptr += __dbg_copy_str(ptr, end-ptr, "Executing syscall natively. "
            "syscall #");
        ptr += __dbg_int2str10_concise(ptr, end-ptr, num);
        *ptr++ = '\n';
        __dbg_print_str(buf, ptr - buf);
      }
      __asm__ (
          "mov %2, %%ebx\n\t"
          "mov %5, %%esi\n\t"
          "mov %6, %%edi\n\t"
          "mov %3, %%ecx\n\t"
          "mov %4, %%edx\n\t"
          "int $0x80\n\t"
          : "=a" (ret)
          : "a" (num),
          "m" (arg1),
          "m" (arg2),
          "m" (arg3),
          "m" (arg4),
          "m" (arg5)
          : "%ebx", "%ecx", "%edx", "%esi", "%edi");
      break;
  }

#if ARCH_SRC == ARCH_PPC
  if (ret > (uint32_t)(-515)) {
    //cpu->crf[0] |= 0x1;
    *(uint32_t *)(QEMU_ENV_CRF(cpu, 0)) |= 0x1;
    //env->crf |= 0x10000000;		/* set 4th bit from left (xer_so) */
    ret = -ret;
  }
#endif
  return ret;
}

void
cpu_loop_exit(void *env)
{
  src_ulong pc, ret;
  int trapnr;

  /* assume that the env is at SRC_ENV_ADDR (shields us from qemufb changes) */
  env = (void *)SRC_ENV_ADDR;
  trapnr = QEMU_ENV_EXCEPTION_INDEX(env);
  //printf("trapnr = %d\n", trapnr);
  switch(trapnr) {
  case 0x80:
      /* linux syscall */
      ret = __do_syscall(env,
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG0),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG1),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG2),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG3),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG4),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG5),
          *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_ARG6),
          0, 0);
      *(src_ulong *)QEMU_ENV_REG(env, R_SYSCALL_RETVAL) = ret;
      break;
#if 0
  case EXCP0B_NOSEG:
  case EXCP0C_STACK:
      info.si_signo = SIGBUS;
      info.si_errno = 0;
      info.si_code = TARGET_SI_KERNEL;
      info._sifields._sigfault._addr = 0;
      queue_signal(env, info.si_signo, &info);
      break;
  case EXCP0D_GPF:
      /* XXX: potential problem if ABI32 */
#ifndef TARGET_X86_64
      if (env->eflags & VM_MASK) {
          handle_vm86_fault(env);
      } else
#endif
      {
          info.si_signo = SIGSEGV;
          info.si_errno = 0;
          info.si_code = TARGET_SI_KERNEL;
          info._sifields._sigfault._addr = 0;
          queue_signal(env, info.si_signo, &info);
      }
      break;
  case EXCP0E_PAGE:
      info.si_signo = SIGSEGV;
      info.si_errno = 0;
      if (!(env->error_code & 1))
          info.si_code = TARGET_SEGV_MAPERR;
      else
          info.si_code = TARGET_SEGV_ACCERR;
      info._sifields._sigfault._addr = env->cr[2];
      queue_signal(env, info.si_signo, &info);
      break;
  case EXCP00_DIVZ:
#ifndef TARGET_X86_64
      if (env->eflags & VM_MASK) {
          handle_vm86_trap(env, trapnr);
      } else
#endif
      {
          /* division by zero */
          info.si_signo = SIGFPE;
          info.si_errno = 0;
          info.si_code = TARGET_FPE_INTDIV;
          info._sifields._sigfault._addr = env->eip;
          queue_signal(env, info.si_signo, &info);
      }
      break;
  case EXCP01_DB:
  case EXCP03_INT3:
#ifndef TARGET_X86_64
      if (env->eflags & VM_MASK) {
          handle_vm86_trap(env, trapnr);
      } else
#endif
      {
          info.si_signo = SIGTRAP;
          info.si_errno = 0;
          if (trapnr == EXCP01_DB) {
              info.si_code = TARGET_TRAP_BRKPT;
              info._sifields._sigfault._addr = env->eip;
          } else {
              info.si_code = TARGET_SI_KERNEL;
              info._sifields._sigfault._addr = 0;
          }
          queue_signal(env, info.si_signo, &info);
      }
      break;
  case EXCP04_INTO:
  case EXCP05_BOUND:
#ifndef TARGET_X86_64
      if (env->eflags & VM_MASK) {
          handle_vm86_trap(env, trapnr);
      } else
#endif
      {
          info.si_signo = SIGSEGV;
          info.si_errno = 0;
          info.si_code = TARGET_SI_KERNEL;
          info._sifields._sigfault._addr = 0;
          queue_signal(env, info.si_signo, &info);
      }
      break;
  case EXCP06_ILLOP:
      info.si_signo = SIGILL;
      info.si_errno = 0;
      info.si_code = TARGET_ILL_ILLOPN;
      info._sifields._sigfault._addr = env->eip;
      queue_signal(env, info.si_signo, &info);
      break;
  case EXCP_INTERRUPT:
      /* just indicate that signals should be handled asap */
      break;
  case EXCP_DEBUG:
      {
    int sig;

          sig = gdb_handlesig(cs, TARGET_SIGTRAP);
          if (sig)
            {
              info.si_signo = sig;
              info.si_errno = 0;
              info.si_code = TARGET_TRAP_BRKPT;
              queue_signal(env, info.si_signo, &info);
            }
      }
      break;
#endif
  default:
      //pc = env->segs[R_CS].base + env->eip;
      pc = QEMU_ENV_PC(env);
      fprintf(stderr, "qemu: 0x%08lx: unhandled CPU exception 0x%x - aborting\n",
              (long)pc, trapnr);
      abort();
  }
}

int pthread_mutex_lock(void *mutex)
{
}

int pthread_mutex_unlock(void *mutex)
{
}

void object_dynamic_cast_assert(void)
{
  //NOT_REACHED();
}
