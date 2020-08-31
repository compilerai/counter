/* openbsd */

typedef unsigned int size_t;
void *memrchr(const void *s, int c, size_t n) {
    const unsigned char *cp;

    if (n != 0) {
        cp = (unsigned char *)s + n;
        do {
            if (*(--cp) == (unsigned char)c)
                return ((void *)cp);
        } while (--n != 0);
    }
    return (0);
}

int main(int argc, char* argv[])
{
  // memchr stops at first byte
  //const char src[] = { 1, 2, 3, 255 };
  //printf("%p %p", src, (int*)memrchr(src, ~0x0, 4));
  return 0;
}
