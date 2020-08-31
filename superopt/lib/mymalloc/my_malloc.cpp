/*
 * my_malloc.c - malloc/free/realloc implemented using segregated explicit free lists
 *
 * This library implements memory allocation operations by using an approximate power-of-two best-fit search.
 * Each allocated payload has a header and a footer that contain the block size, and an isAllocated bit for validity.
 * Each free block of memory also has pointers to the next and previous free block that are of equal or similar size.
 * segregatedStart contains 28 pointers to linked lists. Each linked list connects free chunks of memory that are of similar size
 * The first 8 lists of segregatedStart correspond with free lists for chunks of size 0-127 bytes, 128-255 bytes, ..., 896-1023 bytes.
 * The next  20 lists of segregatedStart correspond with free lists for chunks of size 1024-2047, 2048-4095... 2^29-(2^30-1) bytes.
 * For each of these size intervals, a free list is created that stores all free chunks of size within that interval.
 * The best-fit search goes over each linked list, and tries to find a free block that fits the requested size.
 *
 * malloc() - a free chunk of memory that fits the needed size is found using bestFit(). This block is then cut into two
 * blocks, one for the allocated block, and the other for the leftover memory. The latter is reinserted in the segregated list.
 * In case the heap needed to be extended to allocate this memory, and the allocated size is small, we'll allocate multiple
 * free blocks for future use, to reduce external fragmentation in certain cases. Approximately 4KB bytes worth of free blocks
 * will be allocated.
 *
 * free() - the given block is merged with the preceding and following free chunks. This new chunk of memory is then inserted in
 * the appropriate segregated bin.
 *
 * realloc() - If possible, realloc will try to extend the currently allocated block to the right. If this is not possible, realloc()
 * will instead use malloc(), copy over the stored data, and then free() the old payload.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "memlib.h"

/* every malloc blocks is aligned by this value i.e. each block's address % ALIGHNMENT == 0*/
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// implicit block size
#define HSIZE sizeof(blockHeader)
// additional block size for free list pointers
#define FREEHSIZE 2 * sizeof(blockHeader*)
// intial size of the epilogue block
#define INFSIZE (1 << 30)
// how many size bins we use
#define BINCOUNT 28
// how much storage for additional blocks we allocate when we need
// one block of a certain size, reducing external fragmentation
#define PAGESIZE 4096

#define ASSERT assert
static bool inited = false;

typedef struct blockHeader blockHeader;
struct blockHeader {
    int valid;
    int size;
};

blockHeader* blockHeader_getFooter(blockHeader* p);
blockHeader* blockHeader_getHeader(blockHeader* p);
blockHeader* blockHeader_getNext(blockHeader* p);
void blockHeader_set(blockHeader* p, int valid, int size);
void blockHeader_refreshFooter(blockHeader* p);

blockHeader** freeList_getPrev(void* p);
blockHeader** freeList_getNext(void* p);
void freeList_updateAdjacent(blockHeader* curr);
void freeList_deleteBlock(blockHeader* curr);
void freeList_addBlock(blockHeader* p);

size_t getMostSignificantBit(size_t x);
int getBin(size_t size);
blockHeader* bestFit(size_t allocSize);

int memoryExtend(void* p);
int isEpilogue(int s);

int my_check();

blockHeader** segregatedStart;
blockHeader* prologueBlock;

// START of blockHeader functions------------------------------------------------------------------

/* blockHeaders are the pieces of data preceding and following each allocated and free block
 * they store whether a block is a prologue (-1), is free (0), or is valid (1)
 * and the size of the block (including the memory for the header and footer)
 */

blockHeader* blockHeader_getFooter(blockHeader* p){
    blockHeader* complement = (blockHeader*)((char*) p + (p->size - HSIZE));
    return complement;
}

blockHeader* blockHeader_getHeader(blockHeader* p){
    blockHeader* complement = (blockHeader*)((char*) p - (p->size - HSIZE));
    return complement;
}

blockHeader* blockHeader_getNext(blockHeader* p){
    return (blockHeader*)((char*) p + p->size);
}

// given a block header, update its valid and size variables
// and push those changes to the footer
void blockHeader_set(blockHeader* p, int valid, int size){
    p->valid = valid;
    p->size = size;

    blockHeader_refreshFooter(p);
}

