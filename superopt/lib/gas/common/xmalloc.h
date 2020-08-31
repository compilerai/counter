#ifndef GAS_XMALLOC_H
#define GAS_XMALLOC_H

void xmalloc_set_program_name (char const *s);
void xmalloc_failed (size_t size);
PTR xmalloc (size_t size);
PTR xcalloc (size_t nelem, size_t elsize);
PTR xrealloc (PTR oldmem, size_t size);

#endif /* gas/xmalloc.h */
