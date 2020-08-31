/*
 *  Linux syscalls
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
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <elf.h>
#include <endian.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/swap.h>
#include <signal.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/times.h>
#include <sys/shm.h>
#include <sys/statfs.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "linker/dbg_functions.h"
#include "support/c_utils.h"

#define tget8(addr) ldub(addr)
#define tput8(addr, val) stb(addr, val)
#define tget16(addr) lduw(addr)
#define tput16(addr, val) stw(addr, val)
#define tget32(addr) ldl(addr)
#define tput32(addr, val) stl(addr, val)
#define tget64(addr) ldq(addr)
#define tput64(addr, val) stq(addr, val)
#define tgetl(addr) ldl(addr)
#define tputl(addr, val) stl(addr, val)



#define termios host_termios
#define winsize host_winsize
#define termio host_termio
#define sgttyb host_sgttyb /* same as target */
#define tchars host_tchars /* same as target */
#define ltchars host_ltchars /* same as target */

#include <linux/termios.h>
#include <linux/unistd.h>
#include <linux/utsname.h>
#include <linux/cdrom.h>
#include <linux/hdreg.h>
#include <linux/soundcard.h>
//#include <linux/dirent.h>
#include <dirent.h>
#include <linux/kd.h>

//#include "dbg_functions.h"
#include "linker/linux/thunk.h"
//#include "cpu.h"
#include "linker/linux/syscall_defs.h"
#include "linker/linux/syscall.h"
//#include "qemu.h"


//sorav
#undef HOST_PAGE_ALIGN
#define HOST_PAGE_ALIGN(addr) (((addr) + (1<<12) - 1) & (1<<12))

#define my_gemu_log printf

//#define DEBUG

#define DBGSYS(x,...) do { fprintf(stderr, "%s() %d: " x, __func__, __LINE__, __VA_ARGS__); } while (0)

#undef _syscall0
#undef _syscall1
#undef _syscall2
#undef _syscall3
#undef _syscall4
#undef _syscall5
#undef _syscall6

