// dietlibc
typedef unsigned int size_t;

void* memchr(const void *s, int c, size_t n) {
  const unsigned char *pc = (unsigned char *) s;
  for (;n--;pc++) {
    if (*pc == c)
      return ((void *) pc);
  }
  return 0;
}

//#include "eqchecker_helper.h"
int main(int argc, char* argv[])
{
  // zext(255) != 0xffffffff (~0x0), hence memchr return 0
  //const char src[] = { 255, 1, 2, 3 };
  //printf("%p %p", src, (int*)memchr(src, ~0x0, 4));
  return 0;
}
