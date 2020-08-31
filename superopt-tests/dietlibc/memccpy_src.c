/* dietlibc */
typedef unsigned int size_t;
void *memccpy(void *dst, const void *src, int c, size_t count) {
    char *a = dst;
    const char *b = src;
    while (count--) {
        *a++ = *b;
        if (*b == c) {
        // eqcheck fails because (*b) gets sign-extended and thus may not be equal to c (cf. dst where a cast is performed) when sign bit is set
        // Example input is given below in main
            return (void *)a;
        }
        b++;
    }
    return 0;
}

int main(int argc, char* argv[])
{
  // signext(128) != 128, hence all 4 bytes are written in dst
  /*const char src[] = { 128, 1, 2, 3 };
  char dst[4] = { 'A', 'B', 'C', 'D' };
  memccpy(dst, src, 128, 4);
  for (int i = 0; i < 4; ++i)
    printf("%c", dst[i]);
  */
  return 0;
}
