/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
 *    Konstantin Popov (kost@sics.se)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 *
 *  Copyright:
 *    Ralf Scheidhauer, 1998
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
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

#ifdef CS_PROFILE
Bool across_chunks;
#endif


MemChunks *MemChunks::list = NULL;

unsigned int heapTotalSize;
unsigned int heapTotalSizeBytes;


#ifndef HEAPCURINTOREGISTER
char * _oz_heap_cur;
#endif
char * _oz_heap_end;


void initMemoryManagement(void) {

  // init heap memory
  heapTotalSizeBytes = heapTotalSize = 0;
  _oz_heap_cur       = NULL;
  _oz_heap_end       = NULL;

  // allocate first chunk of memory;
  MemChunks::list = NULL;
  _oz_getNewHeapChunk(0);

  // init free memory list
  FL_Manager::init();

}


inline
Bool checkAddress(void *ptr) {
  void *aux = tagValueOf(makeTaggedMiscp(ptr));
  return (aux == ptr);
}


// ----------------------------------------------------------------
// mem from os with 3 alternatives MMAP, SBRK or MALLOC

//
// kost@: i have not tested that on anything else than 2.0.*
// Linux-i486 (2.0.7-based) and Solaris-Sparc. And don't risk
// otherwise...

//#if !(defined(LINUX_I486) || defined(SOLARIS_SPARC))
//#undef HAVE_MMAP
//#endif

// RS: mmap does not work under Linux 2.2.0 - 2.2.3 (at least)
//     seems to be a bug in Linux 2.2 itself!
#if !defined(SOLARIS_SPARC)
#undef HAVE_MMAP
#endif

// This is optional - that's kind'f not clear which evil is the
// smallest one: rely on "choose a nearest next free address" linux
// behaviour, or to use the discouraged 'MAP_FIXED' option;
#if defined(LINUX_I486)
#define USE_AUTO_PLACEMENT
#endif

//
#if defined(HAVE_MMAP)

#if ( !defined(MAP_ANONYMOUS) && defined(MAP_ANON) )
#define MAP_ANONYMOUS   MAP_ANON
#endif

#include <sys/mman.h>
#include <fcntl.h>

#if !defined(USE_AUTO_PLACEMENT)
//
// Entries are double-linked with a "core" element;
class MappedMemChunkEntry {
private:
  caddr_t addr;
  size_t size;                  // bytes;
  MappedMemChunkEntry *next, *prev;

  //
protected:
#ifdef DEBUG_CHECK
  void cleanup() {
    Assert(addr == (caddr_t) 0);
    Assert(size == (size_t) 0);
    next = (MappedMemChunkEntry *) -1;
    prev = (MappedMemChunkEntry *) -1;
  }
#endif

  //
public:
  // Make a "core" entry linked to itself:
  MappedMemChunkEntry()
    : addr((caddr_t) 0), size((size_t) 0),
      next((MappedMemChunkEntry *) this),
      prev((MappedMemChunkEntry *) this) {}
  // We require explicit linking:
  MappedMemChunkEntry(caddr_t addrIn, size_t sizeIn)
    : addr(addrIn), size(sizeIn) {
    DebugCode(next = prev = (MappedMemChunkEntry *) -1);
  }
  // We require also explicit unlinking:
  ~MappedMemChunkEntry() {
    Assert(next == (MappedMemChunkEntry *) -1);
    Assert(prev == (MappedMemChunkEntry *) -1);
  }

  //
  caddr_t getAddr() { return (addr); }
  size_t getSize() { return (size); }
  //
  // only 'getNext()' is given: traverse it as it is sorted;
  MappedMemChunkEntry* getNext() { return (next); }

  //
  void linkBefore(MappedMemChunkEntry *before) {
    Assert(next == (MappedMemChunkEntry *) -1);
    Assert(prev == (MappedMemChunkEntry *) -1);
    //
    next = before;
    prev = before->prev;
    before->prev = this;
    prev->next = this;
  }
  void unlink() {
    next->prev = prev;
    prev->next = next;
    DebugCode(next = prev = (MappedMemChunkEntry *) -1);
  }
};

//
class MappedMemChunks : private MappedMemChunkEntry {
private:
  // 'top' is the first upper non-valid address;
  caddr_t top;
  size_t pagesize;              // cached up;

