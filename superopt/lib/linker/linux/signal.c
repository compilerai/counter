/*
 *  Emulation of Linux signals
 * 
 *  Copyright (c) 2003 Fabrice Bellard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ucontext.h>
#include <stdint.h>
#include "support/c_utils.h"
#include "linker/linux/syscall_defs.h"
#include "linker/linux/syscall.h"


//#include "qemu.h"
//#include "linux/syscall_defs.h"
//#include "cpu-all.h"
//#include "fpu/softfloat.h"


static uint8_t host_to_target_signal_table[65] = {
    [SIGHUP] = TARGET_SIGHUP,
    [SIGINT] = TARGET_SIGINT,
    [SIGQUIT] = TARGET_SIGQUIT,
    [SIGILL] = TARGET_SIGILL,
    [SIGTRAP] = TARGET_SIGTRAP,
    [SIGABRT] = TARGET_SIGABRT,
/*    [SIGIOT] = TARGET_SIGIOT,*/
    [SIGBUS] = TARGET_SIGBUS,
    [SIGFPE] = TARGET_SIGFPE,
    [SIGKILL] = TARGET_SIGKILL,
    [SIGUSR1] = TARGET_SIGUSR1,
    [SIGSEGV] = TARGET_SIGSEGV,
    [SIGUSR2] = TARGET_SIGUSR2,
    [SIGPIPE] = TARGET_SIGPIPE,
    [SIGALRM] = TARGET_SIGALRM,
    [SIGTERM] = TARGET_SIGTERM,
#ifdef SIGSTKFLT
    [SIGSTKFLT] = TARGET_SIGSTKFLT,
#endif
    [SIGCHLD] = TARGET_SIGCHLD,
    [SIGCONT] = TARGET_SIGCONT,
    [SIGSTOP] = TARGET_SIGSTOP,
    [SIGTSTP] = TARGET_SIGTSTP,
    [SIGTTIN] = TARGET_SIGTTIN,
    [SIGTTOU] = TARGET_SIGTTOU,
    [SIGURG] = TARGET_SIGURG,
    [SIGXCPU] = TARGET_SIGXCPU,
    [SIGXFSZ] = TARGET_SIGXFSZ,
    [SIGVTALRM] = TARGET_SIGVTALRM,
    [SIGPROF] = TARGET_SIGPROF,
    [SIGWINCH] = TARGET_SIGWINCH,
    [SIGIO] = TARGET_SIGIO,
    [SIGPWR] = TARGET_SIGPWR,
    [SIGSYS] = TARGET_SIGSYS,
    /* next signals stay the same */
};
static uint8_t target_to_host_signal_table[65];

static inline int host_to_target_signal(int sig)
{
    return host_to_target_signal_table[sig];
}

static inline int target_to_host_signal(int sig)
{
    return target_to_host_signal_table[sig];
}

static void host_to_target_sigset_internal(target_sigset_t *d, 
                                           const sigset_t *s)
{
    int i;
    unsigned long sigmask;
    uint32_t target_sigmask;
    
    sigmask = ((unsigned long *)s)[0];
    target_sigmask = 0;
    for(i = 0; i < 32; i++) {
        if (sigmask & (1 << i)) 
            target_sigmask |= 1 << (host_to_target_signal(i + 1) - 1);
    }
#if SRC_LONG_BITS == 32 && DST_LONG_BITS == 32
    d->sig[0] = target_sigmask;
    for(i = 1;i < TARGET_NSIG_WORDS; i++) {
        d->sig[i] = ((unsigned long *)s)[i];
    }
#elif SRC_LONG_BITS == 32 && DST_LONG_BITS == 64 && TARGET_NSIG_WORDS == 2
    d->sig[0] = target_sigmask;
    d->sig[1] = sigmask >> 32;
#else
#warning host_to_target_sigset
#endif
}

void host_to_target_sigset(target_sigset_t *d, const sigset_t *s)
{
    target_sigset_t d1;
    int i;

    host_to_target_sigset_internal(&d1, s);
    for(i = 0;i < TARGET_NSIG_WORDS; i++)
        d->sig[i] = tswapl(d1.sig[i]);
}

