/* openbsd */

typedef unsigned int size_t;
void *memccpy(void *t, const void *f, int c, size_t n) {
    if (n) {
        unsigned char *tp = t;
        const unsigned char *fp = f;
        unsigned char uc = c;
        do {
            if ((*tp++ = *fp++) == uc)
                return (tp);
        } while (--n != 0);
    }
    return (0);
}

int main(int argc, char* argv[])
{
  // only 1 byte is written (cf. memccpy_fail_src where 4 bytes are written for same input)
  /*const char src[] = { 128, 1, 2, 3 };
  char dst[4] = { 'A', 'B', 'C', 'D' };
  memccpy(dst, src, 128, 4);
  for (int i = 0; i < 4; ++i)
    printf("%c", dst[i]);
  */
  return 0;
}
