/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Copyright:
 *    Organization or Person (Year(s))
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "mem.hh"
#endif

#include "mem.hh"
#include "os.hh"
#include "am.hh"

#include <unistd.h>

#ifdef CS_PROFILE
Bool across_chunks;
#endif

// ----------------------------------------------------------------
// heap memory

// allocate 1000 kilo byte chuncks of memory

MemChunks *MemChunks::list = NULL;

unsigned int heapTotalSize;
unsigned int heapTotalSizeBytes;

#ifndef HEAPTOPINTOREGISTER
char *heapTop;
#endif

char *heapEnd;

void initMemoryManagement(void) {
  // init free store list
  for(int i=freeListMaxSize; i--; )
    FreeList[i] = NULL;

  nextChopSize = 8; // Do not change until you know what you are doing!

  // init heap memory
  heapTotalSizeBytes = heapTotalSize = 0;
  heapTop       = NULL;

  // allocate first chunck of memory;
  MemChunks::list = NULL;
  (void) getMemFromOS(0);
}


// ----------------------------------------------------------------
// free list memory

void *FreeList[freeListMaxSize];
size_t nextChopSize;

void heapFree(void * /* addr */)
{
//  error("Heap Free: called: @ %d\n",addr);
}

// count bytes free in FreeList memory
unsigned int getMemoryInFreeList() {
  unsigned int sum = 0;
  void *ptr;

  for (int i=0; i<freeListMaxSize; i++) {
    ptr = FreeList[i];
    while(ptr != NULL) {
      sum += i;
      ptr = *(void **)ptr;
    }
  }

  return sum;
}

#ifdef DEBUG_MEM
// this function is intended to check the consistency of the free list
// memory
void scanFreeList(void) {
  for (int i = freeListMaxSize; i--; )
    for (void * ptr = FreeList[i]; ptr; ptr = * (void **) ptr);
}
#endif

void freeListChop(void * addr, size_t size) {
  // Chop the chunk into pieces of the likely sizes
  // Likely sizes are between 8 and 32.

  register size_t cs = nextChopSize;

  if (nextChopSize > 32)
    nextChopSize = 8;
  else
    nextChopSize += 4;

  register size_t s     = size;
  register void ** fl   = &(FreeList[cs]);
  register void * prev  = *fl;
  register void * small = addr;

  do {
    *((void **) small) = prev;
    prev = small;
    small = (void *) ((char *) small + cs);
    s     -= cs;
  } while (s > cs);

  *fl = prev;

}

// ----------------------------------------------------------------
// mem from os with 2 alternatives SBRK or MALLOC

#if !defined(CCMALLOC) && defined(HAVE_SBRK)

/* remember the last sbrk(0), if it changed --> malloc needs more
 * memory, so call fakeMalloc
 */
static void *lastBrk = 0;

class SbrkMemory {
 public:
  /* a list containing all free blocks in ascending order */
  static SbrkMemory *freeList;

  void *oldBrk;   /* brk value before allocation of this block */
  void *newBrk;   /* new brk value after allocation of this block */
  int size;         /* size of this block including size of this class */
  SbrkMemory *next; /* next free block */

  void print() {
    if (this != NULL) {
      printf("new = 0x%p\nsize = %d\nnext = 0x%p\n\n\n",
             newBrk,size,next);
      next->print();
    }
  }

  /* add a block to the free list, ascending order */
  SbrkMemory *add(SbrkMemory*elem)
  {
    if (this == NULL) {
      elem->next = NULL;
      return elem;
    } else {
      if (elem->newBrk < newBrk) {
        elem->next = this;
        return elem;
      } else {
        next = next->add(elem);
        return this;
      }
    }
  }


  /* release all blocks, that are on top of our UNIX processes heap */

