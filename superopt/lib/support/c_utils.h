#pragma once
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
//#include "support/debug.h"
#include "config-host.h"
#include "support/bswap.h"
#include "support/consts.h"
//#include "src-defs.h"

#define NOTREACHED_MACRO(...) ({ printf("notreached_macro\n"); NOT_REACHED(); 0; })
#define NOTREACHED_MACRO_RETVAL NOTREACHED_MACRO

#define xstr(x) _xstr(x)
#define _xstr(...) #__VA_ARGS__

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)  tostring(s)
#define tostring(s) #s

#ifndef ERROR
#define ERROR(x) do { printf x; assert(0); } while(0)
#endif


#define PGSIZE	0x1000
#define DEFAULT_STRSZ  1024

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define INT32_TO_INT8(i32) (*((int8_t *)&(i32)))
#define INT32_TO_INT16(i32) (*((int16_t *)&(i32)))

#define UINT32_TO_UINT8(i32) (*((uint8_t *)&(i32)))
#define UINT32_TO_UINT16(i32) (*((uint16_t *)&(i32)))

#define SIGN_EXT_8BIT_TO_32BIT(x) ((((int32_t)(x)) << 24) >> 24)
#define SIGN_EXT_16BIT_TO_32BIT(x) ((((int32_t)(x)) << 16) >> 16)

#define SIGN_EXT_8BIT_TO_64BIT(x) ((((int64_t)(x)) << 56) >> 56)
#define SIGN_EXT_16BIT_TO_64BIT(x) ((((int64_t)(x)) << 48) >> 48)
#define SIGN_EXT_32BIT_TO_64BIT(x) ((((int64_t)(x)) << 32) >> 32)

#define SIGNUM(x) ((((long)(x)) > 0)?1UL:(((long)(x)) < 0)?-1UL:0UL)

/* Yields X rounded up to the nearest multiple of STEP.
   For X >= 0, STEP >= 1 only. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

#undef DIV_ROUND_UP
/* Yields X divided by STEP, rounded up.
   For X >= 0, STEP >= 1 only. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

/* Yields X rounded down to the nearest multiple of STEP.
   For X >= 0, STEP >= 1 only. */
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

/* There is no DIV_ROUND_DOWN.   It would be simply X / STEP. */

#define LSB0(imm) ((unsigned int)imm&0xff)
#define LSB1(imm) (((unsigned int)imm&0xff00)>>8)
#define LSB2(imm) (((unsigned int)imm&0xff0000)>>16)
#define LSB3(imm) (((unsigned int)imm&0xff000000)>>24)

#define WRITE_DWORD(ptr,data) do {					\
	if ((((unsigned int)ptr+3)%PGSIZE)>(((unsigned int)ptr)%PGSIZE))\
	{								\
	    *(unsigned int *)ptr = (unsigned int)data;			\
	}								\
	else {								\
	    unsigned char *chrptr = (unsigned char *)ptr;		\
	    *chrptr++ = LSB0(data);					\
	    *chrptr++ = LSB1(data);					\
	    *chrptr++ = LSB2(data);					\
	    *chrptr++ = LSB3(data);					\
	}								\
} while (0)

#define LSB32(u64) u64
#define MSB32(u64) (u64>>32)

#ifndef __cplusplus

#undef MIN
#undef MAX
/* "Safe" macros for min() and max(). See [GCC-ext] 4.1 and 4.6. */
#define MIN(a,b)                                                      \
    ({typeof (a) _a = (a); typeof (b) _b = (b);                       \
         _a < _b ? _a : _b;                                           \
        })
#define MAX(a,b)                                                      \
    ({typeof (a) _a = (a); typeof(b) _b = (b);                        \
         _a > _b ? _a : _b;                                           \
        })


#define SWAP(a,b) do { \
    typeof (a) _a = (a); typeof(b) _b = (b); typeof (a) _tmp; \
    _tmp = _a;   _a = _b;   _b = _tmp; \
} while(0)

/*#define IMAX(a,b) (((a)>(b))?(a):(b))
#define IMIN(a,b) (((a)<(b))?(a):(b))*/

#ifndef min
#define min MIN
#endif

#ifndef max
#define max MAX
#endif

#ifndef SQR
#define SQR(a) ((a)*(a))
#endif

#endif

#ifndef ABS
#define ABS(a) ((a)<0?(-(a)):(a))
#endif



