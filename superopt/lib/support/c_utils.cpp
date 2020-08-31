#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <string.h>
#include <time.h>

#include "support/c_utils.h"
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <ftw.h>
#include <unistd.h>
//#define _XOPEN_SOURCE
#include <time.h>
#include "support/types.h"
#include "support/debug.h"
#include "support/timers.h"

#define READ_SIZE 4096

char *strptime(const char *s, const char *format, struct tm *tm); //should have been declared in time.h, but somehow the compiler emits a warning


bool harvest_unsupported_opcodes = true;
bool canonicalize_imms = true;
bool cfg_eliminate_unreachable_bbls = false;

void
debug_backtrace(void **frame_address)
{
  void **frame;
#define PRINT(...) do {printf(__VA_ARGS__);}while(0)

  PRINT("Call stack:");
  for (frame = frame_address;
      frame != NULL && frame[0] != NULL;
      frame = (void **)frame[0])
    PRINT(" %p", frame[1]);
  PRINT(".\n");
}



char *
chop (char *str)
{
  str[strlen(str)-1] = '\0';
  return str;
}

bool
is_hex_chr(char c)
{
  if (c >= '0' && c <= '9') {
    return true;
  }
  if (c >= 'a' && c <= 'f') {
    return true;
  }
  return false;
}

/* Returns the hex representation of the characters in STR (length LEN) in
   RET. If RET is not specified, then a global static buffer is used.
*/
char *
hex_string(uint8_t const *binstr, size_t len, char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  size_t i;

  *ptr = '\0'; /* start with an empty string */

  for (i = 0; i < len; i++) {
    ptr += snprintf(ptr, end - ptr, "%c0x%hhx", i?',':' ', binstr[i]);
  }
  //ptr += snprintf(ptr, end - ptr, "\n");
  return buf;
}

/* Translate the ASCII string containing space-delimited hex strings into
   characters and store them in STR. Return the number of characters in LEN.
*/
unsigned char *
read_hex_string (unsigned char *str, short *len, char const *ascii)
{
  int i = 0 ;
  int n ;
  while ((n = sscanf (ascii, "%hhx ", &str[i++])) != EOF)
    {
      assert (n == 1) ;
      ascii = strchr (ascii, ' ') ;
      if (!ascii)
	break ;
      else
	ascii++ ;
    }
  *len = i ;
  return str ;
}

/* srand_random(): seeds the RNG with a random seed */
void
srand_random(long seed) {
  if (seed == -1) {
    struct timeval tv ;
    if (gettimeofday (&tv, NULL)==-1) {
      fprintf (stderr,"%s(): gettimeofday call failed. error = (%s)\n",
          __func__,strerror(errno)) ;
      exit (1) ;
    }
    srand ((unsigned int)tv.tv_usec) ;
  } else {
    srand(seed);
  }
}

unsigned int
rand_unsigned_int(void)
{
  unsigned int ret = 0x0 ;
  int i ;
  for (i=0;i<32;i++) {
    int cointoss = rand()%2 ;
    if (cointoss==1) {
      ret |= (0x1<<i) ;
    }
  }
  return ret ;
}

unsigned long long
rand_unsigned_ll (void) {
    unsigned long long ret ;
    unsigned int lsb32 = 0x0 ;
    unsigned int msb32 = 0x0 ;
    int i ;
    for (i=0;i<32;i++) {
	int cointoss = rand()%2 ;
	if (cointoss==1) {
	    lsb32 |= (0x1<<i) ;
	}
    }
    for (i=0;i<32;i++) {
	int cointoss = rand()%2 ;
	if (cointoss==1) {
	    msb32 |= (0x1<<i) ;
	}
    }

    ret = ((unsigned long long)msb32<<32) | lsb32 ;
    return ret ;
}

char
rand_alpha (void) {
    return (97+rand()%26) ;
}

int
acquire_file_lock(const char *filename)
{
    int max_lockfn_size = strlen(filename)+16 ;
    char *lockfn = (char *)malloc(sizeof(char)*max_lockfn_size);
    int fd ;
    snprintf(lockfn,max_lockfn_size,"%s.lockfile",filename) ;
    if ((fd=open(lockfn,O_CREAT|O_EXCL, 0))<0) {
	free(lockfn) ;
	return -1 ;
    }
    free(lockfn) ;
    close(fd) ;
    return 0 ;
}

void
release_file_lock(const char *filename)
{
    int max_lockfn_size = strlen(filename)+16 ;
    char *lockfn = (char *)malloc(sizeof(char)*max_lockfn_size);
    snprintf(lockfn,max_lockfn_size,"%s.lockfile",filename) ;
    remove(lockfn);
    free(lockfn);
}

