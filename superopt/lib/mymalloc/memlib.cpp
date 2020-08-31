/*
 * memlib.c - a module that simulates the memory system.  Needed because it
 *            allows us to interleave calls from the student's malloc package
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
//#include <iostream>

//using namespace std;
#include "memlib.h"

/* private variables */
static char *mem_start_brk;  /* points to first byte of heap */
static char *mem_brk;        /* points to last byte of heap */
static char *mem_max_addr;   /* largest legal heap address */

#define HEAP_START_ADDR 0x1000000
#define MAX_HEAP_SIZE 0x7000000

/*
 * mem_init - initialize the memory system model
 */
void mem_init(/*int max_heap_size*/)
{
    /* allocate the storage we will use to model the available VM */

	// NOTE: this is technically an OS malloc() call, but this is
	// only used ONCE at initialization time.

    //if ((mem_start_brk = (char *)malloc(MAX_HEAP_SIZE/*max_heap_size*/)) == NULL) {
    //    fprintf(stderr, "mem_init_vm: malloc error\n");
    //    exit(1);
    //}
    mem_start_brk = (char*)mmap((void*)HEAP_START_ADDR, MAX_HEAP_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0); 
    if (mem_start_brk == MAP_FAILED) {
      //std::cout << __func__ << " " << __LINE__ << ": mmap failed. error = " << strerror(errno) << endl;
      printf("%s() %d: mmap failed. error = %s\n", __func__, __LINE__, strerror(errno));
      exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP_SIZE;  /* max legal heap address */
    mem_brk = mem_start_brk;                  /* heap is empty initially */
}

/*
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/*
 * mem_sbrk - simple model of the sbrk function. Extends the heap
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int incr)
{
    char *old_brk = mem_brk;

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
    errno = ENOMEM;
    fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...; mem_brk=%p, incr=%d, mem_max_addr=%p\n", mem_brk, incr, mem_max_addr);
    return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/*
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize()
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
