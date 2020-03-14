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
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* overwrites the value of the word being pointed to by pointer p with value val */

#define GET_SIZE(p) (GET(p) & ~0x7) /* returns the size of the pointer target. Usually given a pointer to a header or footer. Use 7 because the address is 8-byte aligned, meaning the least significant 3 bits of the address are 0. */
/* This size includes the header, footer, and payload. */
#define GET_ALLOC(p) (GET(p) & 0x1) /* returns only the allocated bit from the given pointer. 1 = allocated, 0 = free. */

#define HDRP(bp) ((char *)(bp)-WSIZE) /* bp is the pointer to the payload. This macro finds the address of its header. */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE) /* returns the address of the footer. subtract the double word size because we are starting at the payload pointer and want to point to the start of the footer. */

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) /* returns the address of the next block's payload */
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) /* returns the address of the previous block's payload */

/* Private Global Variables */

static char *mem_heap; /* points to the first byte of the heap */
static char *mem_brk; /* points to the last byte of the heap plus 1 */
static char *mem_max_addr; /* address of the maximum legal heap address plus 1 */
static char *heap_listp = 0; /* address of the first block in the heap (the prologue block)
The prologue block only contains a header and footer and works to mitigate harmful edge conditions during coalescing
The heap contains one byte of padding, followed by the prologue block */
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/* Function Prototypes */

// void mem_init(void);
// void * mem_sbrk(int incr);
static void * extend_heap(size_t words);
static void *coalesce(void *bp);

// DELETE THESE IF THEY END UP UNUSED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
// /*
//  * mm_init - initialize the memory system model.
//  */
// void mem_init(void)
// {
//   mem_heap - (char *)Malloc(MAX_HEAP);
//   mem_brk = (char *)mem_heap;
//   mem_max_addr = (char *)(mem_heap + MAX_HEAP);
// }

// /*
//  * mem_sbrk - Extends the heap by incr bytes and returns the starting address of the area that was just added onto the heap.
//  */
// void *mem_sbrk(int incr)
// {
//   char *old_brk = mem_brk;
//   if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
//     errno = ENOMEM; /* error signifying no memory */
//     fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
//     return (void *)-1; /* return -1 */
//   }
//   mem_brk += incr; /* increase the heap size by incr bytes */
//   return (void *)old_brk; /* return a pointer to the start of the new part of the heap */
// }

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
  heap_listp += (2*WSIZE); /* point to the footer of the prologue block. */

  if (extend_heap(CHUNKSIZE/WSIZE) == NULL) /* extend the empty heap by CHUNKSIZE bytes */
  {
    return -1;
  }
  return 0;
}

/*
 * extend_heap - Extend the heap by a given number of words by adding a new block
 */
 static void *extend_heap(size_t words)
 {
   char *bp; /* pointer to the payload of the block */
   size_t size; /* (bytes) based on the requested number of words */

   /* ensure that an even number of words is allocated to maintain alignment */
   size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
   if ((long)(bp = mem_sbrk(size)) == -1)
   {
     return NULL; // if we are unable to extend the heap by the requested amount, return null.
   }

   /* initialize the header and footer of the new block and update the location of the epilogue header.  */
   PUT(HDRP(bp), PACK(size, 0)); /* mark the new block as free in its header */
   PUT(FTRP(bp), PACK(size, 0)); /* mark the new block as free in its footer */
   PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); /* move the location of the epilogue. Mark it as allocated */

   /* coalesce if the previous block was free */
   return coalesce(bp); /* return a pointer to the new block */
 }

 /*
  * coalesce - coalesces the requested block with the previous block, next block, both, or neither, depending on if they are allocated.
  */
