/*--------------------------------------------------------------------*/
/* heapmgr.h                                                          */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef HEAPMGR_INCLUDED
#define HEAPMGR_INCLUDED

#include <stddef.h>

/*--------------------------------------------------------------------*/

/* Allocate and return the address of a chunk of memory that is large
   enough to hold an object whose size is uBytes bytes. The chunk is
   guaranteed to be properly aligned for data of any type. Return NULL
   if uBytes is 0 or the request cannot be satisfied. The chunk is
   uninitialized. */

void *HeapMgr_malloc(size_t uBytes);

/*--------------------------------------------------------------------*/

/* Free the chunk of memory pointed to by pv. pv must point to a chunk
   that was allocated by HeapMgr_malloc(). Do nothing if pv is NULL. */

void HeapMgr_free(void *pv);

#endif