// given a block header, make footer reflect the values stored in the header
void blockHeader_refreshFooter(blockHeader* p){
    if(isEpilogue(p->size)){
        return;
    }

    blockHeader* footer = blockHeader_getFooter(p);
    footer->size = p->size;
    footer->valid = p->valid;
}

// END of blockHeader functions--------------------------------------------------------------------

// BEGIN of freeList functions---------------------------------------------------------------------

/* freeLists are doubly linked lists of free blocks.
 * The next and prev pointers are stored at (ptr + HSIZE) and (ptr + HSIZE + PTRSIZE) respectively.
 */

blockHeader** freeList_getPrev(void* p){
    return (blockHeader**)((char*)p + HSIZE + sizeof(blockHeader*));
}

blockHeader** freeList_getNext(void* p){
    return (blockHeader**)((char*)p + HSIZE);
}

// redirects the given header's next and free pointers to point
// to this block
void freeList_updateAdjacent(blockHeader* curr){
    blockHeader** prev = freeList_getPrev(curr);
    blockHeader** next = freeList_getNext(curr);

    if(*prev != NULL){
        *freeList_getNext(*prev) = curr;
    }
    if(*next != NULL){
        *freeList_getPrev(*next) = curr;
    }
}

// deletes the given block by redirecting the next and prev pointers
// and by updating the segregated list head pointer if necessary
void freeList_deleteBlock(blockHeader* curr){
    blockHeader** prev = freeList_getPrev(curr);
    blockHeader** next = freeList_getNext(curr);

    int segList = getBin(curr->size);

    if(segregatedStart[segList] == curr){
        segregatedStart[segList] = *freeList_getNext(segregatedStart[segList]);
    }

    if(*prev != NULL){
        *freeList_getNext(*prev) = *next;
    }
    if(*next != NULL){
        *freeList_getPrev(*next) = *prev;
    }
}

// adds the given free block to the appropriate segregated list by
// inserting it as the new head element
void freeList_addBlock(blockHeader* t){
    int segList = getBin(t->size);
    blockHeader* firstNode = segregatedStart[segList];

    while(t <= firstNode && firstNode < blockHeader_getNext(t)){
        firstNode = *freeList_getNext(firstNode);
    }

    *freeList_getPrev(t) = NULL;
    *freeList_getNext(t) = firstNode;
    segregatedStart[segList] = t;
    freeList_updateAdjacent(t);
}

// END of freeList functions---------------------------------------------------------------------

// MSD bit trick
// @param x, must be below 2^31 in absolute value
size_t getMostSignificantBit(size_t x){
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    x >>= 1;

    return x;
}

// getBin() returns the appropriate segregated list index for the given size
// the smallest 8 bins store lists of size 0-127 bytes, 128-255 bytes, ..., 896-1023 bytes
// the next 20 bins store lists of size 2^i for each i between 10 and 29
int getBin(size_t size){
    if(size < 1024){
        return size / 128;
    } else if(isEpilogue(size)){
        return BINCOUNT - 1;
    }

    size = getMostSignificantBit(size);

    for(int i = 10; i < 30; i++){
        if(size == (1 << i)){
            return 9 + (i - 10);
        }
    }

    return -1;
}

// this subroutine returns a free block of size larger than allocSize
// using an approximate power-of-two best-fit
blockHeader* bestFit(size_t allocSize){
    int binLocation = getBin(allocSize);

    for(int i = binLocation; i < BINCOUNT; i++){
        blockHeader* curr = segregatedStart[i];

        // first-fit traversal
        while(curr != NULL){
            if(curr->size >= allocSize){
                return curr;
            }

            curr = *freeList_getNext(curr);
        }
    }

    return NULL;
}

// extend memory if necessary, and report error
// @param desired memory location to be allocated if necessary
// @returns -1 on failure, 0 when no new memory has been allocated, and 1 when the heap has been extended
int memoryExtend(void* t){
    if(t > mem_heap_hi()){
        void *p = mem_sbrk((char *)t - (char *)mem_heap_hi());
        if (p == (void *)-1){
            return -1;
        }
        return 1;
    }

    return 0;
}

