/* dietlibc */
typedef unsigned int size_t;
void* memrchr(const void *s, int c, size_t n) {
  register const char* t=s;
  register const char* last=0;
  int i;
  for (i=n; i; --i) {
    if (*t==c)
      last=t;
    ++t;
  }
  return (void*)last; /* man, what an utterly b0rken prototype */
}

int main(int argc, char* argv[])
{
  // memchr stops at first byte
  //const char src[] = { 255, 128 };
  //printf("%p %p\n", src, (int*)memrchr(src, 128, 2));
  //if (!memrchr(src, 128, 2))
  //  printf("BUG!\n");
  return 0;
}