bool
file_exists(const char *filename) {
  FILE *fp = fopen(filename,"r") ;
  if (fp == NULL) {
    return false;
  }
  fclose (fp) ;
  return true;
}

off_t
file_length(int fd)
{
  struct stat buf ;
  if (fstat(fd, &buf) < 0) {
    return -1 ;
  }
  return buf.st_size;
}

void
raw_copy(char const *ofile, char const *ifile)
{
  unsigned char buffer[READ_SIZE];
  FILE *ifp, *ofp;
  unsigned int num_read;

  ifp = fopen(ifile, "r");

  if (!ifp) {
    ERROR(("%s() %d: Could not open file %s in 'r' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ifile, strerror(errno)));
  }
  ASSERT(ifp);

  remove(ofile);
  ofp = fopen (ofile, "w");

  if (!ofp) {
    fprintf (stderr, "%s() %d: Could not open file %s in 'w' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ofile, strerror(errno));
    ERROR(("%s() %d: Could not open file %s in 'w' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ofile, strerror(errno)));
  }
  assert(ofp);

  do {
    num_read = fread (buffer, 1, READ_SIZE, ifp);
    if (fwrite (buffer, 1, num_read, ofp) != num_read) {
      fprintf (stderr, "%s() %d: Error writing to output file %s. "
	    "error = %s.\n", __func__, __LINE__, ofile, strerror(errno));
      exit (1);
    }
  } while (num_read == READ_SIZE);

  fclose (ifp);
  fclose (ofp);
}

void
file_append(char const *ofile, char const *ifile)
{
  unsigned char buffer[READ_SIZE];
  FILE *ifp, *ofp;
  unsigned int num_read;

  ifp = fopen(ifile, "r");

  if (!ifp) {
    ERROR(("%s() %d: Could not open file %s in 'r' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ifile, strerror(errno)));
  }
  ASSERT(ifp);

  ofp = fopen (ofile, "a");

  if (!ofp) {
    fprintf (stderr, "%s() %d: Could not open file %s in 'a' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ofile, strerror(errno));
    ERROR(("%s() %d: Could not open file %s in 'a' mode. "
	  "error = %s.\n", __FILE__, __LINE__, ofile, strerror(errno)));
  }
  ASSERT(ofp);

  do {
    num_read = fread (buffer, 1, READ_SIZE, ifp);
    if (fwrite (buffer, 1, num_read, ofp) != num_read) {
      fprintf (stderr, "%s() %d: Error writing to output file %s. "
	    "error = %s.\n", __func__, __LINE__, ofile, strerror(errno));
      exit (1);
    }
  } while (num_read == READ_SIZE);

  fclose (ifp);
  fclose (ofp);
}


/* Computes the number of possible ways N different people can sit in I chairs. nPi */
long long
Perm (int N, int i)
{
  long long ret;
  int j;
  ret = 1;
  assert (i <= N);
  for (j = N; j > N-i; j--)
  {
    ret = ret * j;
  }
  return ret;
}

/* Returns 1 if the number of ones in NUM is even. 0 otherwise. */
int
parity (int num)
{
  int num_ones;
  num_ones = 0;

  while (num != 0)
  {
    num = num & (num - 1);
    num_ones++;
  }

  if (num_ones % 2 == 0)
    return 1;
  else
    return 0;
}

/* Return the position of the least significant zero bit in VAL */
int
find_lsb_zero (uint64_t val)
{
  int i;
  for (i = 0; i < 64; i++)
  {
    if (((val >> i) & 0x1) == 0)
      return i;
  }
  return 64;
}

extern "C" bool
strstart(const char *str, const char *val, const char **ptr)
{
    const char *p, *q;
    p = str;
    q = val;
    
    /* eliminate any whitespace in both strings */
    while (*p == ' ') p++;
    while (*q == ' ') q++;


    while (*q != '\0') {
        if (*p != *q)
	{
	  return false;
	}
        p++;
        q++;
    }
    if (ptr)
        *ptr = p;
    return true;
}

bool
str_ends(const char *str, const char *val, const char **ptr)
{
    const char *p, *q;
    p = str + strlen(str) - 1;
    q = val + strlen(val) - 1;
    
    /* eliminate any whitespace in both strings */
    while (*p == ' ') p--;
    while (*q == ' ') q--;


    while (q > val) {
        if (*p != *q)
	{
	  return false;
	}
        p--;
        q--;
    }
    if (ptr)
        *ptr = p;
    return true;
}



int
is_whitespace (char const *ptr)
{
  while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ptr++;
  if (*ptr == '\0')
    return 1;
  else
    return 0;
}

int
log_base2 (long long n)
{
  int i = 0;
  while (n) { i++; n = n>>1; }
  return i;
}

void
strip_whitespace_suffix(char *ptr)
{
  char *end = ptr + strlen(ptr) - 1;
  while (is_whitespace(end))
  {
    *end-- = '\0';
  }
}

void
compute_n_ones_table (unsigned int *tab, int size)
{
  int i;
  for (i = 0; i < size; i++)
  {
    int n = i;
    int count = 0;
    while (n)
    {
      count++;
      n &= (n - 1);
    }
    tab[i] = count;
  }
}

#define ONES_TAB_NB 8
#define ONES_TAB_MASK ((1 << ONES_TAB_NB) - 1)
int
count_ones(uint64_t llx)
{
  static unsigned int *n_ones_table = NULL;
  if (!n_ones_table) {
    int size = (1<<ONES_TAB_NB);
    n_ones_table = (unsigned int *)malloc(size*sizeof(unsigned int));
    assert(n_ones_table);
    compute_n_ones_table(n_ones_table, size);
  }
  unsigned i;
  int ret = 0;
  for (i = 0; i < (sizeof(uint64_t) * 8)/ONES_TAB_NB; i++) {
    ret += n_ones_table[((llx)>>(i*ONES_TAB_NB)) & ONES_TAB_MASK];
  }
  return ret;
}

/* Copies string SRC to DST.  If SRC is longer than SIZE - 1
   characters, only SIZE - 1 characters are copied.  A null
   terminator is always written to DST, unless SIZE is 0.
   Returns the length of SRC, not including the null terminator.

   strlcpy() is not in the standard C library, but it is an
   increasingly popular extension.  See
   http://www.courtesan.com/todd/papers/strlcpy.html for
   information on strlcpy(). */
extern "C" size_t
strlcpy (char *dst, const char *src, size_t size) 
{
  size_t src_len;

  ASSERT (dst != NULL);
  ASSERT (src != NULL);

  src_len = strlen (src);
  if (size > 0) 
    {
      size_t dst_len = size - 1;
      if (src_len < dst_len)
        dst_len = src_len;
      memcpy (dst, src, dst_len);
      dst[dst_len] = '\0';
    }
  return src_len;
}

static pid_t
__xfork(char const *path, char const * const args[], char const *redir, int fd,
    bool redirfd)
{
  pid_t pid;
  int rc;
  
  pid = vfork();

  if (pid) {
    /* parent */
    return pid;
  }

  /* child */
	/* execv must be called immediately after vfork in child (see vfork manpage) */
  if (!redirfd && redir) {
    close(1);
    open(redir, O_WRONLY);
    close(2);
    rc = dup(1);
    ASSERT(rc >= 0);
  } else if (redirfd) {
    ASSERT(fd != -1 && fd != 1 && fd != 2);
    close(1);
    rc = dup(fd);
    close(2);
    ASSERT(rc >= 0);
    rc = dup(fd);
    ASSERT(rc >= 0);
  }
  execv(path, (char *const *)args);
  printf("execv(%s, %s) failed: %s()\n", path, args[0], strerror(errno));
  exit(1);
}

/*pid_t
xfork(char const *path, char const * const args[], char const *redir)
{
  return __xfork(path, args, redir, -1, false);
}*/

/*pid_t
xforkfd(char const *path, char const * const args[], int fd)
{
  return __xfork(path, args, NULL, fd, true);
}*/

static bool
__xsystem_exec(char const *path, char const * const args[], char const *redir, int fd,
    bool redir_fd)
{
  int status;
  pid_t pid;

  pid = __xfork(path, args, redir, fd, redir_fd);
  if (pid < 0) {
    printf("fork failed\n");
    return false;
  }
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    int exit_status;
    exit_status = WEXITSTATUS(status);
    if (exit_status != 0) {
      printf("%s: exit_status = %d\n", args[0], exit_status);
      //exit(1);
      return false;
    }
    return true;
  }
  printf("%s: Process terminated abnormally. status=%d\n", path, status);
  //exit(1);
  return false;
}

bool
xsystem_exec(char const *path, char const * const args[], char const *redir)
{
  return __xsystem_exec(path, args, redir, -1, false);
}

bool
xsystemfd_exec(char const *path, char const * const args[], int fd)
{
  return __xsystem_exec(path, args, NULL, fd, true);
}

bool
streq_arr(char const *buf, char const *arr[], int n)
{
  int i;
  for (i = 0; i < n; i++) {
    if (!strcmp(buf, arr[i])) {
      return true;
    }
  }
  return false;
}

char const *
strstr_arr(char const *buf, char const *arr[], int n)
{
  char const *ret = NULL;
  int i;
  for (i = 0; i < n; i++) {
    char const *ptr;
    if (   (ptr = strstr(buf, arr[i]))
        /*&& !isalpha(ptr[-1])*/)  {
      if (!ret || ptr < ret) {
        ret = ptr;
      }
    }
  }
  return ret;
}

char *
get_cur_timestamp(char *buf, size_t size)
{
  struct tm *tm, tm_store;
  time_t ltime;

  ltime = time(NULL);
  tm = localtime_r(&ltime, &tm_store);
  strftime(buf, size, "%F.%T", tm);
  return buf;
}

uint64_t
time_since_beginning(char const *timestamp)
{
  struct tm tm;
  time_t time;
  tm.tm_isdst = 0;
  strptime(timestamp, "%F.%T", &tm);
  /*printf("timestamp=%s, tm={%d,%d,%d,%d,%d,%d,%d,%d,%d}\n", timestamp,
      tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday,
      tm.tm_yday, tm.tm_isdst);*/
  time = mktime(&tm);
  return (uint64_t)time;
}

void
list_all_dirents(char ***ids, char const *path, char const *prefix)
{
  DIR *dir;
  struct dirent *dirent;

  *ids = (char **)malloc(sizeof (char **));
  ASSERT(*ids);
  int i = 0;
  (*ids)[0] = NULL;
  dir = opendir(path);
  if (!dir) {
    return;
  }
  while ((dirent = readdir(dir))) {
    if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..")) {
      continue;
    }
    if (prefix && !strstart(dirent->d_name, prefix, NULL)) {
      continue;
    }
    *ids = (char **)realloc(*ids, (i + 2) * (sizeof(char **)));
    ASSERT(*ids);
    (*ids)[i] = strdup(dirent->d_name);
    i++;
  }
  (*ids)[i] = NULL;
  closedir(dir);
}

void
free_listing(char **ids)
{
  char **cur;
  for (cur = ids; *cur; cur++) {
    free(*cur);
  }
  free(ids);
}

/*int
array_search(src_ulong const *haystack, size_t n, src_ulong needle)
{
  size_t i;
  for (i = 0; i < n; i++) {
    if (haystack[i] == needle) {
      return i;
    }
  }
  return -1;
}*/

char const *
array_to_string(uint32_t const *a, long n, char *buf, size_t size)
{
  char *ptr, *end;
  long i;

  ptr = buf;
  end = buf + size;
  *ptr = '\0';
  for (i = 0; i < n; i++) {
    ptr += snprintf(ptr, end - ptr, ", %x", a[i]);
    ASSERT(ptr < end);
  }
  return buf;
}

char const *
long_array_to_string(long const *a, long n, char *buf, size_t size)
{
  char *ptr, *end;
  long i;

  ptr = buf;
  end = buf + size;
  *ptr = '\0';
  for (i = 0; i < n; i++) {
    ptr += snprintf(ptr, end - ptr, ", %lx", a[i]);
    ASSERT(ptr < end);
  }
  return buf;
}



bool
put_string(char const *filename, char const *str)
{
  FILE *fp;
  size_t n;

  fp = fopen(filename, "w");
  if (!fp) {
    return false;
  }
  n = fwrite(str, sizeof(char), strlen(str), fp);
  fclose(fp);
  if (n != strlen(str)) {
    return false;
  }
  return true;
}

bool
append_string(char const *filename, char const *str)
{
  FILE *fp;
  size_t n;

  fp = fopen(filename, "a");
  if (!fp) {
    return false;
  }
  n = fwrite(str, sizeof(char), strlen(str), fp);
  fclose(fp);
  if (n != strlen(str)) {
    return false;
  }
  return true;
}

bool
get_string(char const *filename, char *str, size_t str_size)
{
  FILE *fp;
  size_t n;

  fp = fopen(filename, "r");
  if (!fp) {
    return false;
  }
  n = fread(str, sizeof(char), str_size, fp);
  fclose(fp);
  if (n == str_size) {
    return false;
  }
  str[n] = '\0';
  return true;
}

unsigned long
highest_order_bit(uint64_t x)
{
  long i;
  if (x == 0) {
    return 0;
  }
  for (i = 0; i < 64; i++) {
    if (x <= (1ULL << i)) {
      return i;
    }
  }
  return 64;
}

long
lowest_order_bit(uint64_t x)
{
  long i;
  for (i = 0; i < 64; i++) {
    if ((x >> i) & 1ULL) {
      return i;
    }
  }
  return i;
}


typedef struct malloc_hdr_t {
  bool alloced;
  size_t size;
} malloc_hdr_t;
static malloc_hdr_t *region32 = NULL;
#define REGION32_START 0x70000000 //this addr should have msb==0 so that it does not get sign extended in 64-bit mode
#define REGION32_SIZE  0x08000000
void *
malloc32(size_t size)
{
  size_t orig_size, esize;
  malloc_hdr_t *ptr;
  void *ret, *end;

  if (!region32) {
    region32 = (malloc_hdr_t *)mmap((void *)REGION32_START, REGION32_SIZE,
        PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1,
        0);
    ASSERT(region32 != (void *)-1);
    region32->alloced = false;
    region32->size = REGION32_SIZE;
  }
  ASSERT(region32);
  ASSERT(region32 == (void *)REGION32_START);
  ptr = region32;
  end = (uint8_t *)region32 + REGION32_SIZE;
  esize = size + sizeof(malloc_hdr_t);
  DBG(MALLOC32, "malloc32(%zd): esize = %zd\n", size, esize);
  while (   (void *)ptr < end
         && (ptr->alloced || ptr->size < esize)) {
    DBG(MALLOC32, "ptr = %p, ptr->size = %zd\n", ptr, ptr->size);
    ASSERT(ptr >= region32 && (void *)ptr < end);
    ASSERT(ptr->size >= 0 && (uint8_t *)ptr + ptr->size <= end);
    ptr = (malloc_hdr_t *)((uint8_t *)ptr + ptr->size);
  }
  if (ptr == end) {
    DBG(MALLOC32, "returning NULL for size %zd, esize %zd\n", size, esize);
    return NULL;
  }
  ASSERT(!ptr->alloced);
  ASSERT(ptr->size >= 0 && (uint8_t *)ptr + ptr->size <= end);
  ASSERT(ptr->size >= esize);
  orig_size = ptr->size;
  ptr->alloced = true;
  ret = (uint8_t *)ptr + sizeof(malloc_hdr_t);
  if (orig_size - esize >= sizeof(malloc_hdr_t)) {
    //if there is enough space, break the current free list element into
    //two, else just allocate the entire orig_size list element for this request
    ptr->size = esize;
    ptr = (malloc_hdr_t *)((uint8_t *)ptr + esize);
    ptr->alloced = false;
    ptr->size = orig_size - esize;
    ASSERT(ptr->size >= sizeof(malloc_hdr_t) && (uint8_t *)ptr + ptr->size <= end);
  }
  DBG_EXEC(MALLOC32, static long long n = 0; printf("malloced %lld\n", n++););
  DBG(MALLOC32, "returning %p for size %zd, esize %zd\n", ret, size, esize);
  return ret;
}

void
free32(void *uptr)
{
  malloc_hdr_t *ptr, *nextptr, *prevptr;
  void *end;

  end = (uint8_t *)region32 + REGION32_SIZE;

  ASSERT(region32);
  ptr = (malloc_hdr_t *)((uint8_t *)uptr - sizeof(malloc_hdr_t));
  ASSERT(ptr->size >= 0 && (uint8_t *)ptr + ptr->size <= end);
  ptr->alloced = false;

  nextptr = (malloc_hdr_t *)((uint8_t *)ptr + ptr->size);
  DBG(MALLOC32, "free32(%p): ptr = %p, nextptr = %p\n", uptr, ptr, nextptr);
  if ((void *)nextptr < end && nextptr->alloced == false) {
    ASSERT(nextptr->size > 0 && (uint8_t *)nextptr + nextptr->size <= end);
    ptr->size += nextptr->size;
  }

  prevptr = region32;
  while ((uint8_t *)prevptr + prevptr->size < (uint8_t *)ptr) {
    ASSERT(prevptr->size >= 0 && (uint8_t *)prevptr + prevptr->size <= end);
    prevptr = (malloc_hdr_t *)((uint8_t *)prevptr + prevptr->size);
  }
  ASSERT(   (uint8_t *)prevptr + prevptr->size == (void *)ptr
         || (ptr == region32 && prevptr == region32));
  if (prevptr != ptr && prevptr->alloced == false) {
    prevptr->size += ptr->size;
  }
  DBG_EXEC(MALLOC32, static long long n = 0; printf("freed %lld\n", n++););
}

char *
strbchr(char const *str, int c, char const *start)
{
  /* returns the first occurrence of character 'c' starting at str and
     going backwards. */
  char const *ptr;
  ptr = str;
  while (ptr && *ptr != c && ptr > start) {
    ptr--;
  }
  return (char *)ptr;
}

void
strip_extra_whitespace_and_comments(char *dst, char const *src)
{
  char const *sptr, *send;
  char *dptr, schr;

  dptr = dst;
  sptr = src;
  send = src + strlen(src);
  while (sptr < send) {
    schr = *sptr;
    while (schr != ' ' && schr != '\t' && schr != '#' && sptr < send) {
      *dptr = schr;
      dptr++;
      sptr++;
      schr = *sptr;
    }
    if (schr == ' ' || schr == '\t') {
      sptr++;
      schr = *sptr;
      if (dptr != dst) {
        *dptr = ' ';
        dptr++;
      }
    } else if (schr == '#') {
      sptr++;
      schr = *sptr;
      while (schr != '\n' && sptr < send) {
        sptr++;
        schr = *sptr;
      }
      if (dptr > dst && *(dptr - 1) == ' ') {
        dptr--;
      }
    }
    while ((schr == ' ' || schr == '\t') && sptr < send) {
      sptr++;
      schr = *sptr;
    }
  }
  *dptr = '\0';
}

extern "C"
void
xfwrite(char const *buf, size_t size, size_t nmemb, FILE *stream)
{
  char const *ptr, *end;

  if (nmemb == 0) {
    return;
  }

  ptr = buf;
  end = ptr + (nmemb * size);
  do {
    size_t iret;
    iret = fwrite(ptr, size, (end - ptr)/size, stream);
    ASSERT(iret > 0);
    ptr += iret * size;
  } while (ptr < end);
  ASSERT(ptr == end);
}

void
xfread(char *buf, size_t size, size_t nmemb, FILE *stream)
{
  char *ptr, *end;

  ptr = buf;
  end = ptr + (nmemb * size);

  if (nmemb == 0) {
    return;
  }

  do {
    size_t iret;
    iret = fread(ptr, size, (end - ptr)/size, stream);
    ASSERT(iret > 0);
    ptr += iret * size;
  } while (ptr < end);
  ASSERT(ptr == end);
}

size_t
xread_file(char *buf, size_t buf_size, char const *filename)
{
  off_t len;
  FILE *fp;

  fp = fopen(filename, "r");
  ASSERT(fp);

  len = file_length(fileno(fp));
  ASSERT(len < buf_size);
  xfread(buf, sizeof(char), len, fp);
  buf[len] = '\0';
  fclose(fp);
  return len;
}

void
transfer_contents(FILE *ofp, FILE *ifp)
{
#ifndef READ_SIZE
#define READ_SIZE 4096
#endif
  char buffer[READ_SIZE];
  size_t num_read;
  //int ret;

  do {
    num_read = fread(buffer, 1, READ_SIZE, ifp);
    xfwrite(buffer, 1, num_read, ofp);
  } while (num_read == READ_SIZE);
}

size_t
grep_count(char const *file, char const *pattern)
{
  char buf[2 * READ_SIZE + 1], *ptr, *end, *start, *bufmid, *bufend;
  size_t read_size, ret;
  FILE *fp;

  ASSERT(strlen(pattern) < READ_SIZE);

  fp = fopen(file, "r");
  ASSERT(fp);

  memset(buf, 0, sizeof buf);
  bufmid = (char *)buf + READ_SIZE;
  bufend = (char *)buf + sizeof buf;

  start = bufmid;
  ret = 0;
  while ((read_size = fread(bufmid, 1, READ_SIZE, fp)) != 0) {
    ASSERT(read_size > 0 && read_size <= READ_SIZE);
    end = bufmid + read_size;
    ASSERT(end < bufend);
    *end = '\0';
    while ((ptr = strstr(start, pattern))) {
      start = ptr + strlen(pattern);
      ret++;
    }
    start = end - strlen(pattern) + 1;
    start -= (bufmid - buf);
    ASSERT(start >= buf);
    memcpy(buf, bufmid, read_size);
    bufmid = buf + read_size;
  }
  fclose(fp);
  return ret;
}

char const *
strip_path(char const *pathname)
{
  char const *ptr;
  ptr = strrchr(pathname, '/');
  if (!ptr) {
    ptr = pathname;
  } else {
    ptr++;
  }
  return ptr;
}

char *
strunc(char const *string, size_t trunc_len, char *buf, size_t size)
{
  ASSERT(size > trunc_len + 1);
  if (strlen(string) < trunc_len) {
    trunc_len = strlen(string);
  }
  memcpy(buf, string, trunc_len * sizeof(char));
  buf[trunc_len] = '\0';
  return buf;
}

size_t
longest_prefix_len(char const *a, char const *b)
{
  char const *cur_a, *cur_b;

  for (cur_a = a, cur_b = b; *cur_a == *cur_b; cur_a++, cur_b++);
  return cur_a - a;
}

void
free_array(void **strings, size_t n)
{
  size_t i;
  for (i = 0; i < n; i++) {
    free(strings[i]);
  }
}

long
radix_power(long base, long exp)
{
  long ret;
  int i;
  ret = 1;
  for (i = 0; i < exp; i++) {
    ret *= base;
  }
  return ret;
}

void
radix_break(long val, long base, long *arr, size_t arr_size)
{
  int i;
  for (i = 0; i < arr_size; i++) {
    arr[i] = val % base;
    val = val / base;
  }
}

void
self_cartesian_product_init(long *buf, size_t size, long max)
{
  long i;

  ASSERT(max >= 0);
  for (i = 0; i < size; i++) {
    buf[i] = 0;
  }
}

bool
self_cartesian_product_next(long *buf, size_t size, long max)
{
  long i;

  if (max == 0) {
    return false;
  }
  ASSERT(max > 0);

  for (i = size - 1; i >= 0; i--) {
    ASSERT(buf[i] >= 0 && buf[i] < max);
    buf[i]++;
    if (buf[i] < max) {
      return true;
    }
    buf[i] = 0;
  }
  return false;
}

bool
is_power_of_two(long long val, unsigned *bitpos)
{
  unsigned i;
  for (i = 0; (unsigned)(1<<i) < val; i++);
  if (val == (unsigned)(1 << i)) {
    if (bitpos) {
      *bitpos = i;
    }
    return true;
  }
  return false;
}

FILE *
xfopen(char const *filename, char const *mode)
{
  FILE *fp;
  if (!(fp = fopen(filename, mode))) {
    ERR("Can't open file '%s' : %s\n", filename, strerror(errno));
    exit(1);
  }
  return fp;
}

void
xsystem(char const *command)
{
  int ret;

  ret = system(command);
  if (ret == -1) {
    printf("system failed with retval -1! error = %s.\n", strerror(errno));
    printf("command: %s.\n", command);
    call_on_exit_function();
    exit(1);
  }
  if (WIFSIGNALED(ret)) {
    printf("signal %d received\n", WTERMSIG(ret));
    printf("command: %s.\n", command);
    call_on_exit_function();
    exit(1);
  }
  if (WEXITSTATUS(ret) != 0) {
    printf("non-zero exit status (%d) of command, exiting:\n%s\n", WEXITSTATUS(ret),
        command);
    printf("command: %s.\n", command);
    call_on_exit_function();
    exit(1);
  }
}

unsigned long long
round_up_to_power_of_two(unsigned long long val)
{
  int i;
  for (i = 0; i < sizeof(long long); i++) {
    if ((1ULL << i) >= val) {
      return (1ULL << i);
    }
  }
  return 0;
}

size_t
bool_to_string(bool b, char *buf, size_t size)
{
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  ptr += snprintf(ptr, end - ptr, "%s", b?"true":"false");
  return ptr - buf;
}

bool
bool_from_string(char const *buf, char const **remaining)
{
  if (strstart(buf, "true", remaining)) {
    return true;
  } else if (strstart(buf, "false", remaining)) {
    return false;
  } else if (strstart(buf, "1", remaining)) {
    return true;
  } else if (strstart(buf, "0", remaining)) {
    return true;
  } else {
    DBG(ERR, "buf = %s.\n", buf);
    NOT_REACHED();
  }
}

void
file_truncate(char const *filename)
{
  int ret;
  ret = truncate(filename, 0);
  ASSERT(ret == 0);
}

static int
qshuffle_cmp(const void *a, const void *b)
{
  return (2 * (rand() % 2)) - 1;
}

void
qshuffle(void *base, size_t nmemb, size_t size)
{
  qsort(base, nmemb, size, qshuffle_cmp);
}

/*static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
  int rv = remove(fpath);

  if (rv)
      perror(fpath);

  return rv;
}*/

static int
rmFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
  if (remove(pathname) < 0) {
    perror("ERROR: remove");
    return -1;
  }
  return 0;
}

int rmrf(char const *path)
{
  //char command[strlen(path) + 256];
  //snprintf(command, sizeof command, "rm -rf %s", path);
  //int ret = system(command);
  //return ret;
  if (nftw(path, rmFiles,10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS) < 0) {
    perror("ERROR: ntfw");
    //exit(1);
    return -1;
  }
  //return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
  return 0;
}

void
array_print(char *a, size_t s)
{
  char *ptr;
  ptr = a;
  for (ptr = a; ptr - a < s; ptr++) {
    printf(" %02hhx", *ptr);
  }
  printf("\n");
}

static void full_write(int fd, const char *buf, size_t len)
{
        while (len > 0) {
                ssize_t ret = write(fd, buf, len);

                if ((ret == -1) && (errno != EINTR))
                        break;

                buf += (size_t) ret;
                len -= (size_t) ret;
        }
}

void print_backtrace(void)
{
        static const char start[] = "BACKTRACE ------------\n";
        static const char end[] = "----------------------\n";

        void *bt[1024];
        int bt_size;
        char **bt_syms;
        int i;

        bt_size = backtrace(bt, 1024);
        bt_syms = backtrace_symbols(bt, bt_size);
        full_write(STDERR_FILENO, start, strlen(start));
        for (i = 1; i < bt_size; i++) {
                size_t len = strlen(bt_syms[i]);
                full_write(STDERR_FILENO, bt_syms[i], len);
                full_write(STDERR_FILENO, "\n", 1);
        }
        full_write(STDERR_FILENO, end, strlen(end));
    free(bt_syms);
}

bool
belongs_to_peep_program_slice(src_ulong pc)
{
  static src_ulong last_pc = (src_ulong)-1;

  if (!num_pslices) {
    DBG2(PSLICE, "pc %x: returning true because !num_pslices\n", pc);
    return true;
  }

  last_pc = pc;

  int i;
  for (i = 0; i < num_pslices; i++) {
    if (pc >= pslices_start[i] && pc < pslices_stop[i]) {
      DBG(PSLICE, "returning true because pc %x in interval %d: %lx->%lx\n", pc, i,
          pslices_start[i], pslices_stop[i]);
      return true;
    }
  }

  //XXX: the following loop seems useless. remove! --sorav
  last_pc = (src_ulong)-1;
  for (i = 0; i < num_pslices; i++) {
    if (last_pc >= pslices_start[i] && last_pc < pslices_stop[i]) {
      DBG(PSLICE, "returning true because lastpc %x in interval %d: %lx->%lx\n",
          last_pc, i, pslices_start[i], pslices_stop[i]);
      return true;
    }
  }
  return false;
}

int
print_debug_info_in (unsigned pc)
{
  /* at some point I also want to print the successors of pc */
  if (   belongs_to_peep_program_slice(pc)
      || belongs_to_peep_program_slice(pc - 4)/*XXX*/)
  {
    return 1;
  }

  return 0;
}

int
print_debug_info_out (unsigned pc)
{
  /* at some point I also want to print the successors of pc */
  if (belongs_to_peep_program_slice(pc) && !belongs_to_peep_program_slice(pc+4)) {
    return 1;
  }
  return 0;
}

void
disable_signals(void)
{
  sigset_t intmask;

  if (sigfillset(&intmask) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
  if (sigdelset(&intmask, SIGINT) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
  if (sigprocmask(SIG_BLOCK, &intmask, NULL) == -1) {
    printf("Failed to initialize the signal mask.\n");
    NOT_REACHED();
  }
}

uint64_t
ADD(uint64_t a, uint64_t b)
{
  return a + b;
}
uint64_t
SIGN_EXT(uint64_t x, int64_t n)
{
  assert(n <= 64 && n > -64);
  if (n == 64) {
    return (int64_t)x;
  }
  return (((int64_t)x) << (64 - n)) >> (64 - n);
}
uint64_t
ZERO_EXT(uint64_t x, int64_t n)
{
  assert(n <= 32 && n > -32);
  if (n == 32) {
    return (uint32_t)x;
  }
  return (((uint32_t)x) << (32 - n)) >> (32 - n);
}
uint64_t
UMASK(uint64_t a, int64_t b)
{
  assert(b <= 32 && b > -32);
  if (b >= 0) {
    //return a & ((1 << b) - 1);
    return a & ((1ULL << b) - 1);
  } else {
    return a & ~((1ULL << (32 + b)) - 1);
  }
}
uint64_t
MASK(uint64_t a, int64_t b)
{
  assert(b <= 32 && b > -32);
  if (b >= 0) {
    //return a & ((1 << b) - 1);
    return SIGN_EXT(a & ((1ULL << b) - 1), b);
  } else {
    return a & ~((1ULL << (32 + b)) - 1);
  }
}
uint64_t
MASKBITS(uint64_t mask_begin, uint64_t mask_end)
{
  uint64_t ret;
  if (mask_begin <= mask_end) {
    ret = ((1ULL << mask_begin) - 1) ^ ((1ULL << (mask_end + 1)) - 1);
  } else {
    ret = ~(((1ULL << mask_begin) - 1) ^ ((1ULL << (mask_end + 1)) - 1));
  }
  /*printf("mask_begin=%llx, mask_end=%llx, ret=%llx\n", (long long)mask_begin,
      (long long)mask_end, (long long)ret);*/
  return ret;
}

uint64_t
LSHIFT(uint64_t a, int64_t b)
{
  assert(b < 32 && b > -32);
  if (b >= 0) {
    return a<<b;
  } else {
    return ((uint32_t)a) >> (-b);
  }
}
uint64_t
ASHIFT(uint64_t a, int64_t b)
{
  assert(b < 32 && b > -32);
  if (b >= 0) {
    return a<<b;
  } else {
    return ((int32_t)a) >> (-b);
  }
}

size_t count_occurrences(char const *haystack, char needle)
{
  char const *ptr;
  size_t ret = 0;
  for (ptr = haystack; *ptr; ptr++) {
    if (*ptr == needle) {
      ret++;
    }
  }
  return ret;
}