// @param size of a free block
// @returns whether the given block is the epilogue block
int isEpilogue(int s){
	return (s > INFSIZE / 2);
}

/*
 * my_init - initialize the malloc package.
 */
int my_malloc_init(){
    // initialize segregated list
    segregatedStart = (blockHeader**)mem_sbrk(sizeof(blockHeader*) * BINCOUNT);
    if(segregatedStart == NULL){
        return -1;
    }
    for(int i = 0; i < BINCOUNT; i++){
        segregatedStart[i] = NULL;
    }

    // allocate prologue block
    prologueBlock = (blockHeader*)mem_sbrk(HSIZE);
    if(prologueBlock == NULL){
        return -1;
    }
    blockHeader_set(prologueBlock, -1, HSIZE);

    // allocate epilogue block
    blockHeader* freeBlock = (blockHeader*)mem_sbrk(HSIZE + FREEHSIZE);
    if(freeBlock == NULL){
        return -1;
    }
    blockHeader_set(freeBlock, 0, INFSIZE);
    (*freeList_getNext(freeBlock)) = NULL;
    (*freeList_getPrev(freeBlock)) = NULL;

    // store epilogue block in last segregated bin
    segregatedStart[BINCOUNT - 1] = freeBlock;

    return 0;
}


/*
 * my_malloc - Allocate a block by finding an appropriate allocation location using bestFit
 *             allocating the necessary memory, and creating a new free block if there's residue
 */
extern "C"
void *my_malloc(size_t size){
    if (!inited) {
      mem_init();
      int ret = my_malloc_init();
      ASSERT(ret == 0);
      inited = true;
    }
    size = ALIGN(size);
    int blockSize = 2 * HSIZE + size;

    blockHeader* allocBlock = bestFit(blockSize);

    int spaceLeft = allocBlock->size - blockSize;

    freeList_deleteBlock(allocBlock);
    blockHeader_set(allocBlock, 1, blockSize);

    if(spaceLeft < 2 * HSIZE + FREEHSIZE){
        // if the residue is too small to be a free block
        // incorporate in the currently allocated block
        allocBlock->size += spaceLeft;
    } else if(spaceLeft >= 2 * HSIZE + FREEHSIZE){
        // allocate new free blocks
        void* endNewBlock = blockHeader_getNext(allocBlock);
        void* finalNewFreeBlock = (void *)((char *)endNewBlock + 2 * HSIZE + FREEHSIZE);

        if(finalNewFreeBlock > mem_heap_hi()){
        	int blockAllocCount = 0;
        	if(blockSize <= 128){
		        blockAllocCount = PAGESIZE / (blockSize) - 1;
        	}

        	// allocate multiple free blocks of size blockSize
        	// to prevent cases where a small payload splits a large block
        	// and prevents us from allocating the block
	        for(int i = 0; i < blockAllocCount; i++){
		        void* newFreeBlock = (char*)endNewBlock + blockSize;
		        finalNewFreeBlock = (char*)newFreeBlock + 2 * HSIZE + FREEHSIZE;

		        blockHeader* secAllocBlock = (blockHeader*)endNewBlock;
		        blockHeader_set(secAllocBlock, 0, blockSize);
		        freeList_addBlock(secAllocBlock);
		        endNewBlock = newFreeBlock;
		        spaceLeft -= blockSize;

	        	if(memoryExtend(finalNewFreeBlock) == -1){
	        		return 0;
	        	}
	        }

	        // allocate the header info for the epilogue block
	        if(memoryExtend(finalNewFreeBlock) == -1){
	        	return 0;
	        }
	    }

        blockHeader* currNext = (blockHeader*)endNewBlock;
        blockHeader_set(currNext, 0, spaceLeft);

        freeList_addBlock(currNext);
    }

    blockHeader_refreshFooter(allocBlock);

    return (char*) allocBlock + HSIZE;
}

/*
 * my_free - coalesce the current free block with the previous and next adjacent blocks if they're free
 *           and add the block to the appropriate segregated list
 */