void target_to_host_sigset_internal(sigset_t *d, const target_sigset_t *s)
{
    int i;
    unsigned long sigmask;
    target_ulong target_sigmask;

    target_sigmask = s->sig[0];
    sigmask = 0;
    for(i = 0; i < 32; i++) {
        if (target_sigmask & (1 << i)) 
            sigmask |= 1 << (target_to_host_signal(i + 1) - 1);
    }
#if SRC_LONG_BITS == 32 && DST_LONG_BITS == 32
    ((unsigned long *)d)[0] = sigmask;
    for(i = 1;i < TARGET_NSIG_WORDS; i++) {
        ((unsigned long *)d)[i] = s->sig[i];
    }
#elif SRC_LONG_BITS == 32 && DST_LONG_BITS == 64 && TARGET_NSIG_WORDS == 2
    ((unsigned long *)d)[0] = sigmask | ((unsigned long)(s->sig[1]) << 32);
#else
#warning target_to_host_sigset
#endif /* SRC_LONG_BITS */
}

void target_to_host_sigset(sigset_t *d, const target_sigset_t *s)
{
    target_sigset_t s1;
    int i;

    for(i = 0;i < TARGET_NSIG_WORDS; i++)
        s1.sig[i] = tswapl(s->sig[i]);
    target_to_host_sigset_internal(d, &s1);
}
    
void host_to_target_old_sigset(target_ulong *old_sigset, 
                               const sigset_t *sigset)
{
    target_sigset_t d;
    host_to_target_sigset(&d, sigset);
    *old_sigset = d.sig[0];
}

void target_to_host_old_sigset(sigset_t *sigset, 
                               const target_ulong *old_sigset)
{
    target_sigset_t d;
    int i;

    d.sig[0] = *old_sigset;
    for(i = 1;i < TARGET_NSIG_WORDS; i++)
        d.sig[i] = 0;
    target_to_host_sigset(sigset, &d);
}

/* siginfo conversion */

static inline void host_to_target_siginfo_noswap(target_siginfo_t *tinfo, 
                                                 const siginfo_t *info)
{
    int sig;
    sig = host_to_target_signal(info->si_signo);
    tinfo->si_signo = sig;
    tinfo->si_errno = 0;
    tinfo->si_code = 0;
    if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || 
        sig == SIGBUS || sig == SIGTRAP) {
        /* should never come here, but who knows. The information for
           the target is irrelevant */
        tinfo->_sifields._sigfault._addr = 0;
    } else if (sig >= TARGET_SIGRTMIN) {
        tinfo->_sifields._rt._pid = info->si_pid;
        tinfo->_sifields._rt._uid = info->si_uid;
        /* XXX: potential problem if 64 bit */
        tinfo->_sifields._rt._sigval.sival_ptr = 
            (target_ulong)info->si_value.sival_ptr;
    }
}

static void tswap_siginfo(target_siginfo_t *tinfo, 
                          const target_siginfo_t *info)
{
    int sig;
    sig = info->si_signo;
    tinfo->si_signo = tswap32(sig);
    tinfo->si_errno = tswap32(info->si_errno);
    tinfo->si_code = tswap32(info->si_code);
    if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || 
        sig == SIGBUS || sig == SIGTRAP) {
        tinfo->_sifields._sigfault._addr = 
            tswapl(info->_sifields._sigfault._addr);
    } else if (sig >= TARGET_SIGRTMIN) {
        tinfo->_sifields._rt._pid = tswap32(info->_sifields._rt._pid);
        tinfo->_sifields._rt._uid = tswap32(info->_sifields._rt._uid);
        tinfo->_sifields._rt._sigval.sival_ptr = 
            tswapl(info->_sifields._rt._sigval.sival_ptr);
    }
}


void host_to_target_siginfo(target_siginfo_t *tinfo, const siginfo_t *info)
{
    host_to_target_siginfo_noswap(tinfo, info);
    tswap_siginfo(tinfo, tinfo);
}

/* XXX: we support only POSIX RT signals are used. */
/* XXX: find a solution for 64 bit (additionnal malloced data is needed) */
void target_to_host_siginfo(siginfo_t *info, const target_siginfo_t *tinfo)
{
    info->si_signo = tswap32(tinfo->si_signo);
    info->si_errno = tswap32(tinfo->si_errno);
    info->si_code = tswap32(tinfo->si_code);
    info->si_pid = tswap32(tinfo->_sifields._rt._pid);
    info->si_uid = tswap32(tinfo->_sifields._rt._uid);
    info->si_value.sival_ptr = 
        (void *)tswapl(tinfo->_sifields._rt._sigval.sival_ptr);
}


