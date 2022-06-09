#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "support/debug.h"
#include "rewrite/tagline.h"

long
count_tags(char const *tagline)
{
  char const *ptr, *comma;
  long n;

  n = 0;
  for (ptr = tagline; ptr; ptr = comma?comma + 1:NULL) {
    comma = strchr(ptr, ',');
    n++;
  }
  return n;
}

void
pick_random_tag(char *buf, size_t buf_size, char const *tagline)
{
  char const *ptr, *comma, *end;
  long n, r;

  n = count_tags(tagline);
  r = rand() % n;
  n = 0;
  for (ptr = tagline; ptr; ptr = comma?comma + 1:NULL) {
    comma = strchr(ptr, ',');
    end = comma;
    if (!end) {
      end = ptr + strlen(ptr);
      ASSERT(end[0] == '\0');
    }
    if (n == r) {
      ASSERT(end - ptr < buf_size);
      memcpy(buf, ptr, end - ptr);
      buf[end - ptr] = '\0';
      break;
    }
    n++;
  }
}

size_t
taglines_union(char *a, char const *b, size_t size)
{
  char *aptr, *bptr, *acomma, *bcomma, *aend, *bend;
  char bcopy[strlen(b) + 1];

  strncpy(bcopy, b, sizeof bcopy);
  for (aptr = a; aptr; aptr = acomma?acomma+1:NULL) {
    acomma = strchr(aptr, ',');
    aend = acomma;
    if (!aend) {
      aend = aptr + strlen(aptr);
    }
    for (bptr = bcopy; *bptr;) {
      bcomma = strchr(bptr, ',');
      bend = bcomma;
      if (!bend) {
        bend = bptr + strlen(bptr);
      }
      ASSERT(*bend == '\0' || *bend == ',');
      if (   aend - aptr == bend - bptr
          && !memcmp(aptr, bptr, aend - aptr)) {
        if (*bend == '\0') {
          memmove(bptr, bend, strlen(bend) + 1);
        } else {
          memmove(bptr, bend + 1, strlen(bend + 1) + 1);
        }
      } else {
        if (*bend == '\0') {
          bptr = bend;
        } else {
          bptr = bend + 1;
        }
      }
    }
  }
  ASSERT(strlen(a) + strlen(bcopy) + 1 < size);
  if (strlen(bcopy)) {
    if (strlen(a) != 0) {
      strncat(a, ",", size);
    }
    strncat(a, bcopy, size);
  }
  ASSERT(strlen(a) < size);
  return strlen(a);
}
