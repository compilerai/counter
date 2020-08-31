#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//const char interp_section[] __attribute__((section(".interp"))) = "/path/to/dynamic/linker";

void MYmyexit(int ec)
{
  _exit(ec);
}

#define NDATA (int *)MYmymalloc(ncol * sizeof(int))
#define NLIST (struct _list *)MYmymalloc(sizeof(struct _list))
#define NPLAY (struct _play *)MYmymalloc(sizeof(struct _play))
void MYmymemset(void *s, int c, size_t n)
{
  memset(s,c,n);
}

void *MYmymemcpy(void *a, const void *b, size_t sz)
{
  return memcpy(a, b, sz);
}

int MYmymemcmp(const void *a, const void *b, size_t sz)
{
  return memcmp(a, b, sz);
}

int MYmystrcmp(const char *s1, const char *s2)
{
  return strcmp(s1,s2);
}

int MYmystrncmp(const char *s1, const char *s2, size_t n)
{
  return strncmp(s1,s2,n);
}

void *MYmymalloc(size_t size)
{
  return (void *)malloc(size);
}

void *MYmycalloc(size_t nmemb, size_t size)
{
  return (void *)calloc(nmemb, size);
}

void *MYmyrealloc(void *ptr, size_t size)
{
  return (void *)realloc(ptr, size);
}

FILE *MYmyfopen(char const *path, char const *mode)
{
  return fopen(path, mode);
}

FILE *MYmyfdopen(int fd, char const *mode)
{
  return fdopen(fd, mode);
}

FILE *MYmyfreopen(char const *path, char const *mode, FILE *stream)
{
  return freopen(path, mode, stream);
}

int MYmyfclose(FILE *fp)
{
  return fclose(fp);
}

void MYmyfree(void *ptr)
{
  free(ptr);
}


int
MYmy_atoi(char const *s)
{
  return atoi(s);
}

int MYmyrand()
{
  return rand();
}

char MYmy_char_inc(char *i)
{
  char ret = *i + 1;
  *i = ret;
  return ret;
}

int MYmyrand_char()
{
  return rand() % 255;
}

void
MYmyprint_char(char c)
{
  printf("%c", c);
}

void
MYmyprint_int(int c)
{
  printf("%d", c);
}

char *
MYmystrdup(char const *s)
{
  return strdup(s);
}

int MYmytoupper(int c)
{
  return toupper(c);
}

char*
MYmystrerrorno()
{
  return strerror(errno);
}

void
MYmydebug(int n)
{
  return;
}

void
MYmydebug2(int n)
{
  return;
}

void (*dbgptr)(int) = MYmydebug;
void (*dbgptr2)(int) = MYmydebug2;

//void (*dbgptr)(int);
//void (*dbgptr2)(int);
//#define LEN 30000
//#define LEN2 500
//
//int dummy(float a[LEN], float b[LEN], float c[LEN], float d[LEN], float e[LEN], float aa[LEN2][LEN2], float bb[LEN2][LEN2], float cc[LEN2][LEN2], float s){
//	// --  called in each loop to make all computations appear required
//	return 0;
//}

