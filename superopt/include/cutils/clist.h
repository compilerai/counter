#ifndef __LIB_CLIST_H
#define __LIB_CLIST_H

/* Doubly linked list in C.

   This implementation of a doubly linked list does not require
   use of dynamically allocated memory.  Instead, each structure
   that is a potential list element must embed a struct list_elem
   member.  All of the list functions operate on these `struct
   list_elem's.  The list_entry macro allows conversion from a
   struct list_elem back to a structure object that contains it.

   For example, suppose there is a needed for a list of `struct
   foo'.  `struct foo' should contain a `struct list_elem'
   member, like so:

      struct foo
        {
          struct list_elem elem;
          int bar;
          ...other members...
        };

   Then a list of `struct foo' can be be declared and initialized
   like so:

      struct list foo_list;

      list_init (&foo_list);

   Iteration is a typical situation where it is necessary to
   convert from a struct list_elem back to its enclosing
   structure.  Here's an example using foo_list:

      struct list_elem *e;

      for (e = list_begin (&foo_list); e != list_end (&foo_list);
           e = list_next (e))
        {
          struct foo *f = list_entry (e, struct foo, elem);
          ...do something with f...
        }

   You can find real examples of list usage throughout the
   source; for example, malloc.c, palloc.c, and thread.c in the
   threads directory all use lists.

   The interface for this list is inspired by the list<> template
   in the C++ STL.  If you're familiar with list<>, you should
   find this easy to use.  However, it should be emphasized that
   these lists do *no* type checking and can't do much other
   correctness checking.  If you screw up, it will bite you.

   Glossary of list terms:

     - "front": The first element in a list.  Undefined in an
       empty list.  Returned by list_front().

     - "back": The last element in a list.  Undefined in an empty
       list.  Returned by list_back().

     - "tail": The element figuratively just after the last
       element of a list.  Well defined even in an empty list.
       Returned by list_end().  Used as the end sentinel for an
       iteration from front to back.

     - "beginning": In a non-empty list, the front.  In an empty
       list, the tail.  Returned by list_begin().  Used as the
       starting point for an iteration from front to back.

     - "head": The element figuratively just before the first
       element of a list.  Well defined even in an empty list.
       Returned by list_rend().  Used as the end sentinel for an
       iteration from back to front.

     - "reverse beginning": In a non-empty list, the back.  In an
       empty list, the head.  Returned by list_rbegin().  Used as
       the starting point for an iteration from back to front.

     - "interior element": An element that is not the head or
       tail, that is, a real list element.  An empty list does
       not have any interior elements.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* List element. */
struct clist_elem 
  {
    struct clist_elem *prev;     /* Previous clist element. */
    struct clist_elem *next;     /* Next clist element. */
  };

/* List. */
struct clist 
  {
    struct clist_elem head;      /* List head. */
    struct clist_elem tail;      /* List tail. */
  };

/* Converts pointer to clist element LIST_ELEM into a pointer to
   the structure that LIST_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the clist element.  See the big comment at the top of the
   file for an example. */
#define clist_entry(LIST_ELEM, STRUCT, MEMBER)           \
        ((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
                     - offsetof (STRUCT, MEMBER.next)))

/* List initialization.

   A list may be initialized by calling list_init():

       struct list my_list;
       list_init (&my_list);

   or with an initializer using LIST_INITIALIZER:

       struct list my_list = LIST_INITIALIZER (my_list); */
#define LIST_INITIALIZER(NAME) { { NULL, &(NAME).tail }, \
                                 { &(NAME).head, NULL } }


void clist_init (struct clist *);

/* List traversal. */
struct clist_elem *clist_begin (struct clist const *);
struct clist_elem *clist_next (struct clist_elem const *);
struct clist_elem *clist_end (struct clist const *);

struct clist_elem *clist_rbegin (struct clist *);
struct clist_elem *clist_prev (struct clist_elem *);
struct clist_elem *clist_rend (struct clist *);

struct clist_elem *clist_head (struct clist *);
struct clist_elem *clist_tail (struct clist *);

/* List insertion. */
void clist_insert (struct clist_elem *, struct clist_elem *);
void clist_splice (struct clist_elem *before,
                  struct clist_elem *first, struct clist_elem *last);
void clist_push_front (struct clist *, struct clist_elem *);
void clist_push_back (struct clist *, struct clist_elem *);

