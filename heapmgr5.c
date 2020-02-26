/*--------------------------------------------------------------------*/
/* heapmgr5.c                                                         */
/* Author: Isaac Wolfe and Isaac Hart                                 */
/*--------------------------------------------------------------------*/

#define _GNU_SOURCE

#include "heapmgr.h"
#include "checker5.h"
#include "chunk5.h"
#include <stddef.h>
#include <assert.h>
#include <unistd.h>

/*--------------------------------------------------------------------*/

/* Size of the bin array and maximum. */
enum {BIN_MAX = 1024};

/*--------------------------------------------------------------------*/

/* The state of the HeapMgr. */

/* The address of the start of the heap. */
static Chunk_T oHeapStart = NULL;

/* The address immediately beyond the end of the heap. */
static Chunk_T oHeapEnd = NULL;

/* Integer array to contain the bins */
static Chunk_T bins[BIN_MAX];

/*--------------------------------------------------------------------*/

/* Static function definitions */

/* Get more memory of uUnits. Coalesce as necessary. Returns the 
   increased memory chunk. */
static Chunk_T HeapMgr_getMoreMemory(size_t uUnits);

/* Remove oChunk from the free list. */
static void HeapMgr_remove(Chunk_T oChunk);

/* Use oChunk to store uUnits. Split it if is it sufficiently large
   and coalesce as necessary. Returns the chunk in use. */
static Chunk_T HeapMgr_useChunk(Chunk_T oChunk, size_t uUnits);

/* Use uUnits to find a usable chunk in the bins array. Return the
   chunk of memory that works for uUnits. */
static Chunk_T HeapMgr_findUsableChunk(size_t uUnits);

/*--------------------------------------------------------------------*/

/* Request more memory from the operating system -- enough to store
   uUnits units. Create a new chunk, and appends it to the
   front of its corresponding bin, coalescing with oPrevChunk if
   necessary. Return the address of the new (or enlarged) chunk. */

static Chunk_T HeapMgr_getMoreMemory(size_t uUnits)
{
   const size_t MIN_UNITS_FROM_OS = 512;
   Chunk_T oChunk;
   Chunk_T oNewHeapEnd;
   Chunk_T oPrevChunkInMemory;
   size_t uBytes;
   size_t uNewUnits;
   int iBinSize;
   int iPrevChunkSize;

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

   /* Insert at front of proper bin.
      Check if current bin size is larger than maximum. */
   iBinSize = (int)Chunk_getUnits(oChunk);
   if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

   /* Set pointers. */
   if (bins[iBinSize] != NULL) 
      Chunk_setPrevInList(bins[iBinSize], oChunk);
   Chunk_setNextInList(oChunk, bins[iBinSize]);
   bins[iBinSize] = oChunk;
   Chunk_setPrevInList(bins[iBinSize], NULL);


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

   	/* Insert expanded chunk in correct bin. */
      iBinSize = (int)Chunk_getUnits(oPrevChunkInMemory);
      if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

      /* Set pointers. */
      if (bins[iBinSize] != NULL) 
         Chunk_setPrevInList(bins[iBinSize], oPrevChunkInMemory);
      Chunk_setNextInList(oPrevChunkInMemory, bins[iBinSize]);
      bins[iBinSize] = oPrevChunkInMemory;
      Chunk_setPrevInList(bins[iBinSize], NULL);

   	/* Make oChunk the coalesced chunk. */
      /* Check if it should now be in max bin */
      iPrevChunkSize = (int)Chunk_getUnits(oPrevChunkInMemory);
      if (iPrevChunkSize > BIN_MAX - 1) oChunk = bins[BIN_MAX -1];
      else oChunk = bins[iPrevChunkSize];
   }
 
   return oChunk;
}

/*--------------------------------------------------------------------*/

/* Remove oChunk from free list. */
static void HeapMgr_remove(Chunk_T oChunk)
{
	Chunk_T oPrevChunk = Chunk_getPrevInList(oChunk);
	Chunk_T oNextChunk = Chunk_getNextInList(oChunk);
   int iBinSize = (int)Chunk_getUnits(oChunk);

   /* Check if current bin size is larger than maximum. */
   if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

	/* Make oFreeList NULL if oChunk is the last chunk in list. */
	if ((oNextChunk == NULL) && (oPrevChunk == NULL))
	{
		bins[iBinSize] = NULL;
		return;
	}

	/* If the is no previous chunk, then oChunk is being removed from
	   the front of the list, so make oFreeList the next chunk. */
	if (oPrevChunk == NULL) 
	{
		bins[iBinSize] = oNextChunk;
		Chunk_setPrevInList(bins[iBinSize], NULL);
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
   int iBinSize;

   assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));

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

   /* Insert expanded chunk in correct bin. */
      iBinSize = (int)Chunk_getUnits(oNewChunk);
      if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

      /* Set pointers. */
      if (bins[iBinSize] != NULL) 
         Chunk_setPrevInList(bins[iBinSize], oNewChunk);
      Chunk_setNextInList(oNewChunk, bins[iBinSize]);
      bins[iBinSize] = oNewChunk;
      Chunk_setPrevInList(bins[iBinSize], NULL);

      return oChunk;
}