char *chop(char *str);
bool is_hex_chr(char c);
#ifdef __cplusplus
extern "C"
#endif
char *hex_string(uint8_t const *binstr, size_t len, char *buf, size_t size);
unsigned char *read_hex_string(unsigned char *str, short *len, char const *ascii);
size_t xread_file(char *buf, size_t buf_size, char const *filename);
void srand_random(long seed);
unsigned int rand_unsigned_int (void);
unsigned long long rand_unsigned_ll (void);
char rand_alpha (void);
int acquire_file_lock(const char *filename);
void release_file_lock(const char *filename);
bool file_exists(const char *filename);
off_t file_length(int fd);
void raw_copy(char const *ofile, char const *ifile);
void file_append(char const *ofile, char const *ifile);
long long Perm(int N, int i);
int parity (int num);
int find_lsb_zero (uint64_t val);
#ifdef __cplusplus
extern "C" bool strstart(const char *str, const char *val, const char **ptr);
#else
bool strstart(const char *str, const char *val, const char **ptr);
#endif
bool str_ends(const char *str, const char *val, const char **ptr);
int is_whitespace (char const *ptr);
//void strip_extra_whitespace(char *dst, char const *src);
int log_base2(long long n);
void strip_whitespace_suffix(char *ptr);
void compute_n_ones_table (unsigned int *tab, int size);
int count_ones(uint64_t llx);
#ifdef __cplusplus
extern "C" size_t strlcpy (char *dst, const char *src, size_t size);
#else
size_t strlcpy (char *dst, const char *src, size_t size);
#endif
//pid_t xfork(char const *path, char const * const args[], char const *redir);
//pid_t xforkfd(char const *path, char const * const args[], int fd);
bool xsystem_exec(char const *path, char const * const args[], char const *redir);
bool xsystemfd_exec(char const *path, char const * const args[], int fd);
void xsystem(char const *command);
unsigned long highest_order_bit(uint64_t x);
long lowest_order_bit(uint64_t x);

uint64_t ADD(uint64_t a, uint64_t b);
uint64_t SIGN_EXT(uint64_t x, int64_t n);
uint64_t ZERO_EXT(uint64_t x, int64_t n);
uint64_t UMASK(uint64_t a, int64_t b);
uint64_t MASK(uint64_t a, int64_t b);
uint64_t MASKBITS(uint64_t mask_begin, uint64_t mask_end);

uint64_t LSHIFT(uint64_t a, int64_t b);
uint64_t ASHIFT(uint64_t a, int64_t b);

bool streq_arr(char const *buf, char const *arr[], int n);
char const *strstr_arr(char const *buf, char const *arr[], int n);
char *get_cur_timestamp(char *buf, size_t size);
uint64_t time_since_beginning(char const *timestamp);
void list_all_dirents(char ***ids, char const *path, char const *prefix);
void free_listing(char **ids);
bool put_string(char const *filename, char const *str);
bool append_string(char const *filename, char const *str);
bool get_string(char const *filename, char *str, size_t str_size);
void free_array(void **strings, size_t n);

char *strbchr(char const *str, int c, char const *start);
char const *array_to_string(uint32_t const *a, long n, char *buf, size_t size);
char const *long_array_to_string(long const *a, long n, char *buf, size_t size);

void strip_extra_whitespace_and_comments(char *dst, char const *src);

char const *strip_path(char const *pathname);

long radix_power(long base, long exp);
void radix_break(long val, long base, long *arr, size_t arr_size);

void *malloc32(size_t size);
void free32(void *uptr);

FILE *xfopen(char const *filename, char const *mode);
#ifdef __cplusplus
extern "C"
#endif
void xfwrite(char const *buf, size_t size, size_t nmemb, FILE *stream);
void xfread(void *buf, size_t size, size_t nmemb, FILE *stream);
void transfer_contents(FILE *ofp, FILE *ifp);
size_t grep_count(char const *file, char const *pattern);
char *strunc(char const *string, size_t trunc_len, char *buf, size_t size);
size_t longest_prefix_len(char const *a, char const *b);

void debug_backtrace(void **frame_address);

bool is_power_of_two(long long val, unsigned *bitpos);
unsigned long long round_up_to_power_of_two(unsigned long long val);

void self_cartesian_product_init(long *buf, size_t size, long max);
bool self_cartesian_product_next(long *buf, size_t size, long max);

size_t bool_to_string(bool b, char *buf, size_t size);
bool bool_from_string(char const *buf, char const **remaining);
void file_truncate(char const *filename);
void qshuffle(void *base, size_t nmemb, size_t size);
int rmrf(char const *path);

void print_backtrace(void);
void array_print(char *a, size_t s);

#define rand32() ((uint32_t)((uint64_t)rand()*rand()))

extern bool harvest_unsupported_opcodes, canonicalize_imms;
extern bool cfg_eliminate_unreachable_bbls;

static __inline__ unsigned long long rdtsc()
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static inline int
fd_is_valid(int fd)
{
  printf("fd = %d\n", fd);
  fflush(stdout);
  return 1;
  return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

size_t count_occurrences(char const *haystack, char needle);

void disable_signals(void);

#define SWAP_PTRS(a, b) do { const char *__swap_ptrs_tmp = a; a = b; b = __swap_ptrs_tmp; } while(0)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define SUFFIX_CHAR(pathname, delimiter) (strrchr(pathname, delimiter) ? strrchr(pathname, delimiter) + 1 : pathname)
#define SUFFIX_LEN(name, len) (strlen(name) <= len ? name : name + strlen(name) - len)
#define SUFFIX_FILENAME(pathname) SUFFIX_CHAR(pathname, '/')
#define SUFFIX_FILENAME32(pathname) SUFFIX_LEN(SUFFIX_CHAR(pathname, '/'), 32)