/* List removal. */
struct clist_elem *clist_remove (struct clist_elem *);
struct clist_elem *clist_pop_front (struct clist *);
struct clist_elem *clist_pop_back (struct clist *);

/* List elements. */
struct clist_elem *clist_front (struct clist *);
struct clist_elem *clist_back (struct clist *);

/* List properties. */
size_t clist_size (struct clist *);
bool clist_empty (struct clist *);

/* Miscellaneous. */
//void clist_reverse (struct clist *);

/* Compares the value of two clist elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
typedef bool clist_less_func (const struct clist_elem *a,
                             const struct clist_elem *b,
                             void *aux);

/* Operations on clists with ordered elements. */
void clist_sort (struct clist *,
                clist_less_func *, void *aux);
void clist_insert_ordered (struct clist *, struct clist_elem *,
                          clist_less_func *, void *aux);
void clist_unique (struct clist *, struct clist *duplicates,
                  clist_less_func *, void *aux);

/* Max and min. */
struct clist_elem *clist_max (struct clist *, clist_less_func *, void *aux);
struct clist_elem *clist_min (struct clist *, clist_less_func *, void *aux);

/**
 * clist_for_each_entry	-	iterate over clist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your clist.
 * @member:	the name of the clist_struct within the struct.
 */
#define clist_for_each_entry(pos, head, member)				\
	for (pos = clist_entry((head)->next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head); 	\
	     pos = clist_entry(pos->member.next, typeof(*pos), member))

/**
 * clist_for_each_entry_safe - iterate over clist of given type safe against removal of clist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your clist.
 * @member:	the name of the clist_struct within the struct.
 */
#define clist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = clist_entry((head)->next, typeof(*pos), member),	\
		n = clist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = clist_entry(n->member.next, typeof(*n), member))

/**
 * clist_for_each  - iterate over a clist
 * @pos:  the &struct clist_head to use as a loop cursor.
 * @head: the head for your clist.
 */
#define clist_for_each(pos, head) \
  for (pos = (head)->next; prefetch(pos->next), pos != (head); \
      pos = pos->next)



#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct clist_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct clist_elem *clist)
{
  clist->next = clist;
  clist->prev = clist;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal clist manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct clist_elem *new_elem,
			      struct clist_elem *prev,
			      struct clist_elem *next)
{
	next->prev = new_elem;
	new_elem->next = next;
	new_elem->prev = prev;
	prev->next = new_elem;
}

/**
 * clist_add - add a new entry
 * @new_elem: new entry to be added
 * @head: clist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void clist_add(struct clist_elem *new_elem, struct clist_elem *head)
{
	__list_add(new_elem, head, head->next);
}


/**
 * clist_add_tail - add a new entry
 * @new_elem: new entry to be added
 * @head: clist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void clist_add_tail(struct clist_elem *new_elem, struct clist_elem *head)
{
	__list_add(new_elem, head->prev, head);
}

/*
 * Delete a clist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal clist manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct clist_elem * prev, struct clist_elem * next)
{
	next->prev = prev;
	prev->next = next;
}

/********** include/linux/clist.h **********/
/*
 *  These are non-NULL pointers that will result in page faults
 *  under normal circumstances, used to verify that nobody uses
 *  non-initialized clist entries.
 */
#define LIST_POISON1  NULL //((void *) 0x00100100)
#define LIST_POISON2  NULL //((void *) 0x00200200)

/**
 * clist_del - deletes entry from clist.
 * @entry: the element to delete from the clist.
 * Note: clist_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void clist_del(struct clist_elem *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

/**
 * clist_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new_elem : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void clist_replace(struct clist_elem *old,
				struct clist_elem *new_elem)
{
	new_elem->next = old->next;
	new_elem->next->prev = new_elem;
	new_elem->prev = old->prev;
	new_elem->prev->next = new_elem;
}

static inline void clist_replace_init(struct clist_elem *old,
					struct clist_elem *new_elem)
{
	clist_replace(old, new_elem);
	INIT_LIST_HEAD(old);
}

/**
 * clist_del_init - deletes entry from clist and reinitialize it.
 * @entry: the element to delete from the clist.
 */
