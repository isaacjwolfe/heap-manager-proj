/*--------------------------------------------------------------------*/
/* heapmgr1.c                                                         */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#define _GNU_SOURCE

#include "heapmgr.h"
#include <unistd.h>

/* On our system the largest type is long double. */
typedef long double LargestType;

/*--------------------------------------------------------------------*/

void *HeapMgr_malloc(size_t uBytes)
{
   static char *pcBrk;

   char *pc = pcBrk;
   char *pcNewBrk;
   
   if (uBytes == 0)
      return NULL;
   
   /* Make sure uBytes is a multiple of the size of the largest
      data type. */
   uBytes = 
      (((uBytes - 1) / sizeof(LargestType)) + 1) * sizeof(LargestType);
   
   /* If this is the first call, then initialize. */
   if (pc == NULL)
      pc = sbrk(0);
   
   /* Move the heap end. */
   pcNewBrk = pc + uBytes;
   if (pcNewBrk < pc) /* Check for overflow. */
      return NULL;
   if (brk(pcNewBrk) == -1)
      return NULL;
   pcBrk = pcNewBrk;
   
   return (void*)pc;
}

/*--------------------------------------------------------------------*/

void HeapMgr_free(void *pv)
{
}
