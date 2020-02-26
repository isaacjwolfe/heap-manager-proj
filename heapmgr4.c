/*--------------------------------------------------------------------*/
/* heapmgr4.c                                                         */
/* Author: Isaac Wolfe and Isaac Hart                                 */
/*--------------------------------------------------------------------*/

#define _GNU_SOURCE

#include "heapmgr.h"
#include "checker4.h"
#include "chunk4.h"
#include <stddef.h>
#include <assert.h>
#include <unistd.h>

/*--------------------------------------------------------------------*/

/* The state of the HeapMgr. */

/* The address of the start of the heap. */
static Chunk_T oHeapStart = NULL;

/* The address immediately beyond the end of the heap. */
static Chunk_T oHeapEnd = NULL;

/* The free list is a list of all free Chunks. It is kept in
   ascending order by memory address. */
static Chunk_T oFreeList = NULL;

/*--------------------------------------------------------------------*/

/* Static function definitions */

/* Get more memory of uUnits. Coalesce as necessary. Return new 
   memory chunk.  */
static Chunk_T HeapMgr_getMoreMemory(size_t uUnits);

/* Remove oChunk from the free list. */
static void HeapMgr_remove(Chunk_T oChunk);

/* Use oChunk to store uUnits. Split it if is it sufficiently large
   and coalesce as necessary. Return used chunk. */
static Chunk_T HeapMgr_useChunk(Chunk_T oChunk, size_t uUnits);

/*--------------------------------------------------------------------*/

/* Request more memory from the operating system -- enough to store
   uUnits units. Create a new chunk, and appends it to the
   front of free list, coalescing with oPrevChunk if necessary.
   Return the address of the new (or enlarged) chunk. */

static Chunk_T HeapMgr_getMoreMemory(size_t uUnits)
{
   const size_t MIN_UNITS_FROM_OS = 512;
   Chunk_T oChunk;
   Chunk_T oNewHeapEnd;
   Chunk_T oPrevChunkInMemory;
   size_t uBytes;
   size_t uNewUnits;

   if (uUnits < MIN_UNITS_FROM_OS)
      uUnits = MIN_UNITS_FROM_OS;

   /* Move the program break. */
   uBytes = Chunk_unitsToBytes(uUnits);
   oNewHeapEnd = (Chunk_T)((char*)oHeapEnd + uBytes);
   if (oNewHeapEnd < oHeapEnd)  /* Check for overflow */
      return NULL;
   if (brk(oNewHeapEnd) == -1)
      return NULL;
   oChunk = oHeapEnd;
   oHeapEnd = oNewHeapEnd;

   /* Set the fields of the new chunk. */
   Chunk_setUnits(oChunk, uUnits);
/*   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL); */
   Chunk_setStatus(oChunk, CHUNK_FREE);

   /* Set prev if free list exists. */
   if (oFreeList != NULL) Chunk_setPrevInList(oFreeList, oChunk);
   
   /* Set remaining parameters. */
   Chunk_setNextInList(oChunk, oFreeList);
   oFreeList = oChunk;
   Chunk_setPrevInList(oFreeList, NULL);

   /* Coalesce the new chunk and the previous one if appropriate. */
   oPrevChunkInMemory = Chunk_getPrevInMem(oChunk, oHeapStart);
   if ((oPrevChunkInMemory != NULL) 
   	&& (Chunk_getStatus(oPrevChunkInMemory) == CHUNK_FREE))
   {

   	/* Remove chunks. */
   	HeapMgr_remove(oChunk);
   	HeapMgr_remove(oPrevChunkInMemory);

   	/* Coalesce chunks. */
   	/* Calculate and set units */
   	uNewUnits = Chunk_getUnits(oPrevChunkInMemory) 
   	+ Chunk_getUnits(oChunk);
   	Chunk_setUnits(oPrevChunkInMemory, uNewUnits);

   	/* Insert expanded chunk at front of list. */
   	Chunk_setStatus(oPrevChunkInMemory, CHUNK_FREE);
   	if (oFreeList != NULL) 
   		Chunk_setPrevInList(oFreeList, oPrevChunkInMemory);
   	Chunk_setNextInList(oPrevChunkInMemory, oFreeList);
   	oFreeList = oPrevChunkInMemory;
   	Chunk_setPrevInList(oFreeList, NULL);

   	/* Make oChunk the coalesced chunk. */
   	oChunk = oFreeList;
   }
 
   return oChunk;
}

