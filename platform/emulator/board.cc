/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
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
 *     $MOZARTURL$
 *
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#if defined(INTERFACE)
#pragma implementation "board.hh"
#endif

#include "board.hh"

Equation *ScriptAllocate(int size)
{
  return (Equation *) freeListMalloc(size * sizeof(Equation));
}

void ScriptDealloc(Equation *ptr, int size)
{
  if (ptr == (Equation *)0)
    return;
  freeListDispose(ptr,size * sizeof(Equation));
}

Script::Script(int sizeInit)
{
  first = ScriptAllocate(sizeInit);
  numbOfCons = sizeInit;
}

Script::~Script()
{
  ScriptDealloc(first,numbOfCons);
}

void Script::allocate(int sizeInit)
{
  if (sizeInit != 0)
    first = ScriptAllocate(sizeInit);
  else
    first = (Equation *)NULL;
  numbOfCons = sizeInit;
}

void Script::dealloc()
{
  if (numbOfCons != 0) {
    ScriptDealloc(first,numbOfCons);
    first = (Equation *)NULL;
    numbOfCons = 0;
  }
}



/* some random comments:
   flags:
     type:
       Ask - Wait - Root
         with addition WaitTop
     needs entailment & failure check
       Nervous
     constraints are realized on heap with BIND
       Installed
     garbage collector
       PathMark
     disjunction has elab the guard 'WAIT'
       Waiting
     state
       active: -
       committed: Committed (entailed or unit committed)
       failed: Failed (constraints are inconsistent)
       discarded: Discarded (sibling is committed)
         -> dead = failed or discarded
       */

/*
  class Board
  member data:
    flags: see enum BoardFlags
    suspCount: #tasks + #actors
    body: then continuation
    board: parent board after commitment
    actor: actor who has introced me
     NOTE: parent and actor are shared
    script: the part of trail - when we leave this node;
  comments:
    A series of boards linked with 'parent' is seen as one board with
    the topmost board A as representant.
    It points immediately to A or is a chained link to A,
     if Bo_Board is not set
    */

/*
  enum BoardFlags
    Bo_Ask: is an conditional board
    Bo_Wait: is a disjunctive board
    Bo_Solve: is a solve board;
    Bo_Root: is the root board
      Bo_WaitTop
    Bo_Installed: board is installed
    Bo_Nervous: board should be visited to check entailment/failure
    Bo_Failed: board has failed
    Bo_Committed
    Bo_Discarded
    */

/*
 * return solve board of this
 *   0, if no solve board found
 *      or discard or fail detected below a solve board
 */
Board* Board::getSolveBoard ()
{
  Assert(!isCommitted());
  Board *bb;
  for (bb=this;
       bb!=0 && !bb->isSolve();
       bb=bb->getParentAndTest()) {}
  return bb;
}

Board::Board(Actor *a,int typ)
{
  Assert(a!=NULL || typ==Bo_Root);
  Assert(a==NULL || !a->isCommitted());
  Assert (typ==Bo_Root || typ==Bo_Ask || typ==Bo_Wait || typ==Bo_Solve
          || typ==(Bo_Wait | Bo_Waiting));
  flags     = typ;
  suspCount = 0;
  u.actor   = a;
  gcField   = (Board *) 0;
#ifdef PROP_MERGING
  pq        = (PropagatorQueue *) 0;
#endif
}


/*
 * Before copying all spaces but the space to be copied get marked.
 */
void Board::setGlobalMarks(void) {
  Assert(!_isRoot());

  Board * b = this;

  do {
    b = b->getParent(); b->setGlobalMark();
  } while (!b->_isRoot());

}

/*
 * Purge marks after copying
 */
void Board::unsetGlobalMarks(void) {
  Assert(!_isRoot());

  Board * b = this;

  do {
    b = b->getParent(); b->unsetGlobalMark();
  } while (!b->_isRoot());

}



#ifdef DEBUG_CHECK
/*
 * Check if a board is alive.
 * NOTE: this test can be very expensive !!!
 */
Bool Board::checkAlive()
{
  Board *bb=this;
loop:
  Assert(!bb->isCommitted());
  if (bb->isFailed()) return NO;
  if (bb->_isRoot()) return OK;
  Actor *aa=bb->getActor();
  if (aa->isCommitted()) return NO;
  bb=GETBOARD(aa);
  goto loop;
}
#endif

// -------------------------------------------------------------------------
