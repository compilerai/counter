#include <stdio.h>

//extern "C" int my_malloc_init (void);
extern "C" void *my_malloc (size_t size);
extern "C" void my_free (void *ptr);
extern "C" void *my_realloc(void *ptr, size_t size);
