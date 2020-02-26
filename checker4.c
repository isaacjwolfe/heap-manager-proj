/*--------------------------------------------------------------------*/
/* checker4.c.                                                        */
/* Author: Isaac Wolfe and Isaac Hart                                 */
/*--------------------------------------------------------------------*/

#include "checker4.h"
#include <stdio.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

/*--------------------------------------------------------------------*/

/* Internal function declarations */

/* Takes an oFreeList and ensures that all of the items are pointing
   to the correct respective previous and next. Returns TRUE if 
   everything is aligned correctly, and FALSE otherwise. */
static int Checker_pointCheck(Chunk_T oFreeList);

/* Takes an oFreeList and ensure that every item in the list is,
   in fact, free. Return TRUE if so and FALSE otherwise. */
static int Checker_allFreeListFree(Chunk_T oFreeList);

/* Traverse oFreeList and sees if oQueryChunk is in oFreeList. 
   Return TRUE if it hits and FALSE otherwise. */
static int Checker_inFreeList(Chunk_T oQueryChunk, Chunk_T oFreeList);

/*--------------------------------------------------------------------*/

/* Takes an oFreeList and ensures that all of the items are pointing
   to the correct respective previous and next. Returns TRUE if 
   everything is aligned correctly, and FALSE otherwise. */
static int Checker_pointCheck(Chunk_T oFreeList)
{
	Chunk_T oChunk;
	Chunk_T oPrevChunk;
	Chunk_T oNextChunk;

	for (oChunk = oFreeList;
		oChunk != NULL;
		oChunk = Chunk_getNextInList(oChunk))
	{
		oPrevChunk = Chunk_getPrevInList(oChunk);
		oNextChunk = Chunk_getNextInList(oChunk);

		/* If prev chunk is not NULL, return FALSE if it does not point
		   to oChunk */
		if (oPrevChunk != NULL)
		{
			if (Chunk_getNextInList(oPrevChunk) != oChunk)
				return FALSE;
		}

		/* If next chunk is not NULL, return FALSE if it does not point
		   to oChunk */
		if (oNextChunk != NULL)
		{
			if (Chunk_getPrevInList(oNextChunk) != oChunk)
				return FALSE;
		}
	}

	return TRUE;
}

/*--------------------------------------------------------------------*/

/* Takes an oFreeList and ensure that every item in the list is,
   in fact, free. Return TRUE if so and FALSE otherwise. */
