/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution. REPLACE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Assignment 2",
    /* First member's full name */
    "Daryn McElroy",
    /* First member's email address */
    "DarynMcElroy2022@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Ethan Piper",
    /* Second member's email address (leave blank if none) */
    "Ethanpiper2020@u.northwestern.edu"
};

/* Constants and Macros*/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4  /* (bytes) the size of words, headers, and footers. */
#define DSIZE 8  /* (bytes) double word size */
#define CHUNKSIZE (1<<12) /* (bytes) the amount to extend the heap by */

#define MAX(x,y) ((x) > (y)? (x) : (y)) /* a macro for selecting the maximum out of two values */

#define PACK(size, alloc) ((size | alloc)) /* bit overloading. Combines the block size and the allocated bit into one word. For use in a header or footer. */

#define GET(p) (*(unsigned int*)(p)) /* reads and returns the word being pointed to by pointer p */
#define PUT(p, val) (*unsigned int *)(p) = (val)) /* overwrites the value of the word being pointed to by pointer p with value val */

#define GET_SIZE(p) (GET(P) & ~0x7) /* returns only the size of the pointer. Usually given a pointer to a header or footer. Use 7 because the address is 8-byte aligned, meaning the least significant 3 bits of the address are 0. */
#define GET_ALLOC(p) (GET(P) & 0x1) /* returns only the allocated bit from the given pointer. 1 = allocated, 0 = not allocated. */

#define HDRP(bp) ((char *)(bp)-WSIZE) /* bp is the pointer to the payload. This macro finds the address of its header. */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE) /* returns the address of the footer. subtract the double word size because we are starting at the payload pointer and want to point to the start of the footer. */

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((CHAR *)(bp) - WSIZE))) /* returns the address of the next block */
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) /* returns the address of the previous block */

/* Private Global Variables */

static char *mem_heap; /* points to the first byte of the heap */
static char *mem_brk; /* points to the last byte of the heap plus 1 */
static char *mem_max_addr; /* address of the maximum legal heap address plus 1 */
static char *heap_listp = 0; /* address of the first block in the heap (the prologue block)
The prologue block only contains a header and footer and works to mitigate harmful edge conditions during coalescing
The heap contains one byte of padding, followed by the prologue block */


/* Function Prototypes */

static void mem_init(void);
static void * mem_sbrk(int incr);
static void * extend_heap(size_t words);

/*
 * mm_init - initialize the memory system model.
 */
void mem_init(void)
{
  mem_heap - (char *)Malloc(MAX_HEAP);
  mem_brk = (char *)mem_heap;
  mem_max_addr = (char *)(mem_heap + MAX_HEAP);
}

/*
 * mem_sbrk - Extends the heap by incr bytes and returns the starting address of the area that was just added onto the heap.
 */
void *mem_sbrk(int incr)
{
  char *old_brk = mem_brk;
  if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
    errno = ENOMEM; /* error signifying no memory */
    fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
    return (void *)-1; /* return -1 */
  }
  mem_brk += incr; /* increase the heap size by incr bytes */
  return (void *)old_brk; /* return a pointer to the start of the new part of the heap */
}

/*
 * mm_init - creates the initially empty heap and extends it by CHUNKSIZE bytes.
 */
int mm_init(void)
{
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
  {
    return -1;
  }
  PUT(heap_listp, 0); /* the heap starts with one word of padding */
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* the prologue consists of two words: the header and footer*/
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* mark the footer of the prologue block as allocated */
  PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* since the heap is empty, the epilogue immediately follows the prologue */
  heap_listp =+ (2*WSIZE); /* point to the footer of the prologue block. */

  if (extend_heap(CHUNKSIZE/WSIZE) == NULL) /* extend the empty heap by CHUNKSIZE bytes */
  {
    return -1;
  }
  return 0;
}

/*
 * extend_heap - Extend the heap by a given number of words
 */
 static void *extend_heap(size_t words)
 {
   char *bp;
   size_t size;

   /* ensure that an even number of words is allocated to maintain alignment */

 }



/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
