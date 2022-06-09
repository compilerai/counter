#include <support/debug.h>
#include "cutils/clist.h"

/* Our doubly linked lists have two header elements: the "head"
   just before the first element and the "tail" just after the
   last element.  The `prev' link of the front header is null, as
   is the `next' link of the back header.  Their other two links
   point toward each other via the interior elements of the list.

   An empty list looks like this:

                      +------+     +------+
                  <---| head |<--->| tail |--->
                      +------+     +------+

   A list with two elements in it looks like this:

        +------+     +-------+     +-------+     +------+
    <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
        +------+     +-------+     +-------+     +------+

   The symmetry of this arrangement eliminates lots of special
   cases in list processing.  For example, take a look at
   list_remove(): it takes only two pointer assignments and no
   conditionals.  That's a lot simpler than the code would be
   without header elements.

   (Because only one of the pointers in each header element is used,
   we could in fact combine them into a single header element
   without sacrificing this simplicity.  But using two separate
   elements allows us to do a little bit of checking on some
   operations, which can be valuable.) */

static bool is_sorted (struct clist_elem *a, struct clist_elem *b,
                       clist_less_func *less, void *aux) MY_UNUSED;

/* Returns true if ELEM is a head, false otherwise. */
static inline bool
is_head (struct clist_elem const *elem)
{
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* Returns true if ELEM is an interior element,
   false otherwise. */
static inline bool
is_interior (struct clist_elem const *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* Returns true if ELEM is a tail, false otherwise. */
static inline bool
is_tail (struct clist_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* Initializes LIST as an empty clist. */
void
clist_init (struct clist *clist)
{
  ASSERT (clist != NULL);
  clist->head.prev = NULL;
  clist->head.next = &clist->tail;
  clist->tail.prev = &clist->head;
  clist->tail.next = NULL;
}

/* Returns the beginning of LIST.  */
struct clist_elem *
clist_begin (struct clist const *clist)
{
  ASSERT (clist != NULL);
  return clist->head.next;
}

/* Returns the element after ELEM in its clist.  If ELEM is the
   last element in its clist, returns the clist tail.  Results are
   undefined if ELEM is itself a clist tail. */
struct clist_elem *
clist_next (struct clist_elem const *elem)
{
  ASSERT (is_head (elem) || is_interior (elem));
  return elem->next;
}

/* Returns LIST's tail.

   clist_end() is often used in iterating through a clist from
   front to back.  See the big comment at the top of clist.h for
   an example. */
struct clist_elem *
clist_end (struct clist const *clist)
{
  ASSERT (clist != NULL);
  return (struct clist_elem *)&clist->tail;
}

/* Returns the LIST's reverse beginning, for iterating through
   LIST in reverse order, from back to front. */
struct clist_elem *
clist_rbegin (struct clist *clist) 
{
  ASSERT (clist != NULL);
  return clist->tail.prev;
}

/* Returns the element before ELEM in its clist.  If ELEM is the
   first element in its clist, returns the clist head.  Results are
   undefined if ELEM is itself a clist head. */
struct clist_elem *
clist_prev (struct clist_elem *elem)
{
  ASSERT (is_interior (elem) || is_tail (elem));
  return elem->prev;
}

/* Returns LIST's head.

   clist_rend() is often used in iterating through a clist in
   reverse order, from back to front.  Here's typical usage,
   following the example from the top of clist.h:

      for (e = clist_rbegin (&foo_list); e != clist_rend (&foo_list);
           e = clist_prev (e))
        {
          struct foo *f = clist_entry (e, struct foo, elem);
          ...do something with f...
        }
*/
struct clist_elem *
clist_rend (struct clist *clist) 
{
  ASSERT (clist != NULL);
  return &clist->head;
}

/* Return's LIST's head.

   clist_head() can be used for an alternate style of iterating
   through a clist, e.g.:

      e = clist_head (&clist);
      while ((e = clist_next (e)) != clist_end (&clist)) 
        {
          ...
        }
*/
struct clist_elem *
clist_head (struct clist *clist) 
{
  ASSERT (clist != NULL);
  return &clist->head;
}

/* Return's LIST's tail. */
struct clist_elem *
clist_tail (struct clist *clist) 
{
  ASSERT (clist != NULL);
  return &clist->tail;
}

/* Inserts ELEM just before BEFORE, which may be either an
   interior element or a tail.  The latter case is equivalent to
   clist_push_back(). */
void
clist_insert (struct clist_elem *before, struct clist_elem *elem)
{
  ASSERT (is_interior (before) || is_tail (before));
  ASSERT (elem != NULL);

  elem->prev = before->prev;
  elem->next = before;
  before->prev->next = elem;
  before->prev = elem;
}

/* Removes elements FIRST though LAST (exclusive) from their
   current clist, then inserts them just before BEFORE, which may
   be either an interior element or a tail. */
void
clist_splice (struct clist_elem *before,
             struct clist_elem *first, struct clist_elem *last)
{
  ASSERT (is_interior (before) || is_tail (before));
  if (first == last)
    return;
  last = clist_prev (last);

  ASSERT (is_interior (first));
  ASSERT (is_interior (last));

  /* Cleanly remove FIRST...LAST from its current clist. */
  first->prev->next = last->next;
  last->next->prev = first->prev;

  /* Splice FIRST...LAST into new clist. */
  first->prev = before->prev;
  last->next = before;
  before->prev->next = first;
  before->prev = last;
}

/* Inserts ELEM at the beginning of LIST, so that it becomes the
   front in LIST. */
void
clist_push_front (struct clist *clist, struct clist_elem *elem)
{
  clist_insert (clist_begin (clist), elem);
}

/* Inserts ELEM at the end of LIST, so that it becomes the
   back in LIST. */
void
clist_push_back (struct clist *clist, struct clist_elem *elem)
{
  clist_insert (clist_end (clist), elem);
}

/* Removes ELEM from its clist and returns the element that
   followed it.  Undefined behavior if ELEM is not in a clist.

   It's not safe to treat ELEM as an element in a clist after
   removing it.  In particular, using clist_next() or clist_prev()
   on ELEM after removal yields undefined behavior.  This means
   that a naive loop to remove the elements in a clist will fail:

   ** DON'T DO THIS **
   for (e = clist_begin (&clist); e != clist_end (&clist); e = clist_next (e))
     {
       ...do something with e...
       clist_remove (e);
     }
   ** DON'T DO THIS **

   Here is one correct way to iterate and remove elements from a
   clist:

   for (e = clist_begin (&clist); e != clist_end (&clist); e = clist_remove (e))
     {
       ...do something with e...
     }

   If you need to free() elements of the clist then you need to be
   more conservative.  Here's an alternate strategy that works
   even in that case:

   while (!clist_empty (&clist))
     {
       struct clist_elem *e = clist_pop_front (&clist);
       ...do something with e...
     }
*/
struct clist_elem *
clist_remove (struct clist_elem *elem)
{
  ASSERT (is_interior (elem));
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return elem->next;
}

/* Removes the front element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
struct clist_elem *
clist_pop_front (struct clist *clist)
{
  struct clist_elem *front = clist_front (clist);
  ASSERT(front);
  clist_remove (front);
  return front;
}

/* Removes the back element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
struct clist_elem *
clist_pop_back (struct clist *clist)
{
  struct clist_elem *back = clist_back (clist);
  clist_remove (back);
  return back;
}

/* Returns the front element in LIST.
   Undefined behavior if LIST is empty. */
struct clist_elem *
clist_front (struct clist *clist)
{
  ASSERT (!clist_empty (clist));
  return clist->head.next;
}

/* Returns the back element in LIST.
   Undefined behavior if LIST is empty. */
struct clist_elem *
clist_back (struct clist *clist)
{
  ASSERT (!clist_empty (clist));
  return clist->tail.prev;
}

/* Returns the number of elements in LIST.
   Runs in O(n) in the number of elements. */
size_t
clist_size (struct clist *clist)
{
  struct clist_elem *e;
  size_t cnt = 0;

  for (e = clist_begin (clist); e != clist_end (clist); e = clist_next (e))
    cnt++;
  return cnt;
}

/* Returns true if LIST is empty, false otherwise. */
bool
clist_empty (struct clist *clist)
{
  return clist_begin (clist) == clist_end (clist);
}

/* Swaps the `struct clist_elem *'s that A and B point to. */
static void
swap (struct clist_elem **a, struct clist_elem **b) 
{
  struct clist_elem *t = *a;
  *a = *b;
  *b = t;
}

#if 0
/* Reverses the order of LIST. */
void
clist_reverse (struct clist *clist)
{
  if (!clist_empty (clist)) 
    {
      struct clist_elem *e;

      for (e = clist_begin (clist); e != clist_end (clist); e = e->prev)
        swap (&e->prev, &e->next);
      swap (&clist->head.next, &clist->tail.prev);
      swap (&clist->head.next->prev, &clist->tail.prev->next);
    }
}
#endif

/* Returns true only if the clist elements A through B (exclusive)
   are in order according to LESS given auxiliary data AUX. */
static bool
is_sorted (struct clist_elem *a, struct clist_elem *b,
           clist_less_func *less, void *aux)
{
  if (a != b)
    while ((a = clist_next (a)) != b) 
      if (less (a, clist_prev (a), aux))
        return false;
  return true;
}

/* Finds a run, starting at A and ending not after B, of clist
   elements that are in nondecreasing order according to LESS
   given auxiliary data AUX.  Returns the (exclusive) end of the
   run.
   A through B (exclusive) must form a non-empty range. */
static struct clist_elem *
find_end_of_run (struct clist_elem *a, struct clist_elem *b,
                 clist_less_func *less, void *aux)
{
  ASSERT (a != NULL);
  ASSERT (b != NULL);
  ASSERT (less != NULL);
  ASSERT (a != b);
  
  do 
    {
      a = clist_next (a);
    }
  while (a != b && !less (a, clist_prev (a), aux));
  return a;
}

/* Merges A0 through A1B0 (exclusive) with A1B0 through B1
   (exclusive) to form a combined range also ending at B1
   (exclusive).  Both input ranges must be nonempty and sorted in
   nondecreasing order according to LESS given auxiliary data
   AUX.  The output range will be sorted the same way. */
static void
inplace_merge (struct clist_elem *a0, struct clist_elem *a1b0,
               struct clist_elem *b1,
               clist_less_func *less, void *aux)
{
  ASSERT (a0 != NULL);
  ASSERT (a1b0 != NULL);
  ASSERT (b1 != NULL);
  ASSERT (less != NULL);
  ASSERT (is_sorted (a0, a1b0, less, aux));
  ASSERT (is_sorted (a1b0, b1, less, aux));

  while (a0 != a1b0 && a1b0 != b1)
    if (!less (a1b0, a0, aux)) 
      a0 = clist_next (a0);
    else 
      {
        a1b0 = clist_next (a1b0);
        clist_splice (a0, clist_prev (a1b0), a1b0);
      }
}

/* Sorts LIST according to LESS given auxiliary data AUX, using a
   natural iterative merge sort that runs in O(n lg n) time and
   O(1) space in the number of elements in LIST. */
void
clist_sort (struct clist *clist, clist_less_func *less, void *aux)
{
  size_t output_run_cnt;        /* Number of runs output in current pass. */

  ASSERT (clist != NULL);
  ASSERT (less != NULL);

  /* Pass over the clist repeatedly, merging adjacent runs of
     nondecreasing elements, until only one run is left. */
  do
    {
      struct clist_elem *a0;     /* Start of first run. */
      struct clist_elem *a1b0;   /* End of first run, start of second. */
      struct clist_elem *b1;     /* End of second run. */

      output_run_cnt = 0;
      for (a0 = clist_begin (clist); a0 != clist_end (clist); a0 = b1)
        {
          /* Each iteration produces one output run. */
          output_run_cnt++;

          /* Locate two adjacent runs of nondecreasing elements
             A0...A1B0 and A1B0...B1. */
          a1b0 = find_end_of_run (a0, clist_end (clist), less, aux);
          if (a1b0 == clist_end (clist))
            break;
          b1 = find_end_of_run (a1b0, clist_end (clist), less, aux);

          /* Merge the runs. */
          inplace_merge (a0, a1b0, b1, less, aux);
        }
    }
  while (output_run_cnt > 1);

  ASSERT (is_sorted (clist_begin (clist), clist_end (clist), less, aux));
}

/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
void
clist_insert_ordered (struct clist *clist, struct clist_elem *elem,
                     clist_less_func *less, void *aux)
{
  struct clist_elem *e;

  ASSERT (clist != NULL);
  ASSERT (elem != NULL);
  ASSERT (less != NULL);

  for (e = clist_begin (clist); e != clist_end (clist); e = clist_next (e))
    if (less (elem, e, aux))
      break;
  return clist_insert (e, elem);
}

/* Iterates through LIST and removes all but the first in each
   set of adjacent elements that are equal according to LESS
   given auxiliary data AUX.  If DUPLICATES is non-null, then the
   elements from LIST are appended to DUPLICATES. */
void
clist_unique (struct clist *clist, struct clist *duplicates,
             clist_less_func *less, void *aux)
{
  struct clist_elem *elem, *next;

  ASSERT (clist != NULL);
  ASSERT (less != NULL);
  if (clist_empty (clist))
    return;

  elem = clist_begin (clist);
  while ((next = clist_next (elem)) != clist_end (clist))
    if (!less (elem, next, aux) && !less (next, elem, aux)) 
      {
        clist_remove (next);
        if (duplicates != NULL)
          clist_push_back (duplicates, next);
      }
    else
      elem = next;
}

/* Returns the element in LIST with the largest value according
   to LESS given auxiliary data AUX.  If there is more than one
   maximum, returns the one that appears earlier in the clist.  If
   the clist is empty, returns its tail. */
struct clist_elem *
clist_max (struct clist *clist, clist_less_func *less, void *aux)
{
  struct clist_elem *max = clist_begin (clist);
  if (max != clist_end (clist)) 
    {
      struct clist_elem *e;
      
      for (e = clist_next (max); e != clist_end (clist); e = clist_next (e))
        if (less (max, e, aux))
          max = e; 
    }
  return max;
}

/* Returns the element in LIST with the smallest value according
   to LESS given auxiliary data AUX.  If there is more than one
   minimum, returns the one that appears earlier in the clist.  If
   the clist is empty, returns its tail. */
struct clist_elem *
clist_min (struct clist *clist, clist_less_func *less, void *aux)
{
  struct clist_elem *min = clist_begin (clist);
  if (min != clist_end (clist)) 
    {
      struct clist_elem *e;
      
      for (e = clist_next (min); e != clist_end (clist); e = clist_next (e))
        if (less (e, min, aux))
          min = e; 
    }
  return min;
}
