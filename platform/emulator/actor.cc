/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Contributors:
 *    Christian Schulte (schulte@dfki.de)
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

//  Implementation of proper actors

#if defined(INTERFACE)
#pragma implementation "actor.hh"
#endif

#include "actor.hh"
#include "board.hh"

/*
 * class Actor:
 *    may be any 'proper' actor;
 *    member data:
 *      flags: see enum ActorFlags
 *      board: board in which actor is installed
 *
 * class AWActor:
 *    may be conditional or disjunction
 *    member data:
 *      next: continuation for next clause
 *      childCount:
 *
 * class AskActor:
 *    member data
 *      elsePC: programm counter of else
 *
 * class WaitActor
 *    member data
 *      children: list of children
 */

/* ------------------------------------------------------------------------
   class WaitActor
   ------------------------------------------------------------------------ */

void WaitActor::addWaitChild(Board *bb)
{
  addChild();
  if (!children) {
    children=(Board **) freeListMalloc(3*sizeof(Board *));
    *children++ = (Board *) 2;
    children[0] = bb;
    children[1] = NULL;
    return;
  }
  int32 maxx= ToInt32(children[-1]);
  int i;
  for (i = 0; i < maxx; i++) {
    if (!children[i]) {
      children[i] = bb;
      return;
    }
  }
  int size = 2*maxx;
  Board **cc = (Board **) freeListMalloc((size+1)*sizeof(Board *));
  *cc++ = (Board *) ToPointer(size);
  for (i = 0; i < maxx; i++) {
    cc[i] = children[i];
  }
  freeListDispose(children-1,(ToInt32(children[-1])+1)*sizeof(Board *));
  children = cc;
  children[maxx] = bb;
  for (i = maxx+1; i < size; i++) {
    children[i] = NULL;
  }
}

void WaitActor::failWaitChild(Board *bb)
{
  failChild();
  int32 maxx = ToInt32(children[-1]);
  for (int i = 0; i < maxx; i++) {
    if (children[i] == bb) {
      for (; i < maxx-1; i++) {    // the order must be preserved (for solve);
        children[i] = children[i+1];
      }
      children[maxx-1] = NULL;
      return;
    }
  }
  error("WaitActor::failChildInternal");
}

int WaitActor::selectOrFailChildren(int l, int r) {
  if (l<=r && l>=0 && r<childCount) {

    int i;
    for (i = 0; i < l; i++)
      children[i]->setFailed();
    for (i = l; i <= r; i++)
      children[i-l] = children[i];
    for (i = r-l+1; i <= r; i++)
      children[i] = NULL;
    for (i = r+1; i < childCount; i++) {
      children[i]->setFailed();
      children[i] = NULL;
    }
    childCount = r-l+1;
    return childCount;
  } else {
    return 0;
  }
}

Bool WaitActor::isAliveUpToSolve(void) {
  if (isCommitted())
    return NO;

  Board *bb = GETBOARD(this);
  Actor *aa;

loop:
  // must be applied to a result of 'getBoard()';
  Assert (!(bb->isCommitted ()));

  if (bb->isFailed())
    return NO;
  if (bb->_isRoot() || bb->isSolve())
    return OK;

  aa=bb->getActor();
  if (aa->isCommitted())
    return NO;

  bb = GETBOARD(aa);

  goto loop;
  // should never come here
  return NO;
}