static inline void clist_del_init(struct clist_elem *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * clist_move - delete from one clist and add as another's head
 * @clist: the entry to move
 * @head: the head that will precede our entry
 */
static inline void clist_move(struct clist_elem *clist, struct clist_elem *head)
{
	__list_del(clist->prev, clist->next);
	clist_add(clist, head);
}

/**
 * clist_move_tail - delete from one clist and add as another's tail
 * @clist: the entry to move
 * @head: the head that will follow our entry
 */
static inline void clist_move_tail(struct clist_elem *clist,
				  struct clist_elem *head)
{
	__list_del(clist->prev, clist->next);
	clist_add_tail(clist, head);
}

/**
 * clist_is_last - tests whether @clist is the last entry in clist @head
 * @clist: the entry to test
 * @head: the head of the clist
 */
static inline int clist_is_last(const struct clist_elem *clist,
				const struct clist_elem *head)
{
	return clist->next == head;
}

/**
 * clist_empty - tests whether a clist is empty
 * @head: the clist to test.
 */
static inline int clist_head_empty(const struct clist_elem *head)
{
	return head->next == head;
}

/**
 * clist_empty_careful - tests whether a clist is empty and not being modified
 * @head: the clist to test
 *
 * Description:
 * tests whether a clist is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using clist_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the clist entry is clist_del_init(). Eg. it cannot be used
 * if another CPU could re-clist_add() it.
 */
static inline int clist_empty_careful(const struct clist_elem *head)
{
	struct clist_elem *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * clist_is_singular - tests whether a clist has just one entry.
 * @head: the clist to test.
 */
static inline int clist_is_singular(const struct clist_elem *head)
{
	return !clist_head_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct clist_elem *clist,
		struct clist_elem *head, struct clist_elem *entry)
{
	struct clist_elem *new_first = entry->next;
	clist->next = head->next;
	clist->next->prev = clist;
	clist->prev = entry;
	entry->next = clist;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * clist_cut_position - cut a clist into two
 * @clist: a new clist to add all removed entries
 * @head: a clist with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the clist
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @clist. You should
 * pass on @entry an element you know is on @head. @clist
 * should be an empty clist or a clist you do not care about
 * losing its data.
 *
 */
static inline void clist_cut_position(struct clist_elem *clist,
		struct clist_elem *head, struct clist_elem *entry)
{
	if (clist_head_empty(head))
		return;
	if (clist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(clist);
	else
		__list_cut_position(clist, head, entry);
}

static inline void __list_head_splice(const struct clist_elem *clist,
				 struct clist_elem *prev,
				 struct clist_elem *next)
{
	struct clist_elem *first = clist->next;
	struct clist_elem *last = clist->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

#if 0
/**
 * clist_head_splice - join two clists, this is designed for stacks
 * @clist: the new clist to add.
 * @head: the place to add it in the first clist.
 */
static inline void clist_head_splice(const struct clist_elem *clist,
				struct clist_elem *head)
{
	if (!clist_head_empty(clist))
		__list_head_splice(clist, head, head->next);
}

/**
 * clist_splice_tail - join two clists, each clist being a queue
 * @clist: the new clist to add.
 * @head: the place to add it in the first clist.
 */
static inline void clist_splice_tail(struct clist_elem *clist,
				struct clist_elem *head)
{
	if (!clist_head_empty(clist))
		__list_splice(clist, head->prev, head);
}

/**
 * clist_splice_init - join two clists and reinitialise the emptied clist.
 * @clist: the new clist to add.
 * @head: the place to add it in the first clist.
 *
 * The clist at @clist is reinitialised
 */
static inline void clist_splice_init(struct clist_elem *clist,
				    struct clist_elem *head)
{
	if (!clist_head_empty(clist)) {
		__list_splice(clist, head, head->next);
		INIT_LIST_HEAD(clist);
	}
}

/**
 * clist_splice_tail_init - join two clists and reinitialise the emptied clist
 * @clist: the new clist to add.
 * @head: the place to add it in the first clist.
 *
 * Each of the clists is a queue.
 * The clist at @clist is reinitialised
 */
static inline void clist_splice_tail_init(struct clist_elem *clist,
					 struct clist_elem *head)
{
	if (!clist_head_empty(clist)) {
		__list_splice(clist, head->prev, head);
		INIT_LIST_HEAD(clist);
	}
}
#endif

#endif /* lib/clist.h */