/*--------------------------------------------------------------------*/

/* Remove oChunk from free list. */
static void HeapMgr_remove(Chunk_T oChunk)
{
	Chunk_T oPrevChunk = Chunk_getPrevInList(oChunk);
	Chunk_T oNextChunk = Chunk_getNextInList(oChunk);

	/* Make oFreeList NULL if oChunk is the last chunk in list. */
	if ((oNextChunk == NULL) && (oPrevChunk == NULL))
	{
		oFreeList = NULL;
		return;
	}

	/* If the is no previous chunk, then oChunk is being removed from
	   the front of the list, so make oFreeList the next chunk. */
	if (oPrevChunk == NULL) 
	{
		oFreeList = oNextChunk;
		Chunk_setPrevInList(oFreeList, NULL);
		return;
	}

	/* Close gap if oChunk is somewhere in the middle. */
	if (oNextChunk != NULL)
	{
		Chunk_setNextInList(oPrevChunk, oNextChunk);
		Chunk_setPrevInList(oNextChunk, oPrevChunk);
		return;
	}

	/* Otherwise, oChunk is the last chunk, so set oPrevChunk's 
	   next to NULL */
	else 
	{
		Chunk_setNextInList(oPrevChunk, NULL);
		return;
	}
} 

/*--------------------------------------------------------------------*/

/* If oChunk is close to the right size (as specified by uUnits),
   then splice oChunk out of the free list (using oPrevChunk to do
   so), and return oChunk. If oChunk is too big, split it and return
   the address of the tail end.  */

static Chunk_T HeapMgr_useChunk(Chunk_T oChunk, size_t uUnits)
{
   Chunk_T oNewChunk;
   size_t uChunkUnits;
   size_t newChunkUnits;

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   uChunkUnits = Chunk_getUnits(oChunk);

   /* Remove oChunk from free list */
   HeapMgr_remove(oChunk);
   
   /* If oChunk is close to the right size, then use it. */
   if (uChunkUnits < uUnits + MIN_UNITS_PER_CHUNK)
   {
   	Chunk_setStatus(oChunk, CHUNK_INUSE);
   	return oChunk;
   }

   /* oChunk is too big, so use the front end of it and insert
      tail end back on to free list. */
   /* Calculate units of new chunk. */
   newChunkUnits = uChunkUnits - uUnits;
   Chunk_setUnits(oChunk, uUnits);
   oNewChunk = Chunk_getNextInMem(oChunk, oHeapEnd);
   Chunk_setUnits(oNewChunk, newChunkUnits);

   /* Set statuses of chunks */
   Chunk_setStatus(oChunk, CHUNK_INUSE);
   Chunk_setStatus(oNewChunk, CHUNK_FREE);

   /* Insert new chunk at front of list. */ 
	if (oFreeList != NULL) 
   		Chunk_setPrevInList(oFreeList, oNewChunk);
   	Chunk_setNextInList(oNewChunk, oFreeList);
   	oFreeList = oNewChunk;
   	Chunk_setPrevInList(oFreeList, NULL);

   	return oChunk;
}

/*--------------------------------------------------------------------*/