  SbrkMemory *shrink()
  {
    if (this == NULL)
      return NULL;

    next = next->shrink();

    if (newBrk == sbrk(0)) {
#ifdef DEBUG_TRACEMEM
      printf("*** Returning %d bytes to the operating system\n",size);
#endif
#ifdef NETBSD
      int ret = (int)brk((char*)oldBrk);
#else
      int ret = brk(oldBrk);
#endif
      lastBrk = sbrk(0);
      if (ret == -1) {
        error("*** Something wrong when shrinking memory");
      }
      return NULL;
    }

    return this;
  }


  /* find a free block with size sz and bind it to ptr,
     return new free list */

  SbrkMemory *find(int sz, void *&ptr)
  {
    if (this == NULL) {
      return NULL;
    }

    if (sz <= size) {
      ptr = (void *) (this +1);
#ifdef DEBUG_TRACEMEM
      printf("*** Reusing %d bytes from free list\n", size);
#endif
      return next;
    }

    next = next->find(sz,ptr);
    return this;
  }
};


SbrkMemory *SbrkMemory::freeList = NULL;

/* allocate memory via sbrk, first see if there is
   a block in free list */




/* first we allocate space via malloc and release it directly: this means
 * that future malloc's will use this area. In this way the heaps, that are
 * allocated via sbrk, will be rather surely on top of the UNIX process's
 * heap, so we can release this space again!
 */

static void fakeMalloc(int sz)
{
  void *p = malloc(sz);
  sbrk(sizeof(long)); /* ensures that following free does not hand back mem to OS */
  free(p);
}

void *ozMalloc(int chunk_size)
{
  static int firstCall = 1;

  if (firstCall == 1) {
    firstCall = 0;
    fakeMalloc(3*MB);
  }

  chunk_size += sizeof(SbrkMemory);
  void *ret = NULL;
  SbrkMemory::freeList = SbrkMemory::freeList->find(chunk_size,ret);
  if (ret != NULL) {
    return ret;
  } else {
#ifdef DEBUG_TRACEMEM
    printf("*** Allocating %d bytes\n",chunk_size);
#endif
    void *old = sbrk(0);
    if (lastBrk && old != lastBrk) {
      DebugCheckT(message("fakeMallocing 1MB\n"));
      // message("fakeMallocing 1MB\n");
      fakeMalloc(1*MB);
      old = sbrk(0);
    }
    void *ret_val = sbrk(chunk_size);
    lastBrk = sbrk(0);
    if (ret_val == (caddr_t) - 1) {
      fprintf(stderr,"Virtual memory exhausted\n");
      am.exitOz(1);
    }

    SbrkMemory *newMem = (SbrkMemory *) ret_val;
    newMem->oldBrk = old;
    newMem->newBrk = lastBrk;
    newMem->size   = chunk_size;
    newMem->next   = NULL;

    return (newMem + 1);
  }
}



/* free via sbrk */

void ozFree(void *p)
{
  SbrkMemory::freeList =
    (SbrkMemory::freeList->add(((SbrkMemory *)p)-1))->shrink();
}

#else

void ozFree(void *addr) {
  free(addr);
}

void *ozMalloc(size_t size) {
  return malloc(size);
}

#endif



void MemChunks::deleteChunkChain()
{
  MemChunks *aux = this;
  while (aux) {
#ifdef DEBUG_GC
//    memset(aux->block,0x14,aux->xsize);
    memset(aux->block,-1,aux->xsize);
#endif
    ozFree(aux->block);

    MemChunks *aux1 = aux;
    aux = aux->next;
    delete aux1;
  }
}

// 'inChunkChain' returns 1 if value points into chunk chain otherwise 0.
Bool MemChunks::inChunkChain(void *value)
{
  for (MemChunks *aux = this; aux != NULL; aux = aux->next) {
    if (aux->isInBlock((char*)value))
      return OK;
  }
  return NO;
}

/*
 * debugging aids for memory problems
 */