  //
public:
  MappedMemChunks() {
    pagesize = sysconf(_SC_PAGESIZE);
    top = (caddr_t)
      (((((unsigned int32) -1) >> lostPtrBits) / pagesize) * pagesize);
  }
  ~MappedMemChunks() {
    MappedMemChunkEntry *entry = MappedMemChunkEntry::getNext();
    while (entry != this) {
      MappedMemChunkEntry *next = entry->getNext();
      entry->unlink();
      delete entry;
      entry = next;
    }
    DebugCode(cleanup(););
  }

  //
  // Yields&reserves a largest address that would accept a page of
  // 'reqSize' bytes without interferences with other pages.  The
  // (double-linked) list of pages is sorted (in the 'getNext()'
  // direction) in decreasing addresses;
  caddr_t reservePlace(size_t reqSize) {
    caddr_t upper = top;
    MappedMemChunkEntry* entry = MappedMemChunkEntry::getNext();

    //
    while (entry != this) {
      //
      // Is there - between 'upper' and next's page end - enough room
      // for the page?
      caddr_t addr = upper - reqSize;
      if (entry->getAddr() + entry->getSize() <= addr) {
        MappedMemChunkEntry *newCE =
          new MappedMemChunkEntry(addr, reqSize);
        newCE->linkBefore(entry);
        return (addr);
      } else {
        // no, go to the next one:
        upper = entry->getAddr();
        entry = entry->getNext();
      }
    }

    //
    // None found: make a last one (i.e. prior the core one (itself));
    caddr_t addr = upper - reqSize;
    MappedMemChunkEntry *newCE =
      new MappedMemChunkEntry(addr, reqSize);
    newCE->linkBefore(this);
    return (addr);
  }

  //
  void markFree(caddr_t addr, size_t size) {
    MappedMemChunkEntry* entry = getNext();

    //
    while (entry != this) {
      if (entry->getAddr() == addr) {
        Assert(size == entry->getSize());
        entry->unlink();
        delete entry;
        return;
      }
      entry = entry->getNext();
    }
    OZ_error("mappedChunks: has not found a page for removal!");
  }
};

//
static MappedMemChunks mappedChunks;

#endif

//
// Real stuff;
void ozFree(char *addr, size_t size)
{
#if !defined(USE_AUTO_PLACEMENT)
  mappedChunks.markFree(addr, size);
#endif
  if (munmap(addr, size)) {
    ozperror("munmap");
  }
}

void *ozMalloc(size_t size)
{
  static size_t const pagesize = sysconf(_SC_PAGESIZE);
#if !defined(MAP_ANONYMOUS)
  static int devZeroFD = -1;

  //
  if (devZeroFD == -1) {
    devZeroFD = open("/dev/zero", O_RDWR);
    if (devZeroFD < 0) { ozperror("mmap: open /dev/zero"); }
  }
#endif

  // mm2: the interface of ozMalloc is wrong
  //  when allocating more memory than is really used
  //  the memory of the remainder of the page is not used.
  //  ozMalloc should return the size allocated.
  if (size % pagesize != 0) {
    // OZ_warning("*** WEIRD: ozMalloc: pagesize alignment problem ***\n");
    size = (size / pagesize) * pagesize + pagesize;
  }
#if !defined(USE_AUTO_PLACEMENT)
  char *nextAddr = mappedChunks.reservePlace(size);
#endif

  //
#if defined(MAP_ANONYMOUS)
#if defined(USE_AUTO_PLACEMENT)
  void *ret = mmap((char *) 0x1, size, (PROT_READ|PROT_WRITE),
                   (MAP_PRIVATE|MAP_ANONYMOUS),
                   -1, (off_t) 0);
#else
  void *ret = mmap(nextAddr, size, (PROT_READ|PROT_WRITE),
                   (MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED),
                   -1, (off_t) 0);
#endif
#else
  void *ret = mmap(nextAddr, size, (PROT_READ|PROT_WRITE),
                   (MAP_PRIVATE|MAP_FIXED),
                   devZeroFD, (off_t) 0);
#endif
  if (ret < 0) { ozperror("mmap"); }
#ifdef DEBUG_CHECK
  //
  // make test of the created page: write, read, write, read!
  for (char *addr = (char *) ret; addr < ((char *) ret) + size; addr++)
    *addr = (char) 0;
  for (char *addr = (char *) ret; addr < ((char *) ret) + size; addr++)
    if (*addr != (char) 0)
      OZ_error ("mmap check: failed to read zeros");
  for (char *addr = (char *) ret; addr < ((char *) ret) + size; addr++)
    *addr = (char) 0x4f;
  for (char *addr = (char *) ret; addr < ((char *) ret) + size; addr++)
    if (*addr != (char) 0x4f)
      OZ_error ("mmap check: failed to read values");
#endif

  //
  return (ret);
}