#define _syscall0(type,name)		\
type name (void)			\
{					\
	return syscall(__NR_##name);	\
}

#define _syscall1(type,name,type1,arg1)		\
type name (type1 arg1)				\
{						\
	return syscall(__NR_##name, arg1);	\
}

#define _syscall2(type,name,type1,arg1,type2,arg2)	\
type name (type1 arg1,type2 arg2)			\
{							\
	return syscall(__NR_##name, arg1, arg2);	\
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)	\
type name (type1 arg1,type2 arg2,type3 arg3)			\
{								\
	return syscall(__NR_##name, arg1, arg2, arg3);		\
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)	\
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4)				\
{										\
	return syscall(__NR_##name, arg1, arg2, arg3, arg4);			\
}

#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,	\
		  type5,arg5)							\
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5)		\
{										\
	return syscall(__NR_##name, arg1, arg2, arg3, arg4, arg5);		\
}


#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,	\
		  type5,arg5,type6,arg6)					\
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6)	\
{										\
	return syscall(__NR_##name, arg1, arg2, arg3, arg4, arg5, arg6);	\
}


#define __NR_sys_rt_sigreturn __NR_rt_sigreturn
#define __NR_sys_uname __NR_uname
#define __NR_sys_getcwd1 __NR_getcwd
#define __NR_sys_getdents __NR_getdents
#define __NR_sys_getdents64 __NR_getdents64
#define __NR_sys_rt_sigqueueinfo __NR_rt_sigqueueinfo

#if defined(__alpha__) || defined (__ia64__) || defined(__x86_64__)
#define __NR__llseek __NR_lseek
#endif

#ifdef __NR_gettid
_syscall0(int, gettid)
#else
static int gettid(void) {
    return -ENOSYS;
}
#endif
_syscall1(int,sys_uname,struct new_utsname *,buf)
_syscall1(int,sys_rt_sigreturn,unsigned long, unused)//sorav
_syscall2(int,sys_getcwd1,char *,buf,size_t,size)
_syscall3(int, sys_getdents, uint, fd, struct dirent *, dirp, uint, count);
_syscall3(int, sys_getdents64, uint, fd, struct dirent64 *, dirp, uint, count);
_syscall5(int, _llseek,  uint,  fd, ulong, hi, ulong, lo,
          loff_t *, res, uint, wh);
_syscall3(int,sys_rt_sigqueueinfo,int,pid,int,sig,siginfo_t *,uinfo)
#ifdef __NR_exit_group
_syscall1(int,exit_group,int,error_code)
#endif

extern int personality(int);
extern int flock(int, int);
extern int setfsuid(int);
extern int setfsgid(int);
extern int setresuid(uid_t, uid_t, uid_t);
extern int getresuid(uid_t *, uid_t *, uid_t *);
extern int setresgid(gid_t, gid_t, gid_t);
extern int getresgid(gid_t *, gid_t *, gid_t *);
extern int setgroups(int, gid_t *);

static inline long get_errno(long ret)
{
    if (ret == -1)
        return -errno;
    else
        return ret;
}

static inline int is_error(long ret)
{
    return (unsigned long)ret >= (unsigned long)(-4096);
}

/* user access */

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define access_ok(type,addr,size) (1)

/* NOTE get_user and put_user use host addresses.  */
#define __put_user(x,ptr)\
({\
    int size = sizeof(*ptr);\
    switch(size) {\
    case 1:\
        *(uint8_t *)(ptr) = (typeof(*ptr))(x);\
        break;\
    case 2:\
        *(uint16_t *)(ptr) = tswap16((typeof(*ptr))(x));\
        break;\
    case 4:\
        *(uint32_t *)(ptr) = tswap32((typeof(*ptr))(x));\
        break;\
    case 8:\
        *(uint64_t *)(ptr) = tswap64((typeof(*ptr))(x));\
        break;\
    default:\
        abort();\
    }\
    0;\
})

#define __get_user(x, ptr) \
({\
    int size = sizeof(*ptr);\
    switch(size) {\
    case 1:\
        x = (typeof(*ptr))*(uint8_t *)(ptr);\
        break;\
    case 2:\
        x = (typeof(*ptr))tswap16(*(uint16_t *)(ptr));\
        break;\
    case 4:\
        x = (typeof(*ptr))tswap32(*(uint32_t *)(ptr));\
        break;\
    case 8:\
        x = (typeof(*ptr))tswap64(*(uint64_t *)(ptr));\
        break;\
    default:\
        abort();\
    }\
    0;\
})

#define put_user(x,ptr)\
({\
    int __ret;\
    if (access_ok(VERIFY_WRITE, ptr, sizeof(*ptr)))\
        __ret = __put_user(x, ptr);\
    else\
        __ret = -EFAULT;\
    __ret;\
})

#define get_user(x,ptr)\
({\
    int __ret;\
    if (access_ok(VERIFY_READ, ptr, sizeof(*ptr)))\
        __ret = __get_user(x, ptr);\
    else\
        __ret = -EFAULT;\
    __ret;\
})



/* Functions for accessing guest memory.  The tget and tput functions
   read/write single values, byteswapping as neccessary.  The lock_user
   gets a pointer to a contiguous area of guest memory, but does not perform
   and byteswapping.  lock_user may return either a pointer to the guest
   memory, or a temporary buffer.  */

/* Lock an area of guest memory into the host.  If copy is true then the
   host area will have the same contents as the guest.  */
static inline void *lock_user(target_ulong guest_addr, long len, int copy)
{
#ifdef DEBUG_REMAP
    void *addr;
    addr = malloc(len);
    if (copy)
        memcpy(addr, g2h(guest_addr), len);
    else
        memset(addr, 0, len);
    return addr;
#else
    return (void *)guest_addr;
#endif
}

/* Unlock an area of guest memory.  The first LEN bytes must be flushed back
   to guest memory.  */
static inline void unlock_user(void *host_addr, target_ulong guest_addr,
                                long len)
{
#ifdef DEBUG_REMAP
    if (host_addr == g2h(guest_addr))
        return;
    if (len > 0)
        memcpy(g2h(guest_addr), host_addr, len);
    free(host_addr);
#endif
}

/* Return the length of a string in target memory.  */
static inline int target_strlen(target_ulong ptr)
{
  return strlen((char *)ptr);
}

/* Like lock_user but for null terminated strings.  */
static inline void *lock_user_string(target_ulong guest_addr)
{
    long len;
    len = target_strlen(guest_addr) + 1;
    return lock_user(guest_addr, len, 1);
}

/* Helper macros for locking/ulocking a target struct.  */
#define lock_user_struct(host_ptr, guest_addr, copy) \
    host_ptr = lock_user(guest_addr, sizeof(*host_ptr), copy)
#define unlock_user_struct(host_ptr, guest_addr, copy) \
    unlock_user(host_ptr, guest_addr, (copy) ? sizeof(*host_ptr) : 0)





static target_ulong target_brk;
static target_ulong target_original_brk;

void target_set_brk(target_ulong new_brk)
{
    target_original_brk = target_brk = new_brk;
}


static inline fd_set *target_to_host_fds(fd_set *fds, 
                                         target_long *target_fds, int n)
{
#if !defined(BSWAP_NEEDED) && !defined(DST_WORDS_BIGENDIAN)
    return (fd_set *)target_fds;
#else
    int i, b;
    if (target_fds) {
        FD_ZERO(fds);
        for(i = 0;i < n; i++) {
            b = (tswapl(target_fds[i / TARGET_LONG_BITS]) >>
                 (i & (TARGET_LONG_BITS - 1))) & 1;
            if (b)
                FD_SET(i, fds);
        }
        return fds;
    } else {
        return NULL;
    }
#endif
}

static inline void host_to_target_fds(target_long *target_fds, 
                                      fd_set *fds, int n)
{
#if !defined(BSWAP_NEEDED) && !defined(DST_WORDS_BIGENDIAN)
    /* nothing to do */
#else
    int i, nw, j, k;
    target_long v;

    if (target_fds) {
        nw = (n + TARGET_LONG_BITS - 1) / TARGET_LONG_BITS;
        k = 0;
        for(i = 0;i < nw; i++) {
            v = 0;
            for(j = 0; j < TARGET_LONG_BITS; j++) {
                v |= ((FD_ISSET(k, fds) != 0) << j);
                k++;
            }
            target_fds[i] = tswapl(v);
        }
    }
#endif
}

#if defined(__alpha__)
#define HOST_HZ 1024
#else
#define HOST_HZ 100
#endif

static inline long host_to_target_clock_t(long ticks)
{
#if HOST_HZ == TARGET_HZ
    return ticks;
#else
    return ((int64_t)ticks * TARGET_HZ) / HOST_HZ;
#endif
}

static inline void host_to_target_rusage(target_ulong target_addr,
                                         const struct rusage *rusage)
{
    struct target_rusage *target_rusage;

    lock_user_struct(target_rusage, target_addr, 0);
    target_rusage->ru_utime.tv_sec = tswapl(rusage->ru_utime.tv_sec);
    target_rusage->ru_utime.tv_usec = tswapl(rusage->ru_utime.tv_usec);
    target_rusage->ru_stime.tv_sec = tswapl(rusage->ru_stime.tv_sec);
    target_rusage->ru_stime.tv_usec = tswapl(rusage->ru_stime.tv_usec);
    target_rusage->ru_maxrss = tswapl(rusage->ru_maxrss);
    target_rusage->ru_ixrss = tswapl(rusage->ru_ixrss);
    target_rusage->ru_idrss = tswapl(rusage->ru_idrss);
    target_rusage->ru_isrss = tswapl(rusage->ru_isrss);
    target_rusage->ru_minflt = tswapl(rusage->ru_minflt);
    target_rusage->ru_majflt = tswapl(rusage->ru_majflt);
    target_rusage->ru_nswap = tswapl(rusage->ru_nswap);
    target_rusage->ru_inblock = tswapl(rusage->ru_inblock);
    target_rusage->ru_oublock = tswapl(rusage->ru_oublock);
    target_rusage->ru_msgsnd = tswapl(rusage->ru_msgsnd);
    target_rusage->ru_msgrcv = tswapl(rusage->ru_msgrcv);
    target_rusage->ru_nsignals = tswapl(rusage->ru_nsignals);
    target_rusage->ru_nvcsw = tswapl(rusage->ru_nvcsw);
    target_rusage->ru_nivcsw = tswapl(rusage->ru_nivcsw);
    unlock_user_struct(target_rusage, target_addr, 1);
}

static inline void target_to_host_timeval(struct timeval *tv,
                                          target_ulong target_addr)
{
    struct target_timeval *target_tv;

    lock_user_struct(target_tv, target_addr, 1);
    tv->tv_sec = tswapl(target_tv->tv_sec);
    tv->tv_usec = tswapl(target_tv->tv_usec);
    unlock_user_struct(target_tv, target_addr, 0);
}

static inline void host_to_target_timeval(target_ulong target_addr,
                                          const struct timeval *tv)
{
    struct target_timeval *target_tv;

    lock_user_struct(target_tv, target_addr, 0);
    target_tv->tv_sec = tswapl(tv->tv_sec);
    target_tv->tv_usec = tswapl(tv->tv_usec);
    unlock_user_struct(target_tv, target_addr, 1);
}


static long do_select(long n, 
                      target_ulong rfd_p, target_ulong wfd_p, 
                      target_ulong efd_p, target_ulong target_tv)
{
    fd_set rfds, wfds, efds;
    fd_set *rfds_ptr, *wfds_ptr, *efds_ptr;
    target_long *target_rfds, *target_wfds, *target_efds;
    struct timeval tv, *tv_ptr;
    long ret;
    int ok;

    if (rfd_p) {
        target_rfds = lock_user(rfd_p, sizeof(target_long) * n, 1);
        rfds_ptr = target_to_host_fds(&rfds, target_rfds, n);
    } else {
        target_rfds = NULL;
        rfds_ptr = NULL;
    }
    if (wfd_p) {
        target_wfds = lock_user(wfd_p, sizeof(target_long) * n, 1);
        wfds_ptr = target_to_host_fds(&wfds, target_wfds, n);
    } else {
        target_wfds = NULL;
        wfds_ptr = NULL;
    }
    if (efd_p) {
        target_efds = lock_user(efd_p, sizeof(target_long) * n, 1);
        efds_ptr = target_to_host_fds(&efds, target_efds, n);
    } else {
        target_efds = NULL;
        efds_ptr = NULL;
    }
            
    if (target_tv) {
        target_to_host_timeval(&tv, target_tv);
        tv_ptr = &tv;
    } else {
        tv_ptr = NULL;
    }
    ret = get_errno(select(n, rfds_ptr, wfds_ptr, efds_ptr, tv_ptr));
    ok = !is_error(ret);

    if (ok) {
        host_to_target_fds(target_rfds, rfds_ptr, n);
        host_to_target_fds(target_wfds, wfds_ptr, n);
        host_to_target_fds(target_efds, efds_ptr, n);

        if (target_tv) {
            host_to_target_timeval(target_tv, &tv);
        }
    }
    if (target_rfds)
        unlock_user(target_rfds, rfd_p, ok ? sizeof(target_long) * n : 0);
    if (target_wfds)
        unlock_user(target_wfds, wfd_p, ok ? sizeof(target_long) * n : 0);
    if (target_efds)
        unlock_user(target_efds, efd_p, ok ? sizeof(target_long) * n : 0);

    return ret;
}

static inline void target_to_host_sockaddr(struct sockaddr *addr,
                                           target_ulong target_addr,
                                           socklen_t len)
{
    struct target_sockaddr *target_saddr;

    target_saddr = lock_user(target_addr, len, 1);
    memcpy(addr, target_saddr, len);
    addr->sa_family = tswap16(target_saddr->sa_family);
    unlock_user(target_saddr, target_addr, 0);
}

static inline void host_to_target_sockaddr(target_ulong target_addr,
                                           struct sockaddr *addr,
                                           socklen_t len)
{
    struct target_sockaddr *target_saddr;

    target_saddr = lock_user(target_addr, len, 0);
    memcpy(target_saddr, addr, len);
    target_saddr->sa_family = tswap16(addr->sa_family);
    unlock_user(target_saddr, target_addr, len);
}

/* ??? Should this also swap msgh->name?  */
static inline void target_to_host_cmsg(struct msghdr *msgh,
                                       struct target_msghdr *target_msgh)
{
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(msgh);
    struct target_cmsghdr *target_cmsg = TARGET_CMSG_FIRSTHDR(target_msgh);
    socklen_t space = 0;

    while (cmsg && target_cmsg) {
        void *data = CMSG_DATA(cmsg);
        void *target_data = TARGET_CMSG_DATA(target_cmsg);

        int len = tswapl(target_cmsg->cmsg_len) 
                  - TARGET_CMSG_ALIGN(sizeof (struct target_cmsghdr));

        space += CMSG_SPACE(len);
        if (space > msgh->msg_controllen) {
            space -= CMSG_SPACE(len);
            my_gemu_log("Host cmsg overflow\n");
            break;
        }

        cmsg->cmsg_level = tswap32(target_cmsg->cmsg_level);
        cmsg->cmsg_type = tswap32(target_cmsg->cmsg_type);
        cmsg->cmsg_len = CMSG_LEN(len);

        if (cmsg->cmsg_level != TARGET_SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
            my_gemu_log("Unsupported ancillary data: %d/%d\n", cmsg->cmsg_level, cmsg->cmsg_type);
            memcpy(data, target_data, len);
        } else {
            int *fd = (int *)data;
            int *target_fd = (int *)target_data;
            int i, numfds = len / sizeof(int);

            for (i = 0; i < numfds; i++)
                fd[i] = tswap32(target_fd[i]);
        }

        cmsg = CMSG_NXTHDR(msgh, cmsg);
        target_cmsg = TARGET_CMSG_NXTHDR(target_msgh, target_cmsg);
    }

    msgh->msg_controllen = space;
}

/* ??? Should this also swap msgh->name?  */
static inline void host_to_target_cmsg(struct target_msghdr *target_msgh,
                                       struct msghdr *msgh)
{
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(msgh);
    struct target_cmsghdr *target_cmsg = TARGET_CMSG_FIRSTHDR(target_msgh);
    socklen_t space = 0;

    while (cmsg && target_cmsg) {
        void *data = CMSG_DATA(cmsg);
        void *target_data = TARGET_CMSG_DATA(target_cmsg);

        int len = cmsg->cmsg_len - CMSG_ALIGN(sizeof (struct cmsghdr));

        space += TARGET_CMSG_SPACE(len);
        if (space > tswapl(target_msgh->msg_controllen)) {
            space -= TARGET_CMSG_SPACE(len);
            my_gemu_log("Target cmsg overflow\n");
            break;
        }

        target_cmsg->cmsg_level = tswap32(cmsg->cmsg_level);
        target_cmsg->cmsg_type = tswap32(cmsg->cmsg_type);
        target_cmsg->cmsg_len = tswapl(TARGET_CMSG_LEN(len));

        if (cmsg->cmsg_level != TARGET_SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
            my_gemu_log("Unsupported ancillary data: %d/%d\n", cmsg->cmsg_level, cmsg->cmsg_type);
            memcpy(target_data, data, len);
        } else {
            int *fd = (int *)data;
            int *target_fd = (int *)target_data;
            int i, numfds = len / sizeof(int);

            for (i = 0; i < numfds; i++)
                target_fd[i] = tswap32(fd[i]);
        }

        cmsg = CMSG_NXTHDR(msgh, cmsg);
        target_cmsg = TARGET_CMSG_NXTHDR(target_msgh, target_cmsg);
    }

    msgh->msg_controllen = tswapl(space);
}

static long do_setsockopt(int sockfd, int level, int optname, 
                          target_ulong optval, socklen_t optlen)
{
    int val, ret;
            
    switch(level) {
    case SOL_TCP:
        /* TCP options all take an 'int' value.  */
        if (optlen < sizeof(uint32_t))
            return -EINVAL;
        
        val = tget32(optval);
        ret = get_errno(setsockopt(sockfd, level, optname, &val, sizeof(val)));
        break;
    case SOL_IP:
        switch(optname) {
        case IP_TOS:
        case IP_TTL:
        case IP_HDRINCL:
        case IP_ROUTER_ALERT:
        case IP_RECVOPTS:
        case IP_RETOPTS:
        case IP_PKTINFO:
        case IP_MTU_DISCOVER:
        case IP_RECVERR:
        case IP_RECVTOS:
#ifdef IP_FREEBIND
        case IP_FREEBIND:
#endif
        case IP_MULTICAST_TTL:
        case IP_MULTICAST_LOOP:
            val = 0;
            if (optlen >= sizeof(uint32_t)) {
                val = tget32(optval);
            } else if (optlen >= 1) {
                val = tget8(optval);
            }
            ret = get_errno(setsockopt(sockfd, level, optname, &val, sizeof(val)));
            break;
        default:
            goto unimplemented;
        }
        break;
    case TARGET_SOL_SOCKET:
        switch (optname) {
            /* Options with 'int' argument.  */
        case TARGET_SO_DEBUG:
		optname = SO_DEBUG;
		break;
        case TARGET_SO_REUSEADDR:
		optname = SO_REUSEADDR;
		break;
        case TARGET_SO_TYPE:
		optname = SO_TYPE;
		break;
        case TARGET_SO_ERROR:
		optname = SO_ERROR;
		break;
        case TARGET_SO_DONTROUTE:
		optname = SO_DONTROUTE;
		break;
        case TARGET_SO_BROADCAST:
		optname = SO_BROADCAST;
		break;
        case TARGET_SO_SNDBUF:
		optname = SO_SNDBUF;
		break;
        case TARGET_SO_RCVBUF:
		optname = SO_RCVBUF;
		break;
        case TARGET_SO_KEEPALIVE:
		optname = SO_KEEPALIVE;
		break;
        case TARGET_SO_OOBINLINE:
		optname = SO_OOBINLINE;
		break;
        case TARGET_SO_NO_CHECK:
		optname = SO_NO_CHECK;
		break;
        case TARGET_SO_PRIORITY:
		optname = SO_PRIORITY;
		break;
#ifdef SO_BSDCOMPAT
        case TARGET_SO_BSDCOMPAT:
		optname = SO_BSDCOMPAT;
		break;
#endif
        case TARGET_SO_PASSCRED:
		optname = SO_PASSCRED;
		break;
        case TARGET_SO_TIMESTAMP:
		optname = SO_TIMESTAMP;
		break;
        case TARGET_SO_RCVLOWAT:
		optname = SO_RCVLOWAT;
		break;
        case TARGET_SO_RCVTIMEO:
		optname = SO_RCVTIMEO;
		break;
        case TARGET_SO_SNDTIMEO:
		optname = SO_SNDTIMEO;
		break;
            break;
        default:
            goto unimplemented;
        }
	if (optlen < sizeof(uint32_t))
	return -EINVAL;

	val = tget32(optval);
	ret = get_errno(setsockopt(sockfd, SOL_SOCKET, optname, &val, sizeof(val)));
        break;
    default:
    unimplemented:
        my_gemu_log("Unsupported setsockopt level=%d optname=%d \n", level, optname);
        ret = -ENOSYS;
    }
    return ret;
}

static long do_getsockopt(int sockfd, int level, int optname, 
                          target_ulong optval, target_ulong optlen)
{
    int len, lv, val, ret;

    switch(level) {
    case TARGET_SOL_SOCKET:
    	level = SOL_SOCKET;
	switch (optname) {
	case TARGET_SO_LINGER:
	case TARGET_SO_RCVTIMEO:
	case TARGET_SO_SNDTIMEO:
	case TARGET_SO_PEERCRED:
	case TARGET_SO_PEERNAME:
	    /* These don't just return a single integer */
	    goto unimplemented;
        default:
            goto int_case;
        }
        break;
    case SOL_TCP:
        /* TCP options all take an 'int' value.  */
    int_case:
        len = tget32(optlen);
        if (len < 0)
            return -EINVAL;
        lv = sizeof(int);
        ret = get_errno(getsockopt(sockfd, level, optname, &val, &lv));
        if (ret < 0)
            return ret;
        val = tswap32(val);
        if (len > lv)
            len = lv;
        if (len == 4)
            tput32(optval, val);
        else
            tput8(optval, val);
        tput32(optlen, len);
        break;
    case SOL_IP:
        switch(optname) {
        case IP_TOS:
        case IP_TTL:
        case IP_HDRINCL:
        case IP_ROUTER_ALERT:
        case IP_RECVOPTS:
        case IP_RETOPTS:
        case IP_PKTINFO:
        case IP_MTU_DISCOVER:
        case IP_RECVERR:
        case IP_RECVTOS:
#ifdef IP_FREEBIND
        case IP_FREEBIND:
#endif
        case IP_MULTICAST_TTL:
        case IP_MULTICAST_LOOP:
            len = tget32(optlen);
            if (len < 0)
                return -EINVAL;
            lv = sizeof(int);
            ret = get_errno(getsockopt(sockfd, level, optname, &val, &lv));
            if (ret < 0)
                return ret;
            if (len < sizeof(int) && len > 0 && val >= 0 && val < 255) {
                len = 1;
                tput32(optlen, len);
                tput8(optval, val);
            } else {
                if (len > sizeof(int))
                    len = sizeof(int);
                tput32(optlen, len);
                tput32(optval, val);
            }
            break;
        default:
            goto unimplemented;
        }
        break;
    default:
    unimplemented:
        my_gemu_log("getsockopt level=%d optname=%d not yet supported\n",
                 level, optname);
        ret = -ENOSYS;
        break;
    }
    return ret;
}

static void lock_iovec(struct iovec *vec, target_ulong target_addr,
                       int count, int copy)
{
    struct target_iovec *target_vec;
    target_ulong base;
    int i;

    target_vec = lock_user(target_addr, count * sizeof(struct target_iovec), 1);
    for(i = 0;i < count; i++) {
        base = tswapl(target_vec[i].iov_base);
        vec[i].iov_len = tswapl(target_vec[i].iov_len);
        vec[i].iov_base = lock_user(base, vec[i].iov_len, copy);
    }
    unlock_user (target_vec, target_addr, 0);
}

static void unlock_iovec(struct iovec *vec, target_ulong target_addr,
                         int count, int copy)
{
    struct target_iovec *target_vec;
    target_ulong base;
    int i;

    target_vec = lock_user(target_addr, count * sizeof(struct target_iovec), 1);
    for(i = 0;i < count; i++) {
        base = tswapl(target_vec[i].iov_base);
        unlock_user(vec[i].iov_base, base, copy ? vec[i].iov_len : 0);
    }
    unlock_user (target_vec, target_addr, 0);
}

static long do_socket(int domain, int type, int protocol)
{
#if defined(TARGET_MIPS)
    switch(type) {
    case TARGET_SOCK_DGRAM:
        type = SOCK_DGRAM;
        break;
    case TARGET_SOCK_STREAM:
        type = SOCK_STREAM;
        break;
    case TARGET_SOCK_RAW:
        type = SOCK_RAW;
        break;
    case TARGET_SOCK_RDM:
        type = SOCK_RDM;
        break;
    case TARGET_SOCK_SEQPACKET:
        type = SOCK_SEQPACKET;
        break;
    case TARGET_SOCK_PACKET:
        type = SOCK_PACKET;
        break;
    }
#endif
    return get_errno(socket(domain, type, protocol));
}

static long do_bind(int sockfd, target_ulong target_addr,
                    socklen_t addrlen)
{
    void *addr = alloca(addrlen);
    
    target_to_host_sockaddr(addr, target_addr, addrlen);
    return get_errno(bind(sockfd, addr, addrlen));
}

static long do_connect(int sockfd, target_ulong target_addr,
                    socklen_t addrlen)
{
    void *addr = alloca(addrlen);
    
    target_to_host_sockaddr(addr, target_addr, addrlen);
    return get_errno(connect(sockfd, addr, addrlen));
}

static long do_sendrecvmsg(int fd, target_ulong target_msg,
                           int flags, int send)
{
    long ret;
    struct target_msghdr *msgp;
    struct msghdr msg;
    int count;
    struct iovec *vec;
    target_ulong target_vec;

    lock_user_struct(msgp, target_msg, 1);
    if (msgp->msg_name) {
        msg.msg_namelen = tswap32(msgp->msg_namelen);
        msg.msg_name = alloca(msg.msg_namelen);
        target_to_host_sockaddr(msg.msg_name, tswapl(msgp->msg_name),
                                msg.msg_namelen);
    } else {
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
    }
    msg.msg_controllen = 2 * tswapl(msgp->msg_controllen);
    msg.msg_control = alloca(msg.msg_controllen);
    msg.msg_flags = tswap32(msgp->msg_flags);
    
    count = tswapl(msgp->msg_iovlen);
    vec = alloca(count * sizeof(struct iovec));
    target_vec = tswapl(msgp->msg_iov);
    lock_iovec(vec, target_vec, count, send);
    msg.msg_iovlen = count;
    msg.msg_iov = vec;
    
    if (send) {
        target_to_host_cmsg(&msg, msgp);
        ret = get_errno(sendmsg(fd, &msg, flags));
    } else {
        ret = get_errno(recvmsg(fd, &msg, flags));
        if (!is_error(ret))
            host_to_target_cmsg(msgp, &msg);
    }
    unlock_iovec(vec, target_vec, count, !send);
    return ret;
}

static long do_socketcall(int num, target_ulong vptr)
{
    long ret;
    const int n = sizeof(target_ulong);

    switch(num) {
    case SOCKOP_socket:
	{
            int domain = tgetl(vptr);
            int type = tgetl(vptr + n);
            int protocol = tgetl(vptr + 2 * n);
            ret = do_socket(domain, type, protocol);
	}
        break;
    case SOCKOP_bind:
	{
            int sockfd = tgetl(vptr);
            target_ulong target_addr = tgetl(vptr + n);
            socklen_t addrlen = tgetl(vptr + 2 * n);
            ret = do_bind(sockfd, target_addr, addrlen);
        }
        break;
    case SOCKOP_connect:
        {
            int sockfd = tgetl(vptr);
            target_ulong target_addr = tgetl(vptr + n);
            socklen_t addrlen = tgetl(vptr + 2 * n);
            ret = do_connect(sockfd, target_addr, addrlen);
        }
        break;
    case SOCKOP_listen:
        {
            int sockfd = tgetl(vptr);
            int backlog = tgetl(vptr + n);
            ret = get_errno(listen(sockfd, backlog));
        }
        break;
    case SOCKOP_accept:
        {
            int sockfd = tgetl(vptr);
            target_ulong target_addr = tgetl(vptr + n);
            target_ulong target_addrlen = tgetl(vptr + 2 * n);
            socklen_t addrlen = tget32(target_addrlen);
            void *addr = alloca(addrlen);

            ret = get_errno(accept(sockfd, addr, &addrlen));
            if (!is_error(ret)) {
                host_to_target_sockaddr(target_addr, addr, addrlen);
                tput32(target_addrlen, addrlen);
            }
        }
        break;
    case SOCKOP_getsockname:
        {
            int sockfd = tgetl(vptr);
            target_ulong target_addr = tgetl(vptr + n);
            target_ulong target_addrlen = tgetl(vptr + 2 * n);
            socklen_t addrlen = tget32(target_addrlen);
            void *addr = alloca(addrlen);

            ret = get_errno(getsockname(sockfd, addr, &addrlen));
            if (!is_error(ret)) {
                host_to_target_sockaddr(target_addr, addr, addrlen);
                tput32(target_addrlen, addrlen);
            }
        }
        break;
    case SOCKOP_getpeername:
        {
            int sockfd = tgetl(vptr);
            target_ulong target_addr = tgetl(vptr + n);
            target_ulong target_addrlen = tgetl(vptr + 2 * n);
            socklen_t addrlen = tget32(target_addrlen);
            void *addr = alloca(addrlen);

            ret = get_errno(getpeername(sockfd, addr, &addrlen));
            if (!is_error(ret)) {
                host_to_target_sockaddr(target_addr, addr, addrlen);
                tput32(target_addrlen, addrlen);
            }
        }
        break;
    case SOCKOP_socketpair:
        {
            int domain = tgetl(vptr);
            int type = tgetl(vptr + n);
            int protocol = tgetl(vptr + 2 * n);
            target_ulong target_tab = tgetl(vptr + 3 * n);
            int tab[2];

            ret = get_errno(socketpair(domain, type, protocol, tab));
            if (!is_error(ret)) {
                tput32(target_tab, tab[0]);
                tput32(target_tab + 4, tab[1]);
            }
        }
        break;
    case SOCKOP_send:
        {
            int sockfd = tgetl(vptr);
            target_ulong msg = tgetl(vptr + n);
            size_t len = tgetl(vptr + 2 * n);
            int flags = tgetl(vptr + 3 * n);
            void *host_msg;

            host_msg = lock_user(msg, len, 1);
            ret = get_errno(send(sockfd, host_msg, len, flags));
            unlock_user(host_msg, msg, 0);
        }
        break;
    case SOCKOP_recv:
        {
            int sockfd = tgetl(vptr);
            target_ulong msg = tgetl(vptr + n);
            size_t len = tgetl(vptr + 2 * n);
            int flags = tgetl(vptr + 3 * n);
            void *host_msg;

            host_msg = lock_user(msg, len, 0);
            ret = get_errno(recv(sockfd, host_msg, len, flags));
            unlock_user(host_msg, msg, ret);
        }
        break;
    case SOCKOP_sendto:
        {
            int sockfd = tgetl(vptr);
            target_ulong msg = tgetl(vptr + n);
            size_t len = tgetl(vptr + 2 * n);
            int flags = tgetl(vptr + 3 * n);
            target_ulong target_addr = tgetl(vptr + 4 * n);
            socklen_t addrlen = tgetl(vptr + 5 * n);
            void *addr = alloca(addrlen);
            void *host_msg;

            host_msg = lock_user(msg, len, 1);
            target_to_host_sockaddr(addr, target_addr, addrlen);
            ret = get_errno(sendto(sockfd, host_msg, len, flags, addr, addrlen));
            unlock_user(host_msg, msg, 0);
        }
        break;
    case SOCKOP_recvfrom:
        {
            int sockfd = tgetl(vptr);
            target_ulong msg = tgetl(vptr + n);
            size_t len = tgetl(vptr + 2 * n);
            int flags = tgetl(vptr + 3 * n);
            target_ulong target_addr = tgetl(vptr + 4 * n);
            target_ulong target_addrlen = tgetl(vptr + 5 * n);
            socklen_t addrlen = tget32(target_addrlen);
            void *addr = alloca(addrlen);
            void *host_msg;

            host_msg = lock_user(msg, len, 0);
            ret = get_errno(recvfrom(sockfd, host_msg, len, flags, addr, &addrlen));
            if (!is_error(ret)) {
                host_to_target_sockaddr(target_addr, addr, addrlen);
                tput32(target_addrlen, addrlen);
                unlock_user(host_msg, msg, len);
            } else {
                unlock_user(host_msg, msg, 0);
            }
        }
        break;
    case SOCKOP_shutdown:
        {
            int sockfd = tgetl(vptr);
            int how = tgetl(vptr + n);

            ret = get_errno(shutdown(sockfd, how));
        }
        break;
    case SOCKOP_sendmsg:
    case SOCKOP_recvmsg:
        {
            int fd;
            target_ulong target_msg;
            int flags;

            fd = tgetl(vptr);
            target_msg = tgetl(vptr + n);
            flags = tgetl(vptr + 2 * n);

            ret = do_sendrecvmsg(fd, target_msg, flags, 
                                 (num == SOCKOP_sendmsg));
        }
        break;
    case SOCKOP_setsockopt:
        {
            int sockfd = tgetl(vptr);
            int level = tgetl(vptr + n);
            int optname = tgetl(vptr + 2 * n);
            target_ulong optval = tgetl(vptr + 3 * n);
            socklen_t optlen = tgetl(vptr + 4 * n);

            ret = do_setsockopt(sockfd, level, optname, optval, optlen);
        }
        break;
    case SOCKOP_getsockopt:
        {
            int sockfd = tgetl(vptr);
            int level = tgetl(vptr + n);
            int optname = tgetl(vptr + 2 * n);
            target_ulong optval = tgetl(vptr + 3 * n);
            target_ulong poptlen = tgetl(vptr + 4 * n);

            ret = do_getsockopt(sockfd, level, optname, optval, poptlen);
        }
        break;
    default:
        my_gemu_log("Unsupported socketcall: %d\n", num);
        ret = -ENOSYS;
        break;
    }
    return ret;
}


/*
#define N_SHM_REGIONS	32

static struct shm_region {
    uint32_t	start;
    uint32_t	size;
} shm_regions[N_SHM_REGIONS];
*/

/* kernel structure types definitions */
#define IFNAMSIZ        16

#define STRUCT(name, list...) STRUCT_ ## name,
#define STRUCT_SPECIAL(name) STRUCT_ ## name,
enum {
#include "linker/linux/syscall_types.h"
};
#undef STRUCT
#undef STRUCT_SPECIAL

#define STRUCT(name, list...) const argtype struct_ ## name ## _def[] = { list, TYPE_NULL };
#define STRUCT_SPECIAL(name)
#include "linker/linux/syscall_types.h"
#undef STRUCT
#undef STRUCT_SPECIAL

#define IOC_R 0x0001
#define IOC_W 0x0002
#define IOC_RW (IOC_R | IOC_W)

#define MAX_STRUCT_SIZE 4096

bitmask_transtbl iflag_tbl[] = {
        { TARGET_IGNBRK, TARGET_IGNBRK, IGNBRK, IGNBRK },
        { TARGET_BRKINT, TARGET_BRKINT, BRKINT, BRKINT },
        { TARGET_IGNPAR, TARGET_IGNPAR, IGNPAR, IGNPAR },
        { TARGET_PARMRK, TARGET_PARMRK, PARMRK, PARMRK },
        { TARGET_INPCK, TARGET_INPCK, INPCK, INPCK },
        { TARGET_ISTRIP, TARGET_ISTRIP, ISTRIP, ISTRIP },
        { TARGET_INLCR, TARGET_INLCR, INLCR, INLCR },
        { TARGET_IGNCR, TARGET_IGNCR, IGNCR, IGNCR },
        { TARGET_ICRNL, TARGET_ICRNL, ICRNL, ICRNL },
        { TARGET_IUCLC, TARGET_IUCLC, IUCLC, IUCLC },
        { TARGET_IXON, TARGET_IXON, IXON, IXON },
        { TARGET_IXANY, TARGET_IXANY, IXANY, IXANY },
        { TARGET_IXOFF, TARGET_IXOFF, IXOFF, IXOFF },
        { TARGET_IMAXBEL, TARGET_IMAXBEL, IMAXBEL, IMAXBEL },
        { 0, 0, 0, 0 }
};

bitmask_transtbl oflag_tbl[] = {
	{ TARGET_OPOST, TARGET_OPOST, OPOST, OPOST },
	{ TARGET_OLCUC, TARGET_OLCUC, OLCUC, OLCUC },
	{ TARGET_ONLCR, TARGET_ONLCR, ONLCR, ONLCR },
	{ TARGET_OCRNL, TARGET_OCRNL, OCRNL, OCRNL },
	{ TARGET_ONOCR, TARGET_ONOCR, ONOCR, ONOCR },
	{ TARGET_ONLRET, TARGET_ONLRET, ONLRET, ONLRET },
	{ TARGET_OFILL, TARGET_OFILL, OFILL, OFILL },
	{ TARGET_OFDEL, TARGET_OFDEL, OFDEL, OFDEL },
	{ TARGET_NLDLY, TARGET_NL0, NLDLY, NL0 },
	{ TARGET_NLDLY, TARGET_NL1, NLDLY, NL1 },
	{ TARGET_CRDLY, TARGET_CR0, CRDLY, CR0 },
	{ TARGET_CRDLY, TARGET_CR1, CRDLY, CR1 },
	{ TARGET_CRDLY, TARGET_CR2, CRDLY, CR2 },
	{ TARGET_CRDLY, TARGET_CR3, CRDLY, CR3 },
	{ TARGET_TABDLY, TARGET_TAB0, TABDLY, TAB0 },
	{ TARGET_TABDLY, TARGET_TAB1, TABDLY, TAB1 },
	{ TARGET_TABDLY, TARGET_TAB2, TABDLY, TAB2 },
	{ TARGET_TABDLY, TARGET_TAB3, TABDLY, TAB3 },
	{ TARGET_BSDLY, TARGET_BS0, BSDLY, BS0 },
	{ TARGET_BSDLY, TARGET_BS1, BSDLY, BS1 },
	{ TARGET_VTDLY, TARGET_VT0, VTDLY, VT0 },
	{ TARGET_VTDLY, TARGET_VT1, VTDLY, VT1 },
	{ TARGET_FFDLY, TARGET_FF0, FFDLY, FF0 },
	{ TARGET_FFDLY, TARGET_FF1, FFDLY, FF1 },
	{ 0, 0, 0, 0 }
};

bitmask_transtbl cflag_tbl[] = {
	{ TARGET_CBAUD, TARGET_B0, CBAUD, B0 },
	{ TARGET_CBAUD, TARGET_B50, CBAUD, B50 },
	{ TARGET_CBAUD, TARGET_B75, CBAUD, B75 },
	{ TARGET_CBAUD, TARGET_B110, CBAUD, B110 },
	{ TARGET_CBAUD, TARGET_B134, CBAUD, B134 },
	{ TARGET_CBAUD, TARGET_B150, CBAUD, B150 },
	{ TARGET_CBAUD, TARGET_B200, CBAUD, B200 },
	{ TARGET_CBAUD, TARGET_B300, CBAUD, B300 },
	{ TARGET_CBAUD, TARGET_B600, CBAUD, B600 },
	{ TARGET_CBAUD, TARGET_B1200, CBAUD, B1200 },
	{ TARGET_CBAUD, TARGET_B1800, CBAUD, B1800 },
	{ TARGET_CBAUD, TARGET_B2400, CBAUD, B2400 },
	{ TARGET_CBAUD, TARGET_B4800, CBAUD, B4800 },
	{ TARGET_CBAUD, TARGET_B9600, CBAUD, B9600 },
	{ TARGET_CBAUD, TARGET_B19200, CBAUD, B19200 },
	{ TARGET_CBAUD, TARGET_B38400, CBAUD, B38400 },
	{ TARGET_CBAUD, TARGET_B57600, CBAUD, B57600 },
	{ TARGET_CBAUD, TARGET_B115200, CBAUD, B115200 },
	{ TARGET_CBAUD, TARGET_B230400, CBAUD, B230400 },
	{ TARGET_CBAUD, TARGET_B460800, CBAUD, B460800 },
	{ TARGET_CSIZE, TARGET_CS5, CSIZE, CS5 },
	{ TARGET_CSIZE, TARGET_CS6, CSIZE, CS6 },
	{ TARGET_CSIZE, TARGET_CS7, CSIZE, CS7 },
	{ TARGET_CSIZE, TARGET_CS8, CSIZE, CS8 },
	{ TARGET_CSTOPB, TARGET_CSTOPB, CSTOPB, CSTOPB },
	{ TARGET_CREAD, TARGET_CREAD, CREAD, CREAD },
	{ TARGET_PARENB, TARGET_PARENB, PARENB, PARENB },
	{ TARGET_PARODD, TARGET_PARODD, PARODD, PARODD },
	{ TARGET_HUPCL, TARGET_HUPCL, HUPCL, HUPCL },
	{ TARGET_CLOCAL, TARGET_CLOCAL, CLOCAL, CLOCAL },
	{ TARGET_CRTSCTS, TARGET_CRTSCTS, CRTSCTS, CRTSCTS },
	{ 0, 0, 0, 0 }
};

bitmask_transtbl lflag_tbl[] = {
	{ TARGET_ISIG, TARGET_ISIG, ISIG, ISIG },
	{ TARGET_ICANON, TARGET_ICANON, ICANON, ICANON },
	{ TARGET_XCASE, TARGET_XCASE, XCASE, XCASE },
	{ TARGET_ECHO, TARGET_ECHO, ECHO, ECHO },
	{ TARGET_ECHOE, TARGET_ECHOE, ECHOE, ECHOE },
	{ TARGET_ECHOK, TARGET_ECHOK, ECHOK, ECHOK },
	{ TARGET_ECHONL, TARGET_ECHONL, ECHONL, ECHONL },
	{ TARGET_NOFLSH, TARGET_NOFLSH, NOFLSH, NOFLSH },
	{ TARGET_TOSTOP, TARGET_TOSTOP, TOSTOP, TOSTOP },
	{ TARGET_ECHOCTL, TARGET_ECHOCTL, ECHOCTL, ECHOCTL },
	{ TARGET_ECHOPRT, TARGET_ECHOPRT, ECHOPRT, ECHOPRT },
	{ TARGET_ECHOKE, TARGET_ECHOKE, ECHOKE, ECHOKE },
	{ TARGET_FLUSHO, TARGET_FLUSHO, FLUSHO, FLUSHO },
	{ TARGET_PENDIN, TARGET_PENDIN, PENDIN, PENDIN },
	{ TARGET_IEXTEN, TARGET_IEXTEN, IEXTEN, IEXTEN },
	{ 0, 0, 0, 0 }
};

static void target_to_host_termios (void *dst, const void *src)
{
    struct host_termios *host = dst;
    const struct target_termios *target = src;
    
    host->c_iflag = 
        target_to_host_bitmask(tswap32(target->c_iflag), iflag_tbl);
    host->c_oflag = 
        target_to_host_bitmask(tswap32(target->c_oflag), oflag_tbl);
    host->c_cflag = 
        target_to_host_bitmask(tswap32(target->c_cflag), cflag_tbl);
    host->c_lflag = 
        target_to_host_bitmask(tswap32(target->c_lflag), lflag_tbl);
    host->c_line = target->c_line;
    
    host->c_cc[VINTR] = target->c_cc[TARGET_VINTR]; 
    host->c_cc[VQUIT] = target->c_cc[TARGET_VQUIT]; 
    host->c_cc[VERASE] = target->c_cc[TARGET_VERASE];       
    host->c_cc[VKILL] = target->c_cc[TARGET_VKILL]; 
    host->c_cc[VEOF] = target->c_cc[TARGET_VEOF];   
    host->c_cc[VTIME] = target->c_cc[TARGET_VTIME]; 
    host->c_cc[VMIN] = target->c_cc[TARGET_VMIN];   
    host->c_cc[VSWTC] = target->c_cc[TARGET_VSWTC]; 
    host->c_cc[VSTART] = target->c_cc[TARGET_VSTART];       
    host->c_cc[VSTOP] = target->c_cc[TARGET_VSTOP]; 
    host->c_cc[VSUSP] = target->c_cc[TARGET_VSUSP]; 
    host->c_cc[VEOL] = target->c_cc[TARGET_VEOL];   
    host->c_cc[VREPRINT] = target->c_cc[TARGET_VREPRINT];   
    host->c_cc[VDISCARD] = target->c_cc[TARGET_VDISCARD];   
    host->c_cc[VWERASE] = target->c_cc[TARGET_VWERASE];     
    host->c_cc[VLNEXT] = target->c_cc[TARGET_VLNEXT];       
    host->c_cc[VEOL2] = target->c_cc[TARGET_VEOL2]; 
}
  
static void host_to_target_termios (void *dst, const void *src)
{
    struct target_termios *target = dst;
    const struct host_termios *host = src;

    target->c_iflag = 
        tswap32(host_to_target_bitmask(host->c_iflag, iflag_tbl));
    target->c_oflag = 
        tswap32(host_to_target_bitmask(host->c_oflag, oflag_tbl));
    target->c_cflag = 
        tswap32(host_to_target_bitmask(host->c_cflag, cflag_tbl));
    target->c_lflag = 
        tswap32(host_to_target_bitmask(host->c_lflag, lflag_tbl));
    target->c_line = host->c_line;
  
    target->c_cc[TARGET_VINTR] = host->c_cc[VINTR];
    target->c_cc[TARGET_VQUIT] = host->c_cc[VQUIT];
    target->c_cc[TARGET_VERASE] = host->c_cc[VERASE];
    target->c_cc[TARGET_VKILL] = host->c_cc[VKILL];
    target->c_cc[TARGET_VEOF] = host->c_cc[VEOF];
    target->c_cc[TARGET_VTIME] = host->c_cc[VTIME];
    target->c_cc[TARGET_VMIN] = host->c_cc[VMIN];
    target->c_cc[TARGET_VSWTC] = host->c_cc[VSWTC];
    target->c_cc[TARGET_VSTART] = host->c_cc[VSTART];
    target->c_cc[TARGET_VSTOP] = host->c_cc[VSTOP];
    target->c_cc[TARGET_VSUSP] = host->c_cc[VSUSP];
    target->c_cc[TARGET_VEOL] = host->c_cc[VEOL];
    target->c_cc[TARGET_VREPRINT] = host->c_cc[VREPRINT];
    target->c_cc[TARGET_VDISCARD] = host->c_cc[VDISCARD];
    target->c_cc[TARGET_VWERASE] = host->c_cc[VWERASE];
    target->c_cc[TARGET_VLNEXT] = host->c_cc[VLNEXT];
    target->c_cc[TARGET_VEOL2] = host->c_cc[VEOL2];
}

StructEntry struct_termios_def = {
    .convert = { host_to_target_termios, target_to_host_termios },
    .size = { sizeof(struct target_termios), sizeof(struct host_termios) },
    .align = { __alignof__(struct target_termios), __alignof__(struct host_termios) },
};

static bitmask_transtbl mmap_flags_tbl[] = {
	{ TARGET_MAP_SHARED, TARGET_MAP_SHARED, MAP_SHARED, MAP_SHARED },
	{ TARGET_MAP_PRIVATE, TARGET_MAP_PRIVATE, MAP_PRIVATE, MAP_PRIVATE },
	{ TARGET_MAP_FIXED, TARGET_MAP_FIXED, MAP_FIXED, MAP_FIXED },
	{ TARGET_MAP_ANONYMOUS, TARGET_MAP_ANONYMOUS, MAP_ANONYMOUS, MAP_ANONYMOUS },
	{ TARGET_MAP_GROWSDOWN, TARGET_MAP_GROWSDOWN, MAP_GROWSDOWN, MAP_GROWSDOWN },
	{ TARGET_MAP_DENYWRITE, TARGET_MAP_DENYWRITE, MAP_DENYWRITE, MAP_DENYWRITE },
	{ TARGET_MAP_EXECUTABLE, TARGET_MAP_EXECUTABLE, MAP_EXECUTABLE, MAP_EXECUTABLE },
	{ TARGET_MAP_LOCKED, TARGET_MAP_LOCKED, MAP_LOCKED, MAP_LOCKED },
	{ 0, 0, 0, 0 }
};

/* this stack is the equivalent of the kernel stack associated with a
   thread/process */
#define NEW_STACK_SIZE 8192

static int clone_func(void *arg)
{
    //CPUState *env = arg;
    //cpu_loop(env);	//XXX
    /* never exits */
    return 0;
}
#ifdef USE_UID16

static inline int high2lowuid(int uid)
{
    if (uid > 65535)
        return 65534;
    else
        return uid;
}

static inline int high2lowgid(int gid)
{
    if (gid > 65535)
        return 65534;
    else
        return gid;
}

static inline int low2highuid(int uid)
{
    if ((int16_t)uid == -1)
        return -1;
    else
        return uid;
}

static inline int low2highgid(int gid)
{
    if ((int16_t)gid == -1)
        return -1;
    else
        return gid;
}

#endif /* USE_UID16 */

static inline uint64_t target_offset64(uint32_t word0, uint32_t word1)
{
#ifdef SRC_WORDS_BIG_ENDIAN
    return ((uint64_t)word0 << 32) | word1;
#else
    return ((uint64_t)word1 << 32) | word0;
#endif
}

#ifdef TARGET_NR_truncate64
static inline long target_truncate64(void *cpu_env, const char *arg1,
                                     long arg2, long arg3, long arg4)
{
#ifdef TARGET_ARM
    if (((CPUARMState *)cpu_env)->eabi)
      {
        arg2 = arg3;
        arg3 = arg4;
      }
#endif
    return get_errno(truncate64(arg1, target_offset64(arg2, arg3)));
}
#endif

#ifdef TARGET_NR_ftruncate64
static inline long target_ftruncate64(void *cpu_env, long arg1, long arg2,
                                      long arg3, long arg4)
{
#ifdef TARGET_ARM
    if (((CPUARMState *)cpu_env)->eabi)
      {
        arg2 = arg3;
        arg3 = arg4;
      }
#endif
    return get_errno(ftruncate64(arg1, target_offset64(arg2, arg3)));
}
#endif

static inline void target_to_host_timespec(struct timespec *host_ts,
                                           target_ulong target_addr)
{
    struct target_timespec *target_ts;

    lock_user_struct(target_ts, target_addr, 1);
    host_ts->tv_sec = tswapl(target_ts->tv_sec);
    host_ts->tv_nsec = tswapl(target_ts->tv_nsec);
    unlock_user_struct(target_ts, target_addr, 0);
}

static inline void host_to_target_timespec(target_ulong target_addr,
                                           struct timespec *host_ts)
{
    struct target_timespec *target_ts;

    lock_user_struct(target_ts, target_addr, 0);
    target_ts->tv_sec = tswapl(host_ts->tv_sec);
    target_ts->tv_nsec = tswapl(host_ts->tv_nsec);
    unlock_user_struct(target_ts, target_addr, 1);
}

#define path(x) (x)


#define	VFAT_IOCTL_READDIR_BOTH		_IOR('r', 1, struct dirent [2])
#define	VFAT_IOCTL_READDIR_SHORT	_IOR('r', 2, struct dirent [2])
typedef struct IOCTLEntry {
    unsigned int target_cmd;
    unsigned int host_cmd;
    const char *name;
    int access;
    const argtype arg_type[5];
} IOCTLEntry;

#define IOC_R 0x0001
#define IOC_W 0x0002
#define IOC_RW (IOC_R | IOC_W)

#define MAX_STRUCT_SIZE 4096

IOCTLEntry ioctl_entries[] = {
#define IOCTL(cmd, access, types...) \
    { TARGET_ ## cmd, cmd, #cmd, access, { types } },
#include "linker/linux/ioctls.h"
    { 0, 0, },
};



/* ??? Implement proper locking for ioctls.  */
static long do_ioctl(long fd, long cmd, long arg)
{
    const IOCTLEntry *ie;
    const argtype *arg_type;
    long ret;
    uint8_t buf_temp[MAX_STRUCT_SIZE];
    int target_size;
    void *argptr;

    ie = ioctl_entries;
    for(;;) {
        if (ie->target_cmd == 0) {
            //my_gemu_log("Unsupported ioctl: cmd=0x%04lx\n", cmd);
            return -ENOSYS;
        }
        if (ie->target_cmd == cmd)
            break;
        ie++;
    }
    arg_type = ie->arg_type;
    printf("ioctl: cmd=0x%04lx (%s)\n", cmd, ie->name);

    switch(arg_type[0]) {
    case TYPE_NULL:
        /* no argument */
        ret = get_errno(ioctl(fd, ie->host_cmd));
        break;
    case TYPE_PTRVOID:
    case TYPE_INT:
        /* int argment */
        ret = get_errno(ioctl(fd, ie->host_cmd, arg));
        break;
    case TYPE_PTR:
        arg_type++;
        target_size = thunk_type_size(arg_type, 0);
        switch(ie->access) {
        case IOC_R:
            ret = get_errno(ioctl(fd, ie->host_cmd, buf_temp));
            if (!is_error(ret)) {
                argptr = lock_user(arg, target_size, 0);
                thunk_convert(argptr, buf_temp, arg_type, THUNK_TARGET);
                unlock_user(argptr, arg, target_size);
            }
            break;
        case IOC_W:
            argptr = lock_user(arg, target_size, 1);
            thunk_convert(buf_temp, argptr, arg_type, THUNK_HOST);
            unlock_user(argptr, arg, 0);
            ret = get_errno(ioctl(fd, ie->host_cmd, buf_temp));
            break;
        default:
        case IOC_RW:
            argptr = lock_user(arg, target_size, 1);
            thunk_convert(buf_temp, argptr, arg_type, THUNK_HOST);
            unlock_user(argptr, arg, 0);
            ret = get_errno(ioctl(fd, ie->host_cmd, buf_temp));
            if (!is_error(ret)) {
                argptr = lock_user(arg, target_size, 0);
                thunk_convert(argptr, buf_temp, arg_type, THUNK_TARGET);
                unlock_user(argptr, arg, target_size);
            }
            break;
        }
        break;
    default:
        //gemu_log("Unsupported ioctl type: cmd=0x%04lx type=%d\n", cmd, arg_type[0]);
        ret = -ENOSYS;
        break;
    }
    return ret;
}

static bitmask_transtbl fcntl_flags_tbl[] = {
	{ TARGET_O_ACCMODE,   TARGET_O_WRONLY,    O_ACCMODE,   O_WRONLY,    },
	{ TARGET_O_ACCMODE,   TARGET_O_RDWR,      O_ACCMODE,   O_RDWR,      },
	{ TARGET_O_CREAT,     TARGET_O_CREAT,     O_CREAT,     O_CREAT,     },
	{ TARGET_O_EXCL,      TARGET_O_EXCL,      O_EXCL,      O_EXCL,      },
	{ TARGET_O_NOCTTY,    TARGET_O_NOCTTY,    O_NOCTTY,    O_NOCTTY,    },
	{ TARGET_O_TRUNC,     TARGET_O_TRUNC,     O_TRUNC,     O_TRUNC,     },
	{ TARGET_O_APPEND,    TARGET_O_APPEND,    O_APPEND,    O_APPEND,    },
	{ TARGET_O_NONBLOCK,  TARGET_O_NONBLOCK,  O_NONBLOCK,  O_NONBLOCK,  },
	{ TARGET_O_SYNC,      TARGET_O_SYNC,      O_SYNC,      O_SYNC,      },
	{ TARGET_FASYNC,      TARGET_FASYNC,      FASYNC,      FASYNC,      },
	{ TARGET_O_DIRECTORY, TARGET_O_DIRECTORY, O_DIRECTORY, O_DIRECTORY, },
	{ TARGET_O_NOFOLLOW,  TARGET_O_NOFOLLOW,  O_NOFOLLOW,  O_NOFOLLOW,  },
	{ TARGET_O_LARGEFILE, TARGET_O_LARGEFILE, O_LARGEFILE, O_LARGEFILE, },
#if defined(O_DIRECT)
	{ TARGET_O_DIRECT,    TARGET_O_DIRECT,    O_DIRECT,    O_DIRECT,    },
#endif
	{ 0, 0, 0, 0 }
};



static long do_fcntl(int fd, int cmd, target_ulong arg)
{
    struct flock fl;
    struct target_flock *target_fl;
    long ret;

    switch(cmd) {
    case TARGET_F_GETLK:
        ret = fcntl(fd, cmd, &fl);
        if (ret == 0) {
            lock_user_struct(target_fl, arg, 0);
            target_fl->l_type = tswap16(fl.l_type);
            target_fl->l_whence = tswap16(fl.l_whence);
            target_fl->l_start = tswapl(fl.l_start);
            target_fl->l_len = tswapl(fl.l_len);
            target_fl->l_pid = tswapl(fl.l_pid);
            unlock_user_struct(target_fl, arg, 1);
        }
        break;
        
    case TARGET_F_SETLK:
    case TARGET_F_SETLKW:
        lock_user_struct(target_fl, arg, 1);
        fl.l_type = tswap16(target_fl->l_type);
        fl.l_whence = tswap16(target_fl->l_whence);
        fl.l_start = tswapl(target_fl->l_start);
        fl.l_len = tswapl(target_fl->l_len);
        fl.l_pid = tswapl(target_fl->l_pid);
        unlock_user_struct(target_fl, arg, 0);
        ret = fcntl(fd, cmd, &fl);
        break;
        
    case TARGET_F_GETLK64:
    case TARGET_F_SETLK64:
    case TARGET_F_SETLKW64:
        ret = -1;
        errno = EINVAL;
        break;

    case F_GETFL:
        ret = fcntl(fd, cmd, arg);
        ret = host_to_target_bitmask(ret, fcntl_flags_tbl);
        break;

    case F_SETFL:
        ret = fcntl(fd, cmd, target_to_host_bitmask(arg, fcntl_flags_tbl));
        break;

    default:
        ret = fcntl(fd, cmd, arg);
        break;
    }
    return ret;
}




long do_syscall(void *cpu_env, int num, long arg1, long arg2, long arg3, 
                long arg4, long arg5, long arg6, long arg7, long arg8)
{
    long ret;
    struct stat st;
    struct statfs stfs;
    void *p;
    
    switch(num) {
    case TARGET_NR_oldstat:
        goto unimplemented;
    case TARGET_NR_oldfstat:
        goto unimplemented;
    case TARGET_NR_utime:
        {
            struct utimbuf tbuf, *host_tbuf;
            struct target_utimbuf *target_tbuf;
            if (arg2) {
                lock_user_struct(target_tbuf, arg2, 1);
                tbuf.actime = tswapl(target_tbuf->actime);
                tbuf.modtime = tswapl(target_tbuf->modtime);
                unlock_user_struct(target_tbuf, arg2, 0);
                host_tbuf = &tbuf;
            } else {
                host_tbuf = NULL;
            }
            p = lock_user_string(arg1);
            ret = get_errno(utime(p, host_tbuf));
            unlock_user(p, arg1, 0);
        }
        break;
    case TARGET_NR_ustat:
        goto unimplemented;
    case TARGET_NR_sigaction:
        {
            struct target_old_sigaction *old_act;
            struct target_sigaction act, oact, *pact;
            memset(&oact, 0, sizeof oact);
            if (arg2) {
                lock_user_struct(old_act, arg2, 1);
                act._sa_handler = old_act->_sa_handler;
                target_siginitset(&act.sa_mask, old_act->sa_mask);
                act.sa_flags = old_act->sa_flags;
                act.sa_restorer = old_act->sa_restorer;
                unlock_user_struct(old_act, arg2, 0);
                pact = &act;
            } else {
                pact = NULL;
            }
            ret = 0;
            //ret = get_errno(do_sigaction(arg1, pact, &oact));
            if (!is_error(ret) && arg3) {
                lock_user_struct(old_act, arg3, 0);
                old_act->_sa_handler = oact._sa_handler;
                old_act->sa_mask = oact.sa_mask.sig[0];
                old_act->sa_flags = oact.sa_flags;
                old_act->sa_restorer = oact.sa_restorer;
                unlock_user_struct(old_act, arg3, 1);
            }
        }
        break;
    case TARGET_NR_sigpending:
        {
            sigset_t set;
            ret = get_errno(sigpending(&set));
            if (!is_error(ret)) {
                p = lock_user(arg1, sizeof(target_sigset_t), 0);
                host_to_target_old_sigset(p, &set);
                unlock_user(p, arg1, sizeof(target_sigset_t));
            }
        }
        break;
    case TARGET_NR_setrlimit:
        {
            /* XXX: convert resource ? */
            int resource = arg1;
            struct target_rlimit *target_rlim;
            struct rlimit rlim;
            lock_user_struct(target_rlim, arg2, 1);
            rlim.rlim_cur = tswapl(target_rlim->rlim_cur);
            rlim.rlim_max = tswapl(target_rlim->rlim_max);
            unlock_user_struct(target_rlim, arg2, 0);
            ret = get_errno(setrlimit(resource, &rlim));
        }
        break;
    case TARGET_NR_getrlimit:
        {
            /* XXX: convert resource ? */
            int resource = arg1;
            struct target_rlimit *target_rlim;
            struct rlimit rlim;
            
            ret = get_errno(getrlimit(resource, &rlim));
            if (!is_error(ret)) {
                lock_user_struct(target_rlim, arg2, 0);
                rlim.rlim_cur = tswapl(target_rlim->rlim_cur);
                rlim.rlim_max = tswapl(target_rlim->rlim_max);
                unlock_user_struct(target_rlim, arg2, 1);
            }
        }
        break;
    case TARGET_NR_getrusage:
        {
            struct rusage rusage;
            ret = get_errno(getrusage(arg1, &rusage));
            if (!is_error(ret)) {
                host_to_target_rusage(arg2, &rusage);
            }
        }
        break;
    case TARGET_NR_gettimeofday:
        {
            struct timeval tv;
            ret = get_errno(gettimeofday(&tv, NULL));
            if (!is_error(ret)) {
                host_to_target_timeval(arg1, &tv);
            }
        }
        break;
    case TARGET_NR_settimeofday:
        {
            struct timeval tv;
            target_to_host_timeval(&tv, arg1);
            ret = get_errno(settimeofday(&tv, NULL));
        }
        break;
#ifdef TARGET_NR_select
    case TARGET_NR_select:
        {
            struct target_sel_arg_struct *sel;
            target_ulong inp, outp, exp, tvp;
            long nsel;

            lock_user_struct(sel, arg1, 1);
            nsel = tswapl(sel->n);
            inp = tswapl(sel->inp);
            outp = tswapl(sel->outp);
            exp = tswapl(sel->exp);
            tvp = tswapl(sel->tvp);
            unlock_user_struct(sel, arg1, 0);
            //ret = do_select(nsel, inp, outp, exp, tvp);
            ret = select(nsel, (fd_set *)inp, (fd_set *)outp,
		(fd_set *)exp, (struct timeval *)tvp);
        }
        break;
#endif
#ifdef TARGET_NR_getgroups32
    case TARGET_NR_getgroups32:
        {
            int gidsetsize = arg1;
            uint32_t *target_grouplist;
            gid_t *grouplist;
            int i;

            grouplist = alloca(gidsetsize * sizeof(gid_t));
            ret = get_errno(getgroups(gidsetsize, grouplist));
            if (!is_error(ret)) {
                target_grouplist = lock_user(arg2, gidsetsize * 4, 0);
                for(i = 0;i < gidsetsize; i++)
                    target_grouplist[i] = tswap32(grouplist[i]);
                unlock_user(target_grouplist, arg2, gidsetsize * 4);
            }
        }
        break;
#endif
#ifdef TARGET_NR_setgroups32
    case TARGET_NR_setgroups32:
        {
            int gidsetsize = arg1;
            uint32_t *target_grouplist;
            gid_t *grouplist;
            int i;
            
            grouplist = alloca(gidsetsize * sizeof(gid_t));
            target_grouplist = lock_user(arg2, gidsetsize * 4, 1);
            for(i = 0;i < gidsetsize; i++)
                grouplist[i] = tswap32(target_grouplist[i]);
            unlock_user(target_grouplist, arg2, 0);
            ret = get_errno(setgroups(gidsetsize, grouplist));
        }
        break;
#endif
#ifdef TARGET_NR_oldlstat
    case TARGET_NR_oldlstat:
        goto unimplemented;
#endif
    case TARGET_NR_mmap:
        //ret = get_errno(target_mmap(arg1, arg2, arg3, 
        ret = get_errno((int)mmap((void *)arg1, arg2, arg3, 
                                    target_to_host_bitmask(arg4, mmap_flags_tbl), 
                                    arg5,
                                    arg6));
        break;
    case TARGET_NR_statfs:
        p = lock_user_string(arg1);
        ret = get_errno(statfs(path(p), &stfs));
        unlock_user(p, arg1, 0);
    convert_statfs:
        if (!is_error(ret)) {
            struct target_statfs *target_stfs;
            
            lock_user_struct(target_stfs, arg2, 0);
            /* ??? put_user is probably wrong.  */
            put_user(stfs.f_type, &target_stfs->f_type);
            put_user(stfs.f_bsize, &target_stfs->f_bsize);
            put_user(stfs.f_blocks, &target_stfs->f_blocks);
            put_user(stfs.f_bfree, &target_stfs->f_bfree);
            put_user(stfs.f_bavail, &target_stfs->f_bavail);
            put_user(stfs.f_files, &target_stfs->f_files);
            put_user(stfs.f_ffree, &target_stfs->f_ffree);
            put_user(stfs.f_fsid.__val[0], &target_stfs->f_fsid);
            put_user(stfs.f_namelen, &target_stfs->f_namelen);
            unlock_user_struct(target_stfs, arg2, 1);
        }
        break;
    case TARGET_NR_fstatfs:
        ret = get_errno(fstatfs(arg1, &stfs));
        goto convert_statfs;
#ifdef TARGET_NR_statfs64
    case TARGET_NR_statfs64:
        p = lock_user_string(arg1);
        ret = get_errno(statfs(path(p), &stfs));
        unlock_user(p, arg1, 0);
    convert_statfs64:
        if (!is_error(ret)) {
            struct target_statfs64 *target_stfs;
            
            lock_user_struct(target_stfs, arg3, 0);
            /* ??? put_user is probably wrong.  */
            put_user(stfs.f_type, &target_stfs->f_type);
            put_user(stfs.f_bsize, &target_stfs->f_bsize);
            put_user(stfs.f_blocks, &target_stfs->f_blocks);
            put_user(stfs.f_bfree, &target_stfs->f_bfree);
            put_user(stfs.f_bavail, &target_stfs->f_bavail);
            put_user(stfs.f_files, &target_stfs->f_files);
            put_user(stfs.f_ffree, &target_stfs->f_ffree);
            put_user(stfs.f_fsid.__val[0], &target_stfs->f_fsid);
            put_user(stfs.f_namelen, &target_stfs->f_namelen);
            unlock_user_struct(target_stfs, arg3, 0);
        }
        break;
    case TARGET_NR_fstatfs64:
        ret = get_errno(fstatfs(arg1, &stfs));
        goto convert_statfs64;
#endif
    case TARGET_NR_setitimer:
        {
            struct itimerval value, ovalue, *pvalue;

            if (arg2) {
                pvalue = &value;
                target_to_host_timeval(&pvalue->it_interval, 
                                       arg2);
                target_to_host_timeval(&pvalue->it_value, 
                                       arg2 + sizeof(struct target_timeval));
            } else {
                pvalue = NULL;
            }
            ret = get_errno(setitimer(arg1, pvalue, &ovalue));
            if (!is_error(ret) && arg3) {
                host_to_target_timeval(arg3,
                                       &ovalue.it_interval);
                host_to_target_timeval(arg3 + sizeof(struct target_timeval),
                                       &ovalue.it_value);
            }
        }
        break;
    case TARGET_NR_getitimer:
        {
            struct itimerval value;
            
            ret = get_errno(getitimer(arg1, &value));
            if (!is_error(ret) && arg2) {
                host_to_target_timeval(arg2,
                                       &value.it_interval);
                host_to_target_timeval(arg2 + sizeof(struct target_timeval),
                                       &value.it_value);
            }
        }
        break;
    case TARGET_NR_stat:
        p = lock_user_string(arg1);
        ret = get_errno(stat(path(p), &st));
        unlock_user(p, arg1, 0);
        goto do_stat;
    case TARGET_NR_lstat:
        p = lock_user_string(arg1);
        ret = get_errno(lstat(path(p), &st));
        unlock_user(p, arg1, 0);
        goto do_stat;
    case TARGET_NR_fstat:
        {
            ret = get_errno(fstat(arg1, &st));
        do_stat:
            if (!is_error(ret)) {
                struct target_stat *target_st;
                
                lock_user_struct(target_st, arg2, 0);
                target_st->st_dev = tswap16(st.st_dev);
                target_st->st_ino = tswapl(st.st_ino);
#if defined(TARGET_PPC)
                target_st->st_mode = tswapl(st.st_mode); /* XXX: check this */
                target_st->st_uid = tswap32(st.st_uid);
                target_st->st_gid = tswap32(st.st_gid);
#else
                target_st->st_mode = tswap16(st.st_mode);
                target_st->st_uid = tswap16(st.st_uid);
                target_st->st_gid = tswap16(st.st_gid);
#endif
                target_st->st_nlink = tswap16(st.st_nlink);
                target_st->st_rdev = tswap16(st.st_rdev);
                target_st->st_size = tswapl(st.st_size);
                target_st->st_blksize = tswapl(st.st_blksize);
                target_st->st_blocks = tswapl(st.st_blocks);
                target_st->target_st_atime = tswapl(st.st_atime);
                target_st->target_st_mtime = tswapl(st.st_mtime);
                target_st->target_st_ctime = tswapl(st.st_ctime);
                unlock_user_struct(target_st, arg2, 1);
            }
        }
        break;
#ifdef TARGET_NR_oldolduname
    case TARGET_NR_oldolduname:
        goto unimplemented;
#endif
    case TARGET_NR_wait4:
        {
            int status;
            target_long status_ptr = arg2;
            struct rusage rusage, *rusage_ptr;
            target_ulong target_rusage = arg4;
            if (target_rusage)
                rusage_ptr = &rusage;
            else
                rusage_ptr = NULL;
            ret = get_errno(wait4(arg1, &status, arg3, rusage_ptr));
            if (!is_error(ret)) {
                if (status_ptr)
                    tputl(status_ptr, status);
                if (target_rusage) {
                    host_to_target_rusage(target_rusage, &rusage);
                }
            }
        }
        break;
    case TARGET_NR_uname:
        /* no need to transcode because we use the linux syscall */
        {
            struct new_utsname * buf;
    
            lock_user_struct(buf, arg1, 0);
            ret = get_errno(sys_uname(buf));
            if (!is_error(ret)) {
                /* Overrite the native machine name with whatever is being
                   emulated. */
                strcpy (buf->machine, UNAME_MACHINE);
		if (dyn_debug_fd != -1)
		{
		  DBGSYS("syscall #%d (uname): strcpy(%p, %s)\n",
		      TARGET_NR_uname, buf->machine, UNAME_MACHINE);
		}
                /* Allow the user to override the reported release.  */
		/*
                if (qemu_uname_release && *qemu_uname_release)
                  strcpy (buf->release, qemu_uname_release);
		  */
            }
            unlock_user_struct(buf, arg1, 1);
        }
        break;
    case TARGET_NR_adjtimex:
        goto unimplemented;
    case TARGET_NR_sigprocmask:
        {
            int how = arg1;
            sigset_t set, oldset, *set_ptr;
            
            if (arg2) {
                switch(how) {
                case TARGET_SIG_BLOCK:
                    how = SIG_BLOCK;
                    break;
                case TARGET_SIG_UNBLOCK:
                    how = SIG_UNBLOCK;
                    break;
                case TARGET_SIG_SETMASK:
                    how = SIG_SETMASK;
                    break;
                default:
                    ret = -EINVAL;
                    goto fail;
                }
                p = lock_user(arg2, sizeof(target_sigset_t), 1);
                target_to_host_old_sigset(&set, p);
                unlock_user(p, arg2, 0);
                set_ptr = &set;
            } else {
                how = 0;
                set_ptr = NULL;
            }
            ret = get_errno(sigprocmask(arg1, set_ptr, &oldset));
            if (!is_error(ret) && arg3) {
                p = lock_user(arg3, sizeof(target_sigset_t), 0);
                host_to_target_old_sigset(p, &oldset);
                unlock_user(p, arg3, sizeof(target_sigset_t));
            }
        }
        break;
    case TARGET_NR_sigsuspend:
        {
            sigset_t set;
            p = lock_user(arg1, sizeof(target_sigset_t), 1);
            target_to_host_old_sigset(&set, p);
            unlock_user(p, arg1, 0);
            ret = get_errno(sigsuspend(&set));
        }
        break;
    case TARGET_NR_sigreturn:
	assert (0);
#if 0
        /* NOTE: ret is eax, so not transcoding must be done */
        //ret = do_sigreturn(cpu_env);
        ret = sigreturn(cpu_env);
#endif
        break;
    case TARGET_NR_rt_sigreturn:
        /* NOTE: ret is eax, so not transcoding must be done */
        //ret = do_rt_sigreturn(cpu_env);
	{
	unsigned long dummy = 0;
        ret = sys_rt_sigreturn(dummy);
	}
        break;
    case TARGET_NR_init_module:
        goto unimplemented;
    case TARGET_NR_get_kernel_syms:
        goto unimplemented;
    case TARGET_NR__llseek:
        {
#if defined (__x86_64__)
            ret = get_errno(lseek(arg1, ((uint64_t )arg2 << 32) | arg3, arg5));
            tput64(arg4, ret);
#else
            int64_t res;
            ret = get_errno(_llseek(arg1, arg2, arg3, &res, arg5));
            tput64(arg4, res);
#endif
        }
        break;
    case TARGET_NR__newselect:
        //ret = do_select(arg1, arg2, arg3, arg4, arg5);
        ret = select(arg1, (fd_set *)arg2, (fd_set *)arg3, (fd_set *)arg4,
	    (struct timeval *)arg5);
        break;
    case TARGET_NR_readv:
        {
            int count = arg3;
            struct iovec *vec;

            vec = alloca(count * sizeof(struct iovec));
            lock_iovec(vec, arg2, count, 0);
            ret = get_errno(readv(arg1, vec, count));
            unlock_iovec(vec, arg2, count, 1);
        }
        break;
    case TARGET_NR_writev:
        {
            int count = arg3;
            struct iovec *vec;

            vec = alloca(count * sizeof(struct iovec));
            lock_iovec(vec, arg2, count, 1);
            ret = get_errno(writev(arg1, vec, count));
            unlock_iovec(vec, arg2, count, 0);
        }
        break;
    case TARGET_NR__sysctl:
        /* We don't implement this, but ENODIR is always a safe
           return value. */
        return -ENOTDIR;
    case TARGET_NR_sched_setparam:
        {
            struct sched_param *target_schp;
            struct sched_param schp;

            lock_user_struct(target_schp, arg2, 1);
            schp.sched_priority = tswap32(target_schp->sched_priority);
            unlock_user_struct(target_schp, arg2, 0);
            ret = get_errno(sched_setparam(arg1, &schp));
        }
        break;
    case TARGET_NR_sched_getparam:
        {
            struct sched_param *target_schp;
            struct sched_param schp;
            ret = get_errno(sched_getparam(arg1, &schp));
            if (!is_error(ret)) {
                lock_user_struct(target_schp, arg2, 0);
                target_schp->sched_priority = tswap32(schp.sched_priority);
                unlock_user_struct(target_schp, arg2, 1);
            }
        }
        break;
    case TARGET_NR_sched_setscheduler:
        {
            struct sched_param *target_schp;
            struct sched_param schp;
            lock_user_struct(target_schp, arg3, 1);
            schp.sched_priority = tswap32(target_schp->sched_priority);
            unlock_user_struct(target_schp, arg3, 0);
            ret = get_errno(sched_setscheduler(arg1, arg2, &schp));
        }
        break;
    case TARGET_NR_sched_getscheduler:
        ret = get_errno(sched_getscheduler(arg1));
        break;
    case TARGET_NR_sched_rr_get_interval:
        {
            struct timespec ts;
            ret = get_errno(sched_rr_get_interval(arg1, &ts));
            if (!is_error(ret)) {
                host_to_target_timespec(arg2, &ts);
            }
        }
        break;
    case TARGET_NR_nanosleep:
        {
            struct timespec req, rem;
            target_to_host_timespec(&req, arg1);
            ret = get_errno(nanosleep(&req, &rem));
            if (is_error(ret) && arg2) {
                host_to_target_timespec(arg2, &rem);
            }
        }
        break;
#ifdef TARGET_NR_setresuid32
    case TARGET_NR_setresuid32:
        ret = get_errno(setresuid(arg1, arg2, arg3));
        break;
#endif
#ifdef TARGET_NR_getresuid32
    case TARGET_NR_getresuid32:
        {
            uid_t ruid, euid, suid;
            ret = get_errno(getresuid(&ruid, &euid, &suid));
            if (!is_error(ret)) {
                tput32(arg1, tswap32(ruid));
                tput32(arg2, tswap32(euid));
                tput32(arg3, tswap32(suid));
            }
        }
        break;
#endif
#ifdef TARGET_NR_setresgid32
    case TARGET_NR_setresgid32:
        ret = get_errno(setresgid(arg1, arg2, arg3));
        break;
#endif
#ifdef TARGET_NR_getresgid32
    case TARGET_NR_getresgid32:
        {
            gid_t rgid, egid, sgid;
            ret = get_errno(getresgid(&rgid, &egid, &sgid));
            if (!is_error(ret)) {
                tput32(arg1, tswap32(rgid));
                tput32(arg2, tswap32(egid));
                tput32(arg3, tswap32(sgid));
            }
        }
        break;
#endif
    case TARGET_NR_query_module:
        goto unimplemented;
    case TARGET_NR_poll:
        {
            struct target_pollfd *target_pfd;
            unsigned int nfds = arg2;
            int timeout = arg3;
            struct pollfd *pfd;
            unsigned int i;

            target_pfd = lock_user(arg1, sizeof(struct target_pollfd) * nfds, 1);
            pfd = alloca(sizeof(struct pollfd) * nfds);
            for(i = 0; i < nfds; i++) {
                pfd[i].fd = tswap32(target_pfd[i].fd);
                pfd[i].events = tswap16(target_pfd[i].events);
            }
            ret = get_errno(poll(pfd, nfds, timeout));
            if (!is_error(ret)) {
                for(i = 0; i < nfds; i++) {
                    target_pfd[i].revents = tswap16(pfd[i].revents);
                }
                ret += nfds * (sizeof(struct target_pollfd)
                               - sizeof(struct pollfd));
            }
            unlock_user(target_pfd, arg1, ret);
        }
        break;
    case TARGET_NR_rt_sigaction:
        {
            struct target_sigaction *act;
            struct target_sigaction *oact;

            if (arg2)
                lock_user_struct(act, arg2, 1);
            else
                act = NULL;
            if (arg3)
                lock_user_struct(oact, arg3, 0);
            else
                oact = NULL;
	    ret = 0;
            //ret = get_errno(do_sigaction(arg1, act, oact));//XXX
            if (arg2)
                unlock_user_struct(act, arg2, 0);
            if (arg3)
                unlock_user_struct(oact, arg3, 1);
        }
        break;
    case TARGET_NR_rt_sigprocmask:
        {
            int how = arg1;
            sigset_t set, oldset, *set_ptr;
            
            if (arg2) {
                switch(how) {
                case TARGET_SIG_BLOCK:
                    how = SIG_BLOCK;
                    break;
                case TARGET_SIG_UNBLOCK:
                    how = SIG_UNBLOCK;
                    break;
                case TARGET_SIG_SETMASK:
                    how = SIG_SETMASK;
                    break;
                default:
                    ret = -EINVAL;
                    goto fail;
                }
                p = lock_user(arg2, sizeof(target_sigset_t), 1);
                target_to_host_sigset(&set, p);
                unlock_user(p, arg2, 0);
                set_ptr = &set;
            } else {
                how = 0;
                set_ptr = NULL;
            }
            ret = get_errno(sigprocmask(how, set_ptr, &oldset));
            if (!is_error(ret) && arg3) {
                p = lock_user(arg3, sizeof(target_sigset_t), 0);
                host_to_target_sigset(p, &oldset);
                unlock_user(p, arg3, sizeof(target_sigset_t));
            }
        }
        break;
    case TARGET_NR_rt_sigpending:
        {
            sigset_t set;
            ret = get_errno(sigpending(&set));
            if (!is_error(ret)) {
                p = lock_user(arg1, sizeof(target_sigset_t), 0);
                host_to_target_sigset(p, &set);
                unlock_user(p, arg1, sizeof(target_sigset_t));
            }
        }
        break;
    case TARGET_NR_rt_sigtimedwait:
        {
            sigset_t set;
            struct timespec uts, *puts;
            siginfo_t uinfo;
            
            p = lock_user(arg1, sizeof(target_sigset_t), 1);
            target_to_host_sigset(&set, p);
            unlock_user(p, arg1, 0);
            if (arg3) {
                puts = &uts;
                target_to_host_timespec(puts, arg3);
            } else {
                puts = NULL;
            }
            ret = get_errno(sigtimedwait(&set, &uinfo, puts));
            if (!is_error(ret) && arg2) {
                p = lock_user(arg2, sizeof(target_sigset_t), 0);
                host_to_target_siginfo(p, &uinfo);
                unlock_user(p, arg2, sizeof(target_sigset_t));
            }
        }
        break;
    case TARGET_NR_rt_sigqueueinfo:
        {
            siginfo_t uinfo;
            p = lock_user(arg3, sizeof(target_sigset_t), 1);
            target_to_host_siginfo(&uinfo, p);
            unlock_user(p, arg1, 0);
            ret = get_errno(sys_rt_sigqueueinfo(arg1, arg2, &uinfo));
        }
        break;
    case TARGET_NR_rt_sigsuspend:
        {
            sigset_t set;
            p = lock_user(arg1, sizeof(target_sigset_t), 1);
            target_to_host_sigset(&set, p);
            unlock_user(p, arg1, 0);
            ret = get_errno(sigsuspend(&set));
        }
        break;
    case TARGET_NR_sigaltstack:
        goto unimplemented;
    case TARGET_NR_sendfile:
        goto unimplemented;
#ifdef TARGET_NR_stat64
    case TARGET_NR_stat64:
        p = lock_user_string(arg1);
        ret = get_errno(stat(path(p), &st));
        unlock_user(p, arg1, 0);
        goto do_stat64;
#endif
#ifdef TARGET_NR_lstat64
    case TARGET_NR_lstat64:
        p = lock_user_string(arg1);
        ret = get_errno(lstat(path(p), &st));
        unlock_user(p, arg1, 0);
        goto do_stat64;
#endif
#ifdef TARGET_NR_fstat64
    case TARGET_NR_fstat64:
        {
            ret = get_errno(fstat(arg1, &st));
        do_stat64:
            if (!is_error(ret)) {
#ifdef TARGET_ARM
                if (((CPUARMState *)cpu_env)->eabi) {
                    struct target_eabi_stat64 *target_st;
                    lock_user_struct(target_st, arg2, 1);
                    memset(target_st, 0, sizeof(struct target_eabi_stat64));
                    /* put_user is probably wrong.  */
                    put_user(st.st_dev, &target_st->st_dev);
                    put_user(st.st_ino, &target_st->st_ino);
#ifdef TARGET_STAT64_HAS_BROKEN_ST_INO
                    put_user(st.st_ino, &target_st->__st_ino);
#endif
                    put_user(st.st_mode, &target_st->st_mode);
                    put_user(st.st_nlink, &target_st->st_nlink);
                    put_user(st.st_uid, &target_st->st_uid);
                    put_user(st.st_gid, &target_st->st_gid);
                    put_user(st.st_rdev, &target_st->st_rdev);
                    /* XXX: better use of kernel struct */
                    put_user(st.st_size, &target_st->st_size);
                    put_user(st.st_blksize, &target_st->st_blksize);
                    put_user(st.st_blocks, &target_st->st_blocks);
                    put_user(st.st_atime, &target_st->target_st_atime);
                    put_user(st.st_mtime, &target_st->target_st_mtime);
                    put_user(st.st_ctime, &target_st->target_st_ctime);
                    unlock_user_struct(target_st, arg2, 0);
                } else
#endif
                {
                    struct target_stat64 *target_st;
                    lock_user_struct(target_st, arg2, 1);
                    memset(target_st, 0, sizeof(struct target_stat64));
                    /* ??? put_user is probably wrong.  */
                    put_user(st.st_dev, &target_st->st_dev);
                    put_user(st.st_ino, &target_st->st_ino);
#ifdef TARGET_STAT64_HAS_BROKEN_ST_INO
                    put_user(st.st_ino, &target_st->__st_ino);
#endif
                    put_user(st.st_mode, &target_st->st_mode);
                    put_user(st.st_nlink, &target_st->st_nlink);
                    put_user(st.st_uid, &target_st->st_uid);
                    put_user(st.st_gid, &target_st->st_gid);
                    put_user(st.st_rdev, &target_st->st_rdev);
                    /* XXX: better use of kernel struct */
                    put_user(st.st_size, &target_st->st_size);
                    put_user(st.st_blksize, &target_st->st_blksize);
                    put_user(st.st_blocks, &target_st->st_blocks);
                    put_user(st.st_atime, &target_st->target_st_atime);
                    put_user(st.st_mtime, &target_st->target_st_mtime);
                    put_user(st.st_ctime, &target_st->target_st_ctime);
                    unlock_user_struct(target_st, arg2, 0);
                }
            }
        }
        break;
#endif
    case TARGET_NR_times:
        {
            struct target_tms *tmsp;
            struct tms tms;
            ret = get_errno(times(&tms));
            if (arg1) {
                tmsp = lock_user(arg1, sizeof(struct target_tms), 0);
                tmsp->tms_utime = tswapl(host_to_target_clock_t(tms.tms_utime));
                tmsp->tms_stime = tswapl(host_to_target_clock_t(tms.tms_stime));
                tmsp->tms_cutime = tswapl(host_to_target_clock_t(tms.tms_cutime));
                tmsp->tms_cstime = tswapl(host_to_target_clock_t(tms.tms_cstime));
            }
            if (!is_error(ret))
                ret = host_to_target_clock_t(ret);
        }
        break;
    case TARGET_NR_ioctl:
        ret = do_ioctl(arg1, arg2, arg3);
        break;

    case TARGET_NR_fcntl64:
    {
	struct flock64 fl;
	struct target_flock64 *target_fl;

        switch(arg2) {
        case F_GETLK64:
            ret = get_errno(fcntl(arg1, arg2, &fl));
	    if (ret == 0) {
	      lock_user_struct(target_fl, arg3, 0);
	      target_fl->l_type = tswap16(fl.l_type);
	      target_fl->l_whence = tswap16(fl.l_whence);
	      target_fl->l_start = tswap64(fl.l_start);
	      target_fl->l_len = tswap64(fl.l_len);
	      target_fl->l_pid = tswapl(fl.l_pid);
	      unlock_user_struct(target_fl, arg3, 1);
	    }
	    break;

        case F_SETLK64:
        case F_SETLKW64:
            {
                lock_user_struct(target_fl, arg3, 1);
                fl.l_type = tswap16(target_fl->l_type);
                fl.l_whence = tswap16(target_fl->l_whence);
                fl.l_start = tswap64(target_fl->l_start);
                fl.l_len = tswap64(target_fl->l_len);
                fl.l_pid = tswapl(target_fl->l_pid);
                unlock_user_struct(target_fl, arg3, 0);
            }
            ret = get_errno(fcntl(arg1, arg2, &fl));
	    break;
        default:
            ret = get_errno(do_fcntl(arg1, arg2, arg3));
            break;
        }
	break;
    }


    default:
    unimplemented:
        fprintf(stderr, "qemu: Unsupported syscall: %d\n", num);
	assert (0);
        break;
    }
 fail:
    return ret;
}