extern "C"
void my_free(void *ptr){
    if (!inited) {
      mem_init();
      int ret = my_malloc_init();
      ASSERT(ret == 0);
      inited = true;
    }
    blockHeader* newPtr = (blockHeader*)ptr;
    newPtr--;

    assert(newPtr->valid == 1);

    // coalesce with previous block
    blockHeader* backPtr = newPtr - 1;
    if(!backPtr->valid){
        backPtr = blockHeader_getHeader(backPtr);
        freeList_deleteBlock(backPtr);
        backPtr->size += newPtr->size;
        newPtr = backPtr;
    }

    // coalesce with next block
    blockHeader* nextPtr = blockHeader_getNext(newPtr);
    if(!nextPtr->valid){
        freeList_deleteBlock(nextPtr);
        newPtr->size += nextPtr->size;
    }

    blockHeader_set(newPtr, 0, newPtr->size);

    // add the free block to the appropriate segregated list
    freeList_addBlock(newPtr);
}

/*
 * my_realloc - contains two optimization to increase throughput
 *              generally, will use a malloc() and free() operation
                to generate the reallocated block
 */
extern "C"
void *my_realloc(void *oldptr, size_t size){
    if (!inited) {
      mem_init();
      int ret = my_malloc_init();
      ASSERT(ret == 0);
      inited = true;
    }
    blockHeader* t = (blockHeader*)((char*)oldptr - HSIZE);

    // if ptr == NULL, realloc is equivalent to malloc(size)
    if(oldptr == NULL){
        return my_malloc(size);
    }

    // if size == 0, realloc is equivalent to free(ptr);
    if(size == 0){
        my_free(oldptr);
        return NULL;
    }

    // optimization #1
    // if the request is for a smaller block, just reduce the current block size
    if(t->size - 2 * HSIZE >= size){
        int newBlockSize = 2 * HSIZE + size;
        int residue = t->size - newBlockSize;

        // create a new free block if space allows
        if(residue >= 2 * HSIZE + FREEHSIZE){
            blockHeader* newFreeBlock = (blockHeader*)((char*) t + newBlockSize);
            blockHeader_set(newFreeBlock, 0, residue);
            t->size = newBlockSize;

            freeList_addBlock(newFreeBlock);
        }

        return oldptr;
    }

    // optimization #2
    // in case the following block is free and large enough
    // extend the current block, and place the current allocated block
    blockHeader* nextBlock = blockHeader_getNext(t);
    if(!nextBlock->valid && nextBlock->size + t->size >= size + 2 * HSIZE + FREEHSIZE){
        int oldCurrBlockSize = t->size;
        int oldNextBlockSize = nextBlock->size;

        if(memoryExtend((char*)oldptr + size + 2 * HSIZE + FREEHSIZE) == -1){
            return NULL;
        }

        freeList_deleteBlock(nextBlock);

        t->size = size + 2 * HSIZE;
        blockHeader_refreshFooter(t);

        nextBlock = blockHeader_getNext(t);
        blockHeader_set(nextBlock, 0, oldNextBlockSize - size + oldCurrBlockSize);

        freeList_addBlock(nextBlock);

        return oldptr;
    }

    // otherwise, just use malloc() and free()
    size_t copySize = t->size - 2 * HSIZE;
    void* newptr = my_malloc(size);
    if (newptr == NULL){
        return NULL;
    }
    memcpy(newptr, oldptr, copySize);
    my_free(oldptr);
    return newptr;
}