static int Checker_allFreeListFree(Chunk_T oFreeList)
{
	Chunk_T oChunk;

	for (oChunk = oFreeList;
		oChunk != NULL;
		oChunk = Chunk_getNextInList(oChunk))
	{
		/* Return false if oChunk status is not FREE. */
		if (Chunk_getStatus(oChunk) != CHUNK_FREE)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*--------------------------------------------------------------------*/

/* Traverse oFreeList and sees if oQueryChunk is in oFreeList. 
   Return TRUE if it hits and FALSE otherwise. */
static int Checker_inFreeList(Chunk_T oQueryChunk, Chunk_T oFreeList)
{
	Chunk_T oChunk;

	for (oChunk = oFreeList;
		oChunk != NULL;
		oChunk = Chunk_getNextInList(oChunk))
	{
		/* Return TRUE if query chunk hits. */
		if (oChunk == oQueryChunk) return TRUE;
	}

	return FALSE;
}

/*--------------------------------------------------------------------*/

int Checker_isValid(Chunk_T oHeapStart, Chunk_T oHeapEnd,
   Chunk_T oFreeList)
{
   Chunk_T oChunk;
   Chunk_T oNextChunkInMemory;
   Chunk_T oTortoiseChunk;
   Chunk_T oHareChunk;
   int memCount = 0;
   int listCount = 0;

   /* Do oHeapStart and oHeapEnd have non-NULL values? */
   if (oHeapStart == NULL)
   {
      fprintf(stderr, "The heap start is uninitialized.\n"); 
      return FALSE; 
   }
   if (oHeapEnd == NULL)
   {
      fprintf(stderr, "The heap end is uninitialized.\n");
      return FALSE;
   }

   /* If the heap is empty, is the free list empty too? */
   if (oHeapStart == oHeapEnd)
   {
      if (oFreeList == NULL)
         return TRUE;
      else
      {
         fprintf(stderr, "The heap is empty, but the list is not.\n");
         return FALSE;
      }
   }

   /* Traverse memory. */
    for (oChunk = oHeapStart;
    	oChunk != NULL;
    	oChunk = Chunk_getNextInMem(oChunk, oHeapEnd))
    {

    	/* Is the chunk valid? */
    	if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
    	{
    		fprintf(stderr,"Traversing memory detected a bad chunk.\n");
    		return FALSE;	
    	}

    	/* Is every chunk in memory either in use or in the free list? */
    	if ((Checker_inFreeList(oChunk, oFreeList) == FALSE) 
    		&& (Chunk_getStatus(oChunk) != CHUNK_INUSE))
   		{
   			fprintf(stderr, "A free chunk is not in the free list.\n");
   			return FALSE;
   		}

   		if (Chunk_getStatus(oChunk) == CHUNK_FREE) memCount++;
   	}


   /* Is the list devoid of cycles? Use Floyd's algorithm to find out.
      See the Wikipedia "Cycle detection" page for a description. */
   oTortoiseChunk = oFreeList;
   oHareChunk = oFreeList;
   if (oHareChunk != NULL)
      oHareChunk = Chunk_getNextInList(oHareChunk);
   while (oHareChunk != NULL)
   {
      if (oTortoiseChunk == oHareChunk)
      {
         fprintf(stderr, "The list has a cycle.\n");  
         return FALSE;
      }
      /* Move oTortoiseChunk one step. */
      oTortoiseChunk = Chunk_getNextInList(oTortoiseChunk);
      /* Move oHareChunk two steps, if possible. */
      oHareChunk = Chunk_getNextInList(oHareChunk);
      if (oHareChunk != NULL)
         oHareChunk = Chunk_getNextInList(oHareChunk);
   }

   /* Traverse the free list. */
   for (oChunk = oFreeList;
   	    oChunk != NULL;
   	    oChunk = Chunk_getNextInList(oChunk))
   {
      /* Is the chunk valid? */
   	   if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
   	{
   		fprintf(stderr, 
   			"Traversing the list detected a bad chunk.\n");
   		return FALSE;
   	}

   	if (Chunk_getStatus(oChunk) == CHUNK_FREE) listCount++;

      /* Is the next chunk in memory free? */
   	oNextChunkInMemory = Chunk_getNextInMem(oChunk, oHeapEnd);
   	if ((oNextChunkInMemory != NULL) 
   		&& (Chunk_getStatus(oNextChunkInMemory) == CHUNK_FREE))
   		{
   			fprintf(stderr, 
   				"The heap contains contiguous free chunks.\n");
   			return FALSE;
   		}
   	}
         

   /* Check if memCount of free chunks equals listCount. */
   if (memCount != listCount)
   {
   	fprintf(stderr, 
   		"Number of free chunks in list and mem not equal\n");
   	return FALSE;
   }

   /* Ensure that all items in oFreeList are pointed at their correct
   	  respective prevous and next items. */
   if (! Checker_pointCheck(oFreeList)) 
   {
   	fprintf(stderr, 
   		"One of the items in the free list is misaligned\n");
   	return FALSE;
   }

   /* Ensure that all chunks in the free list are free. */
   if (! Checker_allFreeListFree(oFreeList)) 
   {
   	fprintf(stderr, 
   		"One of the items in the free list is not free\n");
   	return FALSE;
   }

   /* Ensure that oFreeList does not point to a previous. */
   if (oFreeList != NULL)
   {
   	if (Chunk_getPrevInList(oFreeList) != NULL) 
   	{
   		fprintf(stderr, "Previous of oFreeList is non-NULL\n");
   		return FALSE;
   	}
   }

   return TRUE;
}


