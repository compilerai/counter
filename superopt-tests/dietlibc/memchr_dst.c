// openbsd
typedef unsigned int size_t;
void *
memchr(const void *s, int c, size_t n)
{
	if (n != 0) {
		const unsigned char *p = s;

		do {
			if (*p++ == (unsigned char)c)
				return ((void *)(p - 1));
		} while (--n != 0);
	}
	return (0);
}

//#include "eqchecker_helper.h"
int main(int argc, char* argv[])
{
  // memchr stops at first byte
  //const char src[] = { 255, 1, 2, 3 };
  //printf("%p %p", src, (int*)memchr(src, ~0x0, 4));
  return 0;
}