#elif defined(HAVE_SBRK)

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

  void print();

  /* add a block to the free list, ascending order */
  SbrkMemory *add(SbrkMemory*elem);

  /* release all blocks, that are on top of our UNIX processes heap */
  SbrkMemory *shrink();

  /* find a free block with size sz and bind it to ptr,
     return new free list */
  SbrkMemory *find(int sz, void *&ptr);

};


void SbrkMemory::print() {
  if (this != NULL) {
    printf("new = 0x%p\nsize = %d\nnext = 0x%p\n\n\n",
           newBrk,size,next);
    next->print();
  }
}

/* add a block to the free list, ascending order */
SbrkMemory * SbrkMemory::add(SbrkMemory*elem)
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

SbrkMemory * SbrkMemory::shrink()
{
  if (this == NULL)
    return NULL;

  next = next->shrink();

  if (newBrk == sbrk(0)) {
#ifdef DEBUG_TRACEMEM
    printf("*** Returning %d bytes to the operating system\n",size);
#endif
#if defined(NETBSD) || defined(OSF1_ALPHA)
    int ret = (int)brk((char*)oldBrk);
#else
    int ret = brk(oldBrk);
#endif
    lastBrk = sbrk(0);
    if (ret == -1) {
      OZ_error("*** Something wrong when shrinking memory");
    }
    return NULL;
  }

  return this;
}


/* find a free block with size sz and bind it to ptr,
   return new free list */

SbrkMemory * SbrkMemory::find(int sz, void *&ptr)
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

SbrkMemory *SbrkMemory::freeList = NULL;

/* allocate memory via sbrk, first see if there is
   a block in free list */




/* first we allocate space via malloc and release it directly: this means
 * that future malloc's will use this area. In this way the heaps, that are
 * allocated via sbrk, will be rather surely on top of the UNIX process's
 * heap, so we can release this space again!
 * Note: linux allocates small chunks from a different area than large ones
 *   so we allocate in many small pieces.
 */

