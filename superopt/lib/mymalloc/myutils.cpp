#include "mymalloc/myutils.h"

extern "C"
int
print_icount_and_fflush(FILE* stream)
{
  static long long icount = 0;
  printf("icount = %lld\n", icount++);
  return fflush(stream);
}
