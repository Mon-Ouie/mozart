/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
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
 *     http://www.mozart-oz.org
 * 
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
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
    Board ** c = (Board **) freeListMalloc(3*sizeof(Board *));
    *c++ = (Board *) 2;
    children = c;
    c[0] = bb;
    c[1] = NULL;
    return;
  }

  Board ** c = children;

  int32 maxx = ToInt32(c[-1]);

  int i;
  
  for (i = 0; i < maxx; i++) {
    if (!c[i]) {
      c[i] = bb;
      return;
    }
  }

  int size = 2*maxx;
  
  Board **cc = (Board **) freeListMalloc((size+1)*sizeof(Board *));

  *cc++ = (Board *) ToPointer(size);
  
  for (i = maxx; i--; )
    cc[i] = c[i];

  freeListDispose(c-1,(ToInt32(c[-1])+1)*sizeof(Board *));
  
  children = cc;

  cc[maxx] = bb;

  for (i = maxx+1; i < size; i++)
    cc[i] = NULL;

}

void WaitActor::failWaitChild(Board *bb) {
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
  Assert(0);
}

void WaitActor::selectOrFailChildren(int l, int r) {
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

  }
}

Bool WaitActor::isAliveUpToSolve(void) {
  if (isCommitted())
    return NO;

  Board *bb = GETBOARD(this);

  while (1) {
    if (bb->isFailed()) 
      return NO;
    if (bb->_isRoot() || bb->isSolve())
      return OK;
  
    Actor * aa = bb->getActor();
    if (aa->isCommitted()) 
      return NO;

    bb = GETBOARD(aa);
  }

  Assert(0);
}


