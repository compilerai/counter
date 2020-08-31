#ifndef __ENUMERATOR_H
#define __ENUMERATOR_H

typedef enum { CHOOSE, PERM } enum_type_t;

typedef struct enum_t
{
  int valid:1;
  enum_type_t type;
  int n;
  int i;

  struct {
    char *elem;
    int x;
    int y;
  } cur_iter;
} enum_t;

void enum_set_choose (enum_t *e, int n, int i);
void *enum_iter_begin(enum_t *e, char *elem);
void *enum_iter_next(enum_t *e, void *iter, char *elem);
void enum_set_free (enum_t *e);

#endif