void *HeapMgr_malloc(size_t uBytes)
{
   Chunk_T oChunk;
   size_t uUnits;

   if (uBytes == 0)
      return NULL;

   /* Step 1: Initialize the heap manager if this is the first call. */
   if (oHeapStart == NULL)
   {
      oHeapStart = (Chunk_T)sbrk(0);
      oHeapEnd = oHeapStart;
   }

   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

   /* Step 2: Determine the number of units the new chunk should
      contain. */
   uUnits = Chunk_bytesToUnits(uBytes);

   /* Step 3: For each chunk in the free list... */
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      /* If oChunk is big enough, then use it. */
      if (Chunk_getUnits(oChunk) >= uUnits)
      {
         oChunk = HeapMgr_useChunk(oChunk, uUnits);
         assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
         return Chunk_toPayload(oChunk);
      }
   }

   /* Step 4: Ask the OS for more memory, and create a new chunk (or
      expand the existing chunk) at the front of the free list. */
   oChunk =  HeapMgr_getMoreMemory(uUnits);
   if (oChunk == NULL)
   {
      assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
      return NULL;
   }

   /* Step 5: oChunk is big enough, so use it. */
   oChunk = HeapMgr_useChunk(oChunk, uUnits);
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
   return Chunk_toPayload(oChunk);
}

/*--------------------------------------------------------------------*/

void HeapMgr_free(void *pv)
{
   Chunk_T oChunk;
   Chunk_T oNextChunk;
   Chunk_T oPrevChunk;

   assert(pv != NULL);
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

   if (pv == NULL)
      return;

   oChunk = Chunk_fromPayload(pv);


   /* Insert given chunk at front of free list. */
   	if (oFreeList != NULL) 
   		Chunk_setPrevInList(oFreeList, oChunk);
   	Chunk_setNextInList(oChunk, oFreeList);
   	oFreeList = oChunk;
   	Chunk_setPrevInList(oFreeList, NULL);
   	/* Set staus of given chunk to free. */
   Chunk_setStatus(oChunk, CHUNK_FREE);

   	/* If appropriate, coalesce the given chunk and the next or
   	   prev one in memory. */
   	oPrevChunk = Chunk_getPrevInMem(oChunk, oHeapStart);
   	oNextChunk = Chunk_getNextInMem(oChunk, oHeapEnd);



   	 /* Check oNextCHunk... */
   	if ((oNextChunk != NULL)
   	 && (Chunk_getStatus(oNextChunk) == CHUNK_FREE)) 
   	 {

   	 	/* Coalesce it if it's free. */
   	 	HeapMgr_remove(oNextChunk);
   	 	HeapMgr_remove(oChunk);

   	 	Chunk_setUnits(oChunk, Chunk_getUnits(oChunk) +
   	 		Chunk_getUnits(oNextChunk));

   	 /* Insert chunk in front of list if it was coalesced */
   	 Chunk_setStatus(oChunk, CHUNK_FREE);
   	 if (oFreeList != NULL) 
   	 	Chunk_setPrevInList(oFreeList, oChunk);
   	 Chunk_setNextInList(oChunk, oFreeList);
   	 oFreeList = oChunk;
   	 Chunk_setPrevInList(oFreeList, NULL);
   	 }	

   	/* Check oPrevChunk... */
   	if ((oPrevChunk != NULL)
   	 && (Chunk_getStatus(oPrevChunk) == CHUNK_FREE)) 
   	 {
   	 	/* Coalesce it if it's free. */
   	 	HeapMgr_remove(oChunk);
   	 	HeapMgr_remove(oPrevChunk);

   	 	Chunk_setUnits(oPrevChunk, Chunk_getUnits(oPrevChunk) +
   	 		Chunk_getUnits(oChunk));


   	 /* Insert chunk in front of list if it was coalesced */
   	 Chunk_setStatus(oPrevChunk, CHUNK_FREE);
   	 if (oFreeList != NULL) 
   	 	Chunk_setPrevInList(oFreeList, oPrevChunk);
   	 Chunk_setNextInList(oPrevChunk, oFreeList);
   	 oFreeList = oPrevChunk;
   	 Chunk_setPrevInList(oFreeList, NULL);
   	 }	

   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
}