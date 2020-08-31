#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cutils/enumerator.h"

void enum_set_choose (enum_t *e, int n, int i)
{
  e->type = CHOOSE;
  if (n > 0)
    e->n = n;
  else
    e->n = 0;

  e->i = i;
  if (i >= 0 && i <= e->n && e->n > 0)
  {
    e->cur_iter.elem = (char *)malloc(e->n*sizeof(char));
  }
  else
  {
    e->cur_iter.elem = NULL;
  }
  e->valid = 1;

}

void *
enum_iter_begin(enum_t *e, char *elem)
{
  assert(e->valid);
  if (e->i < 0 || e->i > e->n)
    return NULL;

  assert(!e->n || e->cur_iter.elem);

  int j;
  for (j = 0; j < e->n; j++) {
    if (j < e->i) {
      e->cur_iter.elem[j] = 1;
		} else {
      e->cur_iter.elem[j] = 0;
		}
  }
  e->cur_iter.x = -1;
  e->cur_iter.y = -1;

  if (e->n) {
    memcpy(elem, e->cur_iter.elem, e->n);
	}
  return &e->cur_iter;
}

/* for algorithm and code, see "Generating combinations by prefix shifts"
   [http://www.cs.uvic.ca/~ruskey/Publications/Coollex/Coollex.html]
 */
void *
enum_iter_next(enum_t *e, void *iter, char *elem)
{
  assert(e->valid);
  assert(e->i >= 0 && e->i <= e->n);
  assert(iter == &e->cur_iter);

  char *b = e->cur_iter.elem;
  int x = e->cur_iter.x;
  int y = e->cur_iter.y;

  if (x == -1) {
    if (e->i == 0 || e->i == e->n) {
      return NULL;
		}
    b[e->i] = 1; b[0] = 0;
    memcpy(elem, b, e->n);
    e->cur_iter.x = 1;
    e->cur_iter.y = 0;
    return &e->cur_iter;
  }

  if (x >= e->n - 1) {
    return NULL;
	}
  b[x++] = 0; b[y++] = 1;
  if (b[x] == 0) {
    b[x] = 1; b[0] = 0;
    if (y > 1) x = 1;
    y = 0;
  }
	memcpy(elem, b, e->n);
  e->cur_iter.x = x;
  e->cur_iter.y = y;

  return &e->cur_iter;
}

void enum_set_free (enum_t *e)
{
  if (e->cur_iter.elem)
    free(e->cur_iter.elem);
}