// my_check() checks for heap consistency
// @returns 1 if the heap is consistent, 0 otherwise
int my_check(){
    // checking for prologue consistency
    if(prologueBlock->valid != -1 || prologueBlock->size != HSIZE){
        printf("ERR: PROLOGUE MODIFIED\n");
        return 0;
    }

    blockHeader* currBlock = blockHeader_getNext(prologueBlock);

    int freeCount = 0;
    int epilogueFound = 0;

    while(currBlock){
        // checking for size consistency
        if(isEpilogue(currBlock->size)){
            // control has reached an epilogue block

            // checking for epilogue consistency
            if(currBlock->valid != 0){
                printf("ERR: EPILOGUE BLOCK IS NOT FREE");
                return 0;
            }

            currBlock->valid |= 2;
            freeCount++;

            epilogueFound = 1;
            break;
        } else if(currBlock->size <= 0 || currBlock->size > (char*)mem_heap_hi() - (char*)mem_heap_lo()){
            printf("ERR: INCORRECT BLOCK SIZE\n");
            return 0;
        }

        if(currBlock->valid == 0){
            blockHeader footer = *blockHeader_getFooter(currBlock);

            // checking for free block size consistency
            if(currBlock->size < 2 * HSIZE + FREEHSIZE){
                printf("ERR: FREE BLOCK TOO SMALL\n");
                return 0;
            }

            // checking for header/footer consistency
            if(currBlock->valid != footer.valid || currBlock->size != footer.size){
                printf("ERR: MISMATCHED HEADER & FOOTER\n");
                return 0;
            }

            blockHeader* nextBlock = blockHeader_getNext(currBlock);

            // checking for coalescion
            if(!nextBlock->valid){
                printf("ERR: TWO ADJACENT FREE BLOCKS FOUND\n");
                return 0;
            }

            // tag the current free block, and untag later in segregated list traversal
            currBlock->valid |= 2;
            freeCount++;
        } else if(currBlock->valid == 1){
            blockHeader footer = *blockHeader_getFooter(currBlock);

            // checking for allocated block size consistency
            if(currBlock->size < 2 * HSIZE){
                printf("ERR: ALLOCATED BLOCK TOO SMALL\n");
                return 0;
            }

            // checking for header/footer consistency
            if(currBlock->valid != footer.valid || currBlock->size != footer.size){
                printf("ERR: MISMATCHED HEADER & FOOTER\n");
                return 0;
            }
        } else {
            // valid mask consistency
            printf("ERR: INVALID BLOCK HEADER\n");
            return 0;
        }

        // checking for alignment
        if(((long) currBlock + HSIZE) % ALIGNMENT != 0){
            printf("ERR: MISALIGNED BLOCK\n");
            return 0;
        }

        currBlock = blockHeader_getNext(currBlock);
    }

    // did the implicit list traversal find the epilogue block?
    if(!epilogueFound){
        printf("ERR: IMPLICIT LIST TRAVERSAL DOESN'T END AT EPILOGUE\n");
        return 0;
    }

    for(int i = 0; i < BINCOUNT; i++){
        blockHeader* currBlock = segregatedStart[i];

        if(!currBlock){
            continue;
        }

        // checking for head->prev == NULL
        if(*freeList_getPrev(currBlock)){
            printf("ERR: HEAD OF EXPLICIT LIST HAS A NON-NULL PREV POINTER\n");
            return 0;
        }

        while(currBlock){
            // checking for explicit list pointer validity
            if((void*) currBlock < mem_heap_lo() || mem_heap_hi() <= (void*) currBlock){
                printf("ERR: FREE LIST POINTS OUTSIDE OF HEAP\n");
                return 0;
            }

            // checking if this free block has been unmasked previously
            if(!(currBlock->valid & 2)){
                printf("ERR: SEGREGATED LISTS CONTAIN SAME BLOCK TWICE\n");
                return 0;
            }

            currBlock->valid ^= 2;
            freeCount--;

            // checking if explicit list contains an allocated block
            if(currBlock->valid){
                printf("ERR: EXPLICIT LIST CONTAINS NON-FREE BLOCK\n");
                return 0;
            }

            // checking if this block is in the proper size bin
            if(getBin(currBlock->size) != i){
                printf("ERR: FREE BLOCK STORED IN WRONG SEGREGATED LIST\n");
                return 0;
            }

            // does next->prev return the current block?
            if(*freeList_getNext(currBlock) && *freeList_getPrev(*freeList_getNext(currBlock)) != currBlock){
                printf("ERR: NEXT->PREV doesn't return the same element\n");
                return 0;
            }

            // does prev->next return the current block?
            if(*freeList_getPrev(currBlock) && *freeList_getNext(*freeList_getPrev(currBlock)) != currBlock){
                printf("ERR: PREV->NEXT doesn't return the same element\n");
                return 0;
            }

            currBlock = *freeList_getNext(currBlock);
        }
    }

    // did we leave some blocks unmasked?
    if(freeCount > 0){
        printf("ERR: FREE BLOCKS NOT FOUND IN SEGREGATED LIST\n");
        return 0;
    }

    return 1;
}