Bool MemChunks::isInHeap(TaggedRef term)
{
  if (oz_isRef(term) && term != makeTaggedNULL() &&
      !list->inChunkChain(tagged2Ref(term))) {
    return NO;
  }
  if (!oz_isRef(term)) {
    switch (tagTypeOf (term)) {
    case UVAR:
      // FUT
    case LTUPLE:
    case OZCONST:
      // if (oz_isBigInt(term)) return OK; // mm2
      if (oz_isBuiltin(term)) return OK;
      // no break
    case SRECORD:
      if (!list->inChunkChain(tagValueOf(term))) {
        return NO;
      }
      break;
    default:
      break;
    }
  }
  return OK;
}

Bool MemChunks::areRegsInHeap(TaggedRef *regs, int sz)
{
  for (int i=0; i<sz; i++) {
    if (!isInHeap(regs[i])) {
      return NO;
    }
  }
  return OK;
}


void MemChunks::print()
{
  MemChunks *aux = this;
  while (aux) {
    printf(" chunk( from: 0x%p, to: 0x%p )\n",
           aux->block, aux->block + aux->xsize);
    aux = aux->next;
    if (aux) {
      printf("  --> ");
    }
  }
}


void *heapMallocOutline(size_t chunk_size)
{
  Assert((int) chunk_size <= ozconf.heapBlockSize);

  return heapMalloc(chunk_size);
}


char *getMemFromOS(size_t sz) {
  int thisBlockSz = ozconf.heapBlockSize;
  if ((int)sz > ozconf.heapBlockSize) {
    thisBlockSz = sz;
    warning("Allocating very large heap block: size = %d kB",sz/KB);
    //    warning("Memory chunk too big (size=%d)\nTry\n\tsetenv OZHEAPBLOCKSIZE <x>\nwhere <x> is greater than %d.\n",sz,ozconf.heapBlockSize);
    //    am.exitOz(1);
  }

  heapTotalSize      += thisBlockSz/KB;
  heapTotalSizeBytes += thisBlockSz;

  if (ozconf.heapMaxSize != -1 &&
      (heapTotalSize > ((100 + ozconf.heapTolerance) *
                        (unsigned long) ozconf.heapMaxSize) / 100)) {
    int newSize = (heapTotalSize * 3) / 2;

    if (ozconf.runningUnderEmacs) {
      printf("\n\n*** Heap maxsize exceeded. Increase from %d to %d? (y/n) ",
             ozconf.heapMaxSize,newSize);
      prefixError();
      fflush(stdout);
      char buf[1000];

      if (osfgets(buf, 1000, stdin) == 0 || buf[0] == 'n')
        am.exitOz(1);
    }

    ozconf.heapMaxSize = newSize;
  }

  heapEnd = (char *) ozMalloc(thisBlockSz);

  if (heapEnd == NULL) {
    fprintf(stderr,"Virtual memory exceeded\n");
    am.exitOz(1);
  }

  /* align heapEnd to word boundaries */
  while(ToInt32(heapEnd)%WordSize != 0) {
    heapEnd++;
  }

  /* initialize with zeros */
  DebugCheckT(memset(heapEnd,0,thisBlockSz));

  heapTop = heapEnd+thisBlockSz;

  // message("heapTop: %p\n",heapTop);

  void *aux = tagValueOf(makeTaggedMiscp(heapTop));
  if (aux != heapTop) {
    warning("Oz address space exhausted: %p != %p\n", aux, heapTop);
    am.exitOz(1);
  }

  MemChunks::list = new MemChunks(heapEnd,MemChunks::list,thisBlockSz);

  //  DebugCheck(heapTotalSize > thisBlockSz/KB,
  //message("Increasing heap memory to %d kilo bytes\n",heapTotalSize));

#ifdef CS_PROFILE
  across_chunks = OK;
#endif

  heapTop -= sz;
  return heapTop;
}


#ifdef OUTLINE
#define inline
#include "mem.icc"
#undef inline
#endif
