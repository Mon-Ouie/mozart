/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
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

#ifndef __STACK_H__
#define __STACK_H__

#ifdef INTERFACE
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "mem.hh"

//*****************************************************************************
//                Definition of class Stack 
//*****************************************************************************

typedef void* StackEntry;

typedef enum {
  Stack_WithMalloc,
  Stack_WithFreelist
} StackAllocation;

class Stack {
protected:
  StackEntry *tos;   // top of stack: pointer to first UNUSED cell
  StackEntry *array; 
  StackEntry *stackEnd;

  StackAllocation stkalloc;
  void resize(int newSize);

  void reallocate(int newsize);

  void deallocate(StackEntry *p, int n) 
  { 
    if (stkalloc==Stack_WithMalloc)
      free(p);
    else
      freeListDispose(p, n*sizeof(StackEntry));
  }
  
  void allocate(int sz, int alloc)
  {
    int auxsz = sz*sizeof(StackEntry);
    array = alloc==Stack_WithMalloc 
         ? (StackEntry*)malloc(auxsz):(StackEntry*)freeListMalloc(auxsz);
    if(array==NULL) { // mm2: bad message and crash follows
      OZ_error("Cannot alloc stack memory at %s:%d.", __FILE__, __LINE__);
    }
    tos = array;
    stackEnd = array+sz;
  }

  void allocate(int sz) { allocate(sz,stkalloc); }
  

public:
  Stack(int sz, StackAllocation alloc) : stkalloc(alloc) { allocate(sz,alloc); }
  ~Stack() { deallocate(array,stackEnd-array); }

  void mkEmpty(void) { tos = array; }
  Bool isEmpty(void) { return (tos <= array); }
  StackEntry *ensureFree(int n)
  {
    StackEntry *ret = tos;
    if (stackEnd <= tos+n) {
      resize(n);
      ret = tos;
    }
    return ret;
  }
  void checkConsistency()
  {
    Assert((tos >= array) && (tos <= stackEnd));
  }

  void push(StackEntry value, Bool check=OK)
  {
    checkConsistency();
    if (check) { ensureFree(1); }
    *tos = value;
    tos++;
  }

  StackEntry topElem() { return *(tos-1); }

  StackEntry pop(int n=1)
  {
    checkConsistency();
    Assert(isEmpty() == NO);
    tos -= n;
    return *tos;
  }

  int getMaxSize()  { return (stackEnd-array); }
  int getUsed()     { return (tos-array); }
};


#endif //__STACK_H__
