/*--------------------------------------------------------------------*/
/* heapmgr2.c                                                         */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#define _GNU_SOURCE

#include "heapmgr.h"
#include <unistd.h>

/* On our system the largest type is long double. */
typedef long double LargestType;

#define MAX(x, y) (x) < (y) ? (y) : (x)

void *HeapMgr_malloc(size_t uBytes)
{  
   enum {MIN_ALLOC = 8192};

   static char *pcBrk;
   static char *pcPad;

   char *pc;
   char *pcNewBrk;
   
   if (uBytes == 0)
      return NULL;
   
   /* Make sure uBytes is a multiple of the size of the largest
      data type. */
   uBytes = 
      (((uBytes - 1) / sizeof(LargestType)) + 1) * sizeof(LargestType);
      
   /* If this is the first call, then initialize. */
   if (pcBrk == NULL)
   { 
      pcBrk = sbrk(0);
      pcPad = pcBrk;
   }
   
   /* Move the heap end (and thereby increase the size of the pad)
      if necessary. */
   if (pcPad + uBytes > pcBrk)
   {  
      pcNewBrk = MAX(pcPad + uBytes, pcBrk + MIN_ALLOC);
      if (pcNewBrk < pcBrk) /* Check for overflow. */
         return NULL;
      if (brk(pcNewBrk) == -1)
         return NULL;
      pcBrk = pcNewBrk;
   }
   
   /* Allocate memory from the pad. */
   pc = pcPad;
   pcPad = pcPad + uBytes;
   return (void*)pc;
}

void HeapMgr_free(void *pv)
{
}