static void fakeMalloc(int sz)
{
  /* not needed under Windows, since we there use VirtualAlloc */
#ifndef WINDOWS
  int chunksz = 1024;
  void **array = new void*[sz/chunksz + 1];
  int i=0;
  while (sz>0) {
    array[i++]= malloc(chunksz);
    sz -= chunksz;
  }
  sbrk(sizeof(long)); /* ensures that following free does not hand back mem to OS */

  while(--i>=0) {
    free(array[i]);
  }
  delete array;
#endif
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
      // DebugCheckT(message("fakeMallocing 2MB\n"));
      fakeMalloc(2*MB);
      old = sbrk(0);
    }
    void *ret_val = sbrk(chunk_size);
    lastBrk = sbrk(0);
    if (ret_val == (caddr_t) - 1) {
      fprintf(stderr,"Mozart: Virtual memory exhausted\n");
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

void ozFree(char *p, size_t ignored)
{
  SbrkMemory::freeList =
    (SbrkMemory::freeList->add(((SbrkMemory *)p)-1))->shrink();
}

#elif defined(WINDOWS)

/* need windows.h: */
#include "wsock.hh"


void ozFree(char *ptr, int sz) {
  if (ptr && VirtualFree(ptr,0,MEM_RELEASE) != TRUE) {
    OZ_warning("free(0x%p) failed: %d\n",ptr,GetLastError());
  }
}


char *ozMalloc0(int sz) {
  char *ret = (char *)VirtualAlloc(0,sz,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);

  if (ret==0)
    return ret;

  if (checkAddress(ret+sz))
    return ret;

  ozFree(ret,sz);

  /* address space exhausted, so we walk the whole address space: */
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  char *base = (char*)si.lpMinimumApplicationAddress;

  while(1) {
    ret = (char*)VirtualAlloc(base,sz,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if (!checkAddress(ret+sz)) {
      ozFree(ret,sz);
      return NULL;
    }

    if (ret!=NULL) {
      return ret;
    }

    if (!checkAddress(base+sz)) // address space exhausted ?
      return NULL;

    base += si.dwAllocationGranularity;
  }
}


char *ozMalloc(int sz)
{
  char *aux = ozMalloc0(sz);
  return aux;
}

#else

void ozFree(char *addr, size_t ignored) {
  free(addr);
}

void *ozMalloc(size_t size) {
  return malloc(size);
}

#endif



void MemChunks::deleteChunkChain() {
  MemChunks *aux = this;
  while (aux) {

#ifdef DEBUG_GC
    memset(aux->block,-1,aux->xsize);
#endif

    ozFree(aux->block,aux->xsize);

    MemChunks *aux1 = aux;
    aux = aux->next;
    delete aux1;
  }
}


#ifdef DEBUG_MEM

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

Bool MemChunks::areRegsInHeap(TaggedRef *regs, int sz) {
  for (int i=0; i<sz; i++) {
    if (!isInHeap(regs[i])) {
      return NO;
    }
  }
  return OK;
}


void MemChunks::print() {
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

#endif


void _oz_getNewHeapChunk(const size_t sz) {
  int thisBlockSz = max(HEAPBLOCKSIZE, (int) sz+WordSize);

  heapTotalSize      += thisBlockSz/KB;
  heapTotalSizeBytes += thisBlockSz;

  if (ozconf.heapMaxSize != -1 &&
      (heapTotalSize > ((100 + ozconf.heapTolerance) *
                        (unsigned long) ozconf.heapMaxSize) / 100)) {
    int newSize = (heapTotalSize * 3) / 2;

    if (ozconf.runningUnderEmacs) {
      printf("\n*** Heap Max Size exceeded. Increasing from %d to %d.\n",
             ozconf.heapMaxSize,newSize);
      prefixError();
      fflush(stdout);
    }

    ozconf.heapMaxSize = newSize;
  }

  size_t malloc_size  = thisBlockSz;
  char * malloc_block = (char *) ozMalloc(malloc_size);

  _oz_heap_end = malloc_block;

  if (_oz_heap_end == NULL) {
    OZ_warning("Mozart: virtual memory exhausted.\n");
    am.exitOz(1);
  }

  /* align _oz_heap_end to word boundaries */
  while (ToInt32(_oz_heap_end) % WordSize != 0) {
    thisBlockSz--;
    _oz_heap_end++;
  }

  DebugCheckT(memset(_oz_heap_end,0,thisBlockSz));

  _oz_heap_cur = _oz_heap_end + thisBlockSz;

  if (!checkAddress(_oz_heap_cur)) {
    OZ_warning("Mozart: address space exhausted.\n");
    am.exitOz(1);
  }

  MemChunks::list = new MemChunks(malloc_block,MemChunks::list,malloc_size);

#ifdef CS_PROFILE
  across_chunks = OK;
#endif

  _oz_heap_cur -= sz;

}

/*
 * Free List Management
 *
 */

FL_Small * FL_Manager::small[FL_SizeToIndex(FL_MaxSize) + 1];
FL_Large * FL_Manager::large;

void FL_Manager::init(void) {
  large    = (FL_Large *) NULL;
  small[0] = NULL;

  for (int i = FL_SizeToIndex(FL_MaxSize); i>0; i--) {
    FL_Small * f = (FL_Small *) heapMalloc(FL_IndexToSize(i));
    f->setNext(NULL);
    small[i] = f;
  }
}

#define FL_RFL_Small 24
#define FL_RFL_N1    4
#define FL_RFL_N2    32

void FL_Manager::refill(const size_t sz) {
  register char * block;
  register size_t n;

  /*
   * Take a large block, if available
   *
   */

  if (large != NULL) {
    FL_Large * l = large;
    large = l->getNext();
    block = (char *) l;
    n     = l->getSize();
  } else {
    n     = ((sz > FL_RFL_Small) ? FL_RFL_N1 : FL_RFL_N2) * sz;
    block = (char *) heapMalloc(n);
  }

  small[FL_SizeToIndex(sz)] = (FL_Small *) block;

  n -= sz;

  while (n >= sz) {
    block += sz;
    n     -= sz;
    ((FL_Small *) (block-sz))->setNext((FL_Small *) block);
  }

  ((FL_Small *) block)->setNext(NULL);

  if (n > 0)
    free(block+sz,n);

}

unsigned int FL_Manager::getSize(void) {
  unsigned int s = 0;

  for (int i = 1; i <= FL_SizeToIndex(FL_MaxSize); i++) {
    FL_Small * f = small[i];
    while (f) {
      s += FL_IndexToSize(i); f = f->getNext();
    }
  }

  FL_Large * f = large;

  while (f) {
    s += f->getSize(); f = f->getNext();
  }

  return s;
}