/*--------------------------------------------------------------------*/

/* Use uUnits to find a chunk in a bin that is usable. Return NULL
   if there is no such chunk */
static Chunk_T HeapMgr_findUsableChunk(size_t uUnits)
{
   Chunk_T oChunk;
   int startBin;
   int currentBin;

   oChunk = NULL;

   /* Find the right bin in integer form to start with, or max if it 
      is sufficiently large. */
   startBin = (int) uUnits;
   if (startBin > BIN_MAX - 1) startBin = BIN_MAX - 1;

   /* Starting with the start bin, go through each bin until a usable
      chunk is found. */
   currentBin = startBin;
   while (currentBin < BIN_MAX)
   {
      /* Set oChunk. */
      oChunk = bins[currentBin];
      while (oChunk != NULL)
      {
         if (Chunk_getUnits(oChunk) >= uUnits) return oChunk;
         oChunk = Chunk_getNextInList(oChunk);
      }
      currentBin++;
   }
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

   assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));

   /* Step 2: Determine the number of units the new chunk should
      contain. */
   uUnits = Chunk_bytesToUnits(uBytes);

   /* Find a usable chunk for uUnits */
   oChunk = HeapMgr_findUsableChunk(uUnits);

   /* If a usable chunk was found, use it! */
   if (oChunk != NULL) 
   {
      oChunk = HeapMgr_useChunk(oChunk, uUnits);

      /* Check validity */
      assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));

      /* Retuen payload of oChunk. */
      return Chunk_toPayload(oChunk); 
   }


   /* If no usable chunk was found, ask the OS for more memory, and 
      create a new chunk (or expand the existing chunk) at the front
      of the appropriate bin. */
   oChunk =  HeapMgr_getMoreMemory(uUnits);
   if (oChunk == NULL)
   {
      assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));
      return NULL;
   }

   /* Step 5: oChunk is big enough, so use it. (Now it should be
      big enough!) */
   oChunk = HeapMgr_useChunk(oChunk, uUnits);
   assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));
   return Chunk_toPayload(oChunk);
}

/*--------------------------------------------------------------------*/

void HeapMgr_free(void *pv)
{
   Chunk_T oChunk;
   Chunk_T oNextChunk;
   Chunk_T oPrevChunk;
   int iBinSize;

   assert(pv != NULL);

   assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));

   if (pv == NULL)
      return;

   oChunk = Chunk_fromPayload(pv);


   /* Insert given chunk in its corresponding bin. */
   iBinSize = (int)Chunk_getUnits(oChunk);
   if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

   /* Set pointers. */
   if (bins[iBinSize] != NULL) 
      Chunk_setPrevInList(bins[iBinSize], oChunk);
   Chunk_setNextInList(oChunk, bins[iBinSize]);
   bins[iBinSize] = oChunk;
   Chunk_setPrevInList(bins[iBinSize], NULL);

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

      /* Insert chunk in corresponding bin. */
      Chunk_setStatus(oChunk, CHUNK_FREE);
      iBinSize = (int)Chunk_getUnits(oChunk);
      if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

      /* Set pointers. */
      if (bins[iBinSize] != NULL) 
         Chunk_setPrevInList(bins[iBinSize], oChunk);
      Chunk_setNextInList(oChunk, bins[iBinSize]);
      bins[iBinSize] = oChunk;
      Chunk_setPrevInList(bins[iBinSize], NULL);
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

      /* Insert chunk in correspinding bin. */
      Chunk_setStatus(oPrevChunk, CHUNK_FREE);
      iBinSize = (int)Chunk_getUnits(oPrevChunk);
      if (iBinSize > BIN_MAX - 1) iBinSize = BIN_MAX - 1;

      /* Set pointers. */
      if (bins[iBinSize] != NULL) 
         Chunk_setPrevInList(bins[iBinSize], oPrevChunk);
      Chunk_setNextInList(oPrevChunk, bins[iBinSize]);
      bins[iBinSize] = oPrevChunk;
      Chunk_setPrevInList(bins[iBinSize], NULL);
   } 

   assert(Checker_isValid(oHeapStart, oHeapEnd, bins, BIN_MAX));
}