static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); /* determines if the previous block is allocated. 1 = allocated, 0 = free */
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); /* determines if the next block is allocated */
  size_t size = GET_SIZE(HDRP(bp)); /* size of the newly created free block */

  /* Case 1: both prev and next are allocated. */
  if (prev_alloc && next_alloc)
  {
    return bp;
  }

  /* Case 2: prev is allocated, next is free */
  else if (prev_alloc && !next_alloc)
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))); /* add the size of the next block to our current block */
    PUT(HDRP(bp), PACK(size, 0)); /* update the size of our block in both its header and footer */
    PUT(FTRP(bp), PACK(size, 0));
  }

  /* Case 3: prev is free, next is allocated */
  else if (!prev_alloc && next_alloc)
  {
    size += GET_SIZE(FTRP(PREV_BLKP(bp))); /* add the size of the previous block */
    PUT(FTRP(bp), PACK(size, 0)); /* update the footer of our new block with the new size */
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); /* update the header of the prev block (which will now be part of our current block) with the new size */
    bp = PREV_BLKP(bp); /* point to the new start of the payload */
  }
  /* Case 4: both prev and next are free */
  else
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); /* add the size of the prev and next blocks */
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); /* update the header of the prev block (which will now be part of our current block) with the new size */
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); /* update the footer of the next block (which will now be part of our current block) with the new size */
    bp = PREV_BLKP(bp); /* point to the new start of the payload */
  }
  return bp;
}

/*
 * mm_malloc - adjust the requested block size based on overhead and alignment requirements. Then, search for a fit in the list of free blocks.
 * If not fit can be found, extend the heap. Return the pointer to the payload of the allocated block.
 */
void *mm_malloc(size_t size)
{
  size_t asize; /* adjust the given block size if needed */
  size_t extendsize; /* amount to extend the heap if no fit is found */
  char *bp; /* pointer to the payload */

  if (size == 0)
  {
    return NULL;
  }

  /* adjust the block size if it is less than what is needed for the header and footer, as well as for alignment requirements. */
  if (size <= DSIZE)
  {
    asize = 2 * DSIZE;
  }
  else
  {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);
  }

  /* search the list of free blocks for a fit */
  if ((bp = find_fit(asize)) != NULL) { /* if we are able to find a fit */
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE); /* if we are NOT able to find a fit, extend the heap */
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL) // if we cannot extend the heap, return null */
  {
    return NULL;
  }
  place(bp, asize);
  return bp;
}

/*
 * find_fit - find a free block using a first-fit approach.
 */
static void *find_fit(size_t asize)
{
  void *bp;

  /* start at the prologue and move from block to block until you reach the epilogue (which has size 0) */
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
  {
    /* check each block to see if it is both free and large enough */
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
    {
      return bp;
    }
  }
  return NULL; /* if no fit is found */
}

/*
 * place - allocate the free block you have found, potentially splitting it.
 */
 static void place(void *bp, size_t asize)
 {
   size_t csize = GET_SIZE(HDRP(bp)); /* size of the block */

   if ((csize - asize) >= (ALIGNMENT)) /* if the block is larger than the requested block by at least the minimum word size (8 bytes), split the free block */
   {
     PUT(HDRP(bp), PACK(asize, 1)); /* mark the block as allocated. Split off only the requested (and adjusted) size. */
     PUT(FTRP(bp), PACK(asize, 1));
     bp = NEXT_BLKP(bp);
     PUT(HDRP(bp), PACK(csize-asize, 0)); /* the leftover space is kept free */
     PUT(FTRP(bp), PACK(csize - asize, 0));
   }
   else /* just use the entire free block */
   {
     PUT(HDRP(bp), PACK(csize, 1));
     PUT(FTRP(bp), PACK(csize, 1));
   }
 }

/*
 * mm_free - alter the header and footer to indicate that the given block is now free. Coalesce with surrounding blocks if possible.
 */
void mm_free(void* bp)
{
  size_t size = GET_SIZE(HDRP(bp)); /* get the size of the current block */

  PUT(HDRP(bp), PACK(size, 0)); /* indicate that the block is now free in its header */
  PUT(FTRP(bp), PACK(size, 0)); /* indicate that the block is now free in its footer */
  coalesce(bp); /* coalesce with the surrounding blocks, if possible, and return the pointer to the payload */
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
